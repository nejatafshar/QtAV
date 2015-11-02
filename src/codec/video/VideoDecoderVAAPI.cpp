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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include <algorithm>
#include <list>
#include <QtCore/QList>
#include <QtCore/QMetaEnum>
#include <QtCore/QStringList>
#include <QtCore/QThread>
extern "C" {
#include <libavcodec/vaapi.h>
}
#include <fcntl.h> //open()
#include <unistd.h> //close()
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "vaapi/SurfaceInteropVAAPI.h"
#include "utils/Logger.h"

#define VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

//ffmpeg_vaapi patch: http://lists.libav.org/pipermail/libav-devel/2013-November/053515.html

namespace QtAV {
using namespace vaapi;

class VideoDecoderVAAPIPrivate;
class VideoDecoderVAAPI : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVAAPI)
    Q_PROPERTY(bool derive READ derive WRITE setDerive)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
    //Q_PROPERTY(QStringList displayPriority READ displayPriority WRITE setDisplayPriority)
    Q_PROPERTY(DisplayType display READ display WRITE setDisplay)
    Q_ENUMS(DisplayType)
public:
        // TODO: interopType: GLX, X11, DMA, DRM
    enum DisplayType {
        X11,
        GLX,
        DRM,
        EGL
    };
    VideoDecoderVAAPI();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;

    // QObject properties
    void setDerive(bool y);
    bool derive() const;
    void setSurfaces(int num);
    int surfaces() const;
    void setDisplayPriority(const QStringList& priority);
    QStringList displayPriority() const;
    DisplayType display() const;
    void setDisplay(DisplayType disp);
};
extern VideoDecoderId VideoDecoderId_VAAPI;
FACTORY_REGISTER(VideoDecoder, VAAPI, "VAAPI")

const char* getProfileName(AVCodecID id, int profile)
{
    AVCodec *c = avcodec_find_decoder(id);
    if (!c)
        return "Unknow";
    return av_get_profile_name(c, profile);
}

typedef struct {
    AVCodecID codec;
    int profile;
    VAProfile va_profile; //TODO: use an array like dxva does
} codec_profile_t;
#define VAProfileNone ((VAProfile)-1) //maybe not defined for old va

static const  codec_profile_t va_profiles[] = {
    { QTAV_CODEC_ID(MPEG1VIDEO), FF_PROFILE_UNKNOWN, VAProfileMPEG2Main }, //vlc
    { QTAV_CODEC_ID(MPEG2VIDEO), FF_PROFILE_MPEG2_MAIN, VAProfileMPEG2Main },
    { QTAV_CODEC_ID(MPEG2VIDEO),  FF_PROFILE_MPEG2_SIMPLE, VAProfileMPEG2Simple },
    { QTAV_CODEC_ID(H263), FF_PROFILE_UNKNOWN, VAProfileH263Baseline }, //xbmc use mpeg4
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_ADVANCED_SIMPLE, VAProfileMPEG4AdvancedSimple },
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_MAIN, VAProfileMPEG4Main },
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_SIMPLE, VAProfileMPEG4Simple },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_HIGH, VAProfileH264High },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_MAIN, VAProfileH264Main },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_BASELINE, VAProfileH264Baseline },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_BASELINE, VAProfileH264ConstrainedBaseline },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_BASELINE, VAProfileH264Main },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_CONSTRAINED_BASELINE, VAProfileH264ConstrainedBaseline }, //mpv force main
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_CONSTRAINED_BASELINE, VAProfileH264Baseline }, //yami
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_CONSTRAINED_BASELINE, VAProfileH264Main },
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_ADVANCED, VAProfileVC1Advanced },
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_MAIN, VAProfileVC1Main },
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_SIMPLE, VAProfileVC1Simple },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_ADVANCED, VAProfileVC1Advanced },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_MAIN, VAProfileVC1Main },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_SIMPLE, VAProfileVC1Simple },
#if VA_CHECK_VERSION(0, 38, 0)
#ifdef FF_PROFILE_HEVC_MAIN
    { QTAV_CODEC_ID(HEVC), FF_PROFILE_HEVC_MAIN, VAProfileHEVCMain},
#endif
#ifdef FF_PROFILE_HEVC_MAIN_10
    { QTAV_CODEC_ID(HEVC), FF_PROFILE_HEVC_MAIN_10, VAProfileHEVCMain10},
#endif
#if FFMPEG_MODULE_CHECK(LIBAVCODEC, 54, 92, 100) || LIBAV_MODULE_CHECK(LIBAVCODEC, 55, 34, 1) //ffmpeg1.2 libav10
    { QTAV_CODEC_ID(VP9), FF_PROFILE_UNKNOWN, VAProfileVP9Profile0},
