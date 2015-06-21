/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/AVTranscoder.h"
#include "QtAV/AVPlayer.h"
#include "QtAV/AVMuxer.h"
#include "QtAV/EncodeFilter.h"
#include "utils/BlockingQueue.h"
#include "utils/Logger.h"

namespace QtAV {

class AVTranscoder::Private
{
public:
    Private()
        : started(false)
        , encoded_frames(0)
        , source_player(0)
        , afilter(0)
        , vfilter(0)
    {}

    ~Private() {
        muxer.close();
        if (afilter) {
            delete afilter;
        }
        if (vfilter) {
            delete vfilter;
        }
    }

    bool started;
    int encoded_frames;
    AVPlayer *source_player;
    AudioEncodeFilter *afilter;
    VideoEncodeFilter *vfilter;
    //BlockingQueue<Packet> aqueue, vqueue; // TODO: 1 queue if packet.mediaType is enabled
    AVMuxer muxer;
    QString format;
};

AVTranscoder::AVTranscoder(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
}

AVTranscoder::~AVTranscoder()
{
}

void AVTranscoder::setMediaSource(AVPlayer *player)
{
    if (d->source_player) {
        disconnect(d->source_player, SIGNAL(stopped()), this, SLOT(stop()));
    }
    d->source_player = player;
    connect(d->source_player, SIGNAL(stopped()), this, SLOT(stop()));
}

AVPlayer* AVTranscoder::sourcePlayer() const
{
    return d->source_player;
}

QString AVTranscoder::outputFile() const
{
    return d->muxer.fileName();
}

QIODevice* AVTranscoder::outputDevice() const
{
    return d->muxer.ioDevice();
}

MediaIO* AVTranscoder::outputMediaIO() const
{
    return d->muxer.mediaIO();
}

void AVTranscoder::setOutputMedia(const QString &fileName)
{
    d->muxer.setMedia(fileName);
}

void AVTranscoder::setOutputMedia(QIODevice *dev)
{
    d->muxer.setMedia(dev);
}

void AVTranscoder::setOutputMedia(MediaIO *io)
{
    d->muxer.setMedia(io);
}

void AVTranscoder::setOutputFormat(const QString &fmt)
{
    d->format = fmt;
    d->muxer.setFormat(fmt);
}

QString AVTranscoder::outputFormatForced() const
{
    return d->format;
}

void AVTranscoder::setOutputOptions(const QVariantHash &dict)
{
    d->muxer.setOptions(dict);
}

QVariantHash AVTranscoder::outputOptions() const
{
    return d->muxer.options();
}

bool AVTranscoder::createVideoEncoder(const QString &name)
{
    if (!d->vfilter) {
        d->vfilter = new VideoEncodeFilter();
        connect(d->vfilter, SIGNAL(readyToEncode()), SLOT(prepareMuxer()), Qt::DirectConnection);
        // direct: can ensure delayed frames (when stop()) are written at last
        connect(d->vfilter, SIGNAL(frameEncoded(QtAV::Packet)), SLOT(writeVideo(QtAV::Packet)), Qt::DirectConnection);
    }
    return !!d->vfilter->createEncoder(name);
}

VideoEncoder* AVTranscoder::videoEncoder() const
{
    if (!d->vfilter)
        return 0;
    return d->vfilter->encoder();
}

bool AVTranscoder::createAudioEncoder(const QString &name)
{
    if (!d->afilter) {
        d->afilter = new AudioEncodeFilter();
        connect(d->afilter, SIGNAL(readyToEncode()), SLOT(prepareMuxer()), Qt::DirectConnection);
        // direct: can ensure delayed frames (when stop()) are written at last
        connect(d->afilter, SIGNAL(frameEncoded(QtAV::Packet)), SLOT(writeAudio(QtAV::Packet)), Qt::DirectConnection);
    }
    return !!d->afilter->createEncoder(name);
}

AudioEncoder* AVTranscoder::audioEncoder() const
{
    if (!d->afilter)
        return 0;
    return d->afilter->encoder();
}

bool AVTranscoder::isRunning() const
{
    return d->started;
}

void AVTranscoder::start()
{
    if (!videoEncoder())
        return;
    if (!sourcePlayer())
        return;
    d->encoded_frames = 0;
    d->started = true;
    if (sourcePlayer()) {
        if (d->afilter)
            sourcePlayer()->installAudioFilter(d->afilter);
        if (d->vfilter)
            sourcePlayer()->installVideoFilter(d->vfilter);
    }
    Q_EMIT started();
}

void AVTranscoder::stop()
{
    if (!isRunning())
        return;
    if (!d->muxer.isOpen())
        return;
    // get delayed frames. call VideoEncoder.encode() directly instead of through filter
    while (audioEncoder()->encode()) {
        qDebug("encode delayed audio frames...");
        Packet pkt(audioEncoder()->encoded());
        d->muxer.writeAudio(pkt);
    }
    while (videoEncoder()->encode()) {
        qDebug("encode delayed video frames...");
        Packet pkt(videoEncoder()->encoded());
        d->muxer.writeVideo(pkt);
    }
    if (sourcePlayer()) {
        sourcePlayer()->uninstallFilter(d->afilter);
        sourcePlayer()->uninstallFilter(d->vfilter);
    }
    audioEncoder()->close();
    videoEncoder()->close();
    d->muxer.close();
    d->started = false;
    Q_EMIT stopped();
}

void AVTranscoder::prepareMuxer()
{
    // open muxer only if all encoders are open
    if (audioEncoder() && videoEncoder()) {
        if (!audioEncoder()->isOpen() || !videoEncoder()->isOpen())
            return;
    }
    if (audioEncoder())
        d->muxer.copyProperties(audioEncoder());
    if (videoEncoder())
        d->muxer.copyProperties(videoEncoder());
    if (!d->format.isEmpty())
        d->muxer.setFormat(d->format); // clear when media changed
    if (!d->muxer.open()) {
        qWarning("Failed to open muxer");
        return;
    }
}

void AVTranscoder::writeAudio(const QtAV::Packet &packet)
{
    // TODO: muxer maybe is not open. queue the packet
    if (!d->muxer.isOpen()) {
        //d->aqueue.put(packet);
        return;
    }
    d->muxer.writeAudio(packet);
    Q_EMIT audioFrameEncoded(packet.pts);

    if (d->vfilter)
        return;
    // TODO: startpts, duration, encoded size
    d->encoded_frames++;
    qDebug("encoded frames: %d, pos: %lld", d->encoded_frames, packet.position);
}

void AVTranscoder::writeVideo(const QtAV::Packet &packet)
{
    // TODO: muxer maybe is not open. queue the packet
    if (!d->muxer.isOpen())
        return;
    d->muxer.writeVideo(packet);
    Q_EMIT videoFrameEncoded(packet.pts);

    // TODO: startpts, duration, encoded size
    d->encoded_frames++;
    printf("encoded frames: %d, pos: %lld\r", d->encoded_frames, packet.position);
    fflush(0);
}

} //namespace QtAV
