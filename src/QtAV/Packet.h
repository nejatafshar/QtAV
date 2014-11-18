/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_PACKET_H
#define QAV_PACKET_H

#include <QtCore/QByteArray>
#include <QtAV/QtAV_Global.h>

struct AVPacket;

namespace QtAV {

class Q_AV_EXPORT Packet
{
public:
    //const avpkt? if no use ref
    static Packet fromAVPacket(const AVPacket* avpkt, double time_base);
    static bool fromAVPacket(Packet *pkt, const AVPacket *avpkt, double time_base);

    Packet();
    Packet(const Packet& other);
    ~Packet();

    Packet& operator =(const Packet& other);

    inline bool isValid() const;
    inline bool isEnd() const;
    void markEnd();
    /// Packet takes the owner ship. time unit is ms
    AVPacket* toAVPacket() const;

    bool hasKeyFrame;
    bool isCorrupt;
    QByteArray data;
    // time unit is s.
    qreal pts, duration;
    qreal dts;
    qint64 position; // position in source file byte stream

private:
    static const qreal kEndPts;
    // TODO: implicity shared. can not use QSharedData
    mutable AVPacket *avpkt;
};

bool Packet::isValid() const
{
    return !isCorrupt && !data.isNull() && pts >= 0 && duration >= 0; //!data.isEmpty()?
}

bool Packet::isEnd() const
{
    return pts == kEndPts;
}

} //namespace QtAV

#endif // QAV_PACKET_H