#endif
    { QTAV_CODEC_ID(VP8), FF_PROFILE_UNKNOWN, VAProfileVP8Version0_3}, //defined in 0.37
#endif //VA_CHECK_VERSION(0, 38, 0)
    { QTAV_CODEC_ID(NONE), FF_PROFILE_UNKNOWN, VAProfileNone }
};

static bool isProfileSupportedByRuntime(const VAProfile *profiles, int count, VAProfile p)
{
    for (int i = 0; i < count; ++i) {
        if (profiles[i] == p)
            return true;
    }
    return false;
}

const codec_profile_t* findProfileEntry(AVCodecID codec, int profile, const codec_profile_t* p0 = NULL)
{
    if (codec == QTAV_CODEC_ID(NONE))
        return 0;
    if (p0 && p0->codec == QTAV_CODEC_ID(NONE)) //search from the end
        return 0;
    const codec_profile_t* pe0 = p0 ? ++p0 : va_profiles;
    for (const codec_profile_t* p = pe0 ; p->codec != QTAV_CODEC_ID(NONE); ++p) {
        if (codec != p->codec || profile != p->profile)
            continue;
        // return the first profile entry if given profile is unknow
        if (profile == FF_PROFILE_UNKNOWN
                || p->profile == FF_PROFILE_UNKNOWN // force the profile
                || profile == p->profile)
            return p;
    }
    return 0;
}


struct fmtentry {
    uint32_t va;
    VideoFormat::PixelFormat fmt;
};
static const struct fmtentry va_to_imgfmt[] = {
    {VA_FOURCC_NV12, VideoFormat::Format_NV12},
    {VA_FOURCC_YV12,  VideoFormat::Format_YUV420P},
    {VA_FOURCC_IYUV, VideoFormat::Format_YUV420P},
    {VA_FOURCC_UYVY, VideoFormat::Format_UYVY},
    {VA_FOURCC_YUY2, VideoFormat::Format_YUYV},
    // Note: not sure about endian issues
#ifndef VA_FOURCC_RGBX
#define VA_FOURCC_RGBX		0x58424752
#endif
#ifndef VA_FOURCC_BGRX
    #define VA_FOURCC_BGRX		0x58524742
#endif
    {VA_FOURCC_RGBA, VideoFormat::Format_RGB32},
    {VA_FOURCC_RGBX, VideoFormat::Format_RGB32},
    {VA_FOURCC_BGRA, VideoFormat::Format_RGB32},
    {VA_FOURCC_BGRX, VideoFormat::Format_RGB32},
    {0             , VideoFormat::Format_Invalid}
};
VideoFormat::PixelFormat pixelFormatFromVA(uint32_t fourcc)
{
    for (const struct fmtentry *entry = va_to_imgfmt; entry->va; ++entry) {
        if (entry->va == fourcc)
            return entry->fmt;
    }
    return VideoFormat::Format_Invalid;
}

class VideoDecoderVAAPIPrivate Q_DECL_FINAL: public VideoDecoderFFmpegHWPrivate, protected VAAPI_DRM, protected VAAPI_X11
#ifndef QT_NO_OPENGL
        , protected VAAPI_GLX
