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

#ifndef QTAV_QUICKFBORENDERER_H
#define QTAV_QUICKFBORENDERER_H

#include <QtAV/VideoRenderer.h>
#include <QtQuick/QQuickFramebufferObject>

namespace QtAV {
class QuickFBORendererPrivate;
class QuickFBORenderer : public QQuickFramebufferObject, public VideoRenderer
{
    Q_OBJECT
    Q_DISABLE_COPY(QuickFBORenderer)
    DPTR_DECLARE_PRIVATE(QuickFBORenderer)
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

    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    explicit QuickFBORenderer(QQuickItem *parent = 0);
    virtual VideoRendererId id() const Q_DECL_OVERRIDE;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;

    QObject *source() const;
    void setSource(QObject *source);

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    bool isOpenGL() const;
    void setOpenGL(bool o);
    void fboSizeChanged(const QSize& size);
    void renderToFbo();
Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged(QuickFBORenderer::FillMode);
    void orientationChanged();
    void regionOfInterestChanged();
    void openGLChanged();    
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE;
    void frameSizeChanged();
protected:
    virtual bool event(QEvent *e) Q_DECL_OVERRIDE;
    virtual bool receiveFrame(const VideoFrame &frame) Q_DECL_OVERRIDE;
    virtual bool needUpdateBackground() const Q_DECL_OVERRIDE;
    virtual void drawBackground() Q_DECL_OVERRIDE;
    virtual bool needDrawFrame() const Q_DECL_OVERRIDE;
    virtual void drawFrame() Q_DECL_OVERRIDE;
private:
    virtual bool onSetRegionOfInterest(const QRectF& roi) Q_DECL_OVERRIDE;
    virtual bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    virtual void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    void onFrameSizeChanged(const QSize& size) Q_DECL_OVERRIDE;
    void updateRenderRect();
};
typedef QuickFBORenderer VideoRendererQuickFBO;
} //namespace QtAV
QML_DECLARE_TYPE(QtAV::QuickFBORenderer)
#endif // QTAV_QUICKFBORENDERER_H
