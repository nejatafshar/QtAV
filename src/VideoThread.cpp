/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/VideoThread.h>
#include <private/AVThread_p.h>
#include <QtAV/Packet.h>
#include <QtAV/AVClock.h>

namespace QtAV {

const double kSyncThreshold = 0.005; // 5 ms

class VideoThreadPrivate : public AVThreadPrivate
{
public:
    VideoThreadPrivate():delay(0){}
    double delay;
};

VideoThread::VideoThread(QObject *parent) :
    AVThread(*new VideoThreadPrivate(), parent)
{
}

//TODO: if output is null or dummy, the use duration to wait
void VideoThread::run()
{
    if (!d_ptr->dec || !d_ptr->writer)
        return;
    resetState();
    Q_ASSERT(d_ptr->clock != 0);
    Q_D(VideoThread);
    while (!d_ptr->stop) {
        d_ptr->mutex.lock();
        if (d_ptr->packets.isEmpty() && !d_ptr->stop) {
            d_ptr->stop = d_ptr->demux_end;
            if (d_ptr->stop) {
                d_ptr->mutex.unlock();
                break;
            }
        }
        Packet pkt = d_ptr->packets.take(); //wait to dequeue
        //Compare to the clock
        if (pkt.pts <= 0) {
            d_ptr->mutex.unlock();
            continue;
        }
        //TODO: use QThread wait instead
        d->delay = pkt.pts  - (d_ptr->clock->value() + d_ptr->clock->delay());
        if (d->delay > kSyncThreshold) { //Slow down
            //d->delay_cond.wait(&d->mutex, d->delay*1000); //replay may fail. why?
            usleep(d->delay * 1000000);
        } else if (d->delay < -kSyncThreshold) { //Speed up. drop frame?
            //d_ptr->mutex.unlock();
            //continue;
        }
        //qDebug("audio data size after dequeue: %d", d_ptr->packets.size());
        if (d_ptr->dec->decode(pkt.data)) {
            if (d_func()->writer)
                d_func()->writer->write(d_ptr->dec->data());
            //qApp->processEvents(QEventLoop::AllEvents);
        }
        d_ptr->mutex.unlock();
    }
    qDebug("Video thread stops running...");
}

} //namespace QtAV