#endif //QT_NO_OPENGL
        , protected X11_API
{
    DPTR_DECLARE_PUBLIC(VideoDecoderVAAPI)
public:
    VideoDecoderVAAPIPrivate()
        : support_4k(true)
    {
        if (VAAPI_DRM::isLoaded())
            display_type = VideoDecoderVAAPI::DRM;
#ifndef QT_NO_OPENGL
        if (VAAPI_GLX::isLoaded()) {
            display_type = VideoDecoderVAAPI::GLX;
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
        }
#endif //QT_NO_OPENGL
        if (VAAPI_X11::isLoaded()) {
            display_type = VideoDecoderVAAPI::X11;
        //TODO: OpenGLHelper::isEGL()
/*
#if QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_ES_2)
        display_type = VideoDecoderVAAPI::EGL;
#else
        // FIXME: may fallback to xcb_glx(default)
        static const bool use_egl = qgetenv("QT_XCB_GL_INTEGRATION") == "xcb_egl";
        if (use_egl)
            display_type = VideoDecoderVAAPI::EGL;
#endif //defined(QT_OPENGL_ES_2)
#endif //QTAV_HAVE(EGL_CAPI)
*/
#ifndef QT_NO_OPENGL
#if VA_X11_INTEROP
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
#endif //VA_X11_INTEROP
#endif //QT_NO_OPENGL
        }
        drm_fd = -1;
        display_x11 = 0;
        config_id = VA_INVALID_ID;
        context_id = VA_INVALID_ID;
        version_major = 0;
        version_minor = 0;
        surface_width = 0;
        surface_height = 0;
        image.image_id = VA_INVALID_ID;
        supports_derive = false;
        // set by user. don't reset in when call destroy
        nb_surfaces = 0;
        disable_derive = true;
        image_fmt = VideoFormat::Format_Invalid;
    }
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    bool ensureSurfaces(int count, int w, int h, bool discard_old = false);
    bool prepareVAImage(int w, int h);
    bool setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;
    bool getBuffer(void **opaque, uint8_t **data) Q_DECL_OVERRIDE;
    void releaseBuffer(void *opaque, uint8_t *data) Q_DECL_OVERRIDE;
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return QTAV_PIX_FMT_C(VAAPI_VLD); }

    bool support_4k;
    VideoDecoderVAAPI::DisplayType display_type;
    QList<VideoDecoderVAAPI::DisplayType> display_priority;
    Display *display_x11;
    int drm_fd;
    display_ptr display;

    VAConfigID    config_id;
    VAContextID   context_id;

    struct vaapi_context hw_ctx;
    int version_major;
    int version_minor;
    int surface_width;
    int surface_height;

    int nb_surfaces;
    QVector<VASurfaceID> surfaces;
    std::list<surface_ptr> surfaces_free, surfaces_used;
    VAImage image;
    bool disable_derive;
    bool supports_derive;
    VideoFormat::PixelFormat image_fmt;

    QString vendor;
    InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoderFFmpegHW(*new VideoDecoderVAAPIPrivate())
{
    setDisplayPriority(QStringList() << QStringLiteral("X11") << QStringLiteral("GLX") <<  QStringLiteral("DRM") << QStringLiteral("EGL"));
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces") + QStringLiteral(" ") + tr("0: auto"));
    setProperty("detail_derive", tr("Maybe faster"));
    setProperty("detail_display", QString("%1\n%2\n%3")
                .arg("X11: support all copy modes. libva-x11.so is required")
                .arg("GLX: support 0-copy only. libva-glx.so is required")
                .arg("DRM: does not support 0-copy. libva-egl.so is required")
                .arg("EGL: support all copy modes")
                );
}

VideoDecoderId VideoDecoderVAAPI::id() const
{
    return VideoDecoderId_VAAPI;
}

QString VideoDecoderVAAPI::description() const
{
    if (!d_func().description.isEmpty())
        return d_func().description;
    return QStringLiteral("Video Acceleration API");
}

void VideoDecoderVAAPI::setDerive(bool y)
{
    d_func().disable_derive = !y;
}

bool VideoDecoderVAAPI::derive() const
{
    return !d_func().disable_derive;
}

void VideoDecoderVAAPI::setSurfaces(int num)
{
    DPTR_D(VideoDecoderVAAPI);
    d.nb_surfaces = num;
    const int kMaxSurfaces = 32;
    if (num > kMaxSurfaces) {
        qWarning("VAAPI- Too many surfaces. requested: %d, maximun: %d", num, kMaxSurfaces);
    }
}

int VideoDecoderVAAPI::surfaces() const
{
    return d_func().nb_surfaces;
}

VideoDecoderVAAPI::DisplayType VideoDecoderVAAPI::display() const
{
    return d_func().display_type;
}

void VideoDecoderVAAPI::setDisplay(DisplayType disp)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    d.display_priority.append(disp);
    d.display_type = disp;
}

