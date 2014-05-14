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

#include <QtAV/VideoDecoder.h>
#include <QtAV/private/VideoDecoder_p.h>
#include <QtCore/QSize>
#include "QtAV/factory.h"

namespace QtAV {

FACTORY_DEFINE(VideoDecoder)

extern void RegisterVideoDecoderFFmpeg_Man();
extern void RegisterVideoDecoderDXVA_Man();

void VideoDecoder_RegisterAll()
{
    RegisterVideoDecoderFFmpeg_Man();
#if QTAV_HAVE(DXVA)
    RegisterVideoDecoderDXVA_Man();
#endif //QTAV_HAVE(DXVA)
}


VideoDecoder::VideoDecoder()
    :AVDecoder(*new VideoDecoderPrivate())
{
}

VideoDecoder::VideoDecoder(VideoDecoderPrivate &d):
    AVDecoder(d)
{
}

QString VideoDecoder::name() const
{
    return QString(VideoDecoderFactory::name(id()).c_str());
}

bool VideoDecoder::prepare()
{
    return AVDecoder::prepare();
}

//TODO: use ipp, cuda decode and yuv functions. is sws_scale necessary?
bool VideoDecoder::decode(const QByteArray &encoded)
{
    Q_UNUSED(encoded);
    return false;
}

void VideoDecoder::resizeVideoFrame(const QSize &size)
{
    resizeVideoFrame(size.width(), size.height());
}

/*
 * width, height: the decoded frame size
 * 0, 0 to reset to original video frame size
 */
void VideoDecoder::resizeVideoFrame(int width, int height)
{
    DPTR_D(VideoDecoder);
    d.width = width;
    d.height = height;
}

int VideoDecoder::width() const
{
    return d_func().width;
}

int VideoDecoder::height() const
{
    return d_func().height;
}

VideoFrame VideoDecoder::frame()
{
    DPTR_D(VideoDecoder);
    if (d.width <= 0 || d.height <= 0 || !d.codec_ctx)
        return VideoFrame(0, 0, VideoFormat(VideoFormat::Format_Invalid));
    //DO NOT make frame as a memeber, because VideoFrame is explictly shared!
    float displayAspectRatio = 0;
    if (d.codec_ctx->sample_aspect_ratio.den > 0)
        displayAspectRatio = ((float)d.frame->width / (float)d.frame->height) *
            ((float)d.codec_ctx->sample_aspect_ratio.num / (float)d.codec_ctx->sample_aspect_ratio.den);

    VideoFrame frame(d.frame->width, d.frame->height, VideoFormat((int)d.codec_ctx->pix_fmt));
    frame.setDisplayAspectRatio(displayAspectRatio);
    frame.setBits(d.frame->data);
    frame.setBytesPerLine(d.frame->linesize);
    return frame;
}

} //namespace QtAV
