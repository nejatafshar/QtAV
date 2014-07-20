/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoShader.h"
#include "QtAV/private/VideoShader_p.h"
#include "QtAV/ColorTransform.h"
#include "utils/OpenGLHelper.h"
#include <cmath>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

/*
 * TODO: glActiveTexture for Qt4
 * texture target (rectangle for VDA)
 */

namespace QtAV {

VideoShader::VideoShader(VideoShaderPrivate &d):
    DPTR_INIT(&d)
{
}

VideoShader::VideoShader()
{
    //d.planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    //d.packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
}

VideoShader::~VideoShader()
{
}

/*
 * use gl api to get active attributes/uniforms
 * use glsl parser to get attributes?
 */
char const *const* VideoShader::attributeNames() const
{
    static const char *names[] = {
        "a_Position",
        "a_TexCoords",
        0
    };
    return names;
}

const char* VideoShader::vertexShader() const
{
    static const char kVertexShader[] =
        "attribute vec4 a_Position;\n"
        "attribute vec2 a_TexCoords;\n"
        "uniform mat4 u_MVP_matrix;\n"
        "varying vec2 v_TexCoords;\n"
        "void main() {\n"
        "  gl_Position = u_MVP_matrix * a_Position;\n"
        "  v_TexCoords = a_TexCoords; \n"
        "}\n";
    return kVertexShader;
}

const char* VideoShader::fragmentShader() const
{
    DPTR_D(const VideoShader);
    // because we have to modify the shader, and shader source must be kept, so read the origin
    if (d.video_format.isPlanar()) {
        d.planar_frag = shaderSourceFromFile("shaders/planar.f.glsl");
    } else {
        d.packed_frag = shaderSourceFromFile("shaders/rgb.f.glsl");
    }
    QByteArray& frag = d.video_format.isPlanar() ? d.planar_frag : d.packed_frag;
    if (frag.isEmpty()) {
        qWarning("Empty fragment shader!");
        return 0;
    }
    if (d.video_format.isRGB()) {
        frag.prepend("#define INPUT_RGB\n");
    } else {
        // color space in glsl does not work unless we define 'YUV_MAT_GLSL'. we compute it in color matrix by cpu
#if 0
        switch (d.color_space) {
        case ColorTransform::BT601:
            frag.prepend("#define CS_BT601\n");
            break;
        case ColorTransform::BT709:
            frag.prepend("#define CS_BT709\n");
        default:
            break;
        }
#endif
        if (d.video_format.isPlanar() && d.video_format.bytesPerPixel(0) == 2) {
            if (d.video_format.isBigEndian())
                frag.prepend("#define YUV16BITS_BE_LUMINANCE_ALPHA\n");
            else
                frag.prepend("#define YUV16BITS_LE_LUMINANCE_ALPHA\n");
        }
        frag.prepend(QString("#define YUV%1P\n").arg(d.video_format.bitsPerPixel(0)).toUtf8());
    }
    return frag.constData();
}

void VideoShader::initialize(QOpenGLShaderProgram *shaderProgram)
{
    DPTR_D(VideoShader);
    if (!shaderProgram) {
        shaderProgram = program();
    }
    if (!shaderProgram->isLinked()) {
        compile(shaderProgram);
    }
    d.u_MVP_matrix = shaderProgram->uniformLocation("u_MVP_matrix");
    // fragment shader
    d.u_colorMatrix = shaderProgram->uniformLocation("u_colorMatrix");
    d.u_bpp = shaderProgram->uniformLocation("u_bpp");
    d.u_Texture.resize(textureLocationCount());
    for (int i = 0; i < d.u_Texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        d.u_Texture[i] = shaderProgram->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d", tex_var.toUtf8().constData(), d.u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d", d.u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d", d.u_colorMatrix);
    qDebug("glGetUniformLocation(\"u_bpp\") = %d", d.u_bpp);
}

int VideoShader::textureLocationCount() const
{
    DPTR_D(const VideoShader);
    if (d.video_format.isRGB() && !d.video_format.isPlanar())
        return 1;
    return d.video_format.channels();
}

int VideoShader::textureLocation(int channel) const
{
    DPTR_D(const VideoShader);
    Q_ASSERT(channel < d.u_Texture.size());
    return d.u_Texture[channel];
}

int VideoShader::matrixLocation() const
{
    return d_func().u_MVP_matrix;
}

int VideoShader::colorMatrixLocation() const
{
    return d_func().u_colorMatrix;
}

int VideoShader::bppLocation() const
{
    return d_func().u_bpp;
}

VideoFormat VideoShader::videoFormat() const
{
    return d_func().video_format;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
    d_func().video_format = format;
}

QOpenGLShaderProgram* VideoShader::program()
{
    DPTR_D(VideoShader);
    if (!d.program)
        d.program = new QOpenGLShaderProgram();
    return d.program;
}

void VideoShader::update(VideoMaterial *material)
{
    material->bind();

    // uniforms begin
    program()->bind(); //glUseProgram(id). for glUniform
    // all texture ids should be binded when renderering even for packed plane!
    const int nb_planes = videoFormat().planeCount(); //number of texture id
    for (int i = 0; i < nb_planes; ++i) {
        // use glUniform1i to swap planes. swap uv: i => (3-i)%3
        // TODO: in shader, use uniform sample2D u_Texture[], and use glUniform1iv(u_Texture, 3, {...})
        program()->setUniformValue(textureLocation(i), (GLint)i);
    }
    if (nb_planes < textureLocationCount()) {
        for (int i = nb_planes; i < textureLocationCount(); ++i) {
            program()->setUniformValue(textureLocation(i), (GLint)(nb_planes - 1));
        }
    }
    program()->setUniformValue(colorMatrixLocation(), material->colorMatrix());
    program()->setUniformValue(bppLocation(), (GLfloat)material->bpp());
    //program()->setUniformValue(matrixLocation(), material->matrix()); //what about sgnode? state.combindMatrix()?
    // uniform end. attribute begins
}

QByteArray VideoShader::shaderSourceFromFile(const QString &fileName) const
{
    QFile f(qApp->applicationDirPath() + "/" + fileName);
    if (!f.exists()) {
        f.setFileName(":/" + fileName);
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Can not load shader %s: %s", f.fileName().toUtf8().constData(), f.errorString().toUtf8().constData());
        return QByteArray();
    }
    QByteArray src = f.readAll();
    f.close();
    return src;
}

void VideoShader::compile(QOpenGLShaderProgram *shaderProgram)
{
    Q_ASSERT_X(!shaderProgram.isLinked(), "VideoShader::compile()", "Compile called multiple times!");
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader());
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader());
    int maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    char const *const *attr = attributeNames();
    for (int i = 0; attr[i]; ++i) {
        if (i >= maxVertexAttribs) {
            qFatal("List of attribute names is either too long or not null-terminated.\n"
                   "Maximum number of attributes on this hardware is %i.\n"
                   "Vertex shader:\n%s\n"
                   "Fragment shader:\n%s\n",
                   maxVertexAttribs, vertexShader(), fragmentShader());
        }
        // why must min location == 0?
        if (*attr[i])
            shaderProgram->bindAttributeLocation(attr[i], i);
    }