extern ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs);
VideoFrame VideoDecoderVAAPI::frame()
{
    DPTR_D(VideoDecoderVAAPI);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    VASurfaceID surface_id = (VASurfaceID)(uintptr_t)d.frame->data[3];
    VAStatus status = VA_STATUS_SUCCESS;
    if ( (copyMode() == ZeroCopy && (display() == X11 || display() == EGL))
         ||display() == GLX) {
        surface_ptr p;
        std::list<surface_ptr>::iterator it = d.surfaces_used.begin();
        for (; it != d.surfaces_used.end() && !p; ++it) {
            if((*it)->get() == surface_id) {
                p = *it;
                break;
            }
        }
        if (!p) {
            for (it = d.surfaces_free.begin(); it != d.surfaces_free.end() && !p; ++it) {
                if((*it)->get() == surface_id) {
                    p = *it;
                    break;
                }
            }
        }
        if (!p) {
            qWarning("VAAPI - Unable to find surface");
            return VideoFrame();
        }
        SurfaceInteropVAAPI *interop = new SurfaceInteropVAAPI(d.interop_res);
        interop->setSurface(p, d.width, d.height);

        VideoFormat fmt(VideoFormat::Format_RGB32);
        VAImage img;
        if (display() == EGL) {
            vaDeriveImage(d.display->get(), p->get(), &img);
            fmt = pixelFormatFromVA(img.format.fourcc);
            qDebug() << pixelFormatFromVA(img.format.fourcc);
        }
        VideoFrame f(d.width, d.height, fmt);//d.image_fmt);
        f.setBytesPerLine(d.width*fmt.bytesPerPixel(0), 0); //used by gl to compute texture size
        //qDebug("bpl0: %d", f.bytesPerLine(0));
        //qDebug() << fmt; //nv12 bpl[1] = bpl[0]?
        if (display() == EGL) {
            for (int i = 0; i < fmt.planeCount(); ++i) {
                f.setBytesPerLine(img.pitches[i], i);// f.bytesPerLine(0), i);//fmt.width(f.bytesPerLine(0), i), i); //used by gl to compute texture size
                //qDebug("bpl%d: %d/%d", i, fmt.width(f.bytesPerLine(0), i), f.bytesPerLine(i));
            }
            vaDestroyImage(d.display->get(), img.image_id);
        }
        f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        f.setTimestamp(double(d.frame->pkt_pts)/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));

        ColorSpace cs = colorSpaceFromFFmpeg(av_frame_get_colorspace(d.frame));
        if (cs != ColorSpace_Unknow)
            cs = colorSpaceFromFFmpeg(d.codec_ctx->colorspace);
        if (cs == ColorSpace_BT601)
            p->setColorSpace(VA_SRC_BT601);
        else
            p->setColorSpace(VA_SRC_BT709);
        return f;
    }
#if VA_CHECK_VERSION(0,31,0)
    if ((status = vaSyncSurface(d.display->get(), surface_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaSyncSurface(VADisplay:%p, VASurfaceID:%#x) == %#x", d.display->get(), surface_id, status);
#else
    if (vaSyncSurface(d.display->get(), d.context_id, surface_id)) {
        qWarning("vaSyncSurface(VADisplay:%#x, VAContextID:%#x, VASurfaceID:%#x) == %#x", d.display, d.context_id, surface_id, status);
#endif
        return VideoFrame();
    }

    if (!d.disable_derive && d.supports_derive) {
        /*
         * http://web.archiveorange.com/archive/v/OAywENyq88L319OcRnHI
         * vaDeriveImage is faster than vaGetImage. But VAImage is uncached memory and copying from it would be terribly slow
         * TODO: copy from USWC, see vlc and https://github.com/OpenELEC/OpenELEC.tv/pull/2937.diff
         * https://software.intel.com/en-us/articles/increasing-memory-throughput-with-intel-streaming-simd-extensions-4-intel-sse4-streaming-load
         */
        VA_ENSURE_TRUE(vaDeriveImage(d.display->get(), surface_id, &d.image), VideoFrame());
    } else {
        VA_ENSURE_TRUE(vaGetImage(d.display->get(), surface_id, 0, 0, d.surface_width, d.surface_height, d.image.image_id), VideoFrame());
    }

    void *p_base;
    VA_ENSURE_TRUE(vaMapBuffer(d.display->get(), d.image.buf, &p_base), VideoFrame());

    VideoFormat::PixelFormat pixfmt = pixelFormatFromVA(d.image.format.fourcc);
    bool swap_uv = d.disable_derive || !d.supports_derive || d.image.format.fourcc == VA_FOURCC_IYUV;
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vaapi pixel format: %#x", d.image.format.fourcc);
        return VideoFrame();
    }
    const VideoFormat fmt(pixfmt);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        src[i] = (uint8_t*)p_base + d.image.offsets[i];
        pitch[i] = d.image.pitches[i];
    }
    VideoFrame frame(copyToFrame(fmt, d.surface_height, src, pitch, swap_uv));
    VAWARN(vaUnmapBuffer(d.display->get(), d.image.buf));
    if (!d.disable_derive && d.supports_derive) {
        vaDestroyImage(d.display->get(), d.image.image_id);
        d.image.image_id = VA_INVALID_ID;
    }
    return frame;
}

void VideoDecoderVAAPI::setDisplayPriority(const QStringList &priority)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    int idx = staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = staticMetaObject.enumerator(idx);
    foreach (const QString& disp, priority) {
        d.display_priority.push_back((DisplayType)me.keyToValue(disp.toUtf8().constData()));
    }
}

