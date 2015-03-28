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

#include "AudioThread.h"
#include "AVThread_p.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/Packet.h"
#include "QtAV/AudioFormat.h"
#include "QtAV/AudioOutput.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/AVClock.h"
#include "QtAV/Filter.h"
#include "output/OutputSet.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include "utils/Logger.h"

namespace QtAV {

class AudioThreadPrivate : public AVThreadPrivate
{
public:
    void init() {
        resample = false;
        last_pts = 0;
    }

    bool resample;
    qreal last_pts; //used when audio output is not available, to calculate the aproximate sleeping time
};

AudioThread::AudioThread(QObject *parent)
    :AVThread(*new AudioThreadPrivate(), parent)
{
}

void AudioThread::applyFilters(AudioFrame &frame)
{
    DPTR_D(AudioThread);
    //QMutexLocker locker(&d.mutex);
    //Q_UNUSED(locker);
    if (!d.filters.isEmpty()) {
        //sort filters by format. vo->defaultFormat() is the last
        foreach (Filter *filter, d.filters) {
            AudioFilter *af = static_cast<AudioFilter*>(filter);
            if (!af->isEnabled())
                continue;
            af->apply(d.statistics, &frame);
        }
    }
}
/*
 *TODO:
 * if output is null or dummy, the use duration to wait
 */
void AudioThread::run()
{
    DPTR_D(AudioThread);
    //No decoder or output. No audio output is ok, just display picture
    if (!d.dec || !d.dec->isAvailable() || !d.outputSet)
        return;
    resetState();
    Q_ASSERT(d.clock != 0);
    d.init();
    //TODO: bool need_sync in private class
    bool is_external_clock = d.clock->clockType() == AVClock::ExternalClock;
    Packet pkt;
    while (!d.stop) {
        processNextTask();
        //TODO: why put it at the end of loop then playNextFrame() not work?
        if (tryPause()) { //DO NOT continue, or playNextFrame() will fail
            if (d.stop)
                break; //the queue is empty and may block. should setBlocking(false) wake up cond empty?
        } else {
            if (isPaused())
                continue;
        }
        if (!pkt.isValid()) {
            pkt = d.packets.take(); //wait to dequeue
        }
        if (pkt.isEOF()) {
            d.stop = true;
            qDebug("audio thread gets an eof packet. exit.");
            break;
        }
        if (!pkt.isValid()) {
            qDebug("Invalid packet! flush audio codec context!!!!!!!! audio queue size=%d", d.packets.size());
            QMutexLocker locker(&d.mutex);
            Q_UNUSED(locker);
            if (d.dec) //maybe set to null in setDecoder()
                d.dec->flush();
            continue;
        }
        qreal dts = pkt.dts; //FIXME: pts and dts
        bool skip_render = pkt.pts < d.render_pts0;
        // audio has no key frame, skip rendering equals to skip decoding
        if (skip_render) {
            d.clock->updateValue(pkt.pts);
            /*
             * audio may be too fast than video if skip without sleep
             * a frame is about 20ms. sleep time must be << frame time
             */
            qreal a_v = dts - d.clock->videoTime();
            //qDebug("skip audio decode at %f/%f v=%f a-v=%fms", dts, d.render_pts0, d.clock->videoTime(), a_v*1000.0);
            if (a_v > 0) {
                msleep(qMin((ulong)20, ulong(a_v*1000.0)));
            } else {
                // audio maybe too late compared with video packet before seeking backword. so just ignore
                msleep(1);
            }
            pkt = Packet(); //mark invalid to take next
            continue;
        }
        d.render_pts0 = 0;
        if (is_external_clock) {
            d.delay = dts - d.clock->value();
            /*
             *after seeking forward, a packet may be the old, v packet may be
             *the new packet, then the d.delay is very large, omit it.
             *TODO: 1. how to choose the value
             * 2. use last delay when seeking
            */
            if (qAbs(d.delay) < 2.0) {
                if (d.delay < -kSyncThreshold) { //Speed up. drop frame?
                    //continue;
                }
                if (d.delay > 0)
                    waitAndCheck(d.delay, dts);
            } else { //when to drop off?
                if (d.delay > 0) {
                    msleep(64);
                } else {
                    //audio packet not cleaned up?
                    continue;
                }
            }
        } else {
            d.clock->updateValue(pkt.pts);
        }

        /* lock here to ensure decoder and ao can complete current work before they are changed
         * current packet maybe not supported by new decoder
         */
        // TODO: smaller scope
        QMutexLocker locker(&d.mutex);
        Q_UNUSED(locker);
        AudioDecoder *dec = static_cast<AudioDecoder*>(d.dec);
        if (!dec)
            continue;
        AudioOutput *ao = 0;
        // first() is not null even if list empty
        if (!d.outputSet->outputs().isEmpty())
            ao = static_cast<AudioOutput*>(d.outputSet->outputs().first());

        //DO NOT decode and convert if ao is not available or mute!
        bool has_ao = ao && ao->isAvailable();
        //if (!has_ao) {//do not decode?
        // TODO: move resampler to AudioFrame, like VideoFrame does
        if (has_ao && dec->resampler()) {
            if (dec->resampler()->speed() != ao->speed()
                    || dec->resampler()->outAudioFormat() != ao->audioFormat()) {
                //resample later to ensure thread safe. TODO: test
                if (d.resample) {
                    qDebug() << "ao.format " << ao->audioFormat();
                    qDebug() << "swr.format " << dec->resampler()->outAudioFormat();
                    qDebug("decoder set speed: %.2f", ao->speed());
                    dec->resampler()->setOutAudioFormat(ao->audioFormat());
                    dec->resampler()->setSpeed(ao->speed());
                    dec->resampler()->prepare();
                    d.resample = false;
                } else {
                    d.resample = true;
                }
            }
        } else {
            if (dec->resampler() && dec->resampler()->speed() != d.clock->speed()) {
                if (d.resample) {
                    qDebug("decoder set speed: %.2f", d.clock->speed());
                    dec->resampler()->setSpeed(d.clock->speed());
                    dec->resampler()->prepare();
                    d.resample = false;
                } else {
                    d.resample = true;
                }
            }
        }
        if (d.stop) {
            qDebug("audio thread stop before decode()");
            break;
        }
        if (!dec->decode(pkt)) {
            qWarning("Decode audio failed. undecoded: %d", dec->undecodedSize());
            qreal dt = dts - d.last_pts;
            if (dt > 0.5 || dt < 0) {
                dt = 0;
            }
            if (!qFuzzyIsNull(dt)) {
                msleep((unsigned long)(dt*1000.0));
            }
            pkt = Packet();
            d.last_pts = d.clock->value(); //not pkt.pts! the delay is updated!
            continue;
        }
#if USE_AUDIO_FRAME
        AudioFrame frame(dec->frame());
        if (frame) {
            //TODO: apply filters here
            if (has_ao) {
                applyFilters(frame);
                frame.setAudioResampler(dec->resampler()); //!!!
                // FIXME: resample is required for audio frames from ffmpeg
                //if (ao->audioFormat() != frame.format()) {
                    frame = frame.to(ao->audioFormat());
                //}
            }
        } // no continue if frame is invalid. decoder may need more data to get a frame

        QByteArray decoded(frame.data());
#else
        QByteArray decoded(dec->data());
#endif
        int decodedSize = decoded.size();
        int decodedPos = 0;
        qreal delay = 0;
        //AudioFormat.durationForBytes() calculates int type internally. not accurate
        const AudioFormat &af = dec->resampler()->outAudioFormat();
        const qreal byte_rate = af.bytesPerSecond();
        while (decodedSize > 0) {
            if (d.stop) {
                qDebug("audio thread stop after decode()");
                break;
            }
            // TODO: set to format.bytesPerFrame()*1024?
            const int chunk = qMin(decodedSize, has_ao ? ao->bufferSize() : 1024*4);//int(max_len*byte_rate));
            //AudioFormat.bytesForDuration
            const qreal chunk_delay = (qreal)chunk/(qreal)byte_rate;
            pkt.pts += chunk_delay;
            pkt.dts += chunk_delay;
            if (has_ao) {
                QByteArray decodedChunk = QByteArray::fromRawData(decoded.constData() + decodedPos, chunk);
                ao->play(decodedChunk, pkt.pts);
                d.clock->updateValue(ao->timestamp());
            } else {
                d.clock->updateDelay(delay += chunk_delay);
            /*
             * why need this even if we add delay? and usleep sounds weird
             * the advantage is if no audio device, the play speed is ok too
             * So is portaudio blocking the thread when playing?
             */
                static bool sWarn_no_ao = true; //FIXME: no warning when replay. warn only once
                if (sWarn_no_ao) {
                    qDebug("Audio output not available! msleep(%lu)", (unsigned long)((qreal)chunk/(qreal)byte_rate * 1000));
                    sWarn_no_ao = false;
                }
                //TODO: avoid acummulative error. External clock?
                msleep((unsigned long)(chunk_delay * 1000.0));
            }
            decodedPos += chunk;
            decodedSize -= chunk;
        }
        if (has_ao)
            emit frameDelivered();
        int undecoded = dec->undecodedSize();
        if (undecoded > 0) {
            pkt.data.remove(0, pkt.data.size() - undecoded);
        } else {
            pkt = Packet();
        }

        d.last_pts = d.clock->value(); //not pkt.pts! the delay is updated!
    }
    d.packets.clear();
    qDebug("Audio thread stops running...");
}

} //namespace QtAV
