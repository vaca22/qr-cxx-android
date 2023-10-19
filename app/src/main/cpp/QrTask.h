//
// Created by liguanghao on 2023/10/9.
//

#ifndef QR_CXX_ANDROID_QRTASK_H
#define QR_CXX_ANDROID_QRTASK_H

#include <mutex>
#include <jni.h>
#include "wirehair/wirehair.h"
#include <vector>
using namespace std;

class QrTask {
public:
    static char global_md5[33];
    static bool isInit;
    static WirehairCodec decoder;
    static bool isEnd;
    static std::mutex mtx_decode;
    static int progress_num;
    static int last_progress;
    static int progress;
    static char md5_str[33];

    //create a decode success callback
    typedef void (*DecodeSuccessCallback)(char *decoded_data,int len);

    static DecodeSuccessCallback decodeSuccessCallback;

    //create a decode fail callback
    typedef void (*DecodeFailCallback)();

    static DecodeFailCallback decodeFailCallback;

    //create a progress callback
    typedef void (*ProgressCallback)(int progress);

    static ProgressCallback progressCallback;



private:
    int len;
    unsigned char *buffer_data;

    static int *index_list;
    static int index_list_len;

    void charsMd5(char * decoded_data,int len);

    int  decodeBase64Data(char *input_data, char *out_data);

    void parseJson(char *json);


public:
    QrTask( unsigned char *buffer_data, int len){
        this->buffer_data = buffer_data;
        this->len = len;
    }

    void operator()();

    static void setDecodeSuccessCallback(void (*callback)(char *, int));

    static void setProgressCallback(void (*callback)(int));

    static jint getProgress();
};



#endif //QR_CXX_ANDROID_QRTASK_H
