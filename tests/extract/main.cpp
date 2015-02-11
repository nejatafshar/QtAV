/******************************************************************************
    VideoGroup:  this file is part of QtAV examples
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

#include <QtAV/VideoFrameExtractor.h>
#include <QApplication>
#include <QWidget>
#include <QtAVWidgets>
#include <QtDebug>
#include <QtCore/QElapsedTimer>

using namespace QtAV;
class VideoFrameObserver : public QObject
{
    Q_OBJECT
public:
    VideoFrameObserver(QObject *parent = 0) : QObject(parent)
    {
        view = VideoRendererFactory::create(VideoRendererId_GLWidget2);
        view->widget()->resize(400, 300);
        view->widget()->show();
    }

public Q_SLOTS:
    void onVideoFrameExtracted(const QtAV::VideoFrame& frame) {
        view->receive(frame);
        qApp->processEvents();
        qDebug("frame %dx%d @%f", frame.width(), frame.height(), frame.timestamp());
    }
private:
    VideoRenderer *view;
};

int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    int idx = a.arguments().indexOf("-f");
    if (idx < 0) {
        qDebug("-f file -t sec -n count -asyc");
        return -1;
    }
    QString file = a.arguments().at(idx+1);
    idx = a.arguments().indexOf("-t");
    int t = 0;
    if (idx > 0)
        t = a.arguments().at(idx+1).toInt();
    int n = 1;
    idx = a.arguments().indexOf("-n");
    if (idx > 0)
        n = a.arguments().at(idx+1).toInt();
    bool async = a.arguments().contains("-async");


    VideoFrameExtractor extractor;
    extractor.setAsync(async);
    VideoFrameObserver obs;
    QObject::connect(&extractor, SIGNAL(frameExtracted(QtAV::VideoFrame)), &obs, SLOT(onVideoFrameExtracted(QtAV::VideoFrame)));
    extractor.setSource(file);

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < n; ++i) {
        // async does not work. you have to set a new position when frameExtracted is emitted
        extractor.setPosition(t + 1000*i);
    }
    qDebug("elapsed: %lld", timer.elapsed());
    return a.exec();
}

#include "main.moc"
