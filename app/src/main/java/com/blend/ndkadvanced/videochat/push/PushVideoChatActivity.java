package com.blend.ndkadvanced.videochat.push;

import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.blend.ndkadvanced.databinding.ActivityPushVideoChatBinding;
import com.blend.ndkadvanced.socket.PushSocketLive;
import com.blend.ndkadvanced.socket.SocketCallback;
import com.blend.ndkadvanced.videochat.DecoderPlayerLiveH265;
import com.blend.ndkadvanced.videochat.LocalSurfaceView;

public class PushVideoChatActivity extends AppCompatActivity implements SocketCallback {

    private ActivityPushVideoChatBinding mBinding;

    private DecoderPlayerLiveH265 decoderPlayerLiveH265;
    private Surface surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBinding = ActivityPushVideoChatBinding.inflate(getLayoutInflater());
        setContentView(mBinding.getRoot());

        // 获取远端的数据,需要解码
        mBinding.remotePushSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                // 获取到surface
                surface = holder.getSurface();
                // 解码远端的数据
                decoderPlayerLiveH265 = new DecoderPlayerLiveH265();
                decoderPlayerLiveH265.initDecoder(surface, 1080, 2340);

            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            }
        });

        // 本地摄像头,编码视频数据
        mBinding.btnPushVideoCall.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PushSocketLive pushSocketLive = new PushSocketLive(PushVideoChatActivity.this);
                mBinding.localPushSurfaceView.startCapture(pushSocketLive);
            }
        });
    }

    // 获取到远端的数据
    @Override
    public void callBack(byte[] data) {
        if (decoderPlayerLiveH265 != null) {
            decoderPlayerLiveH265.callBack(data);
        }
    }
}