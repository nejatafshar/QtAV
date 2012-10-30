#include <QtAV/AVDemuxThread.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/QAVPacketQueue.h>
#include <QtAV/AVThread.h>
#include <QtAV/QAVPacket.h>

namespace QtAV {

AVDemuxThread::AVDemuxThread(QObject *parent) :
    QThread(parent),end(false),demuxer(0),audio_thread(0),video_thread(0)
{
}

AVDemuxThread::AVDemuxThread(AVDemuxer *dmx, QObject *parent) :
    QThread(parent),end(false),audio_thread(0),video_thread(0)
{
    setDemuxer(dmx);
}

void AVDemuxThread::setDemuxer(AVDemuxer *dmx)
{
    demuxer = dmx;
    connect(dmx, SIGNAL(finished()), SLOT(stop()));
}

void AVDemuxThread::setAudioThread(AVThread *thread)
{
    if (audio_thread) {
        audio_thread->stop();
        delete audio_thread;
        audio_thread = 0;
    }
    audio_thread = thread;
}

void AVDemuxThread::setVideoThread(AVThread *thread)
{
    if (video_thread) {
        video_thread->stop();
        delete video_thread;
        video_thread = 0;
    }
    video_thread = thread;
}

void AVDemuxThread::stop()
{
    end = true;
}

void AVDemuxThread::run()
{
    qDebug("%s %d", __FUNCTION__, __LINE__);
    Q_ASSERT(audio_thread != 0);
    Q_ASSERT(video_thread != 0);
    qDebug("%s %d", __FUNCTION__, __LINE__);
    if (!audio_thread->isRunning())
        audio_thread->start(QThread::HighPriority);
    if (!video_thread->isRunning())
        video_thread->start();

    qDebug("%s %d", __FUNCTION__, __LINE__);
    audio_stream = demuxer->audioStream();
    qDebug("%s %d", __FUNCTION__, __LINE__);
    video_stream = demuxer->videoStream();
    qDebug("%s %d", __FUNCTION__, __LINE__);
    //demuxer->read()
    //enqueue()
    int index = 0;
    QAVPacket pkt;
    while (!end) {
        if (!demuxer->readFrame()) {
            continue;
        }
        index = demuxer->stream();
        pkt = *demuxer->packet(); //TODO: how to avoid additional copy?
        if (index == audio_stream) {
            audio_thread->packetQueue()->enqueue(pkt);
            audio_thread->wakeAll();
        } else if (index == video_stream) {
            video_thread->packetQueue()->enqueue(pkt);
            video_thread->wakeAll(); //need it?
        } else { //subtitle
            continue;
        }
    }
    qDebug("stop demux thread....");
}

}
