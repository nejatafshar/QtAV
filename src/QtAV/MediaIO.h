/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_MediaIO_H
#define QTAV_MediaIO_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/FactoryDefine.h>
#include <QtCore/QStringList>
#include <QtCore/QObject>

namespace QtAV {

/*!
 * \brief MediaIO
 * Built-in io (use MediaIO::create(name), example: MediaIO *qio = MediaIO::create("QIODevice"))
 * "QIODevice":
 *   properties:
 *     device - read/write. parameter: QIODevice*. example: io->setDevice(mydev)
 * "QFile"
 *   properties:
 *     device - read only. example: io->device()
 *   protocols: "", "qrc"
 */

typedef int MediaIOId;
class MediaIO;
FACTORY_DECLARE(MediaIO)

class MediaIOPrivate;
class Q_AV_EXPORT MediaIO : public QObject
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(MediaIO)
    Q_DISABLE_COPY(MediaIO)
    Q_ENUMS(AccessMode)
public:
    enum AccessMode {
        Read, // default
        Write
    };

    /// Registered MediaIO::name(): "QIODevice", "QFile"
    static QStringList builtInNames();
    static MediaIO* create(const QString& name);
    /*!
     * \brief createForProtocol
     * If an MediaIO subclass SomeInput.protocols() contains the protocol, return it's instance.
     * "QFile" input has protocols "qrc"(and empty "" means "qrc")
     * \return Null if none of registered MediaIO supports the protocol
     */
    static MediaIO* createForProtocol(const QString& protocol);
    /*!
     * \brief createForUrl
     * Create a MediaIO and setUrl(url) if protocol of url is supported.
     * Example: MediaIO *qrc = MediaIO::createForUrl("qrc:/icon/test.mkv");
     * \return MediaIO instance with url set. Null if protocol is not supported.
     */
    static MediaIO* createForUrl(const QString& url);

    virtual ~MediaIO();
    virtual QString name() const = 0;
    virtual void setUrl(const QString& url);
    QString url() const;
    /*!
     * \brief setAccessMode
     * A MediaIO instance can be 1 mode, Read (default) or Write. If !isWritable(), then set to Write will fail and mode does not change
     * Call it before any function!
     * \return false if set failed
     */
    bool setAccessMode(AccessMode value);
    AccessMode accessMode() const;

    /// supported protocols. default is empty
    virtual const QStringList& protocols() const;
    virtual bool isSeekable() const = 0;
    virtual bool isWritable() const = 0;
    /*!
     * \brief read
     * read at most maxSize bytes to data, and return the bytes were actually read
     */
    virtual qint64 read(char *data, qint64 maxSize) = 0;
    /*!
     * \brief write
     * write at most maxSize bytes from data, and return the bytes were actually written
     */
    virtual qint64 write(const char* data, qint64 maxSize) = 0;
    /*!
     * \brief seek
     * \param from
     * 0: seek to offset from beginning position
     * 1: from current position
     * 2: from end position
     * \return true if success
     */
    virtual bool seek(qint64 offset, int from) = 0;
    /*!
     * \brief position
     * MUST implement this. Used in seek
     */
    virtual qint64 position() const = 0;
    /*!
     * \brief size
     * \return <=0 if not support
     */
    virtual qint64 size() const = 0;
    /*!
     * \brief isVariableSize
     * Experiment: A hack for size() changes during playback.
     * If true, containers that estimate duration from pts(or bit rate) will get an invalid duration. Thus no eof get
     * when the size of playback start reaches. So playback will not stop.
     * Demuxer seeking should work for this case.
     */
    virtual bool isVariableSize() const { return false;}
    // The followings are for internal use. used by AVDemuxer, AVMuxer
    //struct AVIOContext; //anonymous struct in FFmpeg1.0.x
    void* avioContext(); //const?
    void release(); //TODO: how to remove it?
protected:
    MediaIO(MediaIOPrivate& d, QObject* parent = 0);
    /*!
     * \brief onUrlChanged
     * Here you can close old url, parse new url() and open it
     */
    virtual void onUrlChanged();
    DPTR_DECLARE(MediaIO)
//private: // must add QT+=av-private if default ctor is private
    // base class, not direct create. only final class has public ctor is enough
    // FIXME: it's required by Q_DECLARE_METATYPE (also copy ctor)
    MediaIO(QObject* parent = 0);
};
Q_DECL_DEPRECATED typedef MediaIO AVInput; // for source compatibility
} //namespace QtAV

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtCore/QMetaType>
Q_DECLARE_METATYPE(QtAV::MediaIO*)
Q_DECLARE_METATYPE(QIODevice*)
#endif
#endif // QTAV_MediaIO_H
