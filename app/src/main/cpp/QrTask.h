//
// Created by liguanghao on 2023/10/9.
//

#ifndef QR_CXX_ANDROID_QRTASK_H
#define QR_CXX_ANDROID_QRTASK_H



class QrTask {
public:
    static char global_md5[33];
    static char** data_buffer_array;

private:
    int len;
    unsigned char *buffer_data;

    void charsMd5(char * decoded_data,char * md5_str);

    void decodeBase64Data(char *input_data, char *out_data);

    void parseJson(char *json);


public:
    QrTask( unsigned char *buffer_data, int len){
        this->buffer_data = buffer_data;
        this->len = len;
    }

    void operator()();

};



#endif //QR_CXX_ANDROID_QRTASK_H
