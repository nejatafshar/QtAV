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

#ifndef QTAV_FILTERCONTEXT_H
#define QTAV_FILTERCONTEXT_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QByteArray>
#include <QtCore/QRect>
#include <QtGui/QPainter>
/*
 * QPainterFilterContext, D2DFilterContext, ...
 */

class QPainter;
class QPaintDevice;
class QTextDocument;
namespace QtAV {

class VideoFrame;
class Q_AV_EXPORT VideoFilterContext
{
public:
    enum Type { ////audio and video...
        QtPainter,
        OpenGL, //Not implemented
        Direct2D, //Not implemeted
        GdiPlus, //Not implemented
        XV, //Not implemented
        None //user defined filters, no context can be used
    };
    static VideoFilterContext* create(Type t);
    VideoFilterContext();
    virtual ~VideoFilterContext();
    virtual Type type() const = 0;

    // map to Qt types
    //drawSurface?
    virtual void drawImage(const QPointF& pos, const QImage& image, const QRectF& source, Qt::ImageConversionFlags flags = Qt::AutoColor);
    // if target is null, draw image at target.topLeft(). if source is null, draw the whole image
    virtual void drawImage(const QRectF& target, const QImage& image, const QRectF& source, Qt::ImageConversionFlags flags = Qt::AutoColor);
    virtual void drawPlainText(const QPointF& pos, const QString& text);
    // if rect is null, draw single line text at rect.topLeft(), ignoring flags
    virtual void drawPlainText(const QRectF& rect, int flags, const QString& text);
    virtual void drawRichText(const QRectF& rect, const QString& text, bool wordWrap = true);
    /*
     * TODO: x, y, width, height: |?|>=1 is in pixel unit, otherwise is ratio of video context rect
     * filter.x(a).y(b).width(c).height(d)
     */
    QRectF rect;
    // Fallback to QPainter if no other paint engine implemented
    QPainter *painter; //TODO: shared_ptr?
    qreal opacity;
    QTransform transform;
    QPainterPath clip_path;
    QFont font;
    QPen pen;
    QBrush brush;
    /*
     * for the filters apply on decoded data, paint_device must be initialized once the data changes
     * can we allocate memory on stack?
     */
    QPaintDevice *paint_device;
    int video_width, video_height; //original size

protected:
    bool own_painter;
    bool own_paint_device;
protected:
    virtual bool isReady() const = 0;
    // save the state then setup new parameters
    virtual bool prepare() = 0;

    virtual void initializeOnFrame(VideoFrame *frame); //private?
    // share qpainter etc.
    virtual void shareFrom(VideoFilterContext *vctx);
    friend class VideoFilter;
};

class VideoFrameConverter;
//TODO: font, pen, brush etc?
class Q_AV_EXPORT QPainterFilterContext : public VideoFilterContext
{
public:
    QPainterFilterContext();
    virtual ~QPainterFilterContext();
    virtual Type type() const; //QtPainter
    // empty source rect equals to the whole source rect
    virtual void drawImage(const QPointF& pos, const QImage& image, const QRectF& source = QRectF(), Qt::ImageConversionFlags flags = Qt::AutoColor);
    virtual void drawImage(const QRectF& target, const QImage& image, const QRectF& source = QRectF(), Qt::ImageConversionFlags flags = Qt::AutoColor);
    virtual void drawPlainText(const QPointF& pos, const QString& text);
    // if rect is null, draw single line text at rect.topLeft(), ignoring flags
    virtual void drawPlainText(const QRectF& rect, int flags, const QString& text);
    virtual void drawRichText(const QRectF& rect, const QString& text, bool wordWrap = true);

protected:
    virtual bool isReady() const;
    virtual bool prepare();
    virtual void initializeOnFrame(VideoFrame *vframe);

    QTextDocument *doc;
    VideoFrameConverter *cvt;
};

} //namespace QtAV

#endif // QTAV_FILTERCONTEXT_H
