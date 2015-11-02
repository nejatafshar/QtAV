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
#include "utils/Logger.h"
#if VA_X11_INTEROP
#ifdef QT_X11EXTRAS_LIB
#include <QtX11Extras/QX11Info>
#endif //QT_X11EXTRAS_LIB
#include <va/va_x11.h>
#if QTAV_HAVE(EGL_CAPI)
#define EGL_CAPI_NS
#include "capi/egl_api.h"
#include <EGL/eglext.h>
#if VA_CHECK_VERSION(0, 33, 0)
#include <va/va_drmcommon.h>
#else
/** \brief Kernel DRM buffer memory type.  */
#define VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM		0x10000000
/** \brief DRM PRIME memory type. */
#define VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME		0x20000000
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#if !defined(QT_OPENGL_ES_2)
#include <GL/glx.h>
#endif //!defined(QT_OPENGL_ES_2)
#endif //VA_X11_INTEROP

namespace QtAV {
VideoFormat::PixelFormat pixelFormatFromVA(uint32_t fourcc);
namespace vaapi {
void SurfaceInteropVAAPI::setSurface(const surface_ptr& surface,  int w, int h) {
    m_surface = surface;
    frame_width = (w ? w : surface->width());
    frame_height = (h ? h : surface->height());
}

void* SurfaceInteropVAAPI::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
   // if (!fmt.isRGB())
      //  return 0;

    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_surface, *((GLuint*)handle), frame_width, frame_height, plane))
            return handle;
        return NULL;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInteropVAAPI::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

void* SurfaceInteropVAAPI::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    int nb_fmts = vaMaxNumImageFormats(m_surface->vadisplay());
    //av_mallocz_array
    VAImageFormat *p_fmt = (VAImageFormat*)calloc(nb_fmts, sizeof(*p_fmt));
    if (!p_fmt) {
        return NULL;
    }
    if (vaQueryImageFormats(m_surface->vadisplay(), p_fmt, &nb_fmts)) {
        free(p_fmt);
        return NULL;
    }
    VAImage image;
    for (int i = 0; i < nb_fmts; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
            p_fmt[i].fourcc == VA_FOURCC_IYUV ||
            p_fmt[i].fourcc == VA_FOURCC_NV12) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(m_surface->vadisplay(), &p_fmt[i], m_surface->width(), m_surface->height(), &image) != VA_STATUS_SUCCESS) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(m_surface->vadisplay(), m_surface->get(), 0, 0, m_surface->width(), m_surface->height(), image.image_id) != VA_STATUS_SUCCESS) {
                vaDestroyImage(m_surface->vadisplay(), image.image_id);
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
    VA_ENSURE_TRUE(vaMapBuffer(m_surface->vadisplay(), image.buf, &p_base), NULL);

    VideoFormat::PixelFormat pixfmt = pixelFormatFromVA(image.format.fourcc);
    bool swap_uv = image.format.fourcc != VA_FOURCC_NV12;
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vaapi pixel format: %#x", image.format.fourcc);
        vaDestroyImage(m_surface->vadisplay(), image.image_id);
        return NULL;
    }
    const VideoFormat fmt(pixfmt);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        src[i] = (uint8_t*)p_base + image.offsets[i];
        pitch[i] = image.pitches[i];
    }
    VideoFrame frame = VideoFrame::fromGPU(fmt, frame_width, frame_height, m_surface->height(), src, pitch, true, swap_uv);
    if (format != fmt)
        frame = frame.to(format);
    VAWARN(vaUnmapBuffer(m_surface->vadisplay(), image.buf));
    vaDestroyImage(m_surface->vadisplay(), image.image_id);
    image.image_id = VA_INVALID_ID;
    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    *f = frame;
    return f;
}
#ifndef QT_NO_OPENGL
surface_glx_ptr GLXInteropResource::surfaceGLX(const display_ptr &dpy, GLuint tex)
{
    surface_glx_ptr glx = glx_surfaces[tex];
    if (glx)
        return glx;
    glx = surface_glx_ptr(new surface_glx_t(dpy));
    if (!glx->create(tex))
        return surface_glx_ptr();
    glx_surfaces[tex] = glx;
    return glx;
}

