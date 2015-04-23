/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "OpenGLHelper.h"
#include <string.h> //strstr
#include <QtCore/QCoreApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
#include <QtOpenGL/QGLFunctions>
#endif
#include <QtOpenGL/QGLContext>
#define QOpenGLContext QGLContext
#endif
#include "utils/Logger.h"

namespace QtAV {
namespace OpenGLHelper {

bool hasExtension(const char *exts[])
{
    const QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return false;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    const char *ext = (const char*)glGetString(GL_EXTENSIONS);
    if (!ext)
        return false;
#endif
    for (int i = 0; exts[i]; ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        if (ctx->hasExtension(exts[i]))
#else
        if (strstr(ext, exts[i]))
#endif
            return true;
    }
    return false;
}

bool isPBOSupported() {
    // check pbo support
    static bool support = false;
    static bool pbo_checked = false;
    if (pbo_checked)
        return support;
    const QOpenGLContext *ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx);
    if (!ctx)
        return false;
    const char* exts[] = {
        "GL_ARB_pixel_buffer_object",
        "GL_EXT_pixel_buffer_object",
        "GL_NV_pixel_buffer_object", //OpenGL ES
        NULL
    };
    support = hasExtension(exts);
    qDebug() << "PBO: " << support;
    pbo_checked = true;
    return support;
}

// glActiveTexture in Qt4 on windows release mode crash for me
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_OPENGL_ES
// APIENTRY may be not defined(why? linux es2). or use QOPENGLF_APIENTRY
// use QGLF_APIENTRY for Qt4 crash, why? APIENTRY is defined in windows header
#ifndef APIENTRY
// QGLF_APIENTRY is in Qt4,8+
#if defined(QGLF_APIENTRY)
#define APIENTRY QGLF_APIENTRY
#elif defined(GL_APIENTRY)
#define APIENTRY GL_APIENTRY
#endif //QGLF_APIENTRY
#endif //APIENTRY
typedef void (APIENTRY *type_glActiveTexture) (GLenum);
static type_glActiveTexture qtav_glActiveTexture = 0;

static void qtavResolveActiveTexture()
{
    const QGLContext *context = QGLContext::currentContext();
    qtav_glActiveTexture = (type_glActiveTexture)context->getProcAddress(QLatin1String("glActiveTexture"));
    if (!qtav_glActiveTexture) {
        qDebug("resolve glActiveTextureARB");
        qtav_glActiveTexture = (type_glActiveTexture)context->getProcAddress(QLatin1String("glActiveTextureARB"));
    }
    //Q_ASSERT(qtav_glActiveTexture);
}
#endif //QT_OPENGL_ES
#endif //QT_VERSION

void glActiveTexture(GLenum texture)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_OPENGL_ES
    if (!qtav_glActiveTexture)
        qtavResolveActiveTexture();
    if (!qtav_glActiveTexture)
        return;
    qtav_glActiveTexture(texture);
#else
    ::glActiveTexture(texture);
#endif //QT_OPENGL_ES
#else
    QOpenGLContext::currentContext()->functions()->glActiveTexture(texture);
#endif
}


bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format, GLenum* data_type)
{
    typedef struct fmt_entry {
        VideoFormat::PixelFormat pixfmt;
        GLint internal_format;
        GLenum format;
        GLenum type;
    } fmt_entry;
    static const fmt_entry pixfmt_to_gles[] = {
        {VideoFormat::Format_ARGB32, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_RGB32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        // TODO:
        {VideoFormat::Format_BGRA32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_ABGR32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_Invalid, 0, 0, 0}
    };
    Q_UNUSED(pixfmt_to_gles);
    static const fmt_entry pixfmt_to_gl[] = {
        {VideoFormat::Format_RGB32,  GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_ARGB32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        // TODO:
        {VideoFormat::Format_ABGR32,  GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_BGRA32,  GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_Invalid, 0, 0, 0}
    };
    const fmt_entry *pixfmt_gl_entry = pixfmt_to_gl;
#ifdef QT_OPENGL_DYNAMIC
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    // desktop can create es compatible context
    const bool isES = qApp->testAttribute(Qt::AA_UseOpenGLES) || (ctx ? ctx->isOpenGLES() : QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL); //
    if (isES)
        pixfmt_gl_entry = pixfmt_to_gles;
#else
# ifdef QT_OPENGL_ES_2
    pixfmt_gl_entry = pixfmt_to_gles;
# endif //QT_OPENGL_ES_2
#endif //QT_OPENGL_DYNAMIC
    // Very special formats, for which OpenGL happens to have direct support
    static const fmt_entry pixfmt_gl_entry_common[] = {
        // TODO: review rgb formats & yuv packed to upload correct rgba
        {VideoFormat::Format_UYVY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_YUYV, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_VYUY, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_YVYU, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_RGBA32, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }, // only tested for osx, win, angle
        {VideoFormat::Format_ABGR32, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }, // only tested for osx, win, angle
        {VideoFormat::Format_RGB48, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_RGB48LE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_RGB48BE, GL_RGB, GL_RGB, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_BGR48, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_BGR48LE, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_BGR48BE, GL_BGR, GL_BGR, GL_UNSIGNED_SHORT },
        {VideoFormat::Format_RGB24,  GL_RGB,  GL_RGB,  GL_UNSIGNED_BYTE },
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_RGB555, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    #endif
        {VideoFormat::Format_RGB565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5}, //GL_UNSIGNED_SHORT_5_6_5_REV?
        //{VideoFormat::Format_BGRA32, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE },
        //{VideoFormat::Format_BGR32,  GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE },
        {VideoFormat::Format_BGR24,  GL_RGB,  GL_BGR,  GL_UNSIGNED_BYTE },
    #ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        {VideoFormat::Format_BGR555, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
    #endif
        {VideoFormat::Format_BGR565, GL_RGB,  GL_RGB,  GL_UNSIGNED_SHORT_5_6_5}, // need swap r b?
    };
    const VideoFormat::PixelFormat pixfmt = fmt.pixelFormat();
    for (unsigned int i = 0; pixfmt_gl_entry[i].pixfmt != VideoFormat::Format_Invalid; ++i) {
        if (pixfmt_gl_entry[i].pixfmt == pixfmt) {
            *internal_format = pixfmt_gl_entry[i].internal_format;
            *data_format = pixfmt_gl_entry[i].format;
            *data_type = pixfmt_gl_entry[i].type;
            return true;
        }
    }
    for (unsigned int i = 0; i < sizeof(pixfmt_gl_entry_common)/sizeof(pixfmt_gl_entry_common[0]); ++i) {
        if (pixfmt_gl_entry_common[i].pixfmt == pixfmt) {
            *internal_format = pixfmt_gl_entry_common[i].internal_format;
            *data_format = pixfmt_gl_entry_common[i].format;
            *data_type = pixfmt_gl_entry_common[i].type;
            return true;
        }
    }
    return false;
}

// TODO: format + datatype? internal format == format?
//https://www.khronos.org/registry/gles/extensions/EXT/EXT_texture_format_BGRA8888.txt
int bytesOfGLFormat(GLenum format, GLenum dataType)
{
    int component_size = 0;
    switch (dataType) {
#ifdef GL_UNSIGNED_INT_8_8_8_8_REV
    case GL_UNSIGNED_INT_8_8_8_8_REV:
        return 4;
#endif
#ifdef GL_UNSIGNED_BYTE_3_3_2
    case GL_UNSIGNED_BYTE_3_3_2:
        return 1;
#endif //GL_UNSIGNED_BYTE_3_3_2
#ifdef GL_UNSIGNED_BYTE_2_3_3_REV
    case GL_UNSIGNED_BYTE_2_3_3_REV:
            return 1;
#endif
        case GL_UNSIGNED_SHORT_5_5_5_1:
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
#endif //GL_UNSIGNED_SHORT_1_5_5_5_REV
#ifdef GL_UNSIGNED_SHORT_5_6_5_REV
        case GL_UNSIGNED_SHORT_5_6_5_REV:
#endif //GL_UNSIGNED_SHORT_5_6_5_REV
    case GL_UNSIGNED_SHORT_5_6_5: // gles
#ifdef GL_UNSIGNED_SHORT_4_4_4_4_REV
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
#endif //GL_UNSIGNED_SHORT_4_4_4_4_REV
    case GL_UNSIGNED_SHORT_4_4_4_4:
        return 2;
    case GL_UNSIGNED_BYTE:
        component_size = 1;
        break;
        // mpv returns 2
#ifdef GL_UNSIGNED_SHORT_8_8_APPLE
    case GL_UNSIGNED_SHORT_8_8_APPLE:
    case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
        return 2;
#endif
    case GL_UNSIGNED_SHORT:
        component_size = 2;
        break;
    }
    switch (format) {
#ifdef GL_YCBCR_422_APPLE
      case GL_YCBCR_422_APPLE:
        return 2;
#endif
#ifdef GL_RGB_422_APPLE
      case GL_RGB_422_APPLE:
        return 2;
#endif
#ifdef GL_BGRA //ifndef GL_ES
      case GL_BGRA:
#endif
      case GL_RGBA:
        return 4*component_size;
#ifdef GL_BGR //ifndef GL_ES
      case GL_BGR:
#endif
      case GL_RGB:
        return 3*component_size;
      case GL_LUMINANCE_ALPHA:
        // mpv returns 2
        return 2*component_size;
      case GL_LUMINANCE:
      case GL_ALPHA:
        return 1*component_size;
#ifdef GL_LUMINANCE16
    case GL_LUMINANCE16:
        return 2*component_size;
#endif //GL_LUMINANCE16
#ifdef GL_ALPHA16
    case GL_ALPHA16:
        return 2*component_size;
#endif //GL_ALPHA16
      default:
        qWarning("bytesOfGLFormat - Unknown format %u", format);
        return 1;
      }
}

GLint GetGLInternalFormat(GLint data_format, int bpp)
{
    if (bpp != 2)
        return data_format;
    switch (data_format) {
#ifdef GL_ALPHA16
    case GL_ALPHA:
        return GL_ALPHA16;
#endif //GL_ALPHA16
#ifdef GL_LUMINANCE16
    case GL_LUMINANCE:
        return GL_LUMINANCE16;
#endif //GL_LUMINANCE16
    default:
        return data_format;
    }
}

} //namespace OpenGLHelper
} //namespace QtAV