QStringList VideoDecoderVAAPI::displayPriority() const
{
    QStringList names;
    int idx = staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = staticMetaObject.enumerator(idx);
    foreach (DisplayType disp, d_func().display_priority) {
        names.append(QString::fromLatin1(me.valueToKey(disp)));
    }
    return names;
}

bool VideoDecoderVAAPIPrivate::open()
{
    if (!prepare())
        return false;
    const codec_profile_t* pe = findProfileEntry(codec_ctx->codec_id, codec_ctx->profile);
    // TODO: allow wrong profile
    // FIXME: sometimes get wrong profile (switch copyMode)
    if (!pe) {
        qWarning("codec(%s) or profile(%s) is not supported", avcodec_get_name(codec_ctx->codec_id), getProfileName(codec_ctx->codec_id, codec_ctx->profile));
        return false;
    }
    /* Create a VA display */
    VADisplay disp = 0;
    foreach (VideoDecoderVAAPI::DisplayType dt, display_priority) {
        if (dt == VideoDecoderVAAPI::DRM) {
            if (!VAAPI_DRM::isLoaded())
                continue;
            qDebug("vaGetDisplay DRM...............");
            // try drmOpen()?
            static const char* drm_dev[] = {
                "/dev/dri/renderD128", // DRM Render-Nodes
                "/dev/dri/card0",
                NULL
            };
            for (int i = 0; drm_dev[i]; ++i) {
                drm_fd = ::open(drm_dev[i], O_RDWR);
                if (drm_fd < 0)
                    continue;
                qDebug("using drm device: %s", drm_dev[i]); //drmGetDeviceNameFromFd
                break;
            }
            if(drm_fd == -1) {
                qWarning("Could not access rendering device");
                continue;
            }
            disp = vaGetDisplayDRM(drm_fd);
            display_type = VideoDecoderVAAPI::DRM;
        } else if (dt == VideoDecoderVAAPI::X11) {
            qDebug("vaGetDisplay X11...............");
            if (!VAAPI_X11::isLoaded())
                continue;
            if (!XInitThreads()) {
                qWarning("XInitThreads failed!");
                continue;
            }
            display_x11 =  XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            disp = vaGetDisplay(display_x11);
            display_type = VideoDecoderVAAPI::X11;
        } else if (dt == VideoDecoderVAAPI::GLX) {
            qDebug("vaGetDisplay GLX...............");
#ifndef QT_NO_OPENGL
            if (!VAAPI_GLX::isLoaded())
                continue;
            if (!XInitThreads()) {
                qWarning("XInitThreads failed!");
                continue;
            }
            display_x11 = XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            disp = vaGetDisplayGLX(display_x11);
            display_type = VideoDecoderVAAPI::GLX;
#else
            qWarning("No OpenGL support in Qt");
#endif //QT_NO_OPENGL
        } else if (dt == VideoDecoderVAAPI::EGL
                   ) {
            qDebug("vaGetDisplay X11 EGL...............");
            if (!XInitThreads()) {
                qWarning("XInitThreads failed!");
                continue;
            }
            // TODO: vaGetDisplayWl
            display_x11 =  XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            disp = vaGetDisplay(display_x11);
            display_type = VideoDecoderVAAPI::EGL;
        }
        if (disp)
            break;
    }
    if (!disp/* || vaDisplayIsValid(display) != 0*/) {
        qWarning("Could not get a VAAPI device");
        return false;
    }
    display = display_ptr(new display_t(disp));
    if (vaInitialize(disp, &version_major, &version_minor)) {
        qWarning("Failed to initialize the VAAPI device");
        return false;
    }
    vendor = QString::fromLatin1(vaQueryVendorString(disp));
    //if (!vendor.toLower().contains(QLatin1Strin("intel")))
      //  copy_uswc = false;

    //disable_derive = !copy_uswc;
    description = QObject::tr("VA API version %1.%2; Vendor: %3;").arg(version_major).arg(version_minor).arg(vendor);
    DPTR_P(VideoDecoderVAAPI);
    int idx = p.staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = p.staticMetaObject.enumerator(idx);
    description += QStringLiteral(" Display: ") + QString::fromLatin1(me.valueToKey(display_type));
    // check 4k support. from xbmc
    int major, minor, micro;
    if (sscanf(vendor.toUtf8().constData(), "Intel i965 driver - %d.%d.%d", &major, &minor, &micro) == 3) {
        /* older version will crash and burn */
        if (VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 0, 17)) {
            qDebug("VAAPI - deinterlace not support on this intel driver version");
        }
        // do the same check for 4K decoding: version < 1.2.0 (stable) and 1.0.21 (staging)
        // cannot decode 4K and will crash the GPU
        if (VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 2, 0) && VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 0, 21)) {
            support_4k = false;
        }
    }
    if (!support_4k && (codec_ctx->width > 1920 || codec_ctx->height > 1088)) {
        qWarning("VAAPI: frame size (%dx%d) is too large", codec_ctx->width, codec_ctx->height);
        return false;
    }
    /* Check if the selected profile is supported */
    qDebug("checking profile: %s, %s",  avcodec_get_name(codec_ctx->codec_id), getProfileName(pe->codec, pe->profile));
    int nb_profiles = vaMaxNumProfiles(disp);
    if (nb_profiles <= 0) {
        qWarning("No profile supported");
        return false;
    }
    QVector<VAProfile> supported_profiles(nb_profiles, VAProfileNone);
    VA_ENSURE_TRUE(vaQueryConfigProfiles(disp, supported_profiles.data(), &nb_profiles), false);
    while (pe && !isProfileSupportedByRuntime(supported_profiles.constData(), nb_profiles, pe->va_profile)) {
        qDebug("Codec or profile %s %d is not directly supported by the hardware. Checking alternative profiles", vaapi::profileName(pe->va_profile), pe->va_profile);
        pe = findProfileEntry(codec_ctx->codec_id, codec_ctx->profile, pe);
    }
    if (!pe) {
        qDebug("Codec or profile is not supported by the hardware.");
        return false;
    }
    qDebug("using profile %s (%d)", vaapi::profileName(pe->va_profile), pe->va_profile);
    /* Create a VA configuration */
    VAConfigAttrib attrib;
    memset(&attrib, 0, sizeof(attrib));
    attrib.type = VAConfigAttribRTFormat;
    VA_ENSURE_TRUE(vaGetConfigAttributes(disp, pe->va_profile, VAEntrypointVLD, &attrib, 1), false);
    /* Not sure what to do if not, I don't have a way to test */
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;

    config_id  = VA_INVALID_ID;
    VA_ENSURE_TRUE(vaCreateConfig(disp, pe->va_profile, VAEntrypointVLD, &attrib, 1, &config_id), false);
    supports_derive = false;
    width = codec_ctx->width;
    height = codec_ctx->height;
    surface_width = codedWidth(codec_ctx);
    surface_height = codedHeight(codec_ctx);

    qDebug("checking surface resolution support");
    VASurfaceID test_surface = VA_INVALID_ID;
    VA_ENSURE_TRUE(vaCreateSurfaces(display->get(), VA_RT_FORMAT_YUV420, surface_width, surface_height,  &test_surface, 1, NULL, 0), false);
    // context create fail but surface create ok (tested 6k for intel)
    context_id = VA_INVALID_ID;
    VA_ENSURE_TRUE(vaCreateContext(display->get(), config_id, surface_width, surface_height, VA_PROGRESSIVE, surfaces.data(), surfaces.size(), &context_id), false);
    VAWARN(vaDestroyContext(display->get(), context_id));
    VAWARN(vaDestroySurfaces(display->get(), &test_surface, 1));
    context_id = VA_INVALID_ID;

