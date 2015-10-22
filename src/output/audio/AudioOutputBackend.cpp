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

#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {

AudioOutputBackend::AudioOutputBackend(AudioOutput::DeviceFeatures f, QObject *parent)
    : QObject(parent)
    , audio(0)
    , available(true)
    , buffer_size(0)
    , buffer_count(0)
    , m_features(f)
{}

void AudioOutputBackend::onCallback()
{
    if (!audio)
        return;
    audio->onCallback();
}


FACTORY_DEFINE(AudioOutputBackend)

void AudioOutput_RegisterAll()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    // check whether ids are registered automatically
    if (!AudioOutputBackendFactory::Instance().registeredIds().empty())
        return;
#if QTAV_HAVE(PORTAUDIO)
    extern bool RegisterAudioOutputBackendPortAudio_Man();
    RegisterAudioOutputBackendPortAudio_Man();
#endif //QTAV_HAVE(PORTAUDIO)
#if QTAV_HAVE(OPENAL)
    extern bool RegisterAudioOutputBackendOpenAL_Man();
    RegisterAudioOutputBackendOpenAL_Man();
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(OPENSL)
    extern bool RegisterAudioOutputBackendOpenSL_Man();
    RegisterAudioOutputBackendOpenSL_Man();
#endif //QTAV_HAVE(OPENSL)
#if QTAV_HAVE(DSOUND)
    extern bool RegisterAudioOutputBackendDSound_Man();
    RegisterAudioOutputBackendDSound_Man();
#endif
#if QTAV_HAVE(XAUDIO2)
    extern bool RegisterAudioOutputBackendXAudio2_Man();
    RegisterAudioOutputBackendXAudio2_Man();
#endif
#if QTAV_HAVE(PULSEAUDIO)
    extern bool RegisterAudioOutputBackendPulse_Man();
    RegisterAudioOutputBackendPulse_Man();
#endif
}

} //namespace QtAV
