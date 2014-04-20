/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEODECODERFFMPEG_H
#define QTAV_VIDEODECODERFFMPEG_H

#include <QtAV/VideoDecoder.h>

namespace QtAV {

class VideoDecoderFFmpegPrivate;
class Q_AV_EXPORT VideoDecoderFFmpeg : public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderFFmpeg)
public:
    VideoDecoderFFmpeg();
    virtual VideoDecoderId id() const;
    //virtual bool prepare();
    virtual bool decode(const QByteArray &encoded);
protected:
    VideoDecoderFFmpeg(VideoDecoderFFmpegPrivate &d);
};

} //namespace QtAV

#endif // QTAV_VIDEODECODERFFMPEG_H
