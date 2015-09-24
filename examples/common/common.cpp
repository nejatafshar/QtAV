/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <QFileOpenEvent>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QCoreApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#include <QtDebug>

static FILE *sLogfile = 0; //'log' is a function in msvc math.h
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class QMessageLogContext {};
typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
QtMsgHandler qInstallMessageHandler(QtMessageHandler h) {
    static QtMessageHandler hh;
    hh = h;
    struct MsgHandlerWrapper {
        static void handler(QtMsgType type, const char *msg) {
            static QMessageLogContext ctx;
            hh(type, ctx, msg);
        }
    };
    return qInstallMsgHandler(MsgHandlerWrapper::handler);
}
#endif
void Logger(QtMsgType type, const QMessageLogContext &, const QString& qmsg)
{
    const QByteArray msgArray = qmsg.toLocal8Bit();
    const char* msg = msgArray.constData();
     switch (type) {
     case QtDebugMsg:
         fprintf(stdout, "Debug: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Debug: %s\n", msg);
         break;
     case QtWarningMsg:
         fprintf(stdout, "Warning: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         if (sLogfile)
            fprintf(sLogfile, "Fatal: %s\n", msg);
         abort();
     }
     fflush(0);
}

QOptions get_common_options()
{
    static QOptions ops = QOptions().addDescription(QString::fromLatin1("Options for QtAV players"))
            .add(QString::fromLatin1("common options"))
            ("help,h", QLatin1String("print this"))
            ("ao", QString(), QLatin1String("audio output. Can be ordered combination of available backends (-ao help). Leave empty to use the default setting. Set 'null' to disable audio."))
            ("-gl", QLatin1String("OpenGL backend for Qt>=5.4(windows). can be 'desktop', 'opengles' and 'software'"))
            ("x", 0, QString())
            ("y", 0, QLatin1String("y"))
            ("-width", 800, QLatin1String("width of player"))
            ("height", 450, QLatin1String("height of player"))
            ("fullscreen", QLatin1String("fullscreen"))
            ("decoder", QLatin1String("FFmpeg"), QLatin1String("use a given decoder"))
            ("decoders,-vd", QLatin1String("cuda;vaapi;vda;dxva;cedarv;ffmpeg"), QLatin1String("decoder name list in priority order seperated by ';'"))
            ("file,f", QString(), QLatin1String("file or url to play"))
            ("language", QLatin1String("system"), QLatin1String("language on UI. can be 'system', 'none' and locale name e.g. zh_CN"))
            ("logfile", QString::fromLatin1("log-%1.txt"), QString::fromLatin1("log to file. Set empty to disable log file (-logfile '')"))
            ;
    return ops;
}

void do_common_options(const QOptions &options, const QString& appName)
{
    if (options.value(QString::fromLatin1("help")).toBool()) {
        options.print();
        exit(0);
    }
    // has no effect if qInstallMessageHandler() called
    //qSetMessagePattern("%{function} @%{line}: %{message}");
    QString app(appName);
    if (app.isEmpty() && qApp)
        app = qApp->applicationName();
    QString logfile(options.option(QString::fromLatin1("logfile")).value().toString().arg(app));
    if (!logfile.isEmpty()) {
        sLogfile = fopen(logfile.toUtf8().constData(), "w+");
        if (!sLogfile) {
            qWarning("Failed to open log file");
            sLogfile = stdout;
        }
        qInstallMessageHandler(Logger);
    }
    // TODO: ffmpeg level depends on libQtAV
}

void load_qm(const QStringList &names, const QString& lang)
{
    if (lang.isEmpty() || lang.toLower() == QLatin1String("none"))
        return;
    QString l(lang);
    if (l.toLower() == QLatin1String("system"))
        l = QLocale::system().name();
    QStringList qms(names);
    qms << QLatin1String("QtAV") << QLatin1String("qt");
    foreach(QString qm, qms) {
        QTranslator *ts = new QTranslator(qApp);
        QString path = qApp->applicationDirPath() + QLatin1String("/i18n/") + qm + QLatin1String("_") + l;
        //qDebug() << "loading qm: " << path;
        if (ts->load(path)) {
            qApp->installTranslator(ts);
        } else {
            path = QString::fromUtf8(":/i18n/%1_%2").arg(qm).arg(l);
            //qDebug() << "loading qm: " << path;
            if (ts->load(path))
                qApp->installTranslator(ts);
            else
                delete ts;
        }
    }
    QTranslator qtts;
    if (qtts.load(QLatin1String("qt_") + QLocale::system().name()))
        qApp->installTranslator(&qtts);
}

void set_opengl_backend(const QString& glopt, const QString &appname)
{
    QString gl = appname.toLower().replace(QLatin1String("\\"), QLatin1String("/"));
    int idx = gl.lastIndexOf(QLatin1String("/"));
    if (idx >= 0)
        gl = gl.mid(idx + 1);
    idx = gl.lastIndexOf(QLatin1String("."));
    if (idx > 0)
        gl = gl.left(idx);
    if (gl.indexOf(QLatin1String("-desktop")) > 0)
        gl = QLatin1String("desktop");
    else if (gl.indexOf(QLatin1String("-es")) > 0 || gl.indexOf(QLatin1String("-angle")) > 0)
        gl = gl.mid(gl.indexOf(QLatin1String("-es")) + 1);
    else if (gl.indexOf(QLatin1String("-sw")) > 0 || gl.indexOf(QLatin1String("-software")) > 0)
        gl = QLatin1String("software");
    else
        gl = glopt.toLower();
    if (gl.isEmpty()) {
        switch (Config::instance().openGLType()) {
        case Config::Desktop:
            gl = QLatin1String("desktop");
            break;
        case Config::OpenGLES:
            gl = QLatin1String("es");
            break;
        case Config::Software:
            gl = QLatin1String("software");
            break;
        default:
            break;
        }
    }
    if (gl == QLatin1String("es") || gl == QLatin1String("angle") || gl == QLatin1String("opengles")) {
        gl = QLatin1String("es_");
        gl.append(Config::instance().getANGLEPlatform().toLower());
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    if (gl.startsWith(QLatin1String("es"))) {
        qApp->setAttribute(Qt::AA_UseOpenGLES);
#ifdef QT_OPENGL_DYNAMIC
        qputenv("QT_OPENGL", "angle");
#endif
#ifdef Q_OS_WIN
        if (gl.endsWith(QLatin1String("d3d11")))
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
        else if (gl.endsWith(QLatin1String("d3d9")))
            qputenv("QT_ANGLE_PLATFORM", "d3d9");
        else if (gl.endsWith(QLatin1String("warp")))
            qputenv("QT_ANGLE_PLATFORM", "warp");
#endif
    } else if (gl == QLatin1String("desktop")) {
        qApp->setAttribute(Qt::AA_UseDesktopOpenGL);
    } else if (gl == QLatin1String("software")) {
        qApp->setAttribute(Qt::AA_UseSoftwareOpenGL);
    }
#endif
}


QString appDataDir()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif //5.4.0
#endif // 5.0.0
}

AppEventFilter::AppEventFilter(QObject *player, QObject *parent)
    : QObject(parent)
    , m_player(player)
{}

bool AppEventFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != qApp)
        return false;
    if (ev->type() != QEvent::FileOpen)
        return false;
    QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(ev);
    if (m_player)
        QMetaObject::invokeMethod(m_player, "play", Q_ARG(QUrl, QUrl(foe->url())));
    return true;
}

static void initResources() {
    Q_INIT_RESOURCE(theme);
}

namespace {
    struct ResourceLoader {
    public:
        ResourceLoader() { initResources(); }
    } qrc;
}


