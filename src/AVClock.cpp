/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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


#include <QtAV/AVClock.h>

namespace QtAV {

AVClock::AVClock(AVClock::ClockType c)
    :auto_clock(true),clock_type(c)
{
    pts_ = pts_v = delay_ = 0;
}

void AVClock::setClockType(ClockType ct)
{
    clock_type = ct;
}

AVClock::ClockType AVClock::clockType() const
{
    return clock_type;
}

void AVClock::setClockAuto(bool a)
{
    auto_clock = a;
}

bool AVClock::isClockAuto() const
{
    return auto_clock;
}

void AVClock::start()
{
    qDebug("AVClock started!!!!!!!!");
    timer.start();
}

void AVClock::pause(bool p)
{
    if (clock_type != ExternalClock)
        return;
    if (p)
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        timer.invalidate();
#else
        timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    else
        timer.start();
}

void AVClock::reset()
{
    pts_ = pts_v = delay_ = 0;
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    timer.invalidate();
#else
    timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
}


} //namespace QtAV
