/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QmlAV/QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>

template<typename ID, typename Factory>
static QStringList idsToNames(QVector<ID> ids) {
    QStringList decs;
    foreach (ID id, ids) {
        decs.append(Factory::name(id).c_str());
    }
    return decs;
}

template<typename ID, typename Factory>
static QVector<ID> idsFromNames(const QStringList& names) {
    QVector<ID> decs;
    foreach (const QString& name, names) {
        if (name.isEmpty())
            continue;
        ID id = Factory::id(name.toStdString(), false);
        if (id == 0)
            continue;
        decs.append(id);
    }
    return decs;
}

static inline QStringList VideoDecodersToNames(QVector<QtAV::VideoDecoderId> ids) {
    return idsToNames<QtAV::VideoDecoderId, VideoDecoderFactory>(ids);
}

static inline QVector<VideoDecoderId> VideoDecodersFromNames(const QStringList& names) {
    return idsFromNames<QtAV::VideoDecoderId, VideoDecoderFactory>(names);
}

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , m_complete(false)
  , mAutoPlay(false)
  , mAutoLoad(false)
  , mLoopCount(1)
  , mPlaybackState(StoppedState)
  , mpPlayer(0)
  , mChannelLayout(ChannelLayoutAuto)
{
}

void QmlAVPlayer::classBegin()
{
    mpPlayer = new AVPlayer(this);
    connect(mpPlayer, SIGNAL(paused(bool)), SLOT(_q_paused(bool)));
    connect(mpPlayer, SIGNAL(started()), SLOT(_q_started()));
    connect(mpPlayer, SIGNAL(stopped()), SLOT(_q_stopped()));
    connect(mpPlayer, SIGNAL(positionChanged(qint64)), SIGNAL(positionChanged()));

    mVideoCodecs << "FFmpeg";

    m_metaData.reset(new MediaMetaData());

    emit mediaObjectChanged();
}

void QmlAVPlayer::componentComplete()
{
    // TODO: set player parameters
    if (mSource.isValid() && (mAutoLoad || mAutoPlay)) {
        if (mSource.isLocalFile()) {
            mpPlayer->setFile(mSource.toLocalFile());
        } else {
            mpPlayer->setFile(mSource.toString());
        }
    }

    m_complete = true;

    if (mAutoPlay) {
        if (!mSource.isValid()) {
            stop();
        } else {
            play();
        }
    }
}

bool QmlAVPlayer::hasAudio() const
{
    return mpPlayer->audioStreamCount() > 0;
}

bool QmlAVPlayer::hasVideo() const
{
    return mpPlayer->videoStreamCount() > 0;
}

QUrl QmlAVPlayer::source() const
{
    return mSource;
}

void QmlAVPlayer::setSource(const QUrl &url)
{
    if (mSource == url)
        return;
    mSource = url;
    if (mSource.isLocalFile()) {
        mpPlayer->setFile(mSource.toLocalFile());
    } else {
        mpPlayer->setFile(mSource.toString());
    }
    emit sourceChanged(); //TODO: emit only when player loaded a new source
    // TODO: in componentComplete()?
    if (m_complete && (mAutoLoad || mAutoPlay)) {
        stop();
        //mpPlayer->load(); //QtAV internal bug: load() or play() results in reload
        if (mAutoPlay) {
            play();
        }
    }
}

bool QmlAVPlayer::isAutoLoad() const
{
    return mAutoLoad;
}

void QmlAVPlayer::setAutoLoad(bool autoLoad)
{
    if (mAutoLoad == autoLoad)
        return;

    mAutoLoad = autoLoad;
    emit autoLoadChanged();
}

bool QmlAVPlayer::autoPlay() const
{
    return mAutoPlay;
}

void QmlAVPlayer::setAutoPlay(bool autoplay)
{
    if (mAutoPlay == autoplay)
        return;

    mAutoPlay = autoplay;
    emit autoPlayChanged();
}

MediaMetaData* QmlAVPlayer::metaData() const
{
    return m_metaData.data();
}

QObject* QmlAVPlayer::mediaObject() const
{
    return mpPlayer;
}

QStringList QmlAVPlayer::videoCodecs() const
{
    return VideoDecodersToNames(QtAV::GetRegistedVideoDecoderIds());
}

void QmlAVPlayer::setVideoCodecPriority(const QStringList &p)
{
    if (!mpPlayer) {
        qWarning("player not ready");
        return;
    }
    mVideoCodecs = p;
    mpPlayer->setPriority(VideoDecodersFromNames(p));
    emit videoCodecPriorityChanged();
}

static AudioFormat::ChannelLayout toAudioFormatChannelLayout(QmlAVPlayer::ChannelLayout ch)
{
    struct {
        QmlAVPlayer::ChannelLayout ch;
        AudioFormat::ChannelLayout a;
    } map[] = {
    { QmlAVPlayer::Left, AudioFormat::ChannelLayout_Left },
    { QmlAVPlayer::Right, AudioFormat::ChannelLayout_Right },
    { QmlAVPlayer::Mono, AudioFormat::ChannelLayout_Mono },
    { QmlAVPlayer::Stero, AudioFormat::ChannelLayout_Stero },
    };
    for (uint i = 0; i < sizeof(map)/sizeof(map[0]); ++i) {
        if (map[i].ch == ch)
            return map[i].a;
    }
    return AudioFormat::ChannelLayout_Unsupported;
}


