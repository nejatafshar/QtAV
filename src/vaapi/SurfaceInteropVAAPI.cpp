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

#include "SurfaceInteropVAAPI.h"
#include "utils/OpenGLHelper.h"
#include "QtAV/VideoFrame.h"
#ifndef QT_OPENGL_ES_2
#include <va/va_x11.h>
#endif
namespace QtAV {
namespace vaapi {

void* SurfaceInteropVAAPI::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
    if (!fmt.isRGB())
        return 0;
    QMutexLocker lock(&mutex);
    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        return mapToTexture(fmt, handle, plane);
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void* SurfaceInteropVAAPI::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    int nb_fmts = vaMaxNumImageFormats(m_surface->display());
    //av_mallocz_array
    VAImageFormat *p_fmt = (VAImageFormat*)calloc(nb_fmts, sizeof(*p_fmt));
    if (!p_fmt) {
        return NULL;
    }
    if (vaQueryImageFormats(m_surface->display(), p_fmt, &nb_fmts)) {
        free(p_fmt);
        return NULL;
    }
    VAImage image;
    for (int i = 0; i < nb_fmts; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
            p_fmt[i].fourcc == VA_FOURCC_IYUV ||
            p_fmt[i].fourcc == VA_FOURCC_NV12) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(m_surface->display(), &p_fmt[i], m_surface->width(), m_surface->height(), &image) != VA_STATUS_SUCCESS) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(m_surface->display(), m_surface->get(), 0, 0, m_surface->width(), m_surface->height(), image.image_id) != VA_STATUS_SUCCESS) {
                vaDestroyImage(m_surface->display(), image.image_id);
                qDebug("vaGetImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                image.image_id = VA_INVALID_ID;
                continue;
            }
            break;
        }
    }
    free(p_fmt);
    if (image.image_id == VA_INVALID_ID)
        return NULL;
    void *p_base;
    VA_ENSURE_TRUE(vaMapBuffer(m_surface->display(), image.buf, &p_base), NULL);

    VideoFormat::PixelFormat pixfmt = VideoFormat::Format_Invalid;
    bool swap_uv = image.format.fourcc != VA_FOURCC_NV12;
    switch (image.format.fourcc) {
    case VA_FOURCC_YV12:
    case VA_FOURCC_IYUV:
        pixfmt = VideoFormat::Format_YUV420P;
        break;
    case VA_FOURCC_NV12:
        pixfmt = VideoFormat::Format_NV12;
        break;
    default:
        break;
    }
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vaapi pixel format: %#x", image.format.fourcc);
        vaDestroyImage(m_surface->display(), image.image_id);
        return NULL;
    }
    const VideoFormat fmt(pixfmt);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        src[i] = (uint8_t*)p_base + image.offsets[i];
        pitch[i] = image.pitches[i];
    }
    if (swap_uv) {
        std::swap(src[1], src[2]);
        std::swap(pitch[1], pitch[2]);
    }
    VideoFrame frame = VideoFrame(m_surface->width(), m_surface->height(), fmt);
    frame.setBits(src);
    frame.setBytesPerLine(pitch);
    frame = frame.to(format);

    VAWARN(vaUnmapBuffer(m_surface->display(), image.buf));
    vaDestroyImage(m_surface->display(), image.image_id);
    image.image_id = VA_INVALID_ID;
    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    *f = frame;
    return f;
}

VAAPI_GLX_Interop::VAAPI_GLX_Interop() : SurfaceInteropVAAPI()
{
}

surface_glx_ptr VAAPI_GLX_Interop::createGLXSurface(void *handle)
{
    GLuint tex = *((GLuint*)handle);
    surface_glx_ptr glx(new surface_glx_t());
    glx->set(m_surface);
    if (!glx->create(tex))
        return surface_glx_ptr();
    glx_surfaces[(GLuint*)handle] = glx;
    return glx;
}

void* VAAPI_GLX_Interop::mapToTexture(const VideoFormat &fmt, void *handle, int plane)
{
    Q_UNUSED(fmt);
    Q_UNUSED(plane);
    surface_glx_ptr glx = glx_surfaces[(GLuint*)handle];
    if (!glx) {
        glx = createGLXSurface(handle);
        if (!glx) {
            qWarning("Fail to create vaapi glx surface");
            return 0;
        }
    }
    glx->set(m_surface);
    if (!glx->copy())
        return 0;
    VAWARN(vaSyncSurface(m_surface->display(), m_surface->get()));
    return handle;
}

#ifndef QT_OPENGL_ES_2
VAAPI_X_GLX_Interop::glXReleaseTexImage_t VAAPI_X_GLX_Interop::glXReleaseTexImage = 0;
VAAPI_X_GLX_Interop::glXBindTexImage_t VAAPI_X_GLX_Interop::glXBindTexImage = 0;

