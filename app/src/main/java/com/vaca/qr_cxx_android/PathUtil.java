package com.vaca.qr_cxx_android;

import android.content.Context;

import java.io.File;
import java.util.ArrayList;

public class PathUtil {

    public static String filePath;
    public static String filePathY;

    public static String getPathX(String s) {
        return filePath + s;
    }

    public static String getPathXo2ring(String s) {
        return filePath + "o2ring/" + s + ".dat";
    }

    public static String getPathXpc60(String s) {
        return filePath + "o2ring/" + s + ".pc60";
    }

    public static String getPathY(String s) {
        return filePathY + s;
    }

    public static void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory())
            for (File child : fileOrDirectory.listFiles())
                deleteRecursive(child);

        fileOrDirectory.delete();
    }

    public static void initVar(Context context) {
        File[] fs = context.getExternalFilesDirs(null);
        if (fs != null && fs.length >= 1) {
            filePath = fs[0].getAbsolutePath() + "/";
        }

        filePathY = context.getFilesDir().getAbsolutePath() + "/";

        File file = new File(getPathX("o2ring"));

        if (!file.exists()) {
            file.mkdir();
        }

    }

    public static ArrayList<String> getAllFilesName() {
        File file = new File(filePath);
        ArrayList<String> fileList = new ArrayList<>();
        File[] tempList = file.listFiles();
        for (int i = 0; i < tempList.length; i++) {
            if (tempList[i].isFile()) {
                System.out.println(tempList[i].getName());
                fileList.add(tempList[i].getName());
            }
        }
        return fileList;
    }


}
