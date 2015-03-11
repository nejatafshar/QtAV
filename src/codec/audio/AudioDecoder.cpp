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

#include "QtAV/AudioDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/AudioResamplerTypes.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {
FACTORY_DEFINE(AudioDecoder)

AudioDecoderPrivate::AudioDecoderPrivate()
    : AVDecoderPrivate()
    , resampler(0)
{
    resampler = AudioResamplerFactory::create(AudioResamplerId_FF);
    if (!resampler)
        resampler = AudioResamplerFactory::create(AudioResamplerId_Libav);
    if (resampler)
        resampler->setOutSampleFormat(AV_SAMPLE_FMT_FLT);
}

AudioDecoderPrivate::~AudioDecoderPrivate()
{
    if (resampler) {
        delete resampler;
        resampler = 0;
    }
}

AudioDecoder* AudioDecoder::create(AudioDecoderId id)
{
    return AudioDecoderFactory::create(id);
}

AudioDecoder* AudioDecoder::create(const QString& name)
{
    return AudioDecoderFactory::create(AudioDecoderFactory::id(name.toUtf8().constData(), false));
}

AudioDecoder::AudioDecoder(AudioDecoderPrivate &d):
    AVDecoder(d)
{
}

QString AudioDecoder::name() const
{
    return QString(AudioDecoderFactory::name(id()).c_str());
}

QByteArray AudioDecoder::data() const
{
    return d_func().decoded;
}

AudioResampler* AudioDecoder::resampler()
{
    return d_func().resampler;
}

} //namespace QtAV
