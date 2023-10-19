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
JavaVM *g_VM;
jobject g_obj;

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



void decodeSuccessCallback(char *decoded_data, int len) {
    JNIEnv *env;
    g_VM->AttachCurrentThread(&env, nullptr);
    jbyteArray jarray = env->NewByteArray(len);
    env->SetByteArrayRegion(jarray, 0, len, (jbyte *) decoded_data);
    jclass clazz = env->GetObjectClass(g_obj);
    jmethodID mid = env->GetMethodID(clazz, "decodeSuccess", "([B)V");
    env->CallVoidMethod(g_obj, mid, jarray);
    env->DeleteLocalRef(jarray);
    g_VM->DetachCurrentThread();
}




extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_inputImage(JNIEnv *env, jobject thiz, jbyteArray b) {
    jsize len = env->GetArrayLength(b);
    jbyte *buffer_img = env->GetByteArrayElements(b, 0);
    auto *buffer_img_cpy = new unsigned char[len];
    memcpy(buffer_img_cpy, buffer_img, len);
    imgQueue.push(buffer_img_cpy);

    //create a QrTask object and push it to the queue
    QrTask task = QrTask(buffer_img_cpy, len);

    tasks.push(task);
    cv.notify_one();

    env->ReleaseByteArrayElements(b, buffer_img, 0);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_initWorker(JNIEnv *env, jobject thiz) {

    (*env).GetJavaVM(&g_VM);
    g_obj = (*env).NewGlobalRef(thiz);

    QrTask::setDecodeSuccessCallback(decodeSuccessCallback);



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

extern "C"
JNIEXPORT jint JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_getProgress(JNIEnv *env, jobject thiz) {
    return QrTask::getProgress();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_resetWorker(JNIEnv *env, jobject thiz) {
    if(QrTask::index_list!= nullptr){
        delete [] QrTask::index_list;
        QrTask::index_list= nullptr;
    }
    QrTask::isInit= false;
    QrTask::isEnd= false;
    QrTask::progress_num = 0;
    QrTask::last_progress = -1;
    QrTask::progress = 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_vaca_qr_1cxx_1android_MainActivity_stopWorker(JNIEnv *env, jobject thiz) {

}