bool GLXInteropResource::map(const surface_ptr& surface, GLuint tex, int w, int h, int)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    surface_glx_ptr glx = surfaceGLX(surface->display(), tex);
    if (!glx) {
        qWarning("Fail to create vaapi glx surface");
        return false;
    }
    if (!glx->copy(surface))
        return false;
    VAWARN(vaSyncSurface(surface->vadisplay(), surface->get()));
    return true;
}
#endif //QT_NO_OPENGL
#if VA_X11_INTEROP
class X11 {
protected:
    Display *display;
public:
    X11() : display(NULL), pixmap(0) {}
    virtual ~X11() {
        if (pixmap)
            XFreePixmap((::Display*)display, pixmap);
        pixmap = 0;
    }
    virtual Display* ensureGL() = 0;
    virtual bool bindPixmap(int w, int h) = 0;
    virtual void bindTexture() = 0;
    Pixmap pixmap;
protected:
    int createPixmap(int w, int h) {
        if (pixmap) {
            qDebug("XFreePixmap");
            XFreePixmap((::Display*)display, pixmap);
            pixmap = 0;
        }
        XWindowAttributes xwa;
        XGetWindowAttributes((::Display*)display, DefaultRootWindow((::Display*)display), &xwa);
        pixmap = XCreatePixmap((::Display*)display, DefaultRootWindow((::Display*)display), w, h, xwa.depth);
        // mpv always use 24 bpp
        qDebug("XCreatePixmap %lu: %dx%d, depth: %d", pixmap, w, h, xwa.depth);
        if (!pixmap) {
            qWarning("X11InteropResource could not create pixmap");
            return 0;
        }
        return xwa.depth;
    }
};

#define RESOLVE_FUNC(func, resolver, st) do {\
    if (!func) { \
        typedef void (*ft)(void); \
        ft f = resolver((st)#func); \
        if (!f) { \
            qWarning(#func " is not available"); \
            return 0; \
        } \
        ft *fp = (ft*)(&func); \
        *fp = f; \
    }} while(0)
#define RESOLVE_EGL(func) RESOLVE_FUNC(func, eglGetProcAddress, const char*)
#define RESOLVE_GLX(func) RESOLVE_FUNC(func, glXGetProcAddressARB, const GLubyte*)

#if QTAV_HAVE(EGL_CAPI)
//static PFNEGLQUERYNATIVEDISPLAYNVPROC eglQueryNativeDisplayNV = NULL;
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;
class X11_EGL Q_DECL_FINAL: public X11 {
public:
    X11_EGL() : dpy(EGL_NO_DISPLAY) {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i)
            image[i] = EGL_NO_IMAGE_KHR;
    }
    ~X11_EGL() {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i) {
            if (image[i] != EGL_NO_IMAGE_KHR) {
                EGL_WARN(eglDestroyImageKHR(dpy, image[i]));
            }
        }
    }
    Display* ensureGL() Q_DECL_OVERRIDE {
        if (display && eglCreateImageKHR && glEGLImageTargetTexture2DOES)
            return display;
        // we must use the native display egl used, otherwise eglCreateImageKHR will fail.
#ifdef QT_X11EXTRAS_LIB
        display = (Display*)QX11Info::display();
#endif //QT_X11EXTRAS_LIB
        //RESOLVE_EGL(eglQueryNativeDisplayNV);
        RESOLVE_EGL(glEGLImageTargetTexture2DOES);
        RESOLVE_EGL(eglCreateImageKHR);
        RESOLVE_EGL(eglDestroyImageKHR);
        return display;
    }
    bool bindPixmap(int w, int h) Q_DECL_OVERRIDE {
        if (!createPixmap(w, h))
            return false;
        if (dpy == EGL_NO_DISPLAY) {
            qDebug("eglGetCurrentDisplay");
            dpy = eglGetCurrentDisplay();
        }
        EGL_ENSURE(image[0] =  eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmap, NULL), false);
        return true;
    }
    void bindTexture() Q_DECL_OVERRIDE {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image[0]);
    }
    EGLDisplay dpy;
    EGLImageKHR image[4];
};
#endif //QTAV_HAVE(EGL_CAPI)
#if !defined(QT_OPENGL_ES_2)
typedef void (*glXBindTexImageEXT_t)(Display *dpy, GLXDrawable draw, int buffer, int *a);
typedef void (*glXReleaseTexImageEXT_t)(Display *dpy, GLXDrawable draw, int buffer);
static glXReleaseTexImageEXT_t glXReleaseTexImageEXT = 0;
static glXBindTexImageEXT_t glXBindTexImageEXT = 0;