    if (!shaderProgram->link()) {
        qWarning("QSGMaterialShader: Shader compilation failed:");
        qWarning() << shaderProgram->log();
    }
}

VideoMaterial::VideoMaterial()
{
}

void VideoMaterial::setCurrentFrame(const VideoFrame &frame)
{
    DPTR_D(VideoMaterial);
    // TODO: lock?
    d.frame = frame;
    d.bpp = frame.format().bitsPerPixel(0);
    // http://forum.doom9.org/archive/index.php/t-160211.html
    ColorTransform::ColorSpace cs = ColorTransform::RGB;
    if (!frame.format().isRGB()) {
        if (d.frame.width() >= 1280 || d.frame.height() > 576) //values from mpv
            cs = ColorTransform::BT709;
        else
            cs = ColorTransform::BT601;
    }
    d.colorTransform.setInputColorSpace(cs);
    d.update_texure = true;
}

VideoShader* VideoMaterial::createShader() const
{
    DPTR_D(const VideoMaterial);
    VideoShader *shader = new VideoShader();
    const VideoFormat fmt(d.frame.format());
    shader->setVideoFormat(fmt);
    return shader;
}

MaterialType* VideoMaterial::type() const
{
    static MaterialType rgbType;
    static MaterialType yuv16leType;
    static MaterialType yuv16beType;
    static MaterialType yuv8Type;
    static MaterialType invalidType;
    const VideoFormat &fmt = d_func().frame.format();
    if (fmt.isRGB() && !fmt.isPlanar())
        return &rgbType;
    if (fmt.bytesPerPixel(0) == 1)
        return &yuv8Type;
    if (fmt.isBigEndian())
        return &yuv16beType;
    else
        return &yuv16leType;
    return &invalidType;
}

