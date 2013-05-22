#include "MainWindow.h"
#include <QtCore/QTimer>
#include <QTimeEdit>
#include <QLabel>
#include <QtCore/QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include "Button.h"
#include "Slider.h"

#define SLIDER_ON_VO 0
//TODO: custome slider, mouse pressevent -> <-

#define AVDEBUG() \
    qDebug("%s %s @%d", __FILE__, __FUNCTION__, __LINE__);

const int kSliderUpdateInterval = 500;
using namespace QtAV;

static void QLabelSetElideText(QLabel *label, QString text, int W = 0)
{
    QFontMetrics metrix(label->font());
    int width = label->width() - label->indent() - label->margin();
    if (W || label->parent()) {
        int w = W;
        if (!w)
            w = ((QWidget*)label->parent())->width();
        width = qMax(w - label->indent() - label->margin() - 13*(30), 0); //TODO: why 30?
    }
    QString clippedText = metrix.elidedText(text, Qt::ElideRight, width);
    label->setText(clippedText);
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent)
  , mIsReady(false)
  , mHasPendingPlay(false)
  , mTimerId(0)
  , mpRenderer(0)
  , mpTempRenderer(0)
{
    connect(this, SIGNAL(ready()), SLOT(processPendingActions()));
    //QTimer::singleShot(10, this, SLOT(setupUi()));
    setupUi();
}

void MainWindow::initPlayer()
{
    mpPlayer = new AVPlayer(this);
    mIsReady = true;
    qDebug("player created");
    connect(mpStopBtn, SIGNAL(clicked()), mpPlayer, SLOT(stop()));
    connect(mpForwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekForward()));
    connect(mpBackwardBtn, SIGNAL(clicked()), mpPlayer, SLOT(seekBackward()));
    connect(mpPlayer, SIGNAL(started()), this, SLOT(onStartPlay()));
    connect(mpPlayer, SIGNAL(stopped()), this, SLOT(onStopPlay()));
    connect(mpPlayer, SIGNAL(paused(bool)), this, SLOT(onPaused(bool)));
    emit ready(); //emit this signal after connection. otherwise the slots may not be called for the first time
}

void MainWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

    mpPlayerLayout = new QVBoxLayout();
    mpControl = new QWidget(this);
    mpControl->setMaximumHeight(22);

    mpTimeSlider = new Slider(mpControl);
    mpTimeSlider->setDisabled(true);
    //mpTimeSlider->setFixedHeight(8);
    mpTimeSlider->setMaximumHeight(8);
    mpTimeSlider->setTracking(true);
    mpTimeSlider->setOrientation(Qt::Horizontal);
    mpTimeSlider->setMinimum(0);
#if SLIDER_ON_VO
    QGraphicsOpacityEffect *oe = new QGraphicsOpacityEffect(this);
    oe->setOpacity(0.5);
    mpTimeSlider->setGraphicsEffect(oe);
