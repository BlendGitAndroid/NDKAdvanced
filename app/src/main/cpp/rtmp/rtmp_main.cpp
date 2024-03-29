#include <jni.h>
#include <string>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"RTMP",__VA_ARGS__)
extern "C" {
#include  "rtmp.h"
}

typedef struct {
    RTMP *rtmp;

    int16_t sps_len;
    int8_t *sps;

    int16_t pps_len;
    int8_t *pps;
} Live;

Live *live = NULL;

//传递第一帧      00000001 6742C01FDA8220796F9A80808083C2010A80 0000000168CE3C80
void prepareVideo(int8_t *data, int len, Live *live) {
    for (int i = 0; i < len; i++) {
        // 防止越界
        if (i + 4 < len) {
            if (data[i] == 0x00 && data[i + 1] == 0x00
                && data[i + 2] == 0x00
                && data[i + 3] == 0x01) {
                // 找到sps末尾的位置,也就是pps的头部
                if (data[i + 4] == 0x68) {
                    // sps的长度减去开头的4个字节
                    live->sps_len = i - 4;
                    // new一个数组
                    live->sps = static_cast<int8_t *>(malloc(live->sps_len));
                    // sps解析出来了
                    memcpy(live->sps, data + 4, live->sps_len);

                    // 解析pps
                    live->pps_len = len - (4 + live->sps_len) - 4;
                    // 实例化PPS 的数组
                    live->pps = static_cast<int8_t *>(malloc(live->pps_len));
                    // 拷贝pps的内容
                    memcpy(live->pps, data + 4 + live->sps_len + 4, live->pps_len);
                    LOGI("sps:%d pps:%d", live->sps_len, live->pps_len);
                    break;
                }
            }

        }
    }
}

