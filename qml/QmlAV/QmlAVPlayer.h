﻿/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

#ifndef QTAV_QML_AVPLAYER_H
#define QTAV_QML_AVPLAYER_H

#include <QtCore/QObject>
#include <QtCore/QStringList> //5.0
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>
#include <QmlAV/MediaMetaData.h>
#include <QmlAV/QuickFilter.h>
#include <QtAV/AVError.h>
#include <QtAV/VideoCapture.h>
#include <QTimer>

#include <QBuffer>

namespace QtAV {
class AVPlayer;
}
using namespace QtAV;
class QmlAVPlayer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QByteArray sourceBytes READ sourceBytes WRITE setSourceBytes NOTIFY sourceBytesChanged)
    Q_PROPERTY(int loops READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
    Q_PROPERTY(qreal bufferProgress READ bufferProgress NOTIFY bufferProgressChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(MediaMetaData *metaData READ metaData CONSTANT)
    Q_PROPERTY(QObject *mediaObject READ mediaObject  NOTIFY mediaObjectChanged SCRIPTABLE false DESIGNABLE false)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(int ffmpegError READ ffmpegError NOTIFY ffmpegErrorChanged)
    Q_PROPERTY(QString ffmpegErrorStr READ ffmpegErrorStr NOTIFY ffmpegErrorChanged)
    Q_ENUMS(Loop)
    Q_ENUMS(PlaybackState)
    Q_ENUMS(Status)
    Q_ENUMS(Error)
    Q_ENUMS(ChannelLayout)
    Q_ENUMS(BufferMode)
    // not supported by QtMultimedia
    Q_ENUMS(PositionValue)
    Q_ENUMS(MediaEndAction)
    Q_PROPERTY(int startPosition READ startPosition WRITE setStartPosition NOTIFY startPositionChanged)
    Q_PROPERTY(int stopPosition READ stopPosition WRITE setStopPosition NOTIFY stopPositionChanged)
    Q_PROPERTY(bool fastSeek READ isFastSeek WRITE setFastSeek NOTIFY fastSeekChanged)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged)
    Q_PROPERTY(bool abortOnTimeout READ abortOnTimeout WRITE setAbortOnTimeout NOTIFY abortOnTimeoutChanged)
    Q_PROPERTY(ChannelLayout channelLayout READ channelLayout WRITE setChannelLayout NOTIFY channelLayoutChanged)
    Q_PROPERTY(QStringList videoCodecs READ videoCodecs)
    Q_PROPERTY(QStringList videoCodecPriority READ videoCodecPriority WRITE setVideoCodecPriority NOTIFY videoCodecPriorityChanged)
    Q_PROPERTY(QVariantMap videoCodecOptions READ videoCodecOptions WRITE setVideoCodecOptions NOTIFY videoCodecOptionsChanged)
    Q_PROPERTY(QVariantMap avFormatOptions READ avFormatOptions WRITE setAVFormatOptions NOTIFY avFormatOptionsChanged)
    Q_PROPERTY(bool useWallclockAsTimestamps READ useWallclockAsTimestamps WRITE setWallclockAsTimestamps NOTIFY useWallclockAsTimestampsChanged)
    Q_PROPERTY(QtAV::VideoCapture *videoCapture READ videoCapture CONSTANT)
    Q_PROPERTY(int audioTrack READ audioTrack WRITE setAudioTrack NOTIFY audioTrackChanged)
    Q_PROPERTY(int videoTrack READ videoTrack WRITE setVideoTrack NOTIFY videoTrackChanged)
    Q_PROPERTY(int buffered READ buffered)
    Q_PROPERTY(int bufferSize READ bufferSize WRITE setBufferSize NOTIFY bufferSizeChanged)
    Q_PROPERTY(BufferMode bufferMode READ bufferMode WRITE setBufferMode NOTIFY bufferModeChanged)
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(bool realtimeDecode READ realtimeDecode WRITE setRealtimeDecode NOTIFY realtimeDecodeChanged)
    Q_PROPERTY(QVariantMap mediaData READ mediaData NOTIFY mediaDataTimerTriggered)
    Q_PROPERTY(int mediaDataTimerInterval READ mediaDataTimerInterval WRITE setMediaDataTimerInterval NOTIFY mediaDataTimerIntervalChanged)
    Q_PROPERTY(int disconnectTimeout READ disconnectTimeout WRITE setDisconnectTimeout NOTIFY disconnectTimeoutChanged)
    Q_PROPERTY(bool receivingFrames READ receivingFrames NOTIFY receivingFramesChanged)
    Q_PROPERTY(MediaEndAction mediaEndAction READ mediaEndAction WRITE setMediaEndAction NOTIFY mediaEndActionChanged)
    Q_PROPERTY(QUrl externalAudio READ externalAudio WRITE setExternalAudio NOTIFY externalAudioChanged)
    Q_PROPERTY(QVariantList internalAudioTracks READ internalAudioTracks NOTIFY internalAudioTracksChanged)
    Q_PROPERTY(QVariantList internalVideoTracks READ internalVideoTracks NOTIFY internalVideoTracksChanged)
    Q_PROPERTY(QVariantList externalAudioTracks READ externalAudioTracks NOTIFY externalAudioTracksChanged)
    Q_PROPERTY(QVariantList internalSubtitleTracks READ internalSubtitleTracks NOTIFY internalSubtitleTracksChanged)
    // internal subtitle, e.g. mkv embedded subtitles
    Q_PROPERTY(int internalSubtitleTrack READ internalSubtitleTrack WRITE setInternalSubtitleTrack NOTIFY internalSubtitleTrackChanged)

    Q_PROPERTY(QQmlListProperty<QuickAudioFilter> audioFilters READ audioFilters)
    Q_PROPERTY(QQmlListProperty<QuickVideoFilter> videoFilters READ videoFilters)
    // TODO: startPosition/stopPosition
    Q_PROPERTY(QStringList audioBackends READ audioBackends WRITE setAudioBackends NOTIFY audioBackendsChanged)
    Q_PROPERTY(QStringList supportedAudioBackends READ supportedAudioBackends)
    Q_PROPERTY(int notifyInterval READ notifyInterval WRITE setNotifyInterval NOTIFY notifyIntervalChanged)
public:
    enum Loop { Infinite = -1 };
    // use (1<<31)-1
    enum PositionValue { PositionMax = int(~0)^(1<<(sizeof(int)*8-1))};
    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };
    enum Status {
        UnknownStatus = QtAV::UnknownMediaStatus, // e.g. user status after interrupt
        NoMedia = QtAV::NoMedia,
        Loading = QtAV::LoadingMedia, // when source is set
        Loaded = QtAV::LoadedMedia, // if auto load and source is set. player is stopped state
        Stalled = QtAV::StalledMedia,
        Buffering = QtAV::BufferingMedia,
        Buffered = QtAV::BufferedMedia, // when playing
        EndOfMedia = QtAV::EndOfMedia,
        InvalidMedia = QtAV::InvalidMedia,
        PausedOnMediaAtEnd = QtAV::PausedOnMediaAtEnd
    };
    enum Error {
        NoError,

        NetworkError,

        OpenTimedout,
        OpenError,
        ParseStreamTimedOut,
        ParseStreamError,
        StreamNotFound,
        ReadTimedout,
        ReadError,
        SeekError,
        ResourceError,

        OpenCodecError,
        CloseCodecError,
        AudioCodecNotFound,
        VideoCodecNotFound,
        SubtitleCodecNotFound,
        CodecError,

        FormatError,

        AccessDenied,
        ServiceMissing
    };
    // currently supported channels<3.
    enum ChannelLayout {
        ChannelLayoutAuto, //the same as source if channels<=2. otherwise resamples to stereo
        Left,
        Right,
        Mono,
        Stereo
    };
    enum BufferMode
    {
       BufferTime    = QtAV::BufferTime,
       BufferBytes   = QtAV::BufferBytes,
       BufferPackets = QtAV::BufferPackets
    };
    enum MediaEndAction {
        MediaEndAction_Default, /// stop playback (if loop end) and clear video renderer
        MediaEndAction_KeepDisplay = 1, /// stop playback but video renderer keeps the last frame
        MediaEndAction_Pause = 1 << 1 /// pause playback. Currently AVPlayer repeat mode will not work if this flag is set
    };

    explicit QmlAVPlayer(QObject *parent = 0);
    //derived
    virtual void classBegin();
    virtual void componentComplete();

    // add QtAV::AVPlayer::isAudioAvailable()?
    bool hasAudio() const;
    bool hasVideo() const;

    QUrl source() const;
    QByteArray sourceBytes() const;
    /*!
     * \brief setSource
     * If url is changed and auto load is true, current playback will stop.
     */
    void setSource(const QUrl& url);
    void setSourceBytes(const QByteArray& bytes);

    // 0,1: play once. MediaPlayer.Infinite: forever.
    // >1: play loopCount() - 1 times. different from Qt
    int loopCount() const;
    void setLoopCount(int c);

    QObject* videoOut();
    void setVideoOut(QObject* out);
    qreal volume() const;
    void setVolume(qreal value);
    bool isMuted() const;
    void setMuted(bool m);
    int duration() const;
    int position() const;
    bool isSeekable() const;

    int startPosition() const;
    void setStartPosition(int value);
    int stopPosition() const;
    /*!
     * \brief setStopPosition
     * You can use MediaPlayer.PositionMax to play until the end of stream.
     */
    void setStopPosition(int value);
    bool isFastSeek() const;
    void setFastSeek(bool value);

    qreal bufferProgress() const;

    Status status() const;
    Error error() const;
    QString errorString() const;
    int ffmpegError() const;
    QString ffmpegErrorStr() const;
    PlaybackState playbackState() const;
    void setPlaybackState(PlaybackState playbackState);
    qreal playbackRate() const;
    void setPlaybackRate(qreal s);
    Q_INVOKABLE void play(const QUrl& url);
    AVPlayer *player();

    bool isAutoLoad() const;
    void setAutoLoad(bool autoLoad);

    MediaMetaData *metaData() const;
    QObject *mediaObject() const;
    QtAV::VideoCapture *videoCapture() const;

    // "FFmpeg", "CUDA", "DXVA", "VAAPI" etc
    QStringList videoCodecs() const;
    QStringList videoCodecPriority() const;
    void setVideoCodecPriority(const QStringList& p);
    QVariantMap videoCodecOptions() const;
    void setVideoCodecOptions(const QVariantMap& value);
    QVariantMap avFormatOptions() const;
    void setAVFormatOptions(const QVariantMap& value);

    bool useWallclockAsTimestamps() const;
    void setWallclockAsTimestamps(bool use_wallclock_as_timestamps);

    void setAudioChannels(int channels);
    int audioChannels() const;
    void setChannelLayout(ChannelLayout channel);
    ChannelLayout channelLayout() const;

    void setTimeout(int value); // ms
    int timeout() const;
    void setAbortOnTimeout(bool value);
    bool abortOnTimeout() const;

    /*!
     * \brief audioTrack
     * The audio stream number in current media or external audio.
     * Value can be: 0, 1, 2.... 0 means the 1st audio stream in current media or external audio
     */
    int audioTrack() const;
    void setAudioTrack(int value);
    QVariantList internalAudioTracks() const;
    /*!
     * \brief videoTrack
     * The video stream number in current media.
     * Value can be: 0, 1, 2.... 0 means the 1st video stream in current media
     */
    int videoTrack() const;
    void setVideoTrack(int value);
    QVariantList internalVideoTracks() const;

    int buffered() const;

    int bufferSize() const;
    void setBufferSize(int value);

    BufferMode bufferMode() const;
    void setBufferMode(BufferMode value);

    qreal frameRate() const;
    void setFrameRate(qreal value);

    bool realtimeDecode() const;
    void setRealtimeDecode(bool value);

    QVariantMap mediaData() const;

    int mediaDataTimerInterval() const;
    void setMediaDataTimerInterval(int value);

    int disconnectTimeout() const;
    void setDisconnectTimeout(int value);

    bool receivingFrames() const;

    MediaEndAction mediaEndAction() const;
    void setMediaEndAction(MediaEndAction value);

    /*!
     * \brief externalAudio
     * If externalAudio url is valid, player will use audioTrack of external audio as audio source.
     * Set an invalid url to disable external audio
     * \return external audio url or an invalid url if use internal audio tracks
     */
    QUrl externalAudio() const;
    void setExternalAudio(const QUrl& url);
    QVariantList externalAudioTracks() const;

    int internalSubtitleTrack() const;
    void setInternalSubtitleTrack(int value);
    QVariantList internalSubtitleTracks() const;

    QQmlListProperty<QuickAudioFilter> audioFilters();
    QQmlListProperty<QuickVideoFilter> videoFilters();

    QStringList supportedAudioBackends() const;
    QStringList audioBackends() const;
    void setAudioBackends(const QStringList& value);
    int notifyInterval() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void stepForward();
    void stepBackward();
    void seek(int offset);
    void seekForward();
    void seekBackward();
    void setNotifyInterval(int notifyInterval);

    bool startRecording(QString filePath, int duration = -1);
    bool stopRecording();

    void resetMediaData();

