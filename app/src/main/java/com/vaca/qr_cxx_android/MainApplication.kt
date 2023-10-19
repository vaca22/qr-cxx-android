package com.vaca.qr_cxx_android

import android.app.Application
import com.tencent.bugly.crashreport.CrashReport

class MainApplication:Application() {
    companion object{
        lateinit var application: MainApplication
    }
    override fun onCreate() {
        super.onCreate()
        application=this
        CrashReport.initCrashReport(this, "8f1ecf4fbc", false)
    }
}