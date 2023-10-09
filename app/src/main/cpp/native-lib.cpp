#include <jni.h>
#include <string>
#include "ReadBarcode.h"
#include "GTIN.h"
#include "ZXVersion.h"
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

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image_write.h>
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "vaca", __VA_ARGS__)
using namespace ZXing;
using namespace std;

extern "C" JNIEXPORT jstring JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jfloat JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_qr(JNIEnv *env, jobject thiz, jbyteArray b) {
    //get the length of the array
    jsize len = env->GetArrayLength(b);
    //allocate a buffer to store the array
    jbyte *buffer_img=env->GetByteArrayElements(b, 0);
    auto start = std::chrono::high_resolution_clock::now();
    DecodeHints hints;
    hints.setTextMode(TextMode::HRI);
    hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
    int width, height, channels;
    std::unique_ptr<stbi_uc, void (*)(void *)> buffer(stbi_load_from_memory(
                                                              reinterpret_cast<const stbi_uc *>(buffer_img), len, &width, &height, &channels, 3),
                                                      stbi_image_free);
    if (buffer == nullptr) {
        LOGE( "Failed to read image");
        return 0;
    }

    ImageView image{buffer.get(), width, height, ImageFormat::RGB};
    auto results = ReadBarcodes(image, hints);
    for (auto &&result: results) {
        LOGE( "Text:       %s", result.text().c_str());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;
    LOGE( "Time:       %f", diff.count());
    //release the buffer when done
    env->ReleaseByteArrayElements(b, buffer_img, 0);
    return diff.count();
}
