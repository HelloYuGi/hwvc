/*
* Copyright (c) 2018-present, aliminabc@gmail.com.
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "AlPicFrameDecoder.h"
#include "AlLogcat.h"

#define TAG "AlPicFrameDecoder"

AlPicFrameDecoder::AlPicFrameDecoder() : AbsVideoDecoder() {
    hwFrameAllocator = new HwFrameAllocator();
}

AlPicFrameDecoder::~AlPicFrameDecoder() {
    if (vPacket) {
        av_packet_free(&vPacket);
        vPacket = nullptr;
    }
    if (vFrame) {
        av_frame_free(&vFrame);
        vFrame = nullptr;
    }
    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }
    if (oHwFrame) {
        oHwFrame->recycle();
        oHwFrame = nullptr;
    }
    delete hwFrameAllocator;
    hwFrameAllocator = nullptr;
}

bool AlPicFrameDecoder::prepare(string path) {
    this->path = path;
    AlFFUtils::init();
    pFormatCtx = avformat_alloc_context();
    //打开输入视频文件
    if (avformat_open_input(&pFormatCtx, path.c_str(), NULL, NULL) != 0) {
        AlLogE(TAG, "failed. Couldn't open input stream.");
        return false;
    }

    vTrack = av_find_best_stream(pFormatCtx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (vTrack < 0) {
        AlLogE(TAG, "failed. Couldn't find video track.");
        return false;
    }

    //获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        AlLogE(TAG, "failed. Couldn't find stream information.");
        return false;
    }
    vPacket = av_packet_alloc();
    vFrame = av_frame_alloc();

    return true;
}

int AlPicFrameDecoder::width() {
    if (!pFormatCtx) return 0;
    return pFormatCtx->streams[vTrack]->codecpar->width;
}

int AlPicFrameDecoder::height() {
    if (!pFormatCtx) return 0;
    return pFormatCtx->streams[vTrack]->codecpar->height;
}

void AlPicFrameDecoder::setOutSampleFormat(HwSampleFormat format) {

}

int AlPicFrameDecoder::getChannels() {
    return 0;
}

int AlPicFrameDecoder::getSampleHz() {
    return 0;
}

int AlPicFrameDecoder::getSampleFormat() {
    return 0;
}

int AlPicFrameDecoder::getSamplesPerBuffer() {
    return 0;
}

void AlPicFrameDecoder::seek(int64_t us) {
    actionSeekInUs = us;
    eof = false;
}

void AlPicFrameDecoder::_handleAction() {
    if (actionSeekInUs >= 0) {
        avcodec_flush_buffers(pFormatCtx->streams[vTrack]->codec);
        int ret = avformat_seek_file(pFormatCtx, -1, INT64_MIN,
                                     actionSeekInUs, INT64_MAX,
                                     AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            AlLogE(TAG, "failed");
            return;
        }
        avcodec_flush_buffers(pFormatCtx->streams[vTrack]->codec);
        AlLogI(TAG, "seek: %lld", actionSeekInUs);
        actionSeekInUs = -1;
    }
}

HwResult AlPicFrameDecoder::grab(HwAbsMediaFrame **frame) {
    _handleAction();
    while (true) {
        int ret = av_read_frame(pFormatCtx, vPacket);
        if (0 == ret) {
            if (vTrack == vPacket->stream_index) {
                avcodec_send_packet(pFormatCtx->streams[vTrack]->codec, vPacket);
            } else {
                continue;
            }
        }
        av_packet_unref(vPacket);
        if (AVERROR_EOF == ret) {
            eof = true;
        }
        //尝试去缓冲区中获取解码完成的视频帧
        if (0 == avcodec_receive_frame(pFormatCtx->streams[vTrack]->codec, vFrame)) {
            vFrame->pts = av_rescale_q_rnd(vFrame->pts,
                                           pFormatCtx->streams[vTrack]->time_base,
                                           oRational,
                                           AV_ROUND_NEAR_INF);
            if (oHwFrame) {
                oHwFrame->recycle();
            }
            oHwFrame = hwFrameAllocator->ref(vFrame);
            *frame = oHwFrame;
            AlLogD(TAG, "%d x %d, %d", vFrame->width, vFrame->height, vFrame->format);
            av_frame_unref(vFrame);
            return Hw::MEDIA_SUCCESS;
        }
        if (eof) {
            AlLogI(TAG, "end");
            return Hw::MEDIA_EOF;
        }
    }
}

int64_t AlPicFrameDecoder::getVideoDuration() {
    return av_rescale_q_rnd(pFormatCtx->streams[vTrack]->duration,
                            pFormatCtx->streams[vTrack]->time_base,
                            oRational,
                            AV_ROUND_NEAR_INF);
}

int64_t AlPicFrameDecoder::getAudioDuration() {
    return 0;
}

int64_t AlPicFrameDecoder::getDuration() {
    return getVideoDuration();
}

int64_t AlPicFrameDecoder::getAudioStartTime() {
    return 0;
}

void AlPicFrameDecoder::start() {

}

void AlPicFrameDecoder::pause() {

}

void AlPicFrameDecoder::stop() {

}