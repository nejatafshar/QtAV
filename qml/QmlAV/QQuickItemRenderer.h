/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>
    theoribeiro <theo@fictix.com.br>

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

#ifndef QTAV_QML_QQUICKRENDERER_H
#define QTAV_QML_QQUICKRENDERER_H

#include <QtAV/VideoRenderer.h>
#include <QtQuick/QQuickItem>

namespace QtAV {
class QQuickItemRendererPrivate;
class QQuickItemRenderer : public QQuickItem, public VideoRenderer
{
    Q_OBJECT
    Q_DISABLE_COPY(QQuickItemRenderer)
    DPTR_DECLARE_PRIVATE(QQuickItemRenderer)
    Q_PROPERTY(bool opengl READ isOpenGL WRITE setOpenGL NOTIFY openGLChanged)
    Q_PROPERTY(QObject* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    // regionOfInterest > sourceRect
    Q_PROPERTY(QRectF regionOfInterest READ regionOfInterest WRITE setRegionOfInterest NOTIFY regionOfInterestChanged)
    Q_ENUMS(FillMode)
public:
    enum FillMode {
        Stretch            = Qt::IgnoreAspectRatio,
        PreserveAspectFit  = Qt::KeepAspectRatio,
        PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
    };

    explicit QQuickItemRenderer(QQuickItem *parent = 0);
    virtual VideoRendererId id() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const;

    QObject *source() const;
    void setSource(QObject *source);

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    bool isOpenGL() const;
    void setOpenGL(bool o);

Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged(QQuickItemRenderer::FillMode);
    void orientationChanged();
    void regionOfInterestChanged();
    void openGLChanged();

protected:
    virtual bool event(QEvent *e);
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

    virtual bool receiveFrame(const VideoFrame &frame);
    virtual bool needUpdateBackground() const;
    virtual bool needDrawFrame() const;
    virtual void drawFrame();

    // QQuickItem interface
    virtual QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
private slots:
    void handleWindowChange(QQuickWindow *win);
    void beforeRendering();
    void afterRendering();
private:
    virtual bool onSetRegionOfInterest(const QRectF& roi);
    virtual bool onSetOrientation(int value);
};
typedef QQuickItemRenderer VideoRendererQQuickItem;
}
QML_DECLARE_TYPE(QtAV::QQuickItemRenderer)

#endif // QTAV_QML_QQUICKRENDERER_H
