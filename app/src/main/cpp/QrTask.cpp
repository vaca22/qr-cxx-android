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
int QrTask::progress_num = 0;
int QrTask::last_progress = -1;
int QrTask::progress = 0;
char QrTask::md5_str[33];

int *QrTask::index_list=nullptr;
int QrTask::index_list_len=0;

//create a decode success callback
QrTask::DecodeSuccessCallback QrTask::decodeSuccessCallback = nullptr;

//create a decode fail callback
QrTask::DecodeFailCallback QrTask::decodeFailCallback = nullptr;


//create a progress callback
QrTask::ProgressCallback QrTask::progressCallback = nullptr;


void QrTask::charsMd5(char * decoded_data,int len){
    char  md5_str_temp[16];
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *) decoded_data, len);
    mbedtls_md5_finish(&ctx, (unsigned char *) md5_str_temp);
    for(int i=0;i<16;i++){
        sprintf(&md5_str[i*2],"%02x",(unsigned char)md5_str_temp[i]);
    }
    md5_str[32]='\0';
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

    if(!QrTask::isInit) {
        index_list= static_cast<int *>(malloc(4 * total->valueint * 20));
        index_list_len=0;
    }
    //check if index_int in index_vector
    for(int i=0;i<index_list_len;i++){
        if(index_list[i]==index_int){
            return;
        }
    }
    index_list[index_list_len]=index_int;
    index_list_len++;


    int total_int = total->valueint;
    char *data_str = data->valuestring;
    char *full_data= static_cast<char *>(malloc(2*strlen(data_str) + 1));
    int size_int=decodeBase64Data(data_str, full_data);
    if(index_int!=last_progress){
        last_progress=index_int;
        progress_num++;
        progress=progress_num*100*size_int/total_int;
    }



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
      //  LOGE("decoded_data:%s\n", decoded.data());

        charsMd5(reinterpret_cast<char *>(decoded.data()), total_int);
        LOGE("md5_str2:%s\n", md5_str);
        if(strcmp(md5_str,global_md5)==0){
            LOGE("md5 check ok\n");
            if (decodeSuccessCallback != nullptr) {
                decodeSuccessCallback(reinterpret_cast<char *>(decoded.data()),total_int);
            }
        }else{
            LOGE("md5 check failed\n");
            if (decodeFailCallback != nullptr) {
                decodeFailCallback();
            }
            if (decodeSuccessCallback != nullptr) {
                decodeSuccessCallback(reinterpret_cast<char *>(decoded.data()),total_int);
            }
        }
        wirehair_free(decoder);

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
    hints.setMaxNumberOfSymbols(2);
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
    {
        std::unique_lock<std::mutex> lock(QrTask::mtx_decode);
        auto results = ReadBarcodes(image, hints);
        for (auto &&result: results) {
            parseJson(const_cast<char *>(result.text().c_str()));
        }
        LOGE("qr number:%d\n", results.size());
    }


    delete[] buffer_data;
}

void QrTask::setDecodeSuccessCallback(void (*callback)(char *, int)) {
    decodeSuccessCallback = callback;

}

void QrTask::setProgressCallback(void (*callback)(int)) {
    progressCallback = callback;

}

jint QrTask::getProgress() {
    return progress;
}
