/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "MiscPage.h"
#include <QLabel>
#include <QLayout>
#include "common/Config.h"

MiscPage::MiscPage()
{
    QGridLayout *gl = new QGridLayout();
    setLayout(gl);
    gl->setSizeConstraint(QLayout::SetFixedSize);
    int r = 0;
    m_preview_on = new QCheckBox(tr("Preview"));
    gl->addWidget(m_preview_on, r++, 0);
    m_preview_w = new QSpinBox();
    m_preview_w->setRange(1, 1920);
    m_preview_h = new QSpinBox();
    m_preview_h->setRange(1, 1080);
    gl->addWidget(new QLabel(tr("Preview") + " " + tr("size") + ": "), r, 0);
    gl->addWidget(m_preview_w, r, 1);
    gl->addWidget(new QLabel("x"), r, 2);
    gl->addWidget(m_preview_h, r, 3);

    r++;
    gl->addWidget(new QLabel(tr("Force fps")), r, 0);
    m_fps = new QDoubleSpinBox();
    m_fps->setMinimum(-m_fps->maximum());
    m_fps->setToolTip("<= 0: " + tr("Ignore"));
    gl->addWidget(m_fps, r++, 1);

    gl->addWidget(new QLabel(tr("Progress update interval") + "(ms)"), r, 0);
    m_notify_interval = new QSpinBox();
    m_notify_interval->setEnabled(false);
    gl->addWidget(m_notify_interval, r++, 1);

    gl->addWidget(new QLabel(tr("Buffer frames")), r, 0);
    m_buffer_value = new QSpinBox();
    m_buffer_value->setRange(-1, 32767);
    m_buffer_value->setToolTip("-1: auto. Reopen to apply");
    gl->addWidget(m_buffer_value, r++, 1);

    m_angle = new QCheckBox("Force OpenGL ANGLE (Windows)");
    gl->addWidget(m_angle, r++, 0);

    applyToUi();
}

QString MiscPage::name() const
{
    return tr("Misc");
}


void MiscPage::applyFromUi()
{
    Config::instance().setPreviewEnabled(m_preview_on->isChecked())
            .setPreviewWidth(m_preview_w->value())
            .setPreviewHeight(m_preview_h->value())
            .setANGLE(m_angle->isChecked())
            .setForceFrameRate(m_fps->value())
            .setBufferValue(m_buffer_value->value())
            ;
}

void MiscPage::applyToUi()
{
    m_preview_on->setChecked(Config::instance().previewEnabled());
    m_preview_w->setValue(Config::instance().previewWidth());
    m_preview_h->setValue(Config::instance().previewHeight());
    m_angle->setChecked(Config::instance().isANGLE());
    m_fps->setValue(Config::instance().forceFrameRate());
    //m_notify_interval->setValue(Config::instance().avfilterOptions());
    m_buffer_value->setValue(Config::instance().bufferValue());
}