class X11_GLX Q_DECL_FINAL: public X11 {
public:
    X11_GLX() : fbc(0), glxpixmap(0) {}
    ~X11_GLX() {
        if (glxpixmap) { //TODO: does the thread matters?
            glXReleaseTexImageEXT(display, glxpixmap, GLX_FRONT_EXT);
            XSync((::Display*)display, False);
            glXDestroyPixmap((::Display*)display, glxpixmap);
        }
        glxpixmap = 0;
    }
    Display* ensureGL() Q_DECL_OVERRIDE {
        if (fbc && display)
            return display;
        if (!display) {
            qDebug("glXGetCurrentDisplay");
            display = (Display*)glXGetCurrentDisplay();
            if (!display)
                return 0;
        }
        int xscr = DefaultScreen(display);
        const char *glxext = glXQueryExtensionsString((::Display*)display, xscr);
        if (!glxext || !strstr(glxext, "GLX_EXT_texture_from_pixmap"))
            return 0;
        RESOLVE_GLX(glXBindTexImageEXT);
        RESOLVE_GLX(glXReleaseTexImageEXT);
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
            GLX_ALPHA_SIZE, 8, //0 for 24 bpp(vdpau)? mpv is 0
            None
        };
        int fbcount;
        GLXFBConfig *fbcs = glXChooseFBConfig((::Display*)display, xscr, attribs, &fbcount);
        if (!fbcount) {
            qWarning("No texture-from-pixmap support");
            return 0;
        }
        if (fbcount)
            fbc = fbcs[0];
        XFree(fbcs);
        return display;
    }
    bool bindPixmap(int w, int h) Q_DECL_OVERRIDE {
        const int depth = createPixmap(w, h);
        if (depth <= 0)
            return false;
        const int attribs[] = {
            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
            GLX_TEXTURE_FORMAT_EXT, depth == 32 ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT,
            GLX_MIPMAP_TEXTURE_EXT, False,
            None,
        };
        glxpixmap = glXCreatePixmap((::Display*)display, fbc, pixmap, attribs);
        return true;
    }
    void bindTexture() Q_DECL_OVERRIDE {
        glXBindTexImageEXT(display, glxpixmap, GLX_FRONT_EXT, NULL);
    }
    GLXFBConfig fbc;
    GLXPixmap glxpixmap;
};
#endif // !defined(QT_OPENGL_ES_2)

X11InteropResource::X11InteropResource()
    : InteropResource()
    , VAAPI_X11()
    , xdisplay(NULL)
    , width(0)
    , height(0)
    , x11(NULL)
{
    qDebug("X11InteropResource");
}

X11InteropResource::~X11InteropResource()
{
    delete x11;
}

bool X11InteropResource::ensurePixmaps(int w, int h)
{
    if (width == w && height == h)
        return true;
    if (!x11) {
#if QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_ES_2)
        static const bool use_egl = true;
#else
        // FIXME: may fallback to xcb_glx(default)
        static const bool use_egl = qgetenv("QT_XCB_GL_INTEGRATION") == "xcb_egl";
#endif //defined(QT_OPENGL_ES_2)
        if (use_egl) {
            // TODO: check EGL_NOK_texture_from_pixmap
            x11 = new X11_EGL();
        } else
#endif //QTAV_HAVE(EGL_CAPI)
        {
#if !defined(QT_OPENGL_ES_2)
            // TODO: check GLX_EXT_texture_from_pixmap
            x11 = new X11_GLX();
#endif // defined(QT_OPENGL_ES_2)
        }
    }
    if (!x11) {
        qWarning("no EGL and GLX interop (TFP) support");
        return false;
    }
    xdisplay  = x11->ensureGL();
    if (!xdisplay)
        return false;
    if (!x11->bindPixmap(w, h))
        return false;
    width = w;
    height = h;
    return true;
}