Q_SIGNALS:
    void volumeChanged();
    void mutedChanged();
    // TODO: signal from QtAV::AVPlayer
    void hasAudioChanged();
    void hasVideoChanged();
    void durationChanged();
    void positionChanged();
    void sourceChanged();
    void sourceBytesChanged();
    void autoLoadChanged();
    void loopCountChanged();
    void videoOutChanged();
    void playbackStateChanged();
    void playbackRateChanged();
    void paused();
    void stopped();
    void playing();
    void startPositionChanged();
    void stopPositionChanged();
    void seekableChanged();
    void seekFinished(); //WARNING: position() now is not the seek finished video timestamp
    void fastSeekChanged();
    void bufferProgressChanged();
    void videoCodecPriorityChanged();
    void videoCodecOptionsChanged();
    void avFormatOptionsChanged();
    void useWallclockAsTimestampsChanged();
    void channelLayoutChanged();
    void timeoutChanged();
    void abortOnTimeoutChanged();
    void audioTrackChanged();
    void internalAudioTracksChanged();
    void videoTrackChanged();
    void internalVideoTracksChanged();
    void externalAudioChanged();
    void externalAudioTracksChanged();
    void internalSubtitleTrackChanged();
    void internalSubtitleTracksChanged();
    void bufferSizeChanged();
    void bufferModeChanged();
    void frameRateChanged();
    void realtimeDecodeChanged();
    void mediaDataTimerTriggered();
    void mediaDataTimerStarted();
    void mediaDataTimerIntervalChanged();
    void disconnectTimeoutChanged();
    void receivingFramesChanged();
    void mediaEndActionChanged();
    void recordFinished(bool success, const QString& format);

    void errorChanged();
    void ffmpegErrorChanged();
    void error(Error error, const QString &errorString, int ffmpegError = 0, const QString& ffmpegErrorStr="");
    void statusChanged();
    void mediaObjectChanged();
    void audioBackendsChanged();
    void notifyIntervalChanged();

