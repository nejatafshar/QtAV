/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QtAVWidgets/OpenGLWidgetRenderer.h"
#include "QtAV/private/OpenGLRendererBase_p.h"
#include <QResizeEvent>

namespace QtAV {

class OpenGLWidgetRendererPrivate : public OpenGLRendererBasePrivate
{
public:
    OpenGLWidgetRendererPrivate(QPaintDevice *pd)
        : OpenGLRendererBasePrivate(pd)
    {}
};

VideoRendererId OpenGLWidgetRenderer::id() const
{
    return VideoRendererId_OpenGLWidget;
}

OpenGLWidgetRenderer::OpenGLWidgetRenderer(QWidget *parent, Qt::WindowFlags f):
    QOpenGLWidget(parent, f)
  , OpenGLRendererBase(*new OpenGLWidgetRendererPrivate(this))
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

void OpenGLWidgetRenderer::initializeGL()
{
    onInitializeGL();
}

void OpenGLWidgetRenderer::paintGL()
{
    onPaintGL();
}

void OpenGLWidgetRenderer::resizeGL(int w, int h)
{
    onResizeGL(w, h);
}

void OpenGLWidgetRenderer::resizeEvent(QResizeEvent *e)
{
    onResizeEvent(e->size().width(), e->size().height());
    QOpenGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

void OpenGLWidgetRenderer::showEvent(QShowEvent *)
{
    onShowEvent();
}

} //namespace QtAV
