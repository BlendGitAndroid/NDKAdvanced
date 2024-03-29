
#include "MNFFmpeg.h"

MNFFmpeg::MNFFmpeg(MNPlaystatus *playstatus, MNCallJava *callJava, const char *url) {
    this->playstatus = playstatus;
    this->callJava = callJava;
    this->url = url;
    pthread_mutex_init(&seek_mutex, NULL);
    pthread_mutex_init(&init_mutex, NULL);
}

void *decodeFFmpeg(void *data)
{
    MNFFmpeg *wlFFmpeg = (MNFFmpeg *) data;
    wlFFmpeg->decodeFFmpegThread();
    pthread_exit(&wlFFmpeg->decodeThread);
}

void MNFFmpeg::parpared() {
    pthread_create(&decodeThread, NULL, decodeFFmpeg, this);
}

void MNFFmpeg::decodeFFmpegThread() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0)
    {
        if(LOG_DEBUG)
        {
            LOGE("can not open url :%s", url);
        }
        return;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        if(LOG_DEBUG)
        {
            LOGE("can not find streams from %s", url);
        }
        return;
    }
    for(int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)//得到音频流
        {
            if(audio == NULL)
            {
                audio = new MNAudio(playstatus, pFormatCtx->streams[i]->codecpar->sample_rate,callJava);
                audio->streamIndex = i;
//                 pFormatCtx->duration  微妙 转换成秒
                audio->duration = pFormatCtx->duration / AV_TIME_BASE;
                audio->time_base = pFormatCtx->streams[i]->time_base;
                audio->codecpar = pFormatCtx->streams[i]->codecpar;
                duration = audio->duration;
            }
        }
    }

    AVCodec *dec = avcodec_find_decoder(audio->codecpar->codec_id);
    if(!dec)
    {
        if(LOG_DEBUG)
        {
            LOGE("can not find decoder");
        }
        return;
    }

    audio->avCodecContext = avcodec_alloc_context3(dec);
    if(!audio->avCodecContext)
    {
        if(LOG_DEBUG)
        {
            LOGE("can not alloc new decodecctx");
        }
        return;
    }

    if(avcodec_parameters_to_context(audio->avCodecContext, audio->codecpar) < 0)
    {
        if(LOG_DEBUG)
        {
            LOGE("can not fill decodecctx");
        }
        return;
    }

    if(avcodec_open2(audio->avCodecContext, dec, 0) != 0)
    {
        if(LOG_DEBUG)
        {
            LOGE("cant not open audio strames");
        }
        return;
    }
    callJava->onCallParpared(CHILD_THREAD);
    pthread_mutex_unlock(&init_mutex);
}

void MNFFmpeg::start() {

    if(audio == NULL)
    {
        if(LOG_DEBUG)
        {
            LOGE("audio is null");
            return;
        }
    }
    audio->play();

    int count = 0;

    while(playstatus != NULL && !playstatus->exit)
    {
//        seek  耗时
        if(playstatus->seek)
        {
            continue;
        }
//        放入队列
        if(audio->queue->getQueueSize() > 40){
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        if(av_read_frame(pFormatCtx, avPacket) == 0)
        {
            if(avPacket->stream_index == audio->streamIndex)
            {
                //解码操作
                count++;
                if(LOG_DEBUG)
                {
                    LOGE("解码第 %d 帧", count);
                }
                audio->queue->putAvpacket(avPacket);
            } else{
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else{
            av_packet_free(&avPacket);
            av_free(avPacket);
            while(playstatus != NULL && !playstatus->exit)
            {
                if(audio->queue->getQueueSize() > 0)
                {
                    continue;
                } else{
                    playstatus->exit = true;
                    break;
                }
            }
        }
    }

    if(LOG_DEBUG)
    {
        LOGD("解码完成");
    }
}

void MNFFmpeg::seek(int64_t secds) {
    if (duration <= 0) {
        return;
    }
    if (secds >= 0 && secds <= duration) {
        if (audio != NULL) {
            playstatus->seek = true;
            audio->queue->clearAvpacket();
            audio->clock = 0;
            audio->last_tiem = 0;
            pthread_mutex_lock(&seek_mutex);
            int64_t rel = secds * AV_TIME_BASE;
            avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
            pthread_mutex_unlock(&seek_mutex);
            playstatus->seek = false;
        }
    }


}

MNFFmpeg::~MNFFmpeg() {
    pthread_mutex_destroy(&seek_mutex);
    pthread_mutex_destroy(&init_mutex);
}

void MNFFmpeg::pause() {
    if(audio != NULL)
    {
        audio->pause();
    }

}

void MNFFmpeg::resume() {
    if(audio != NULL)
    {
        audio->resume();
    }
}

void MNFFmpeg::setMute(int mute) {
    if(audio != NULL)
    {
        audio->setMute(mute);
    }
}
void MNFFmpeg::setVolume(int percent) {
    if(audio != NULL)
    {
        audio->setVolume(percent);
    }
}

void MNFFmpeg::setSpeed(float speed) {
    if(audio != NULL)
    {
        audio->setSpeed(speed);
    }

}

void MNFFmpeg::setPitch(float pitch) {
    if(audio != NULL)
    {
        audio->setPitch(pitch);
    }
}

void MNFFmpeg::release() {

    if(LOG_DEBUG)
    {
        LOGE("开始释放Ffmpe");
    }
    playstatus->exit = true;
//    队列        stop    exit
    int sleepCount = 0;
    pthread_mutex_lock(&init_mutex);
    while (!exit)
    {
        if(sleepCount > 1000)
        {
            exit = true;

        }
        if(LOG_DEBUG)
        {
            LOGE("wait ffmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }

    if(audio != NULL)
    {
        audio->release();
        delete(audio);
        audio = NULL;
    }

    if(LOG_DEBUG)
    {
        LOGE("释放 封装格式上下文");
    }
    if(pFormatCtx != NULL)
    {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if(LOG_DEBUG)
    {
        LOGE("释放 callJava");
    }
    if(callJava != NULL)
    {
        callJava = NULL;
    }
    if(LOG_DEBUG)
    {
        LOGE("释放 playstatus");
    }
    if(playstatus != NULL)
    {
        playstatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

