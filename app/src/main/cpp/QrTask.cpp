//
// Created by liguanghao on 2023/10/9.
//

#include "QrTask.h"
#include <string>
#include "ReadBarcode.h"
#include "GTIN.h"
#include "ZXVersion.h"
#include "cJSON.h"

#include <cctype>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <android/log.h>
#include "mbedtls/base64.h"
#include "mbedtls/md5.h"



#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "vaca", __VA_ARGS__)


using namespace ZXing;
using namespace std;

char QrTask::global_md5[33];
bool QrTask::isInit = false;
bool QrTask::isEnd = false;
std::mutex QrTask::mtx_decode;
WirehairCodec QrTask::decoder;

//create a decode success callback
QrTask::DecodeSuccessCallback QrTask::decodeSuccessCallback = nullptr;

//create a decode fail callback
QrTask::DecodeFailCallback QrTask::decodeFailCallback = nullptr;



void QrTask::charsMd5(char * decoded_data,char * md5_str){
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *) decoded_data, strlen(decoded_data));
    mbedtls_md5_finish(&ctx, (unsigned char *) md5_str);
    mbedtls_md5_free(&ctx);
}

int QrTask::decodeBase64Data(char *input_data, char *out_data) {
    int write_len = 0;
    int source_len = strlen(input_data);
    mbedtls_base64_decode((unsigned char *) out_data, source_len, reinterpret_cast<size_t *>(&write_len),
                          (unsigned char *) input_data,source_len);
    return write_len;
}

void QrTask::parseJson(char *json) {
    cJSON *root = cJSON_Parse(json);
    cJSON *index = cJSON_GetObjectItem(root, "index");
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *md5 = cJSON_GetObjectItem(root, "md5");
    cJSON *total = cJSON_GetObjectItem(root, "total");
    int index_int = index->valueint;
    int total_int = total->valueint;
    char *data_str = data->valuestring;
    char *full_data= static_cast<char *>(malloc(3*strlen(data_str) + 1));
    int size_int=decodeBase64Data(data_str, full_data);
    if(!QrTask::isInit){
        QrTask::isInit=true;
        LOGE("init\n");
        wirehair_init();
        strcpy(global_md5,md5->valuestring);
        QrTask::decoder=wirehair_decoder_create(nullptr, total_int, size_int);
        if (!decoder)
        {
            LOGE("decoder create failed\n");
        }else{
            LOGE("decoder create ok\n");
        }
    }
    if(isEnd){
        return;
    }


    WirehairResult decodeResult = wirehair_decode(
            decoder,
            index_int,
            full_data,
            size_int);
    if (decodeResult == Wirehair_Success) {
        isEnd = true;
        vector<uint8_t> decoded(total_int);

        // Recover original data on decoder side
        WirehairResult decodeResult = wirehair_recover(
                decoder,
                &decoded[0],
                total_int);
        if (decodeResult != Wirehair_Success)
        {
            if (decodeFailCallback != nullptr) {
                decodeFailCallback();
            }
            return;
        }
        LOGE("decoded_data:%s\n", decoded.data());
        char*md5_str= static_cast<char *>(malloc(16));
        charsMd5(reinterpret_cast<char *>(decoded.data()), md5_str);
        char md5_str2[33];
        for(int i=0;i<16;i++){
            sprintf(&md5_str2[i*2],"%02x",(unsigned char)md5_str[i]);
        }
        md5_str2[32]='\0';
        if(strcmp(md5_str2,global_md5)==0){
            LOGE("md5 check ok\n");
            if (decodeSuccessCallback != nullptr) {
                decodeSuccessCallback(reinterpret_cast<char *>(decoded.data()),total_int);
            }
        }else{
            LOGE("md5 check failed\n");
            if (decodeFailCallback != nullptr) {
                decodeFailCallback();
            }
        }
        wirehair_free(decoder);
        free(md5_str);
    }
    if (decodeResult != Wirehair_NeedMore)
    {
        //call fail callback
        if (decodeFailCallback != nullptr) {
            decodeFailCallback();
        }
       LOGE("decodeResult:%d\n", decodeResult);
    }

    free(full_data);
}


void QrTask::operator()() {
    if(QrTask::isEnd){
        return;
    }
    DecodeHints hints;
    hints.setTextMode(TextMode::HRI);
    hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
    hints.setMaxNumberOfSymbols(1);
    hints.setFormats(BarcodeFormat::QRCode);
    int width, height, channels;
    std::unique_ptr<stbi_uc, void (*)(void *)> buffer(stbi_load_from_memory(
                                                              reinterpret_cast<const stbi_uc *>(buffer_data), len, &width, &height, &channels, 3),
                                                      stbi_image_free);
    if (buffer == nullptr) {
        LOGE("Failed to read image");
        return;
    }
    ImageView image{buffer.get(), width, height, ImageFormat::RGB};
    try{
        //lock mutex
        std::lock_guard<std::mutex> lock(QrTask::mtx_decode);

        auto results = ReadBarcodes(image, hints);
        for (auto &&result: results) {
            parseJson(const_cast<char *>(result.text().c_str()));
        }
    }
    catch (const std::exception &e) {
        LOGE("Error: %s", e.what());
    }

    delete[] buffer_data;
}

void QrTask::setDecodeSuccessCallback(void (*callback)(char *, int)) {
    decodeSuccessCallback = callback;

}
