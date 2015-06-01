/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_SUBTITLEFILTER_H
#define QTAV_SUBTITLEFILTER_H

#include <QtAV/Filter.h>
#include <QtAV/Subtitle.h>
//final class

namespace QtAV {

class AVPlayer;
class SubtitleFilterPrivate;
/*!
 * \brief The SubtitleFilter class
 * draw text and image subtitles
 */
class Q_AV_EXPORT SubtitleFilter : public VideoFilter, public SubtitleAPIProxy
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(SubtitleFilter)
    Q_PROPERTY(QByteArray codec READ codec WRITE setCodec NOTIFY codecChanged)
    Q_PROPERTY(QStringList engines READ engines WRITE setEngines NOTIFY enginesChanged)
    Q_PROPERTY(QString engine READ engine NOTIFY engineChanged)
    Q_PROPERTY(bool fuzzyMatch READ fuzzyMatch WRITE setFuzzyMatch NOTIFY fuzzyMatchChanged)
    //Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QStringList dirs READ dirs WRITE setDirs NOTIFY dirsChanged)
    Q_PROPERTY(QStringList suffixes READ suffixes WRITE setSuffixes NOTIFY suffixesChanged)
    Q_PROPERTY(QStringList supportedSuffixes READ supportedSuffixes NOTIFY supportedSuffixesChanged)
    Q_PROPERTY(bool canRender READ canRender NOTIFY canRenderChanged)
    Q_PROPERTY(qreal delay READ delay WRITE setDelay NOTIFY delayChanged)

    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QRectF rect READ rect WRITE setRect NOTIFY rectChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    explicit SubtitleFilter(QObject *parent = 0);
    void setPlayer(AVPlayer* player);
    VideoFilterContext::Type contextType() const {
        return VideoFilterContext::QtPainter;
    }
    /*!
     * \brief setFile
     * load user selected subtitle. autoLoad must be false.
     * if replay the same video, subtitle does not change
     * if play a new video, you have to set subtitle again
     */
    void setFile(const QString& file);
    QString file() const;
    /*!
     * \brief autoLoad
     * auto find a suitable subtitle.
     * if false, load the user selected subtile in setFile() (empty if start a new video)
     * \return
     */
    bool autoLoad() const;
    // <1 means normalized. not valid means the whole target rect. default is (0, 0, 1, 0.9) and align bottom
    void setRect(const QRectF& r);
    QRectF rect() const;
    void setFont(const QFont& f);
    QFont font() const;
    void setColor(const QColor& c);
    QColor color() const;
public slots:
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
signals:
    void rectChanged();
    void fontChanged();
    void colorChanged();
    void autoLoadChanged(bool value);
signals:
    void fileChanged();
    void canRenderChanged();
    void loaded(const QString& path);

    void codecChanged();
    void enginesChanged();
    void fuzzyMatchChanged();
    void contentChanged();
    //void fileNameChanged();
    void dirsChanged();
    void suffixesChanged();
    void supportedSuffixesChanged();
    void engineChanged();
    void delayChanged();

protected:
    virtual void process(Statistics* statistics, VideoFrame* frame);
};

} //namespace QtAV
#endif // QTAV_SUBTITLEFILTER_H