bool X11InteropResource::map(const surface_ptr& surface, GLuint tex, int w, int h, int)
{
    if (surface->width() <= 0 || surface->height() <= 0) {
        qWarning("invalid surface size");
        return false;
    }
    if (!ensurePixmaps(w, h)) //pixmap with frame size
        return false;
    VAWARN(vaSyncSurface(surface->vadisplay(), surface->get()));
    // FIXME: invalid surface at the first time vaPutSurface is called. If return false, vaPutSurface will always fail, why?
    VAWARN(vaPutSurface(surface->vadisplay(), surface->get(), x11->pixmap
                                , 0, 0, w, h
                                , 0, 0, w, h
                                , NULL, 0, VA_FRAME_PICTURE | surface->colorSpace())
                   );
    XSync((::Display*)xdisplay, False);
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    x11->bindTexture();
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}

bool X11InteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
    // can not call glXReleaseTexImageEXT otherwise the texture will containts no image data
    return true;
}
#endif //VA_X11_INTEROP

#if QTAV_HAVE(EGL_CAPI)
#if VA_CHECK_VERSION(0, 38, 0)
#ifndef EGL_LINUX_DMA_BUF_EXT
#define EGL_LINUX_DMA_BUF_EXT 0x3270
#define EGL_LINUX_DRM_FOURCC_EXT          0x3271
#define EGL_DMA_BUF_PLANE0_FD_EXT         0x3272
#define EGL_DMA_BUF_PLANE0_OFFSET_EXT     0x3273
#define EGL_DMA_BUF_PLANE0_PITCH_EXT      0x3274
#endif
class EGL {
public:
    EGL() : dpy(EGL_NO_DISPLAY) {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i)
            image[i] = EGL_NO_IMAGE_KHR;
    }
    ~EGL() {
        for (unsigned i = 0; i < sizeof(image)/sizeof(image[0]); ++i) {
            destroyImages(i);
        }
    }
    void destroyImages(int plane) {
        qDebug("destroyImage %d image:%p, this: %p", plane, image,this);
        if (image[plane] != EGL_NO_IMAGE_KHR) {
            EGL_WARN(eglDestroyImageKHR(dpy, image[plane]));
            image[plane] = EGL_NO_IMAGE_KHR;
        }
    }
    bool ensureGL() {
        if (eglCreateImageKHR && eglDestroyImageKHR && glEGLImageTargetTexture2DOES)
            return true;
        RESOLVE_EGL(glEGLImageTargetTexture2DOES);
        RESOLVE_EGL(eglCreateImageKHR);
        RESOLVE_EGL(eglDestroyImageKHR);
        return eglCreateImageKHR && eglDestroyImageKHR && glEGLImageTargetTexture2DOES;
    }
    bool bindImage(int plane, const EGLint *attrib_list) {
        if (dpy == EGL_NO_DISPLAY) {
            qDebug("eglGetCurrentDisplay");
            dpy = eglGetCurrentDisplay();
        }
        EGL_ENSURE(image[plane] =  eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attrib_list), false);
        return true;
    }
    void bindTexture(int plane) {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image[plane]);
    }
    EGLDisplay dpy;
    EGLImageKHR image[4];
};
EGLInteropResource::EGLInteropResource()
    : InteropResource()
    , vabuf_handle(0)
    , va_dpy(0)
    , egl(0)
{
    va_image.buf = va_image.image_id = VA_INVALID_ID;
}

EGLInteropResource::~EGLInteropResource()
{
    destroy();
    delete egl; // TODO: thread
}

// return the new entry
static EGLint* add_egl_attribute(EGLint* a, EGLint key, EGLint value)
{
    a[0] = key;
    a[1] = value;
    a[2] = EGL_NONE;
    printf("%d:%d ", key, value);
    return a+3;
}