VAAPI_X_GLX_Interop::VAAPI_X_GLX_Interop()
    : SurfaceInteropVAAPI()
    , xdisplay(0)
    , fbc(0)
    , pixmap(0)
    , glxpixmap(0)
    , width(0)
    , height(0)
{}

VAAPI_X_GLX_Interop::~VAAPI_X_GLX_Interop()
{
    if (glxpixmap) {
        glXReleaseTexImage(xdisplay, glxpixmap, GLX_FRONT_EXT);
        XSync((::Display*)xdisplay, False);
        glXDestroyPixmap((::Display*)xdisplay, glxpixmap);
    }
    glxpixmap = 0;

    if (pixmap)
        XFreePixmap((::Display*)xdisplay, pixmap);
    pixmap = 0;
}

bool VAAPI_X_GLX_Interop::ensureGLX()
{
    if (fbc)
        return true;
    xdisplay = (Display*)glXGetCurrentDisplay();
    if (!xdisplay)
        return false;
    int xscr = DefaultScreen(xdisplay);
    const char *glxext = glXQueryExtensionsString((::Display*)xdisplay, xscr);
    if (!glxext || !strstr(glxext, "GLX_EXT_texture_from_pixmap"))
        return false;
    glXBindTexImage = (glXBindTexImage_t)glXGetProcAddressARB((const GLubyte*)"glXBindTexImageEXT");
    if (!glXBindTexImage) {
        qWarning("glXBindTexImage not found");
        return false;
    }
    glXReleaseTexImage = (glXReleaseTexImage_t)glXGetProcAddressARB((const GLubyte*)"glXReleaseTexImageEXT");
    if (!glXReleaseTexImage) {
        qWarning("glXReleaseTexImage not found");
        return false;
    }
    int attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, //xbmc
        GLX_X_RENDERABLE, True, //xbmc
        GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
        GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
        GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
        GLX_Y_INVERTED_EXT, True,
        GLX_DOUBLEBUFFER, False,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        None
    };

    int fbcount;
    GLXFBConfig *fbcs = glXChooseFBConfig((::Display*)xdisplay, xscr, attribs, &fbcount);
    if (!fbcount) {
        qWarning("No texture-from-pixmap support");
        return false;
    }
    if (fbcount)
        fbc = fbcs[0];
    XFree(fbcs);
    return true;
}

bool VAAPI_X_GLX_Interop::ensurePixmaps(int w, int h)
{
    if (pixmap && width == w && height == h)
        return true;
    if (!ensureGLX()) {
        return false;
    }

    XWindowAttributes xwa;
    XGetWindowAttributes((::Display*)xdisplay, RootWindow((::Display*)xdisplay, DefaultScreen((::Display*)xdisplay)), &xwa);
    pixmap = XCreatePixmap((::Display*)xdisplay, RootWindow((::Display*)xdisplay, DefaultScreen((::Display*)xdisplay)), w, h, xwa.depth);
    qDebug("XCreatePixmap: %lu, depth: %d", pixmap, xwa.depth);
    if (!pixmap) {
        qWarning("VAAPI_X_GLX_Interop could not create pixmap");
        return false;
    }

    int attribs[] = {
        GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
        GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
        GLX_MIPMAP_TEXTURE_EXT, False,
        None,
    };
    glxpixmap = glXCreatePixmap((::Display*)xdisplay, fbc, pixmap, attribs);
    width = w;
    height = h;
    return true;
}

void* VAAPI_X_GLX_Interop::mapToTexture(const VideoFormat &fmt, void *handle, int plane)
{
    Q_UNUSED(fmt);
    Q_UNUSED(plane);
    if (!ensurePixmaps(m_surface->width(), m_surface->height()))
        return 0;
    VAWARN(vaSyncSurface(m_surface->display(), m_surface->get()));

    VA_ENSURE_TRUE(vaPutSurface(m_surface->display(), m_surface->get(), pixmap
                                , 0, 0, m_surface->width(), m_surface->height()
                                , 0, 0, m_surface->width(), m_surface->height()
                                , NULL, 0, VA_FRAME_PICTURE | VA_SRC_BT709)
                   , NULL);

    XSync((::Display*)xdisplay, False);
    DYGL(glBindTexture(GL_TEXTURE_2D, *((GLuint*)handle)));
    glXBindTexImage(xdisplay, glxpixmap, GLX_FRONT_EXT, NULL);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));

    return handle;
}

void VAAPI_X_GLX_Interop::unmap(void *handle)
{
    DYGL(glBindTexture(GL_TEXTURE_2D, *((GLuint*)handle)));
    glXReleaseTexImage(xdisplay, glxpixmap, GLX_FRONT_EXT);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
}

#endif //QT_OPENGL_ES_2
} //namespace QtAV
} //namespace vaapi

