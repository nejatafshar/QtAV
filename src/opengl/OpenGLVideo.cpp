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

#include "QtAV/OpenGLVideo.h"
#include <QtGui/QColor>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QSurface>
#endif //5.0
#include "QtAV/SurfaceInterop.h"
#include "QtAV/VideoShader.h"
#include "ShaderManager.h"
#include "opengl/GeometryRenderer.h"
#include "opengl/OpenGLHelper.h"
#include "utils/Logger.h"

namespace QtAV {

// FIXME: why crash if inherits both QObject and DPtrPrivate?
class OpenGLVideoPrivate : public DPtrPrivate<OpenGLVideo>
{
public:
    OpenGLVideoPrivate()
        : ctx(0)
        , manager(0)
        , material(new VideoMaterial())
        , material_type(0)
        , norm_viewport(true)
        , update_geo(true)
        , tex_target(0)
        , valiad_tex_width(1.0)
        , user_shader(NULL)
    {
    }
    ~OpenGLVideoPrivate() {
        if (material) {
            delete material;
            material = 0;
        }
    }

    void resetGL() {
        ctx = 0;
        gr.updateGeometry(NULL);
        if (!manager)
            return;
        manager->setParent(0);
        delete manager;
        manager = 0;
        if (material) {
            delete material;
            material = 0;
        }
    }
    // update geometry(vertex array) set attributes or bind VAO/VBO.
    void updateGeometry(VideoShader* shader, const QRectF& t, const QRectF& r);
public:
    QOpenGLContext *ctx;
    ShaderManager *manager;
    VideoMaterial *material;
    qint64 material_type;
    bool norm_viewport;
    bool has_a;
    bool update_geo;
    int tex_target;
    qreal valiad_tex_width;
    QSize video_size;
    QRectF target;
    QRectF roi; //including invalid padding width
    TexturedGeometry geometry;
    GeometryRenderer gr;
    QRectF rect;
    QMatrix4x4 matrix;
    VideoShader *user_shader;
};

void OpenGLVideoPrivate::updateGeometry(VideoShader* shader, const QRectF &t, const QRectF &r)
{
    // also check size change for normalizedROI computation if roi is not normalized
    const bool roi_changed = valiad_tex_width != material->validTextureWidth() || roi != r || video_size != material->frameSize();
    const int tc = shader->textureLocationCount();
    if (roi_changed) {
        roi = r;
        valiad_tex_width = material->validTextureWidth();
        video_size = material->frameSize();
    }
    if (tex_target != shader->textureTarget()) {
        tex_target = shader->textureTarget();
        update_geo = true;
    }
    // (-1, -1, 2, 2) must flip y
    QRectF target_rect = norm_viewport ? QRectF(-1, 1, 2, -2) : rect;
    if (target.isValid()) {
        if (roi_changed || target != t) {
            target = t;
            update_geo = true;
            //target_rect = target (if valid). // relate to gvf bug?
        }
    } else {
        if (roi_changed) {
            update_geo = true;
        }
    }
    if (!update_geo)
        return;
    //qDebug("updating geometry...");
    // setTextureCount may change the vertex data. Call it before setRect()
    qDebug() << "target rect: " << target_rect ;
    geometry.setTextureCount(shader->textureTarget() == GL_TEXTURE_RECTANGLE ? tc : 1);
    geometry.setRect(target_rect, material->mapToTexture(0, roi));
    if (shader->textureTarget() == GL_TEXTURE_RECTANGLE) {
        for (int i = 1; i < tc; ++i) {
            // tc can > planes, but that will compute chroma plane
            geometry.setTextureRect(material->mapToTexture(i, roi), i);
        }
    }
    update_geo = false;
    gr.updateGeometry(&geometry);
}

OpenGLVideo::OpenGLVideo() {}

bool OpenGLVideo::isSupported(VideoFormat::PixelFormat pixfmt)
{
    return pixfmt != VideoFormat::Format_RGB48BE;
}

void OpenGLVideo::setOpenGLContext(QOpenGLContext *ctx)
{
    DPTR_D(OpenGLVideo);
    if (d.ctx == ctx)
        return;
    qreal b = 0, c = 0, h = 0, s = 0;
    if (d.material) {
        b = d.material->brightness();
        c = d.material->contrast();
        h = d.material->hue();
        s = d.material->saturation();
        delete d.material;
        d.material = 0;
    }
    d.resetGL(); //TODO: is it ok to destroygl resources in another context?
    d.ctx = ctx; // Qt4: set to null in resetGL()
    if (!ctx) {
        return;
    }
    d.material = new VideoMaterial();
    d.material->setBrightness(b);
    d.material->setContrast(c);
    d.material->setHue(h);
    d.material->setSaturation(s);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    d.manager = ctx->findChild<ShaderManager*>(QStringLiteral("__qtav_shader_manager"));
    QSizeF surfaceSize = ctx->surface()->size();
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    surfaceSize *= ctx->screen()->devicePixelRatio();
#else
    surfaceSize *= qApp->devicePixelRatio(); //TODO: window()->devicePixelRatio() is the window screen's
#endif
#else
    QSizeF surfaceSize = QSizeF(ctx->device()->width(), ctx->device()->height());
#endif
    setProjectionMatrixToRect(QRectF(QPointF(), surfaceSize));
    if (d.manager)
        return;
    // TODO: what if ctx is delete?
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    d.manager = new ShaderManager(ctx);
    QObject::connect(ctx, SIGNAL(aboutToBeDestroyed()), this, SLOT(resetGL()), Qt::DirectConnection); //direct?
#else
    d.manager = new ShaderManager(this);
#endif
    d.manager->setObjectName(QStringLiteral("__qtav_shader_manager"));
    /// get gl info here because context is current(qt ensure it)
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
        /// check here with current context can ensure the right result. If the first check is in VideoShader/VideoMaterial/decoder or somewhere else, the context can be null
        bool v = OpenGLHelper::isOpenGLES();
        qDebug("Is OpenGLES: %d", v);
        v = OpenGLHelper::isEGL();
        qDebug("Is EGL: %d", v);
        const int glsl_ver = OpenGLHelper::GLSLVersion();
        qDebug("GLSL version: %d", glsl_ver);
        v = OpenGLHelper::isPBOSupported();
        qDebug("Has PBO: %d", v);
        v = OpenGLHelper::has16BitTexture();
        qDebug("Has 16bit texture: %d", v);
        v = OpenGLHelper::hasRG();
        qDebug("Has RG texture: %d", v);
        qDebug() << ctx->format();
    }
}