#ifndef QT_NO_OPENGL
    if (display_type == VideoDecoderVAAPI::GLX)
        interop_res = InteropResourcePtr(new GLXInteropResource());
#endif //QT_NO_OPENGL
#if VA_X11_INTEROP
    if (display_type == VideoDecoderVAAPI::X11)
        interop_res = InteropResourcePtr(new X11InteropResource());
#endif //VA_X11_INTEROP
#if QTAV_HAVE(EGL_CAPI) && VA_CHECK_VERSION(0, 38, 0)
    if (display_type == VideoDecoderVAAPI::EGL)
        interop_res = InteropResourcePtr(new EGLInteropResource());
#endif
    codec_ctx->hwaccel_context = &hw_ctx; //must set before open
    return true;
}

bool VideoDecoderVAAPIPrivate::ensureSurfaces(int count, int w, int h, bool discard_old)
{
    if (!display) {
        qWarning("no va display");
        return false;
    }
    qDebug("ensureSurfaces %d->%d %dx%d. discard old surfaces: %d", surfaces.size(), count, w, h, discard_old);
    Q_ASSERT(w > 0 && h > 0);
    const int old_size = discard_old ? 0 : surfaces.size();
    if (count <= old_size)
        return true;
    surfaces.resize(old_size); // clear the old surfaces if discard_old. when initializing va-api, we must discard old surfaces (vdpau_video.c:595: vdpau_CreateContext: Assertion `obj_surface->va_context == 0xffffffff' failed.)
    surfaces.resize(count);
    VA_ENSURE_TRUE(vaCreateSurfaces(display->get(), VA_RT_FORMAT_YUV420, w, h,  surfaces.data() + old_size, count - old_size, NULL, 0), false);
    for (int i = old_size; i < surfaces.size(); ++i) {
        //qDebug("surface id: %p %dx%d", surfaces.at(i), w, height);
        surfaces_free.push_back(surface_ptr(new surface_t(w, h, surfaces[i], display)));
    }
    return true;
}