// 发送sps和pps
RTMPPacket *createVideoPackage(Live *live) {
    // 组装sps，pps的packet
    int body_size = 16 + live->sps_len + live->pps_len;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    // 实例化数据包
    RTMPPacket_Alloc(packet, body_size);
    int i = 0;
    // AVC sequence header 与IDR一样,都是0x17,非IDR是0x27
    packet->m_body[i++] = 0x17;
    //AVC sequence header 设置为0x00
    packet->m_body[i++] = 0x00;
    //CompositionTime PTS相对于DTS的偏移值
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    // AVCDecodeConfigurationRecord,用于描述H.264/AVC（Advanced Video Coding）视频流的参数配置信息。
    // 该结构体通常作为视频流的前置数据（也称为SPS和PPS数据）一起发送给解码器，以帮助解码器正确地解码视频流

    //AVC sequence header
    packet->m_body[i++] = 0x01; //configurationVersion 版本号1
    // 原始操作
    packet->m_body[i++] = live->sps[1]; //profile 如baseline、main、 high
    packet->m_body[i++] = live->sps[2]; //profile_compatibility 兼容性
    packet->m_body[i++] = live->sps[3]; //profile level
    packet->m_body[i++] = 0xFF;//profile 如baseline、main、 high
    packet->m_body[i++] = 0xE1; //reserved（111） + lengthSizeMinusOne（5位 sps 个数） 总是0xe1
    // 保存长度值的高八位
    packet->m_body[i++] = (live->sps_len >> 8) & 0xFF;
    // 保存长度值的低八位
    packet->m_body[i++] = live->sps_len & 0xff;
    // 拷贝sps的内容,两个字节
    memcpy(&packet->m_body[i], live->sps, live->sps_len);
    i += live->sps_len;
    // pps
    packet->m_body[i++] = 0x01; //pps number
    //pps length
    packet->m_body[i++] = (live->pps_len >> 8) & 0xff;
    packet->m_body[i++] = live->pps_len & 0xff;
    // 拷贝pps内容
    memcpy(&packet->m_body[i], live->pps, live->pps_len);
    // 视频类型
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    // 视频 04
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

RTMPPacket *createVideoPackage(int8_t *buf, int len, const long tms, Live *live) {
    // 从第四个字节开始读取
    buf += 4;

    // 要减去00 00 00 01的长度
    len -= 4;
    //长度
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    int body_size = len + 9;
    //初始化RTMP内部的body数组
    RTMPPacket_Alloc(packet, body_size);

    if (buf[0] == 0x65) {
        packet->m_body[0] = 0x17;   // 表示关键帧
        LOGI("发送关键帧 data");
    } else {
        packet->m_body[0] = 0x27;   // 表示非关键帧
        LOGI("发送非关键帧 data");
    }
    // 固定的大小
    packet->m_body[1] = 0x01;
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;

    // 长度
    packet->m_body[5] = (len >> 24) & 0xff;
    packet->m_body[6] = (len >> 16) & 0xff;
    packet->m_body[7] = (len >> 8) & 0xff;
    packet->m_body[8] = (len) & 0xff;

    // 数据
    memcpy(&packet->m_body[9], buf, len);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = tms;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

int sendPacket(RTMPPacket *packet) {
    int r = RTMP_SendPacket(live->rtmp, packet, 1);
    RTMPPacket_Free(packet);
    free(packet);
    return r;
}

// 对于每个NAL块的标识头都去掉，因为这部分信息对rtmp流服务器没用,只需要保留NALU的具体数据,都要减去4个字节
int sendVideo(int8_t *buf, int len, long tms) {
    int ret = 0;

    // 判断第一帧      00000001 6742C01FDA8220796F9A80808083C2010A80 0000000168CE3C80
    if (buf[4] == 0x67) {
        // 缓存sps 和pps 到全局 不需要推流
        if (live && (!live->pps || !live->sps)) {
            //缓存，不发送
            prepareVideo(buf, len, live);
        }
        return ret;
    }

    // 判断I帧，00000001 65B84开头的,先发送SPS和PPS
    if (buf[4] == 0x65) { //关键帧
        // sps 和 pps 的packet  发送sps pps
        RTMPPacket *packet = createVideoPackage(live);
        sendPacket(packet);
    }

    // 发送视频数据
    RTMPPacket *packet2 = createVideoPackage(buf, len, tms, live);
    ret = sendPacket(packet2);
    return ret;
}

RTMPPacket *createAudioPacket(int8_t *buf, const int len, const int type, const long tms,
                              Live *live) {

    // 组装音频包，音频包的格式是固定的，在音频数据前拼两个字节，才符合flv/rtmp 的格式
    int body_size = len + 2;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    // 10101111  根据flv的数据结构拼接 10：AAC，3：44100采样率，1：采样长度，1：声道。按照位数表示数据就为：0xAF
    packet->m_body[0] = 0xAF;
    if (type == 1) {    // 如果是音频头
        packet->m_body[1] = 0x00;
    } else {    // 如果是音频数据
        packet->m_body[1] = 0x01;
    }
    memcpy(&packet->m_body[2], buf, len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x05;
    packet->m_nBodySize = body_size;
    packet->m_nTimeStamp = tms;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = live->rtmp->m_stream_id;
    return packet;
}

int sendAudio(int8_t *buf, int len, int type, int tms) {
    // 创建音频包
    RTMPPacket *packet = createAudioPacket(buf, len, type, tms, live);
    int ret = sendPacket(packet);
    return ret;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_blend_ndkadvanced_rtmp_ScreenLiveService_sendData(JNIEnv *env, jobject thiz,
                                                           jbyteArray data_,
                                                           jint len, jlong tms, jint type) {
    int ret;
    // 将java的byte数组转换成c的byte数组
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    switch (type) {
        case 0: //video
            ret = sendVideo(data, len, tms);
            break;
        default: //audio
            ret = sendAudio(data, len, type, tms);
            LOGI("send Audio  lenght :%d", len);
            break;
    }
    // 释放内存 0:JNI_ABORT 释放data数组,不会同步到java数组中
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_blend_ndkadvanced_rtmp_ScreenLiveService_connect(JNIEnv *env, jobject thiz, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    // RTMP链接失败标志位
    int ret;
    do {
        // 实例化结构体,这个结构体包含RTMP,sps和pps
        live = (Live *) malloc(sizeof(Live));

        // void *memset(void *str, int c, size_t n) 复制字符 c（一个无符号字符）到参数 str 所指向的字符串的前 n 个字符。
        // 要被设置的值。该值以 int 形式传递，但是函数在填充内存块时是使用该值的无符号字符形式。
        memset(live, 0, sizeof(Live));

        // 实例化RTMP,设置超时时间
        live->rtmp = RTMP_Alloc();
        RTMP_Init(live->rtmp);
        live->rtmp->Link.timeout = 10;
        LOGI("connect %s", url);

        // 设置RTMP流媒体服务器的URL,需要在RTMP_Connect之前调用
        if (!(ret = RTMP_SetupURL(live->rtmp, (char *) url))) break;
        RTMP_EnableWrite(live->rtmp);
        LOGI("RTMP_Connect %d", ret);

        // 进行连接操作
        if (!(ret = RTMP_Connect(live->rtmp, 0))) break;
        LOGI("RTMP_ConnectStream %d", ret);

        // 连接到RTMP流，并开始发送和接收数据
        if (!(ret = RTMP_ConnectStream(live->rtmp, 0))) break;
        LOGI("connect success %d", ret);
    } while (0);

    // 如果成功，这里返回的都是1
    if (!ret && live) {
        free(live);
        live = nullptr;
    }
    env->ReleaseStringUTFChars(url_, url);
    LOGI("connect result %d", ret);
    return ret;
}