QOpenGLContext* OpenGLVideo::openGLContext()
{
    return d_func().ctx;
}

void OpenGLVideo::setCurrentFrame(const VideoFrame &frame)
{
    d_func().material->setCurrentFrame(frame);
    d_func().has_a = frame.format().hasAlpha();
}

void OpenGLVideo::setProjectionMatrixToRect(const QRectF &v)
{
    setViewport(v);
}

void OpenGLVideo::setViewport(const QRectF &r)
{
    DPTR_D(OpenGLVideo);
    d.rect = r;
    if (d.norm_viewport) {
        d.matrix.setToIdentity();
    } else {
        d.matrix.setToIdentity();
        d.matrix.ortho(r);
        d.update_geo = true; // even true for target_rect != d.rect
    }
    // Mirrored relative to the usual Qt coordinate system with origin in the top left corner.
    //mirrored = mat(0, 0) * mat(1, 1) - mat(0, 1) * mat(1, 0) > 0;
    if (d.ctx && d.ctx == QOpenGLContext::currentContext()) {
        DYGL(glViewport(d.rect.x(), d.rect.y(), d.rect.width(), d.rect.height()));
    }
}

void OpenGLVideo::setBrightness(qreal value)
{
    d_func().material->setBrightness(value);
}

void OpenGLVideo::setContrast(qreal value)
{
    d_func().material->setContrast(value);
}

void OpenGLVideo::setHue(qreal value)
{
    d_func().material->setHue(value);
}

void OpenGLVideo::setSaturation(qreal value)
{
    d_func().material->setSaturation(value);
}

void OpenGLVideo::setUserShader(VideoShader *shader)
{
    d_func().user_shader = shader;
}

VideoShader* OpenGLVideo::userShader() const
{
    return d_func().user_shader;
}

void OpenGLVideo::fill(const QColor &color)
{
    DYGL(glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    DYGL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void OpenGLVideo::render(const QRectF &target, const QRectF& roi, const QMatrix4x4& transform)
{
    DPTR_D(OpenGLVideo);
    Q_ASSERT(d.manager);
    Q_EMIT beforeRendering();
    DYGL(glViewport(d.rect.x(), d.rect.y(), d.rect.width(), d.rect.height())); // viewport was used in gpu filters is wrong, qt quick fbo item's is right(so must ensure setProjectionMatrixToRect was called correctly)
    const qint64 mt = d.material->type();
    if (d.material_type != mt) {
        qDebug() << "material changed: " << VideoMaterial::typeName(d.material_type) << " => " << VideoMaterial::typeName(mt);
        d.material_type = mt;
    }
    if (!d.material->bind()) // bind first because texture parameters(target) mapped from native buffer is unknown before it
        return;
    VideoShader *shader = d.user_shader;
    if (!shader)
        shader = d.manager->prepareMaterial(d.material, mt); //TODO: print shader type name if changed. prepareMaterial(,sample_code, pp_code)
    shader->update(d.material);
    shader->program()->setUniformValue(shader->matrixLocation(), transform*d.matrix);
    // uniform end. attribute begin
    d.updateGeometry(shader, target, roi);
    // normalize?
    const bool blending = d.has_a;
    if (blending) {
        DYGL(glEnable(GL_BLEND));
        gl().BlendFuncSeparate(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA); //
    }
    d.gr.render();
    if (blending)
        DYGL(glDisable(GL_BLEND));
    // d.shader->program()->release(); //glUseProgram(0)
    d.material->unbind();

    Q_EMIT afterRendering();
}

void OpenGLVideo::resetGL()
{
    qDebug("~~~~~~~~~resetGL %p. from sender %p", d_func().manager, sender());
    d_func().resetGL();
}

} //namespace QtAV