#endif //SLIDER_ON_VO

    mpCurrent = new QLabel(mpControl);
    mpCurrent->setMargin(2);
    mpCurrent->setText("00:00");
    mpDuration = new QLabel(mpControl);
    mpDuration->setMargin(2);
    mpDuration->setText("00:00");
    mpTitle = new QLabel(mpControl);
    mpTitle->setIndent(8);

    mPlayPixmap = QPixmap(":/theme/button-play-pause.png");
    int w = mPlayPixmap.width(), h = mPlayPixmap.height();
    mPausePixmap = mPlayPixmap.copy(QRect(w/2, 0, w/2, h));
    mPlayPixmap = mPlayPixmap.copy(QRect(0, 0, w/2, h));
    qDebug("%d x %d", mPlayPixmap.width(), mPlayPixmap.height());
    mpPlayPauseBtn = new Button(mpControl);
    int a = qMin(w/2, h);
    const int kMaxButtonIconWidth = 18;
    const int kMaxButtonIconMargin = kMaxButtonIconWidth/3;
    a = qMin(a, kMaxButtonIconWidth);
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpPlayPauseBtn->setIconSize(QSize(a, a));
    mpPlayPauseBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpStopBtn = new Button(mpControl);
    mpStopBtn->setIconWithSates(QPixmap(":/theme/button-stop.png"));
    mpStopBtn->setIconSize(QSize(a, a));
    mpStopBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpBackwardBtn = new Button(mpControl);
    mpBackwardBtn->setIconWithSates(QPixmap(":/theme/button-rewind.png"));
    mpBackwardBtn->setIconSize(QSize(a, a));
    mpBackwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpForwardBtn = new Button(mpControl);
    mpForwardBtn->setIconWithSates(QPixmap(":/theme/button-fastforward.png"));
    mpForwardBtn->setIconSize(QSize(a, a));
    mpForwardBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpOpenBtn = new Button(mpControl);
    mpOpenBtn->setIconWithSates(QPixmap(":/theme/open_folder.png"));
    mpOpenBtn->setIconSize(QSize(a, a));
    mpOpenBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);

    mpInfoBtn = new Button();
    mpInfoBtn->setIconWithSates(QPixmap(":/theme/info.png"));
    mpInfoBtn->setIconSize(QSize(a, a));
    mpInfoBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpCaptureBtn = new Button();
    mpCaptureBtn->setIconWithSates(QPixmap(":/theme/screenshot.png"));
    mpCaptureBtn->setIconSize(QSize(a, a));
    mpCaptureBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);
    mpMenuBtn = new Button();
    mpMenuBtn->setIconWithSates(QPixmap(":/theme/search-arrow.png"));
    mpMenuBtn->setIconSize(QSize(a, a));
    mpMenuBtn->setMaximumSize(a+kMaxButtonIconMargin+2, a+kMaxButtonIconMargin);


    mainLayout->addLayout(mpPlayerLayout);
    mainLayout->addWidget(mpTimeSlider);
    mainLayout->addWidget(mpControl);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(0);
    controlLayout->setMargin(1);
    mpControl->setLayout(controlLayout);
    controlLayout->addWidget(mpCurrent);
    controlLayout->addWidget(mpTitle);
    QSpacerItem *space = new QSpacerItem(mpPlayPauseBtn->width(), mpPlayPauseBtn->height(), QSizePolicy::MinimumExpanding);
    controlLayout->addSpacerItem(space);
    controlLayout->addWidget(mpCaptureBtn);
    controlLayout->addWidget(mpPlayPauseBtn);
    controlLayout->addWidget(mpStopBtn);
    controlLayout->addWidget(mpBackwardBtn);
    controlLayout->addWidget(mpForwardBtn);
    controlLayout->addWidget(mpOpenBtn);
    controlLayout->addWidget(mpInfoBtn);
    //controlLayout->addWidget(mpSetupBtn);
    controlLayout->addWidget(mpMenuBtn);

    controlLayout->addWidget(mpDuration);

    connect(mpOpenBtn, SIGNAL(clicked()), SLOT(openFile()));
    connect(mpPlayPauseBtn, SIGNAL(clicked()), SLOT(togglePlayPause()));
    connect(mpCaptureBtn, SIGNAL(clicked()), this, SLOT(capture()));
    //valueChanged can be triggered by non-mouse event
    //TODO: connect sliderMoved(int) to preview(int)
    //connect(mpTimeSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekToMSec(int)));
    connect(mpTimeSlider, SIGNAL(sliderPressed()), SLOT(seek()));
    connect(mpTimeSlider, SIGNAL(sliderReleased()), SLOT(seek()));

    QTimer::singleShot(0, this, SLOT(initPlayer()));
}

void MainWindow::processPendingActions()
{
    if (!mpTempRenderer)
        return;
    setRenderer(mpTempRenderer);
    mpTempRenderer = 0;
    if (mHasPendingPlay) {
        mHasPendingPlay = false;
        play(mFile);
    }
}

