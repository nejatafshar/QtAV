/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/AudioDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/AudioResamplerTypes.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include "QtAV/version.h"
#include "utils/Logger.h"

namespace QtAV {

class AudioDecoderFFmpegPrivate;
class Q_AV_EXPORT AudioDecoderFFmpeg : public AudioDecoder
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioDecoderFFmpeg)
    DPTR_DECLARE_PRIVATE(AudioDecoderFFmpeg)
    Q_PROPERTY(QString codecName READ codecName WRITE setCodecName NOTIFY codecNameChanged)
public:
    AudioDecoderFFmpeg();
    AudioDecoderId id() const Q_DECL_FINAL;
    virtual QString description() const Q_DECL_FINAL {
        const int patch = QTAV_VERSION_PATCH(avcodec_version());
        return QString("%1 avcodec %2.%3.%4").arg(patch>=100?"FFmpeg":"Libav").arg(QTAV_VERSION_MAJOR(avcodec_version())).arg(QTAV_VERSION_MINOR(avcodec_version())).arg(patch);
    }
    bool prepare() Q_DECL_FINAL;
    bool decode(const QByteArray &encoded) Q_DECL_FINAL;
    bool decode(const Packet& packet) Q_DECL_FINAL;
    AudioFrame frame() Q_DECL_FINAL;
Q_SIGNALS:
    void codecNameChanged();
};

AudioDecoderId AudioDecoderId_FFmpeg = mkid::id32base36_6<'F','F','m','p','e','g'>::value;
FACTORY_REGISTER_ID_AUTO(AudioDecoder, FFmpeg, "FFmpeg")

void RegisterAudioDecoderFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioDecoder, FFmpeg, "FFmpeg")
}

class AudioDecoderFFmpegPrivate : public AudioDecoderPrivate
{
public:
    AudioDecoderFFmpegPrivate()
        : AudioDecoderPrivate()
        , frame(av_frame_alloc())
    {
        avcodec_register_all();
    }
    ~AudioDecoderFFmpegPrivate() {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
    }

    AVFrame *frame; //set once and not change
};

AudioDecoderId AudioDecoderFFmpeg::id() const
{
    return AudioDecoderId_FFmpeg;
}

bool AudioDecoderFFmpeg::prepare()
{
    DPTR_D(AudioDecoder);
    if (!d.codec_ctx)
        return false;
    if (!d.resampler)
        return true;
    d.resampler->setInChannelLayout(d.codec_ctx->channel_layout);
    d.resampler->setInChannels(d.codec_ctx->channels);
    d.resampler->setInSampleFormat(d.codec_ctx->sample_fmt);
    d.resampler->setInSampleRate(d.codec_ctx->sample_rate);
    d.resampler->prepare();
    return true;
}

AudioDecoderFFmpeg::AudioDecoderFFmpeg()
    : AudioDecoder(*new AudioDecoderFFmpegPrivate())
{
}

bool AudioDecoderFFmpeg::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(AudioDecoderFFmpeg);
    d.decoded.clear();
    int got_frame_ptr = 0;
    // const AVPacket*: ffmpeg >= 1.0. no libav
    int ret = avcodec_decode_audio4(d.codec_ctx, d.frame, &got_frame_ptr, (AVPacket*)packet.asAVPacket());
    d.undecoded_size = qMin(packet.data.size() - ret, packet.data.size());
    if (ret == AVERROR(EAGAIN)) {
        return false;
    }
    if (ret < 0) {
        qWarning("[AudioDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!got_frame_ptr) {
        qWarning("[AudioDecoder] got_frame_ptr=false. decoded: %d, un: %d", ret, d.undecoded_size);
        return true;
    }
#if USE_AUDIO_FRAME
    return true;
#endif
    d.resampler->setInSampesPerChannel(d.frame->nb_samples);
    if (!d.resampler->convert((const quint8**)d.frame->extended_data)) {
        return false;
    }
    d.decoded = d.resampler->outData();
    return true;
    return !d.decoded.isEmpty();
}

//
bool AudioDecoderFFmpeg::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(AudioDecoderFFmpeg);
    d.decoded.clear();
    AVPacket packet;
#if NO_PADDING_DATA
    /*!
      larger than the actual read bytes because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
      The end of the input buffer avpkt->data should be set to 0 to ensure that no overreading happens for damaged MPEG streams
     */
    // auto released by av_buffer_default_free
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
#else
    av_init_packet(&packet);
    packet.size = encoded.size();
    packet.data = (uint8_t*)encoded.constData();
#endif //NO_PADDING_DATA
    int got_frame_ptr = 0;
    int ret = avcodec_decode_audio4(d.codec_ctx, d.frame, &got_frame_ptr, &packet);
    d.undecoded_size = qMin(encoded.size() - ret, encoded.size());
    av_free_packet(&packet);
    if (ret == AVERROR(EAGAIN)) {
        return false;
    }
    if (ret < 0) {
        qWarning("[AudioDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!got_frame_ptr) {
        qWarning("[AudioDecoder] got_frame_ptr=false. decoded: %d, un: %d", ret, d.undecoded_size);
        return true;
    }
#if USE_AUDIO_FRAME
    return true;
#endif
    d.resampler->setInSampesPerChannel(d.frame->nb_samples);
    if (!d.resampler->convert((const quint8**)d.frame->extended_data)) {
        return false;
    }
    d.decoded = d.resampler->outData();
    return true;
    return !d.decoded.isEmpty();
}

AudioFrame AudioDecoderFFmpeg::frame()
{
    DPTR_D(AudioDecoderFFmpeg);
    AudioFormat fmt;
    fmt.setSampleFormatFFmpeg(d.frame->format);
    fmt.setChannelLayoutFFmpeg(d.frame->channel_layout);
    fmt.setSampleRate(d.frame->sample_rate);
    if (!fmt.isValid()) {// need more data to decode to get a frame
        return AudioFrame();
    }
    AudioFrame f(fmt);
    f.setBits(d.frame->extended_data); // TODO: ref
    f.setBytesPerLine(d.frame->linesize[0], 0); // for correct alignment
    f.setSamplesPerChannel(d.frame->nb_samples);
    f.setTimestamp((double)d.frame->pkt_pts/1000.0);
    f.setAudioResampler(d.resampler); // TODO: remove. it's not safe if frame is shared. use a pool or detach if ref >1
    return f;
}

} //namespace QtAV

#include "AudioDecoderFFmpeg.moc"