private Q_SLOTS:
    // connect to signals from player
    void _q_error(const QtAV::AVError& e, int ffmpegError, const QString &ffmpegErrorStr);
    void _q_statusChanged();
    void _q_started();
    void _q_stopped();
    void _q_paused(bool);

private Q_SLOTS:
    void applyVolume();
    void applyChannelLayout();

private:
    static void af_append(QQmlListProperty<QuickAudioFilter> *property, QuickAudioFilter *value);
    static int af_count(QQmlListProperty<QuickAudioFilter> *property);
    static QuickAudioFilter *af_at(QQmlListProperty<QuickAudioFilter> *property, int index);
    static void af_clear(QQmlListProperty<QuickAudioFilter> *property);
    static void vf_append(QQmlListProperty<QuickVideoFilter> *property, QuickVideoFilter *value);
    static int vf_count(QQmlListProperty<QuickVideoFilter> *property);
    static QuickVideoFilter *vf_at(QQmlListProperty<QuickVideoFilter> *property, int index);
    static void vf_clear(QQmlListProperty<QuickVideoFilter> *property);

    Q_DISABLE_COPY(QmlAVPlayer)

    bool mUseWallclockAsTimestamps;
    bool m_complete;
    bool m_mute;
    bool mAutoLoad;
    bool mHasAudio, mHasVideo;
    bool m_fastSeek;
    bool m_loading;
    int mLoopCount;
    int mStart, mStop;
    qreal mPlaybackRate;
    qreal mVolume;
    PlaybackState mPlaybackState;
    Error mError;
    QString mErrorString;
    int mffmpegError;
    QString mffmpegErrorStr;
    QtAV::AVPlayer *mpPlayer;
    QUrl mSource;
    QByteArray mSourceBytes;
    QBuffer mIODevice;
    QStringList mVideoCodecs;
    ChannelLayout mChannelLayout;
    int m_timeout;
    bool m_abort_timeout;
    int m_audio_track;
    int m_video_track;
    QUrl m_audio;
    int m_sub_track;

    QScopedPointer<MediaMetaData> m_metaData;
    QVariantMap vcodec_opt;
    QVariantMap avfmt_opt;

    QList<QuickAudioFilter*> m_afilters;
    QList<QuickVideoFilter*> m_vfilters;
    QStringList m_ao;

    QTimer m_setSourceTimer;
    QUrl m_lastSource;
    int m_setSourceTryCount = 0;
};

#endif // QTAV_QML_AVPLAYER_H
