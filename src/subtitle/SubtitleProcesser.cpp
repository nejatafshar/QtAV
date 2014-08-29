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

#include "QtAV/private/SubtitleProcesser.h"
#include "QtAV/FactoryDefine.h"
#include "QtAV/private/factory.h"
#include <QtCore/QFile>
#include <QtDebug>

namespace QtAV {

FACTORY_DEFINE(SubtitleProcesser)

bool SubtitleProcesser::process(const QString &path)
{
    if (!isSupported(RawData))
        return false;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "open subtitle file error: " << f.errorString();
        return false;
    }
    bool ok = process(&f);
    f.close();
    return ok;
}

} //namespace QtAV
