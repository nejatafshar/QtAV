/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_STATISTICS_H
#define QTAV_STATISTICS_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QHash>
#include <QtCore/QTime>
#include <QtCore/QQueue>
#include <QtCore/QSharedData>

/*
 * time unit is s
 * TODO: frame counter, frame droped. see VLC
 */

/*!
 * values from functions are dynamically calculated
 */
namespace QtAV {

class Q_AV_EXPORT Statistics
{
public:
    Statistics();
    ~Statistics();
    void reset();

    QString url;
    int bit_rate;
    QString format;
    QTime start_time, duration;
    QHash<QString, QString> metadata;
    class Common {
    public:
        Common();
        //TODO: dynamic bit rate compute
        bool available;
        QString codec, codec_long;
        QString decoder;
        QString decoder_detail;
        QTime current_time, total_time, start_time;
        int bit_rate;
        qint64 frames;
        qreal frame_rate; // average fps stored in media stream information
        //union member with ctor, dtor, copy ctor only works in c++11
        /*union {
            audio_only audio;
            video_only video;
        } only;*/
        QHash<QString, QString> metadata;
    private:
        class Private : public QSharedData {
        };
        QExplicitlySharedDataPointer<Private> d;
    } audio, video; //init them

    //from AVCodecContext
    class Q_AV_EXPORT AudioOnly {
    public:
        AudioOnly();
        int sample_rate; ///< samples per second
        int channels;    ///< number of audio channels
        QString channel_layout;
        QString sample_fmt;  ///< sample format
        /**
         * Number of samples per channel in an audio frame.
         * - decoding: may be set by some decoders to indicate constant frame size
         */
        int frame_size;
        /**
         * number of bytes per packet if constant and known or 0
         * Used by some WAV based audio codecs.
         */
        int block_align;
    private:
        class Private : public QSharedData {
        };
        QExplicitlySharedDataPointer<Private> d;
    } audio_only;
    //from AVCodecContext
    class Q_AV_EXPORT VideoOnly {
    public:
        //union member with ctor, dtor, copy ctor only works in c++11
        VideoOnly();
        // compute from pts history
        qreal currentDisplayFPS() const;
        qreal pts() const; // last pts

        int width, height;
        /**
         * Bitstream width / height, may be different from width/height if lowres enabled.
         * - decoding: Set by user before init if known. Codec should override / dynamically change if needed.
         */
        int coded_width, coded_height;
        /**
         * the number of pictures in a group of pictures, or 0 for intra_only
         */
        int gop_size;
        QString pix_fmt;
        /// return current absolute time (seconds since epcho
        qint64 frameDisplayed(qreal pts); // used to compute currentDisplayFPS()
    private:
        class Private : public QSharedData {
        public:
            Private();
            QQueue<qreal> history;
            qreal pts;
        };
        QExplicitlySharedDataPointer<Private> d;
    } video_only;
};

} //namespace QtAV

#endif // QTAV_STATISTICS_H
