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

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <utility>


class QrTask {
    int len;
    unsigned char *buffer_data;
public:
    QrTask( unsigned char *buffer_data, int len){
        this->buffer_data = buffer_data;
        this->len = len;
    }
    void operator()() const {
        auto start = std::chrono::high_resolution_clock::now();
        DecodeHints hints;
        hints.setTextMode(TextMode::HRI);
        hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
        int width, height, channels;
        std::unique_ptr<stbi_uc, void (*)(void *)> buffer(stbi_load_from_memory(
                                                                  reinterpret_cast<const stbi_uc *>(buffer_data), len, &width, &height, &channels, 3),
                                                          stbi_image_free);
        if (buffer == nullptr) {
            LOGE("Failed to read image");
            return;
        }

        ImageView image{buffer.get(), width, height, ImageFormat::RGB};
        auto results = ReadBarcodes(image, hints);
        for (auto &&result: results) {
            LOGE("Text:       %s", result.text().c_str());
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        LOGE("Time:       %f", diff.count());
        delete[] buffer_data;
    }

};

std::queue<QrTask> tasks;
std::queue<unsigned char *> imgQueue;
std::mutex mtx;
std::condition_variable cv;
bool stop = false;


void worker() {
    while (true) {
        std::function<void(unsigned char *, int len)> func;
        QrTask *task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) {
                return;
            }
            task = &tasks.front();
            tasks.pop();
        }
        (*task)();
    }
}




extern "C"
JNIEXPORT jfloat JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_qr(JNIEnv *env, jobject thiz, jbyteArray b) {
    //get the length of the array
    jsize len = env->GetArrayLength(b);
    //allocate a buffer to store the array
    jbyte *buffer_img = env->GetByteArrayElements(b, 0);
    auto start = std::chrono::high_resolution_clock::now();
    DecodeHints hints;
    hints.setTextMode(TextMode::HRI);
    hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
    int width, height, channels;
    std::unique_ptr<stbi_uc, void (*)(void *)> buffer(stbi_load_from_memory(
                                                              reinterpret_cast<const stbi_uc *>(buffer_img), len, &width, &height, &channels, 3),
                                                      stbi_image_free);
    if (buffer == nullptr) {
        LOGE("Failed to read image");
        return 0;
    }

    ImageView image{buffer.get(), width, height, ImageFormat::RGB};
    auto results = ReadBarcodes(image, hints);
    for (auto &&result: results) {
        LOGE("Text:       %s", result.text().c_str());
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    LOGE("Time:       %f", diff.count());
    //release the buffer when done
    env->ReleaseByteArrayElements(b, buffer_img, 0);
    return diff.count();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_inputImage(JNIEnv *env, jobject thiz, jbyteArray b) {
    jsize len = env->GetArrayLength(b);
    jbyte *buffer_img = env->GetByteArrayElements(b, 0);
    auto *buffer_img2 = new unsigned char[len];
    memcpy(buffer_img2, buffer_img, len);
    imgQueue.push(buffer_img2);

    //create a QrTask object and push it to the queue
    QrTask task = QrTask(buffer_img2, len);

    tasks.push(task);
    cv.notify_one();

    env->ReleaseByteArrayElements(b, buffer_img, 0);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_initWorker(JNIEnv *env, jobject thiz) {
    std::vector<std::thread> threads;
    threads.reserve(8);
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker);
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = false;
    }

    for (auto &t: threads) {
        t.detach();
    }
}