void VideoMaterial::bind()
{
    DPTR_D(VideoMaterial);
    d.updateTexturesIfNeeded();
    const int nb_planes = d.textures.size(); //number of texture id
    if (d.update_texure) {
        for (int i = 0; i < nb_planes; ++i) {
            bindPlane(i);
        }
        d.update_texure = false;
        return;
    }
    if (d.textures.isEmpty())
        return;
    for (int i = 0; i < nb_planes; ++i) {
        OpenGLHelper::glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, d.textures[i]);
    }
}

void VideoMaterial::bindPlane(int p)
{
    DPTR_D(VideoMaterial);
    //setupQuality?
    if (d.frame.map(GLTextureSurface, &d.textures[p])) {
        OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p);
        glBindTexture(GL_TEXTURE_2D, d.textures[p]);
        return;
    }
    // FIXME: why happens on win?
    if (d.frame.bytesPerLine(p) <= 0)
        return;
    OpenGLHelper::glActiveTexture(GL_TEXTURE0 + p);
    glBindTexture(GL_TEXTURE_2D, d.textures[p]);
    //d.setupQuality();
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexSubImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , 0                // xoffset
                 , 0                // yoffset
                 , d.texture_upload_size[p].width()
                 , d.texture_upload_size[p].height()
                 , d.data_format[p]          //format, must the same as internal format?
                 , d.data_type[p]
                 , d.frame.bits(p));
}

int VideoMaterial::compare(const VideoMaterial *other) const
{
    DPTR_D(const VideoMaterial);
    for (int i = 0; i < d.textures.size(); ++i) {
        const int diff = d.textures[i] - other->d_func().textures[i];
        if (diff)
            return diff;
    }
    return d.bpp - other->bpp();
}

void VideoMaterial::unbind()
{
    DPTR_D(VideoMaterial);
    for (int i = 0; i < d.textures.size(); ++i) {
        d.frame.unmap(&d.textures[i]);
    }
}

const QMatrix4x4& VideoMaterial::colorMatrix() const
{
    return d_func().colorTransform.matrixRef();
}

const QMatrix4x4& VideoMaterial::matrix() const
{
    return d_func().matrix;
}

int VideoMaterial::bpp() const
{
    return d_func().bpp;
}

int VideoMaterial::planeCount() const
{
    return d_func().frame.planeCount();
}

void VideoMaterial::setBrightness(qreal value)
{
    d_func().colorTransform.setBrightness(value);
}

void VideoMaterial::setContrast(qreal value)
{
    d_func().colorTransform.setContrast(value);
}

void VideoMaterial::setHue(qreal value)
{
    d_func().colorTransform.setHue(value);
}

void VideoMaterial::setSaturation(qreal value)
{
    d_func().colorTransform.setSaturation(value);
}