void QmlAVPlayer::setChannelLayout(ChannelLayout channel)
{
    AudioOutput *ao = mpPlayer->audio();
    if (ao && ao->isAvailable()) {
        AudioFormat af = ao->audioFormat();
        AudioFormat::ChannelLayout ch = toAudioFormatChannelLayout(channel);
        if (channel == ChannelLayoutAuto || ch == af.channelLayout()) {
            return;
        }
        af.setChannelLayout(ch);
        if (!ao->close()) {
            qWarning("close audio failed");
            return;
        }
        ao->setAudioFormat(af);
        if (!ao->open()) {
            qWarning("open audio failed");
            return;
        }
    }
    mChannelLayout = channel;
    if (channel != ChannelLayout())
        emit channelLayoutChanged();
}

QmlAVPlayer::ChannelLayout QmlAVPlayer::channelLayout() const
{
    return mChannelLayout;
}

QStringList QmlAVPlayer::videoCodecPriority() const
{
    return mVideoCodecs;
}

int QmlAVPlayer::loopCount() const
{
    return mLoopCount;
}

void QmlAVPlayer::setLoopCount(int c)
{
    if (c == 0)
        c = 1;
    else if (c < -1)
        c = -1;
    if (mLoopCount == c) {
        return;
    }
    mLoopCount = c;
    emit loopCountChanged();
}

qreal QmlAVPlayer::volume() const
{
    AudioOutput *ao = mpPlayer->audio();
    if (ao && ao->isAvailable()) {
        return ao->volume();
    }
    return 0;
}

void QmlAVPlayer::setVolume(qreal volume)
{
    AudioOutput *ao = mpPlayer->audio();
    if (ao && ao->isAvailable() && ao->volume() != volume) {
        ao->setVolume(volume);
        emit volumeChanged();
    }
}

bool QmlAVPlayer::isMuted() const
{
    return mpPlayer->isMute();
}

void QmlAVPlayer::setMuted(bool m)
{
    if (mpPlayer->isMute() == m)
        return;
    mpPlayer->setMute(m);
    emit mutedChanged();
}

int QmlAVPlayer::duration() const
{
    return mpPlayer->duration();
}

int QmlAVPlayer::position() const
{
    return mpPlayer->position();
}

bool QmlAVPlayer::isSeekable() const
{
    return true;
}

QmlAVPlayer::PlaybackState QmlAVPlayer::playbackState() const
{
    return mPlaybackState;
}

void QmlAVPlayer::setPlaybackState(PlaybackState playbackState)
{
    if (mPlaybackState == playbackState) {
        return;
    }
    if (!m_complete)
        return;
    mPlaybackState = playbackState;
    switch (mPlaybackState) {
    case PlayingState:
        if (mpPlayer->isPaused()) {
            mpPlayer->pause(false);
        } else {
            mpPlayer->setRepeat(mLoopCount - 1);
            mpPlayer->play();
            setChannelLayout(channelLayout());
            // TODO: in load()?
            m_metaData->setValuesFromStatistics(mpPlayer->statistics());
        }
        break;
    case PausedState:
        mpPlayer->pause(true);
        break;
    case StoppedState:
        mpPlayer->stop();
        break;
    default:
        break;
    }
}

qreal QmlAVPlayer::playbackRate() const
{
    return mpPlayer->speed();
}

void QmlAVPlayer::setPlaybackRate(qreal s)
{
    if (mpPlayer->speed() == s)
        return;
    mpPlayer->setSpeed(s);
    emit playbackRateChanged();
}

AVPlayer* QmlAVPlayer::player()
{
    return mpPlayer;
}

void QmlAVPlayer::play(const QUrl &url)
{
    if (mSource == url)
        return;
    setSource(url);
    play();
}

void QmlAVPlayer::play()
{
    setPlaybackState(PlayingState);
}

void QmlAVPlayer::pause()
{
    setPlaybackState(PausedState);
}

void QmlAVPlayer::stop()
{
    setPlaybackState(StoppedState);
}

void QmlAVPlayer::nextFrame()
{
    mpPlayer->playNextFrame();
}

void QmlAVPlayer::seek(int offset)
{
    mpPlayer->seek(qint64(offset));
}

void QmlAVPlayer::seekForward()
{
    mpPlayer->seekForward();
}

void QmlAVPlayer::seekBackward()
{
    mpPlayer->seekBackward();
}

void QmlAVPlayer::_q_paused(bool p)
{
    qDebug("*********%s @%d", __FUNCTION__, __LINE__);
    if (p) {
        mPlaybackState = PausedState;
        emit paused();
    } else {
        mPlaybackState = PlayingState;
        emit playing();
    }
    emit playbackStateChanged();
}

void QmlAVPlayer::_q_started()
{
    qDebug("*********%s @%d", __FUNCTION__, __LINE__);
    mPlaybackState = PlayingState;
    emit playing();
    emit playbackStateChanged();
}

void QmlAVPlayer::_q_stopped()
{
    qDebug("*********%s @%d", __FUNCTION__, __LINE__);
    mPlaybackState = StoppedState;
    emit stopped();
    emit playbackStateChanged();
}
