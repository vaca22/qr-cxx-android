#include <jni.h>
#include <string>
#include "QrTask.h"
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <utility>


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

    QrTask::fec = FEC::New(250, 39, 30);

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
