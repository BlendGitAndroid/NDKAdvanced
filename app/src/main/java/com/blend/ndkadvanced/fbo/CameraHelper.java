package com.blend.ndkadvanced.fbo;

import android.os.HandlerThread;
import android.util.Size;

import androidx.camera.core.CameraX;
import androidx.camera.core.Preview;
import androidx.camera.core.PreviewConfig;
import androidx.lifecycle.LifecycleOwner;

public class CameraHelper {

    private HandlerThread handlerThread;
    private final CameraX.LensFacing currentFacing = CameraX.LensFacing.FRONT;
    private Preview.OnPreviewOutputUpdateListener listener;


    public CameraHelper(LifecycleOwner lifecycleOwner, Preview.OnPreviewOutputUpdateListener listener) {
        this.listener = listener;
        handlerThread = new HandlerThread("Analyze-thread");
        handlerThread.start();
        CameraX.bindToLifecycle(lifecycleOwner, getPreView());
    }

    private Preview getPreView() {
        PreviewConfig previewConfig = new PreviewConfig.Builder().setTargetResolution(new Size(640, 480)).setLensFacing(currentFacing).build();
        Preview preview = new Preview(previewConfig);
        preview.setOnPreviewOutputUpdateListener(listener);
        return preview;
    }

}
