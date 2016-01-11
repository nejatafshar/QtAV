/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/OpenGLRendererBase.h"
#include "QtAV/private/OpenGLRendererBase_p.h"
#include "QtAV/OpenGLVideo.h"
#include "QtAV/FilterContext.h"
#include <QResizeEvent>
#include "utils/OpenGLHelper.h"
#include "utils/Logger.h"

namespace QtAV {

OpenGLRendererBasePrivate::OpenGLRendererBasePrivate(QPaintDevice* pd)
    : painter(new QPainter())
{
    filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
    filter_context->paint_device = pd;
    filter_context->painter = painter;
}

OpenGLRendererBasePrivate::~OpenGLRendererBasePrivate() {
    if (painter) {
        delete painter;
        painter = 0;
    }
}

void OpenGLRendererBasePrivate::setupAspectRatio()
{
    matrix.setToIdentity();
    matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
    if (orientation)
        matrix.rotate(orientation, 0, 0, 1); // Z axis
}

OpenGLRendererBase::OpenGLRendererBase(OpenGLRendererBasePrivate &d)
    : VideoRenderer(d)
{
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
}

OpenGLRendererBase::~OpenGLRendererBase()
{
   d_func().glv.setOpenGLContext(0);
}

bool OpenGLRendererBase::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return OpenGLVideo::isSupported(pixfmt);
}

bool OpenGLRendererBase::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(OpenGLRendererBase);
    d.video_frame = frame;

    d.glv.setCurrentFrame(frame);

    updateUi(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

void OpenGLRendererBase::drawBackground()
{
    const QRegion bgRegion(backgroundRegion());
    if (bgRegion.isEmpty())
        return;
    d_func().glv.fill(backgroundColor());
}

void OpenGLRendererBase::drawFrame()
{
    DPTR_D(OpenGLRendererBase);
    QRect roi = realROI();
    //d.glv.render(QRectF(-1, 1, 2, -2), roi, d.matrix);
    // QRectF() means the whole viewport
    d.glv.render(QRectF(), roi, d.matrix);
}

void OpenGLRendererBase::onInitializeGL()
{
    DPTR_D(OpenGLRendererBase);
    //makeCurrent();
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    initializeOpenGLFunctions();
#endif
    QOpenGLContext *ctx = const_cast<QOpenGLContext*>(QOpenGLContext::currentContext()); //qt4 returns const
    d.glv.setOpenGLContext(ctx);
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    bool hasGLSL = QOpenGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", ctx->format().majorVersion(), ctx->format().minorVersion(), hasGLSL);  
    static bool sInfo = true;
    if (sInfo) {
        sInfo = false;
        qDebug("GL_VERSION: %s", DYGL(glGetString(GL_VERSION)));
        qDebug("GL_VENDOR: %s", DYGL(glGetString(GL_VENDOR)));
        qDebug("GL_RENDERER: %s", DYGL(glGetString(GL_RENDERER)));
        qDebug("GL_SHADING_LANGUAGE_VERSION: %s", DYGL(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    }
}

void OpenGLRendererBase::onPaintGL()
{
    DPTR_D(OpenGLRendererBase);
    /* we can mix gl and qpainter.
     * QPainter painter(this);
     * painter.beginNativePainting();
     * gl functions...
     * painter.endNativePainting();
     * swapBuffers();
     */
    handlePaintEvent();
    //context()->swapBuffers(this);
    if (d.painter && d.painter->isActive())
        d.painter->end();
}

void OpenGLRendererBase::onResizeGL(int w, int h)
{
    if (!QOpenGLContext::currentContext())
        return;
    DPTR_D(OpenGLRendererBase);
    glViewport(0, 0, w, h);
    d.glv.setProjectionMatrixToRect(QRectF(0, 0, w, h));
    d.setupAspectRatio();
}

void OpenGLRendererBase::onResizeEvent(int w, int h)
{
    DPTR_D(OpenGLRendererBase);
    d.update_background = true;
    resizeRenderer(w, h);
    d.setupAspectRatio();
    //QOpenGLWindow::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}
//TODO: out_rect not correct when top level changed
void OpenGLRendererBase::onShowEvent()
{
    DPTR_D(OpenGLRendererBase);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

void OpenGLRendererBase::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(OpenGLRendererBase);
    d.setupAspectRatio();
}

void OpenGLRendererBase::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(OpenGLRendererBase);
    d.setupAspectRatio();
}

bool OpenGLRendererBase::onSetOrientation(int value)
{
    Q_UNUSED(value)
    d_func().setupAspectRatio();
    return true;
}

bool OpenGLRendererBase::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool OpenGLRendererBase::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool OpenGLRendererBase::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool OpenGLRendererBase::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
}

} //namespace QtAV