void MainWindow::setRenderer(QtAV::VideoRenderer *renderer)
{
    qDebug(">>>>>>>>>>%s @%d", __FUNCTION__, __LINE__);
    if (!mIsReady) {
        mpTempRenderer = renderer;
        return;
    }
    if (!renderer)
        return;
#if SLIDER_ON_VO
    int old_pos = 0;
    int old_total = 0;

    if (mpTimeSlider) {
        old_pos = mpTimeSlider->value();
        old_total = mpTimeSlider->maximum();
        mpTimeSlider->hide();
    } else {
    }
#endif //SLIDER_ON_VO
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    if (r) {
        mpPlayerLayout->removeWidget(r);
        r->close();
        delete r;
        r = 0;
    }
    mpRenderer = renderer;
    mpPlayer->setRenderer(mpRenderer);
#if SLIDER_ON_VO
    if (mpTimeSlider) {
        mpTimeSlider->setParent(mpRenderer->widget());
        mpTimeSlider->show();
    }
#endif //SLIDER_ON_VO
    qDebug("add renderer to layout");
    mpPlayerLayout->addWidget(mpRenderer->widget());
    resize(mpRenderer->widget()->size());
}

void MainWindow::play(const QString &name)
{
    mFile = name;
    if (!mIsReady) {
        mHasPendingPlay = true;
        return;
    }
    QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName());
    mpPlayer->play(name);
}

void MainWindow::openFile()
{
    QString file = QFileDialog::getOpenFileName(0, tr("Open a media file"));
    if (file.isEmpty())
        return;
    play(file);
}

void MainWindow::togglePlayPause()
{
    if (mpPlayer->isPlaying()) {
        qDebug("isPaused = %d", mpPlayer->isPaused());
        mpPlayer->pause(!mpPlayer->isPaused());
    } else {
        if (mFile.isEmpty())
            return;
        mpPlayer->play();
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onPaused(bool p)
{
    if (p) {
        qDebug("start pausing...");
        if (mTimerId)
            killTimer(mTimerId);
        mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    } else {
        qDebug("stop pausing...");
        mTimerId = startTimer(kSliderUpdateInterval);
        mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    }
}

void MainWindow::onStartPlay()
{
    mpPlayPauseBtn->setIconWithSates(mPausePixmap);
    mpTimeSlider->setMaximum(mpPlayer->duration()*1000);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>enable slider");
    mpTimeSlider->setEnabled(true);
    mpDuration->setText(QTime().addSecs(mpPlayer->duration()).toString("mm:ss"));
    mTimerId = startTimer(kSliderUpdateInterval);
}

void MainWindow::onStopPlay()
{
    mpPlayPauseBtn->setIconWithSates(mPlayPixmap);
    mpTimeSlider->setValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
    mpTimeSlider->setDisabled(true);
    mpCurrent->setText("00:00");
    mpDuration->setText("00:00");
}

void MainWindow::seekToMSec(int msec)
{
    mpPlayer->seek(qreal(msec)/qreal(mpTimeSlider->maximum()));
}

void MainWindow::seek()
{
    mpPlayer->seek(qreal(mpTimeSlider->value())/qreal(mpTimeSlider->maximum()));
}

void MainWindow::capture()
{
    mpPlayer->captureVideo();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    if (mpTitle)
        QLabelSetElideText(mpTitle, QFileInfo(mFile).fileName(), e->size().width());
#if SLIDER_ON_VO
    int m = 4;
    QWidget *w = static_cast<QWidget*>(mpTimeSlider->parent());
    if (w) {
        mpTimeSlider->resize(w->width() - m*2, 44);
        qDebug("%d %d %d", m, w->height() - mpTimeSlider->height() - m, w->width() - m*2);
        mpTimeSlider->move(m, w->height() - mpTimeSlider->height() - m);
    }
#endif //SLIDER_ON_VO
}

void MainWindow::timerEvent(QTimerEvent *)
{
    int ms = mpPlayer->masterClock()->value()*1000.0;
    mpTimeSlider->setValue(ms);
    mpCurrent->setText(QTime().addMSecs(ms).toString("mm:ss"));
}
