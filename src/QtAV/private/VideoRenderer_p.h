#ifndef QAVVIDEORENDERER_P_H
#define QAVVIDEORENDERER_P_H

#include <qbytearray.h>
#include <private/AVOutput_p.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif //__cplusplus

#if CONFIG_EZX
#define PIX_FMT PIX_FMT_BGR565
#else
#define PIX_FMT PIX_FMT_RGB32
#endif //CONFIG_EZX

struct SwsContext;
namespace QtAV {

class VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():width(0),height(0),pix_fmt(PIX_FMT)
      ,numBytes(0),sws_ctx(0){
    }
    ~VideoRendererPrivate(){
        sws_freeContext(sws_ctx); //NULL: does nothing
        sws_ctx = 0;
    }

    void resizePicture(int width, int height);

    int width, height;
    AVPicture picture;
    enum PixelFormat pix_fmt;
    int numBytes;
    QByteArray data;
    SwsContext *sws_ctx;
};

} //namespace QtAV
#endif // QAVVIDEORENDERER_P_H
