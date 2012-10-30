#ifndef AVTHREAD_P_H
#define AVTHREAD_P_H

//#include <private/qthread_p.h>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QWaitCondition>

#include <QtAV/QAVPacketQueue.h>
#include <QtAV/AVDecoder.h>
#include <QtAV/AVOutput.h>
#include <QtAV/QAVPacketQueue.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AVDecoder;
class QAVPacket;
class AVClock;
class AVThreadPrivate
{
public:
    AVThreadPrivate():stop(false),clock(0),dec(0),writer(0) {}
    virtual ~AVThreadPrivate() {
        /*if (dec) {
            delete dec;
            dec = 0;
        }
        if (writer) {
            delete writer;
            writer = 0;
        }
        if (packets) {
            delete packets;
            packets = 0;
        }*/
    }
    void enqueue(const QAVPacket& pkt) {
        packets.enqueue(pkt);
        condition.wakeAll();
    }


    volatile bool stop;
    AVClock *clock;
    QAVPacketQueue packets;
    AVDecoder *dec;
    AVOutput *writer;
    QMutex mutex;
    QWaitCondition condition;
};
}

#endif // AVTHREAD_P_H