bool EGLInteropResource::map(const surface_ptr &surface, GLuint tex, int w, int h, int plane)
{
    if (!ensure())
        return false;
    qDebug("EGLInteropResource.map %d", plane);
    va_dpy = surface->display();
    if (va_image.image_id == VA_INVALID_ID) {
        VA_ENSURE(vaDeriveImage(va_dpy->get(), surface->get(), &va_image), false);
    }
    if (!vabuf_handle) {
        VABufferInfo vabuf;
        memset(&vabuf, 0, sizeof(vabuf));
        vabuf.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        VA_ENSURE(vaAcquireBufferHandle(va_dpy->get(), va_image.buf, &vabuf), false);
        vabuf_handle = vabuf.handle;
    }
    // (it would be nice if we could use EGL_IMAGE_INTERNAL_FORMAT_EXT)
#define FOURCC(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((unsigned)(d)<<24))
    static const int drm_fmts[] = {
        FOURCC('R', '8', ' ', ' '),   // DRM_FORMAT_R8
        FOURCC('G', 'R', '8', '8'),   // DRM_FORMAT_GR88
        FOURCC('R', 'G', '2', '4'),   // DRM_FORMAT_RGB888
        FOURCC('R', 'A', '2', '4')  // DRM_FORMAT_RGBA8888
    };
#undef FOURCC
    EGLint attribs[20] = {EGL_NONE};
    EGLint *a = attribs;
    // TODO: yv12 swap uv
    const VideoFormat fmt(pixelFormatFromVA(va_image.format.fourcc));
    a = add_egl_attribute(a, EGL_LINUX_DRM_FOURCC_EXT, drm_fmts[fmt.bytesPerPixel(plane) - 1]);
    a = add_egl_attribute(a, EGL_WIDTH, fmt.width(surface->width(), plane));
    a = add_egl_attribute(a, EGL_HEIGHT, fmt.height(surface->height(), plane));
    a = add_egl_attribute(a, EGL_DMA_BUF_PLANE0_FD_EXT, vabuf_handle);
    a = add_egl_attribute(a, EGL_DMA_BUF_PLANE0_OFFSET_EXT, va_image.offsets[plane]);
    a = add_egl_attribute(a, EGL_DMA_BUF_PLANE0_PITCH_EXT, va_image.pitches[plane]);
    printf("---\n");
    if (!egl->bindImage(plane, attribs))
        return false;
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    egl->bindTexture(plane);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    mapped.insert(tex, plane);
    return true;
}

bool EGLInteropResource::unmap(GLuint tex)
{
    if (!egl)
        return false;
    if (!mapped.contains(tex)) {
        if (mapped.isEmpty()) {
            destroy();
        }
        return false;
    }
    const int plane = mapped.value(tex);
    egl->destroyImages(plane);
    mapped.remove(tex);
    qDebug("unmap plane %d", plane);
    if (mapped.isEmpty()) {
        destroy();
    }
    return true;
}

void EGLInteropResource::destroy()
{
    if (va_image.buf != VA_INVALID_ID) {
        VAWARN(vaReleaseBufferHandle(va_dpy->get(), va_image.buf));
        va_image.buf = VA_INVALID_ID;
        vabuf_handle = 0;
        qDebug("vabuf_handle: %#x", vabuf_handle);
    }
    if (va_image.image_id != VA_INVALID_ID) {
        VAWARN(vaDestroyImage(va_dpy->get(), va_image.image_id));
        va_image.image_id = VA_INVALID_ID;
    }
}

bool EGLInteropResource::ensure()
{
    if (egl)
        return true;
    qDebug("EGLInteropResource.ensure");
#if QTAV_HAVE(EGL_CAPI)
#if !defined(QT_OPENGL_ES_2)
    // FIXME: may fallback to xcb_glx(default)
    static const bool use_egl = qgetenv("QT_XCB_GL_INTEGRATION") == "xcb_egl";
    if (!use_egl) {
        qWarning("Not using EGL");
        return false;
    }
#endif //defined(QT_OPENGL_ES_2)
#else
    qWarning("build QtAV with capi is required");
    return false;
#endif //QTAV_HAVE(EGL_CAPI)
    static bool has_ext = false;
    if (!has_ext) {
        QList<QByteArray> eglexts = QByteArray(eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS)).split(' ');
        static QVector<QByteArray> eglexts_required= QVector<QByteArray>() << QByteArrayLiteral("EGL_EXT_image_dma_buf_import")
                                                           << QByteArrayLiteral("EGL_EXT_image_dma_buf_import");
        qDebug() << eglexts;
        foreach (const QByteArray& ext, eglexts_required) {
            if (!eglexts.contains(ext)) {
                qWarning("missing extension: %s", ext.constData());
                return false;
            }
        }
        static const char* glexts[] = { "GL_OES_EGL_image", 0 };
        if (!OpenGLHelper::hasExtension(glexts)) {
            qWarning("missing extension: GL_OES_EGL_image");
            return false;
        }
        has_ext = true;
    }
    if (!egl)
        egl = new EGL();
    if (!egl->ensureGL())
        return false;
    return true;
}
#endif //VA_CHECK_VERSION(0, 38, 0)
#endif //QTAV_HAVE(EGL_CAPI)
} //namespace QtAV
} //namespace vaapi
