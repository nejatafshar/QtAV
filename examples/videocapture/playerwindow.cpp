/******************************************************************************
    Simple Player:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "playerwindow.h"
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>

using namespace QtAV;

PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("QtAV simple player example");
    m_player = new AVPlayer(this);
    QVBoxLayout *vl = new QVBoxLayout();
    setLayout(vl);
    m_vo = new VideoOutput(this);
    if (!m_vo->widget()) {
        QMessageBox::warning(0, "QtAV error", "Can not create video renderer");
        return;
    }
    m_player->setRenderer(m_vo);
    vl->addWidget(m_vo->widget());
    m_slider = new QSlider();
    m_slider->setOrientation(Qt::Horizontal);
    connect(m_slider, SIGNAL(sliderMoved(int)), SLOT(seek(int)));
    connect(m_player, SIGNAL(positionChanged(qint64)), SLOT(updateSlider()));
    connect(m_player, SIGNAL(started()), SLOT(updateSlider()));

    vl->addWidget(m_slider);
    QHBoxLayout *hb = new QHBoxLayout();
    vl->addLayout(hb);
    m_openBtn = new QPushButton("Open");
    m_captureBtn = new QPushButton("Capture video frame");
    hb->addWidget(m_openBtn);
    hb->addWidget(m_captureBtn);

    m_preview = new QLabel("Capture preview");
    m_preview->setFixedSize(200, 150);
    hb->addWidget(m_preview);
    connect(m_openBtn, SIGNAL(clicked()), SLOT(openMedia()));
    connect(m_captureBtn, SIGNAL(clicked()), SLOT(capture()));
    connect(m_player->videoCapture(), SIGNAL(ready(QtAV::VideoFrame)), SLOT(updatePreview(QtAV::VideoFrame)));
    connect(m_player->videoCapture(), SIGNAL(saved(QString)), SLOT(onCaptureSaved(QString)));
    connect(m_player->videoCapture(), SIGNAL(failed()), SLOT(onCaptureError()));
}

void PlayerWindow::openMedia()
{
    QString file = QFileDialog::getOpenFileName(0, "Open a video");
    if (file.isEmpty())
        return;
    m_player->play(file);
}

void PlayerWindow::seek(int pos)
{
    if (!m_player->isPlaying())
        return;
    m_player->seek(pos*1000LL); // to msecs
}

void PlayerWindow::updateSlider()
{
    m_slider->setRange(0, int(m_player->duration()/1000LL));
    m_slider->setValue(int(m_player->position()/1000LL));
}

void PlayerWindow::updatePreview(const VideoFrame &frame)
{
    VideoFrame f(frame);
    ImageConverter *ic = ImageConverterFactory::create(ImageConverterId_FF);
    f.setImageConverter(ic);
    if (!f.convertTo(QImage::Format_ARGB32)) {
        delete ic;
        return;
    }
    delete ic;
    // this img is constructed from raw data. It does not copy the data. You have to use img = img.copy() before if use img somewhere else
    QImage img((const uchar*)frame.frameData().constData(), frame.width(), frame.height(), frame.bytesPerLine(), QImage::Format_ARGB32);
    m_preview->setPixmap(QPixmap::fromImage(img).scaled(m_preview->size()));
}

void PlayerWindow::capture()
{
    //m_player->captureVideo();
    m_player->videoCapture()->request();
}

void PlayerWindow::onCaptureSaved(const QString &path)
{
    setWindowTitle("saved to: " + path);
}

void PlayerWindow::onCaptureError()
{
    QMessageBox::warning(0, "QtAV video capture", "Failed to capture video frame");
}