void VideoMaterial::getTextureCoordinates(const QRect& roi, float* t)
{
    DPTR_D(VideoMaterial);
    /*!
      tex coords: ROI/frameRect()*effective_tex_width_ratio
    */
    t[0] = (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[1] = (GLfloat)roi.y()/(GLfloat)d.frame.height();
    t[2] = (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[3] = (GLfloat)roi.y()/(GLfloat)d.frame.height();
    t[4] = (GLfloat)(roi.x() + roi.width())*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[5] = (GLfloat)(roi.y()+roi.height())/(GLfloat)d.frame.height();
    t[6] = (GLfloat)roi.x()*(GLfloat)d.effective_tex_width_ratio/(GLfloat)d.frame.width();
    t[7] = (GLfloat)(roi.y()+roi.height())/(GLfloat)d.frame.height();
}


bool VideoMaterialPrivate::initTexture(GLuint tex, GLint internal_format, GLenum format, GLenum dataType, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, tex);
    setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , internal_format               //internal format. 4? why GL_RGBA? GL_RGB?
                 , width
                 , height
                 , 0                //border, ES not support
                 , format          //format, must the same as internal format?
                 , dataType
                 , NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool VideoMaterialPrivate::initTextures(const VideoFormat& fmt)
{
    // isSupported(pixfmt)
    if (!fmt.isValid())
        return false;
    video_format.setPixelFormatFFmpeg(fmt.pixelFormatFFmpeg());

    //http://www.berkelium.com/OpenGL/GDC99/internalformat.html
    //NV12: UV is 1 plane. 16 bits as a unit. GL_LUMINANCE4, 8, 16, ... 32?
    //GL_LUMINANCE, GL_LUMINANCE_ALPHA are deprecated in GL3, removed in GL3.1
    //replaced by GL_RED, GL_RG, GL_RGB, GL_RGBA? for 1, 2, 3, 4 channel image
    //http://www.gamedev.net/topic/634850-do-luminance-textures-still-exist-to-opengl/
    //https://github.com/kivy/kivy/issues/1738: GL_LUMINANCE does work on a Galaxy Tab 2. LUMINANCE_ALPHA very slow on Linux
     //ALPHA: vec4(1,1,1,A), LUMINANCE: (L,L,L,1), LUMINANCE_ALPHA: (L,L,L,A)
    /*
     * To support both planar and packed use GL_ALPHA and in shader use r,g,a like xbmc does.
     * or use Swizzle_mask to layout the channels: http://www.opengl.org/wiki/Texture#Swizzle_mask
     * GL ES2 support: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA
     * http://stackoverflow.com/questions/18688057/which-opengl-es-2-0-texture-formats-are-color-depth-or-stencil-renderable
     */
    if (fmt.isRGB()) {
        GLint internal_fmt;
        GLenum data_fmt;
        GLenum data_t;
        if (!OpenGLHelper::videoFormatToGL(fmt, &internal_fmt, &data_fmt, &data_t)) {
            qWarning("no opengl format found");
            return false;
        }
        internal_format = QVector<GLint>(fmt.planeCount(), internal_fmt);
        data_format = QVector<GLenum>(fmt.planeCount(), data_fmt);
        data_type = QVector<GLenum>(fmt.planeCount(), data_t);
    } else {
        internal_format.resize(fmt.planeCount());
        data_format.resize(fmt.planeCount());
        data_type = QVector<GLenum>(fmt.planeCount(), GL_UNSIGNED_BYTE);
        if (fmt.isPlanar()) {
        /*!
         * GLES internal_format == data_format, GL_LUMINANCE_ALPHA is 2 bytes
         * so if NV12 use GL_LUMINANCE_ALPHA, YV12 use GL_ALPHA
         */
            qDebug("///////////bpp %d", fmt.bytesPerPixel());
            internal_format[0] = data_format[0] = GL_LUMINANCE; //or GL_RED for GL
            if (fmt.planeCount() == 2) {
                internal_format[1] = data_format[1] = GL_LUMINANCE_ALPHA;
            } else {
                if (fmt.bytesPerPixel(1) == 2) {
                    // read 16 bits and compute the real luminance in shader
                    internal_format[0] = data_format[0] = GL_LUMINANCE_ALPHA;
                    internal_format[1] = data_format[1] = GL_LUMINANCE_ALPHA; //vec4(L,L,L,A)
                    internal_format[2] = data_format[2] = GL_LUMINANCE_ALPHA;
                } else {
                    internal_format[1] = data_format[1] = GL_LUMINANCE; //vec4(L,L,L,1)
                    internal_format[2] = data_format[2] = GL_ALPHA;//GL_ALPHA;
                }
            }
            for (int i = 0; i < internal_format.size(); ++i) {
                // xbmc use bpp not bpp(plane)
                //internal_format[i] = GetGLInternalFormat(data_format[i], fmt.bytesPerPixel(i));
                //data_format[i] = internal_format[i];
            }
        } else {
            //glPixelStorei(GL_UNPACK_ALIGNMENT, fmt.bytesPerPixel());
            // TODO: if no alpha, data_fmt is not GL_BGRA. align at every upload?
        }
    }
    for (int i = 0; i < fmt.planeCount(); ++i) {
        //qDebug("format: %#x GL_LUMINANCE_ALPHA=%#x", data_format[i], GL_LUMINANCE_ALPHA);
        if (fmt.bytesPerPixel(i) == 2 && fmt.planeCount() == 3) {
            //data_type[i] = GL_UNSIGNED_SHORT;
        }
        int bpp_gl = OpenGLHelper::bytesOfGLFormat(data_format[i], data_type[i]);
        int pad = std::ceil((qreal)(texture_size[i].width() - effective_tex_width[i])/(qreal)bpp_gl);
        texture_size[i].setWidth(std::ceil((qreal)texture_size[i].width()/(qreal)bpp_gl));
        texture_upload_size[i].setWidth(std::ceil((qreal)texture_upload_size[i].width()/(qreal)bpp_gl));
        effective_tex_width[i] /= bpp_gl; //fmt.bytesPerPixel(i);
        //effective_tex_width_ratio =
        qDebug("texture width: %d - %d = pad: %d. bpp(gl): %d", texture_size[i].width(), effective_tex_width[i], pad, bpp_gl);
    }

    /*
     * there are 2 fragment shaders: rgb and yuv.
     * only 1 texture for packed rgb. planar rgb likes yuv
     * To support both planar and packed yuv, and mixed yuv(NV12), we give a texture sample
     * for each channel. For packed, each (channel) texture sample is the same. For planar,
     * packed channels has the same texture sample.
     * But the number of actural textures we upload is plane count.
     * Which means the number of texture id equals to plane count
     */
    if (textures.size() != fmt.planeCount()) {
        glDeleteTextures(textures.size(), textures.data());
        qDebug("delete %d textures", textures.size());
        textures.clear();
        textures.resize(fmt.planeCount());
        glGenTextures(textures.size(), textures.data());
    }
    qDebug("init textures...");
    for (int i = 0; i < textures.size(); ++i) {
        initTexture(textures[i], internal_format[i], data_format[i], data_type[i], texture_size[i].width(), texture_size[i].height());
    }
    return true;
}

void VideoMaterialPrivate::updateTexturesIfNeeded()
{
    const VideoFormat &fmt = frame.format();
    bool update_textures = false;
    if (fmt != video_format) {
        //update_textures = true;
        qDebug("pixel format changed: %s => %s", qPrintable(video_format.name()), qPrintable(fmt.name()));
    }
    // effective size may change even if plane size not changed
    if (update_textures
            || frame.bytesPerLine(0) != plane0Size.width() || frame.height() != plane0Size.height()
            || (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        //qDebug("---------------------update texture: %dx%d, %s", frame.width(), frame.height(), frame.format().name().toUtf8().constData());
        const int nb_planes = fmt.planeCount();
        texture_size.resize(nb_planes);
        texture_upload_size.resize(nb_planes);
        effective_tex_width.resize(nb_planes);
        for (int i = 0; i < nb_planes; ++i) {
            qDebug("plane linesize %d: padded = %d, effective = %d", i, frame.bytesPerLine(i), frame.effectiveBytesPerLine(i));
            qDebug("plane width %d: effective = %d", frame.planeWidth(i), frame.effectivePlaneWidth(i));
            qDebug("planeHeight %d = %d", i, frame.planeHeight(i));
            // we have to consider size of opengl format. set bytesPerLine here and change to width later
            texture_size[i] = QSize(frame.bytesPerLine(i), frame.planeHeight(i));
            texture_upload_size[i] = texture_size[i];
            effective_tex_width[i] = frame.effectiveBytesPerLine(i); //store bytes here, modify as width later
            // TODO: ratio count the GL_UNPACK_ALIGN?
            //effective_tex_width_ratio = qMin((qreal)1.0, (qreal)frame.effectiveBytesPerLine(i)/(qreal)frame.bytesPerLine(i));
        }
        plane1_linesize = 0;
        if (nb_planes > 1) {
            texture_size[0].setWidth(texture_size[1].width() * effective_tex_width[0]/effective_tex_width[1]);
            // height? how about odd?
            plane1_linesize = frame.bytesPerLine(1);
        }
        effective_tex_width_ratio = (qreal)frame.effectiveBytesPerLine(nb_planes-1)/(qreal)frame.bytesPerLine(nb_planes-1);
        qDebug("effective_tex_width_ratio=%f", effective_tex_width_ratio);
        plane0Size.setWidth(frame.bytesPerLine(0));
        plane0Size.setHeight(frame.height());
    }
    if (update_textures) {
        initTextures(fmt);
    }
}

void VideoMaterialPrivate::setupQuality()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

} //namespace QtAV