bool VideoDecoderVAAPIPrivate::prepareVAImage(int w, int h)
{
    /* Find and create a supported image chroma */
    int nb_fmts = vaMaxNumImageFormats(display->get());
    //av_mallocz_array
    VAImageFormat *fmt = (VAImageFormat*)calloc(nb_fmts, sizeof(*fmt));
    if (!fmt)
        return false;
    if (vaQueryImageFormats(display->get(), fmt, &nb_fmts)) {
        free(fmt);
        return false;
    }
    image.image_id = VA_INVALID_ID;
    bool found = false;
    for (int i = 0; i < nb_fmts; i++) {
        qDebug("va image format: %c%c%c%c", fmt[i].fourcc<<24>>24, fmt[i].fourcc<<16>>24, fmt[i].fourcc<<8>>24, fmt[i].fourcc>>24);
        if (fmt[i].fourcc != VA_FOURCC_YV12 &&
                fmt[i].fourcc != VA_FOURCC_IYUV &&
                fmt[i].fourcc != VA_FOURCC_NV12)
            continue;
        VAStatus s = VA_STATUS_SUCCESS;
        if ((s = vaCreateImage(display->get(), &fmt[i], w, h, &image)) != VA_STATUS_SUCCESS) {
            image.image_id = VA_INVALID_ID;
            qDebug("vaCreateImage error: %c%c%c%c (%p) %s", fmt[i].fourcc<<24>>24, fmt[i].fourcc<<16>>24, fmt[i].fourcc<<8>>24, fmt[i].fourcc>>24, s, vaErrorStr(s));
            continue;
        }
        /* Validate that vaGetImage works with this format */
        if ((s = vaGetImage(display->get(), surfaces[0], 0, 0, w, h, image.image_id)) != VA_STATUS_SUCCESS) {
            vaDestroyImage(display->get(), image.image_id);
            qDebug("vaGetImage error: %c%c%c%c  (%p) %s", fmt[i].fourcc<<24>>24, fmt[i].fourcc<<16>>24, fmt[i].fourcc<<8>>24, fmt[i].fourcc>>24, s, vaErrorStr(s));
            image.image_id = VA_INVALID_ID;
            continue;
        }
        image_fmt = pixelFormatFromVA(image.format.fourcc);
        found = true;
        break;
    }
    free(fmt);
    if (!found) {
        return false;
    }
    VAImage test_image;
    if (!disable_derive || display_type == VideoDecoderVAAPI::EGL) {
        if (vaDeriveImage(display->get(), surfaces[0], &test_image) == VA_STATUS_SUCCESS) {
            qDebug("vaDeriveImage supported");
            supports_derive = true;
            image_fmt = pixelFormatFromVA(image.format.fourcc);
            /* from vlc: Use vaDerive() iif it supports the best selected format */
            if (image.format.fourcc == test_image.format.fourcc) {
                qDebug("vaDerive is ok");
//                supports_derive = true;
            }
            vaDestroyImage(display->get(), test_image.image_id);
        }
        if (supports_derive) {
            vaDestroyImage(display->get(), image.image_id);
            image.image_id = VA_INVALID_ID;
        }
    }
    return true;
}

bool VideoDecoderVAAPIPrivate::setup(AVCodecContext *avctx)
{
    if (!display || config_id == VA_INVALID_ID) {
        qWarning("va-api is not initialized. display: %p, config_id: %p", display->get(), config_id);
        return false;
    }
    int surface_count =  nb_surfaces;
    if (surface_count <= 0) {
        surface_count = 2+1;
        qDebug("guess surface count");
        if (codec_ctx->codec_id == QTAV_CODEC_ID(H264))
            surface_count = 16+2;
    #ifdef FF_PROFILE_HEVC_MAIN
        if (codec_ctx->codec_id == QTAV_CODEC_ID(HEVC))
            surface_count = 16+2;
    #endif
        // TODO: vp8,9
        if (codec_ctx->active_thread_type & FF_THREAD_FRAME)
            surface_count += codec_ctx->thread_count;
    }
    if (!ensureSurfaces(surface_count, surface_width, surface_height, true))
        return false;
    if (display_type == VideoDecoderVAAPI::DRM
            || (display_type == VideoDecoderVAAPI::X11 && copy_mode != VideoDecoderFFmpegHW::ZeroCopy)
            || (display_type == VideoDecoderVAAPI::EGL)) {
        // egl needs VAImage too
        if (!prepareVAImage(surface_width, surface_height))
            return false;
        // copy-back mode
        if (display_type != VideoDecoderVAAPI::EGL || copy_mode != VideoDecoderFFmpegHW::ZeroCopy)
            initUSWC(surface_width);
    }
    context_id = VA_INVALID_ID;
    VA_ENSURE_TRUE(vaCreateContext(display->get(), config_id, surface_width, surface_height, VA_PROGRESSIVE, surfaces.data(), surfaces.size(), &context_id), false);
    /* Setup the ffmpeg hardware context */
    memset(&hw_ctx, 0, sizeof(hw_ctx));
    hw_ctx.display = display->get();
    hw_ctx.config_id = config_id;
    hw_ctx.context_id = context_id;
    return true;
}

