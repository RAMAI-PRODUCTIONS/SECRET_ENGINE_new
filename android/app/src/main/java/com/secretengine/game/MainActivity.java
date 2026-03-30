package com.secretengine.game;

import android.app.NativeActivity;
import android.os.Bundle;

public class MainActivity extends NativeActivity {
    static {
        System.loadLibrary("SecretEngine");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}