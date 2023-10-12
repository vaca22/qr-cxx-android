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
char** QrTask::data_buffer_array;
FEC QrTask::fec;

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
    cJSON *total = cJSON_GetObjectItem(root, "total");
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *md5 = cJSON_GetObjectItem(root, "md5");
    if (md5 != NULL) {
        strcpy(global_md5,md5->valuestring);
    }
    int index_int = index->valueint;
    int total_int = total->valueint;

    if(data_buffer_array==nullptr){
        data_buffer_array= static_cast<char **>(malloc(total_int * sizeof(char *)));
        for(int i=0;i<total_int;i++){
            data_buffer_array[i]=nullptr;
        }
    }
    char *data_str = data->valuestring;
    char *full_data= static_cast<char *>(malloc(3*strlen(data_str) + 1));
    int sizex=decodeBase64Data(data_str,full_data);
  //  LOGE("sizex:%d\n", sizex);
    fecPacket pkt;
    auto shared_ptr = std::make_shared<std::vector<unsigned char>>(full_data,full_data+sizex);
    pkt.data=shared_ptr;
    pkt.seqid=index_int;
    if(index_int>=39){
        pkt.flag=typeFEC;
    }else{
        pkt.flag=typeData;
    }
    auto recovered = fec.Input(pkt);
    if(data_buffer_array[index_int]==nullptr){
        if(index_int<39){
            data_buffer_array[index_int]= reinterpret_cast<char *>(malloc(sizex));
            memcpy(data_buffer_array[index_int],full_data,sizex);
        }
    }
    if(!recovered.empty()){
        LOGE("recovered:%u\n", recovered.size());
        int n=0;
        for(int i=0;i<39;i++){
            if(data_buffer_array[i]==nullptr){

                data_buffer_array[i]= reinterpret_cast<char *>(malloc(sizex));
                memcpy(data_buffer_array[i],  recovered[n]->data(), sizex);
                n++;
            }
        }

        char*decoded_data= static_cast<char *>(malloc(sizex*39+1));
        for(int i=0;i<39;i++){
            unsigned char len=data_buffer_array[i][0];
            memcpy(decoded_data+i*len,data_buffer_array[i]+1,len);
            free(data_buffer_array[i]);
        }
        decoded_data[sizex*39]='\0';
        LOGE("decoded_data:%s\n", decoded_data);
        LOGE("n:%d\n", n);
    }


    LOGE("index:%d\n", index_int);

//    int full_flag=1;
//    int total_len=0;
//    for(int i=0;i<total_int;i++){
//        if(data_buffer_array[i]==nullptr){
//            full_flag=0;
//            break;
//        }else{
//            total_len+=strlen(data_buffer_array[i]);
//        }
//    }

//    if(full_flag){
//        char *full_data= static_cast<char *>(malloc(total_len + 1));
//        full_data[0]='\0';
//        for(int i=0;i<total_int;i++){
//            strcat(full_data,data_buffer_array[i]);
//            free(data_buffer_array[i]);
//        }
//        full_data[total_len]='\0';
//        char*decoded_data= static_cast<char *>(malloc(total_len + 1));
//        decodeBase64Data(full_data,decoded_data);
//        char*md5_str= static_cast<char *>(malloc(16));
//        charsMd5(decoded_data,md5_str);
//        char md5_str2[33];
//        for(int i=0;i<16;i++){
//            sprintf(&md5_str2[i*2],"%02x",(unsigned char)md5_str[i]);
//        }
//        md5_str2[32]='\0';
//        if(strcmp(md5_str2,global_md5)==0){
//            LOGE("md5 check ok\n");
//        }else{
//            LOGE("md5 check failed\n");
//        }
//        free(md5_str);
//        free(decoded_data);
//        free(data_buffer_array);
//        data_buffer_array=NULL;
//    }
}


void QrTask::operator()() {
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