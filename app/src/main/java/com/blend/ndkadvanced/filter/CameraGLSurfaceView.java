package com.blend.ndkadvanced.filter;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public class CameraGLSurfaceView extends GLSurfaceView {

    private CameraRender renderer;

    public CameraGLSurfaceView(Context context) {
        super(context);
    }

    public CameraGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        renderer = new CameraRender(this);
        setRenderer(renderer);
        //注意必须在setRenderer 后面。
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public void setFragShader(int type) {
        renderer.setFragShader(type);
    }
}
