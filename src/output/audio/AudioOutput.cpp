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

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"
#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {

/// from libavfilter/af_volume begin
static inline void scale_samples_u8(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8(((((qint64)src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_u8_small(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8((((src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_s16(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst       = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16(((qint64)smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s16_small(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst       = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16((smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s32(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    qint32 *smp_dst       = (qint32 *)dst;
    const qint32 *smp_src = (const qint32 *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clipl_int32((((qint64)smp_src[i] * volume + 128) >> 8));
}
/// from libavfilter/af_volume end

//TODO: simd
template<typename T>
static inline void scale_samples(quint8 *dst, const quint8 *src, int nb_samples, int, float volume)
{
    T *smp_dst = (T *)dst;
    const T *smp_src = (const T *)src;
    for (int i = 0; i < nb_samples; ++i)
        smp_dst[i] = smp_src[i] * (T)volume;
}

scale_samples_func get_scaler(AudioFormat::SampleFormat fmt, qreal vol, int* voli)
{
    int v = (int)(vol * 256.0 + 0.5);
    if (voli)
        *voli = v;
    switch (fmt) {
    case AudioFormat::SampleFormat_Unsigned8:
    case AudioFormat::SampleFormat_Unsigned8Planar:
        return v < 0x1000000 ? scale_samples_u8_small : scale_samples_u8;
    case AudioFormat::SampleFormat_Signed16:
    case AudioFormat::SampleFormat_Signed16Planar:
        return v < 0x10000 ? scale_samples_s16_small : scale_samples_s16;
    case AudioFormat::SampleFormat_Signed32:
    case AudioFormat::SampleFormat_Signed32Planar:
        return scale_samples_s32;
    case AudioFormat::SampleFormat_Float:
    case AudioFormat::SampleFormat_FloatPlanar:
        return scale_samples<float>;
    case AudioFormat::SampleFormat_Double:
    case AudioFormat::SampleFormat_DoublePlanar:
        return scale_samples<double>;
    default:
        return 0;
    }
}

void AudioOutputPrivate::updateSampleScaleFunc()
{
    scale_samples = get_scaler(format.sampleFormat(), vol, &volume_i);
}

AudioOutputPrivate::~AudioOutputPrivate()
{
    if (backend) {
        backend->close();
        delete backend;
    }
}

void AudioOutputPrivate::playInitialData()
{
    if (!backend)
        return;
    const char c = (format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8
                    || format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8Planar)
            ? 0x80 : 0;
    for (quint32 i = 1; i < nb_buffers; ++i) {
        backend->write(QByteArray(buffer_size, c)); // fill silence byte, not always 0. AudioFormat.silenceByte
        nextEnqueueInfo().data_size = buffer_size;
        nextEnqueueInfo().timestamp = 0;
        bufferAdded();
    }
    backend->play();
}

AudioOutput::AudioOutput(QObject* parent)
    : QObject(parent)
    , AVOutput(*new AudioOutputPrivate())
{
    qDebug() << "Registered audio backends: " << AudioOutput::backendsAvailable(); // call this to register
    d_func().format.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    d_func().format.setChannelLayout(AudioFormat::ChannelLayout_Stero);
    static const QStringList all = QStringList()
#if QTAV_HAVE(PULSEAUDIO)
            << "Pulse"
#endif
#if QTAV_HAVE(OPENAL)
            << "OpenAL"
#endif
#if QTAV_HAVE(PORTAUDIO)
            << "PortAudio"
#endif
#if QTAV_HAVE(OPENSL)
            << "OpenSL"
#endif
#if QTAV_HAVE(DSOUND)
            << "DirectSound"
#endif
              ;
    setBackends(all); //ensure a backend is available
}

AudioOutput::~AudioOutput()
{
    close();
}

extern void AudioOutput_RegisterAll(); //why vc link error if put in the following a exported class member function?
QStringList AudioOutput::backendsAvailable()
{
    AudioOutput_RegisterAll();
    static QStringList all;
    if (!all.isEmpty())
        return all;
    std::vector<std::string> a = AudioOutputBackendFactory::registeredNames();
    for (size_t i = 0; i < a.size(); ++i) {
        all.append(QString::fromStdString(a[i]));
    }
    return all;
}

void AudioOutput::setBackends(const QStringList &backendNames)
{
    DPTR_D(AudioOutput);
    if (d.backends == backendNames)
        return;
    d.update_backend = true;
    d.backends = backendNames;
    // create backend here because we have to check format support before open which needs a backend
    d.update_backend = false;
    if (d.backend) {
        d.backend->close();
        delete d.backend;
        d.backend = 0;
    }
    // TODO: empty backends use dummy backend
    if (!d.backends.isEmpty()) {
        foreach (const QString& b, d.backends) {
            d.backend = AudioOutputBackendFactory::create(AudioOutputBackendFactory::id(b.toStdString()));
            if (d.backend)
                break;
        }
    }
    if (d.backend) {
        // default: set all features when backend is ready
        setDeviceFeatures(d.backend->supportedFeatures());
        // connect volumeReported
        connect(d.backend, SIGNAL(volumeReported(qreal)), SLOT(reportVolume(qreal)));
        connect(d.backend, SIGNAL(muteReported(bool)), SLOT(reportMute(bool)));
    }

    emit backendsChanged();
}

QStringList AudioOutput::backends() const
{
    return d_func().backends;
}

QString AudioOutput::backend() const
{
    DPTR_D(const AudioOutput);
    if (d.backend)
        return d.backend->name();
    return QString();
}

bool AudioOutput::open()
{
    DPTR_D(AudioOutput);
    d.resetStatus();
    if (!d.backend)
        return false;
    d.backend->audio = this;
    d.backend->buffer_size = bufferSize();
    d.backend->buffer_count = bufferCount();
    d.backend->format = audioFormat();
    // TODO: open next backend if fail and emit backendChanged()
    if (!d.backend->open())
        return false;
    d.available = true;
    d.playInitialData();
    return true;
}

bool AudioOutput::close()
{
    DPTR_D(AudioOutput);
    d.resetStatus();
    if (!d.backend)
        return false;
    d.backend->audio = 0;
    return d.backend->close();
}

bool AudioOutput::play(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    if (!d.backend)
        return false;
    receiveData(data, pts);
    return d.backend->play();
}

bool AudioOutput::receiveData(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    //DPTR_D(AVOutput);
    //locker(&mutex)
    //TODO: make sure d.data thread safe. lock around here? for audio and video(main thread problem)?
    /* you can use d.data directly in AVThread. In other thread, it's not safe, you must do something
     * to make sure the data is not be modified in AVThread when using it*/
    if (d.paused)
        return false;
    d.data = data;
    if (isMute() && d.sw_mute) {
        if (d.format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8
                || d.format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8Planar)
            d.data.fill((char)0x80);
        else
            d.data.fill(0);
    } else {
        if (!qFuzzyCompare(volume(), (qreal)1.0)
                && d.sw_volume
                && d.scale_samples
                ) {
            // TODO: af_volume needs samples_align to get nb_samples
            const int nb_samples = d.data.size()/d.format.bytesPerSample();
            quint8 *dst = (quint8*)d.data.constData();
            d.scale_samples(dst, dst, nb_samples, d.volume_i, volume());
        }
    }
    // wait after all data processing finished to reduce time error
    waitForNextBuffer();
    d.nextEnqueueInfo().data_size = data.size();
    d.nextEnqueueInfo().timestamp = pts;
    d.bufferAdded();
    if (!d.backend)
        return false;
    return d.backend->write(d.data);
}

void AudioOutput::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioOutput);
    // no support check because that may require an open device(AL) while this function is called before ao.open()
    if (d.format == format)
        return;
    d.updateSampleScaleFunc();
    d.format = format;
}

AudioFormat& AudioOutput::audioFormat()
{
    return d_func().format;
}

const AudioFormat& AudioOutput::audioFormat() const
{
    return d_func().format;
}

void AudioOutput::setSampleRate(int rate)
{
    d_func().format.setSampleRate(rate);
}

int AudioOutput::sampleRate() const
{
    return d_func().format.sampleRate();
}

// TODO: check isSupported
void AudioOutput::setChannels(int channels)
{
    DPTR_D(AudioOutput);
    d.format.setChannels(channels);
}

int AudioOutput::channels() const
{
    return d_func().format.channels();
}

void AudioOutput::setVolume(qreal volume)
{
    DPTR_D(AudioOutput);
    if (volume < 0.0)
        return;
    if (d.vol == volume) //fuzzy compare?
        return;
    d.vol = volume;
    emit volumeChanged(d.vol);
    d.updateSampleScaleFunc();
    if (deviceFeatures() & SetVolume) {
        d.sw_volume = !d.backend->setVolume(d.vol);
        //if (!qFuzzyCompare(deviceGetVolume(), d.vol))
        //    d.sw_volume = true;
        if (d.sw_volume)
            d.backend->setVolume(1.0); // TODO: partial software?
    } else {
        d.sw_volume = true;
    }
}

qreal AudioOutput::volume() const
{
    return qMax<qreal>(d_func().vol, 0);
}

void AudioOutput::setMute(bool value)
{
    DPTR_D(AudioOutput);
    if (d.mute == value)
        return;
    d.mute = value;
    emit muteChanged(value);
    if ((deviceFeatures() & SetMute) && d.backend)
        d.sw_mute = !d.backend->setMute(value);
    else
        d.sw_mute = true;
}

bool AudioOutput::isMute() const
{
    return d_func().mute;
}

void AudioOutput::setSpeed(qreal speed)
{
    d_func().speed = speed;
}

qreal AudioOutput::speed() const
{
    return d_func().speed;
}

bool AudioOutput::isSupported(const AudioFormat &format) const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return false;
    return d.backend->isSupported(format);
}

bool AudioOutput::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return false;
    return d.backend->isSupported(sampleFormat);
}

bool AudioOutput::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return false;
    return d.backend->isSupported(channelLayout);
}

AudioFormat::SampleFormat AudioOutput::preferredSampleFormat() const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return AudioFormat::SampleFormat_Signed16;
    return d.backend->preferredSampleFormat();
}

AudioFormat::ChannelLayout AudioOutput::preferredChannelLayout() const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return AudioFormat::ChannelLayout_Stero;
    return d.backend->preferredChannelLayout();
}

int AudioOutput::bufferSize() const
{
    return d_func().buffer_size;
}

void AudioOutput::setBufferSize(int value)
{
    d_func().buffer_size = value;
}

int AudioOutput::bufferCount() const
{
    return d_func().nb_buffers;
}

void AudioOutput::setBufferCount(int value)
{
    d_func().nb_buffers = value;
}

// no virtual functions inside because it can be called in ctor
void AudioOutput::setDeviceFeatures(DeviceFeatures value)
{
    DPTR_D(AudioOutput);
    //Qt5: QFlags::Int (int or uint)
    const int s(supportedDeviceFeatures());
    const int f(value);
    if (d.features == (f & s))
        return;
    d.features = (f & s);
    emit deviceFeaturesChanged();
}

AudioOutput::DeviceFeatures AudioOutput::deviceFeatures() const
{
    return (DeviceFeature)d_func().features;
}

AudioOutput::DeviceFeatures AudioOutput::supportedDeviceFeatures() const
{
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return NoFeature;
    return d.backend->supportedFeatures();
}

void AudioOutput::waitForNextBuffer()
{
    DPTR_D(AudioOutput);
    if (!d.backend)
        return;
    //don't return even if we can add buffer because we don't know when a buffer is processed and we have /to update dequeue index
    // openal need enqueue to a dequeued buffer! why sl crash
    bool no_wait = false;//d.canAddBuffer();
    const AudioOutputBackend::BufferControl f = d.backend->bufferControl();
    int remove = 0;
    if (f & AudioOutputBackend::Blocking) {
        remove = 1;
    } else if (f & AudioOutputBackend::Callback) {
        int processed = d.processed_remain;
        d.processed_remain = d.backend->getWritableBytes();
        const int next = d.nextDequeueInfo().data_size;
        //qDebug("remain: %d-%d, size: %d, next: %d", processed, d.processed_remain, d.data.size(), next);
        while (d.processed_remain - processed < next || d.processed_remain < d.data.size()) { //implies next > 0
            const qint64 us = d.format.durationForBytes(next - (d.processed_remain - processed));
            QMutexLocker lock(&d.mutex);
            Q_UNUSED(lock);
            d.cond.wait(&d.mutex, us/1000LL);
            d.processed_remain = d.backend->getWritableBytes();
        }
        processed = d.processed_remain - processed;
        d.processed_remain -= d.data.size(); //ensure d.processed_remain later is greater
        remove = -processed; // processed_this_period
    } else if (f & AudioOutputBackend::PlayedBytes) {
        d.processed_remain = d.backend->getPlayedBytes();
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
        // TODO: compare processed_remain with d.data.size because input chuncks can be in different sizes
        while (!no_wait && d.processed_remain < next) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            if (us < 1000LL)
                d.uwait(10000LL);
            else
                d.uwait(us);
            d.processed_remain = d.backend->getPlayedBytes();
            // what if s always 0?
        }
        remove = -d.processed_remain;
    } else if (f & AudioOutputBackend::PlayedCount) {
#if AO_USE_TIMER
        if (!d.timer.isValid())
            d.timer.start();
        qint64 elapsed = 0;
#endif //AO_USE_TIMER
        int c = d.backend->getPlayedCount();
        // TODO: avoid always 0
        qint64 us = 0;
        while (!no_wait && c < 1) {
            if (us <= 0)
                us = d.format.durationForBytes(d.nextDequeueInfo().data_size);
#if AO_USE_TIMER
            elapsed = d.timer.restart();
            if (elapsed > 0 && us > elapsed*1000LL)
                us -= elapsed*1000LL;
            if (us < 1000LL)
                us = 10000LL; //opensl crash if 1
#endif //AO_USE_TIMER
            d.uwait(us);
            c = d.backend->getPlayedCount();
        }
        // what if c always 0?
        remove = c;
    } else if (f & AudioOutputBackend::OffsetBytes) { //TODO: similar to Callback+getWritableBytes()
        int s = d.backend->getOffsetByBytes();
        int processed = s - d.play_pos;
        //qDebug("s: %d, play_pos: %d, processed: %d, bufferSizeTotal: %d", s, d.play_pos, processed, bufferSizeTotal());
        if (processed < 0)
            processed += bufferSizeTotal();
        d.play_pos = s;
        const int next = d.nextDequeueInfo().data_size;
        int writable_size = d.processed_remain + processed;
        while (!no_wait && (/*processed < next ||*/ writable_size < d.data.size()) && next > 0) {
            const qint64 us = d.format.durationForBytes(next - writable_size);
            d.uwait(us);
            s = d.backend->getOffsetByBytes();
            processed += s - d.play_pos;
            if (processed < 0)
                processed += bufferSizeTotal();
            writable_size = d.processed_remain + processed;
            d.play_pos = s;
        }
        d.processed_remain += processed;
        d.processed_remain -= d.data.size(); //ensure d.processed_remain later is greater
        remove = -processed;
    } else if (f & AudioOutputBackend::OffsetIndex) {
        int n = d.backend->getOffset();
        int processed = n - d.play_pos;
        if (processed < 0)
            processed += bufferCount();
        d.play_pos = n;
        // TODO: timer
        // TODO: avoid always 0
        while (!no_wait && processed < 1) {
            d.uwait(d.format.durationForBytes(d.nextDequeueInfo().data_size));
            n = d.backend->getOffset();
            processed = n - d.play_pos;
            if (processed < 0)
                processed += bufferCount();
            d.play_pos = n;
        }
        remove = processed;
    } else {
        qFatal("User defined waitForNextBuffer() not implemented!");
        return;
    }
    if (remove < 0) {
        int next = d.nextDequeueInfo().data_size;
        int free_bytes = -remove;//d.processed_remain;
        while (free_bytes >= next && next > 0) {
            free_bytes -= next;
            if (!d.bufferRemoved()) {//|| d.processed_remain <= 0
                qWarning("buffer queue empty");
                break;
            }
            next = d.nextDequeueInfo().data_size;
        }
        //qDebug("remove: %d, unremoved bytes < %d, writable_bytes: %d", remove, free_bytes, d.processed_remain);
        return;
    }
    //qDebug("remove count: %d", remove);
    while (remove-- > 0) {
        if (!d.bufferRemoved())
            break;
    }
}


qreal AudioOutput::timestamp() const
{
    DPTR_D(const AudioOutput);
    return d.nextDequeueInfo().timestamp;
}

void AudioOutput::reportVolume(qreal value)
{
    if (qFuzzyCompare(value + 1.0, volume() + 1.0))
        return;
    DPTR_D(AudioOutput);
    d.vol = value;
    Q_EMIT volumeChanged(value);
    // skip sw sample scale
    d.sw_volume = false;
}

void AudioOutput::reportMute(bool value)
{
    if (value == isMute())
        return;
    DPTR_D(AudioOutput);
    d.mute = value;
    Q_EMIT muteChanged(value);
    // skip sw sample scale
    d.sw_mute = false;
}

void AudioOutput::onCallback()
{
    d_func().onCallback();
}
} //namespace QtAV
