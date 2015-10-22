/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>
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
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged)
    Q_ENUMS(FillMode)
public:
    enum FillMode {
        Stretch            = Qt::IgnoreAspectRatio,
        PreserveAspectFit  = Qt::KeepAspectRatio,
        PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
    };

    explicit QQuickItemRenderer(QQuickItem *parent = 0);
    VideoRendererId id() const Q_DECL_OVERRIDE;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;

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
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE;
    void frameSizeChanged();
protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

    bool receiveFrame(const VideoFrame &frame) Q_DECL_OVERRIDE;
    bool needUpdateBackground() const Q_DECL_OVERRIDE;
    bool needDrawFrame() const Q_DECL_OVERRIDE;
    void drawFrame() Q_DECL_OVERRIDE;

    // QQuickItem interface
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) Q_DECL_OVERRIDE;
private slots:
    void handleWindowChange(QQuickWindow *win);
    void beforeRendering();
    void afterRendering();
private:
    bool onSetRegionOfInterest(const QRectF& roi) Q_DECL_OVERRIDE;
    bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    void onFrameSizeChanged(const QSize& size) Q_DECL_OVERRIDE;
};
typedef QQuickItemRenderer VideoRendererQQuickItem;
}
QML_DECLARE_TYPE(QtAV::QQuickItemRenderer)

#endif // QTAV_QML_QQUICKRENDERER_H