void VideoDecoderVAAPIPrivate::close()
{
    restore();
    // do not call vaDestroySurfaces here because they are destroyed by surface_t
    if (image.image_id != VA_INVALID_ID) {
        VAWARN(vaDestroyImage(display->get(), image.image_id));
        image.image_id = VA_INVALID_ID;
    }
    if (context_id != VA_INVALID_ID) {
        VAWARN(vaDestroyContext(display->get(), context_id));
        context_id = VA_INVALID_ID;
    }
    if (config_id != VA_INVALID_ID) {
        VAWARN(vaDestroyConfig(display->get(), config_id));
        config_id = VA_INVALID_ID;
    }
    display.clear();
    if (display_x11) {
        //XCloseDisplay(display_x11); //FIXME: why crash?
        display_x11 = 0;
    }
    if (drm_fd >= 0) {
        ::close(drm_fd);
        drm_fd = -1;
    }
    releaseUSWC();
    nb_surfaces = 0;
    surfaces.clear();
    surfaces_free.clear();
    surfaces_used.clear();
    surface_width = 0;
    surface_height = 0;
}

bool VideoDecoderVAAPIPrivate::getBuffer(void **opaque, uint8_t **data)
{
    VASurfaceID id = (VASurfaceID)(quintptr)*data;
    // id is always 0?
    std::list<surface_ptr>::iterator it = surfaces_free.begin();
    if (id && id != VA_INVALID_ID) {
        bool found = false;
        for (; it != surfaces_free.end(); ++it) { //?
            if ((*it)->get() == id) {
                found = true;
                break;
            }
        }
        if (!found) {
            qWarning("surface not found!!!!!!!!!!!!!");
            return false;
        }
    } else {
        for (; it != surfaces_free.end() && it->count() > 1; ++it) {}
        if (it == surfaces_free.end()) {
            if (!surfaces_free.empty())
                qWarning("VAAPI - renderer still using all freed up surfaces by decoder. unable to find free surface, trying to allocate a new one");

            const int kMaxSurfaces = 32;
            if (surfaces.size() + 1 > kMaxSurfaces) {
                qWarning("VAAPI- Too many surfaces. requested: %d, maximun: %d", surfaces.size() + 1, kMaxSurfaces);
            }
            // Set itarator position to the newly allocated surface (end-1)
            const int old_size = surfaces.size();
            if (!ensureSurfaces(old_size + 1, surface_width, surface_height, false)) {
                // destroy the new created surface. Surfaces can only be destroyed after the context associated has been destroyed?
                VAWARN(vaDestroySurfaces(display->get(), surfaces.data() + old_size, 1));
                surfaces.resize(old_size);
            }
            it = surfaces_free.end();
            --it;
        }
    }
    id =(*it)->get();
    surface_t *s = it->get();
    // !erase later. otherwise it may be destroyed
    surfaces_used.push_back(*it);
    // TODO: why QList may erase an invalid iterator(first iterator)at the same position?
    surfaces_free.erase(it); //ref not increased, but can not be used.
    *data = (uint8_t*)(quintptr)id;
    *opaque = s;
    return true;
}

void VideoDecoderVAAPIPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(opaque);
    VASurfaceID id = (VASurfaceID)(uintptr_t)data;
    for (std::list<surface_ptr>::iterator it = surfaces_used.begin(); it != surfaces_used.end(); ++it) {
        if((*it)->get() == id) {
            surfaces_free.push_back(*it);
            surfaces_used.erase(it);
            break;
        }
    }
}

} // namespace QtAV
// used by .moc QMetaType::Bool
#ifdef Bool
#undef Bool
#endif
#include "VideoDecoderVAAPI.moc"
