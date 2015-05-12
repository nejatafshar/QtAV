/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "internal.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {
namespace Internal {

QString options2StringHelper(void* obj, const char* unit)
{
    QString s;
    const AVOption* opt = NULL;
    while ((opt = av_opt_next(obj, opt))) {
        if (opt->type == AV_OPT_TYPE_CONST) {
            if (!unit)
                continue;
            if (!qstrcmp(unit, opt->unit))
                s.append(QString(" %1=%2").arg(opt->name).arg(opt->default_val.i64));
            continue;
        } else {
            if (unit)
                continue;
        }
        s.append(QString("\n%1: ").arg(opt->name));
        switch (opt->type) {
        case AV_OPT_TYPE_FLAGS:
        case AV_OPT_TYPE_INT:
        case AV_OPT_TYPE_INT64:
            s.append(QString("(%1)").arg(opt->default_val.i64));
            break;
        case AV_OPT_TYPE_DOUBLE:
        case AV_OPT_TYPE_FLOAT:
            s.append(QString("(%1)").arg(opt->default_val.dbl, 0, 'f'));
            break;
        case AV_OPT_TYPE_STRING:
            if (opt->default_val.str)
                s.append(QString("(%1)").arg(opt->default_val.str));
            break;
        case AV_OPT_TYPE_RATIONAL:
            s.append(QString("(%1/%2)").arg(opt->default_val.q.num).arg(opt->default_val.q.den));
            break;
        default:
            break;
        }
        if (opt->help)
            s.append(" ").append(opt->help);
        if (opt->unit && opt->type != AV_OPT_TYPE_CONST)
            s.append("\n ").append(options2StringHelper(obj, opt->unit));
    }
    return s;
}

QString optionsToString(void* obj) {
    return options2StringHelper(obj, NULL);
}

void setOptionsToFFmpegObj(const QVariant& opt, void* obj)
{
    if (!opt.isValid())
        return;
    AVClass *c = obj ? *(AVClass**)obj : 0;
    if (c)
        qDebug() << QString("%1.%2 options:").arg(c->class_name).arg(c->item_name(obj));
    else
        qDebug() << "options:";
    if (opt.type() == QVariant::Map) {
        QVariantMap options(opt.toMap());
        if (options.isEmpty())
            return;
        QMapIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            const QVariant::Type vt = i.value().type();
            if (vt == QVariant::Map)
                continue;
            const QByteArray key(i.key().toLower().toUtf8());
            qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
            if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_opt_set_int(obj, key.constData(), i.value().toInt(), 0);
            } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
                av_opt_set_int(obj, key.constData(), i.value().toLongLong(), 0);
            } else if (vt == QVariant::Double) {
                av_opt_set_double(obj, key.constData(), i.value().toDouble(), 0);
            }
        }
        return;
    }
    QVariantHash options(opt.toHash());
    if (options.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toLower().toUtf8());
        qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
            av_opt_set_int(obj, key.constData(), i.value().toInt(), 0);
        } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
            av_opt_set_int(obj, key.constData(), i.value().toLongLong(), 0);
        }
    }
}

void setOptionsToDict(const QVariant& opt, AVDictionary** dict)
{
    if (!opt.isValid())
        return;
    if (opt.type() == QVariant::Map) {
        QVariantMap options(opt.toMap());
        if (options.isEmpty())
            return;
        QMapIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            const QVariant::Type vt = i.value().type();
            if (vt == QVariant::Map)
                continue;
            const QByteArray key(i.key().toLower().toUtf8());
            switch (vt) {
            case QVariant::Bool: {
                // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
                av_dict_set(dict, key.constData(), QByteArray::number(i.value().toInt()), 0);
            }
                break;
            default:
                // key and value are in lower case
                av_dict_set(dict, i.key().toLower().toUtf8().constData(), i.value().toByteArray().toLower().constData(), 0);
                break;
            }
            qDebug("dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
        return;
    }
    QVariantHash options(opt.toHash());
    if (options.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toLower().toUtf8());
        switch (vt) {
        case QVariant::Bool: {
            // QVariant.toByteArray(): "true" or "false", can not recognized by avcodec
            av_dict_set(dict, key.constData(), QByteArray::number(i.value().toInt()), 0);
        }
            break;
        default:
            // key and value are in lower case
            av_dict_set(dict, i.key().toLower().toUtf8().constData(), i.value().toByteArray().toLower().constData(), 0);
            break;
        }
        qDebug("dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

void setOptionsForQObject(const QVariant& opt, QObject *obj)
{
    if (!opt.isValid())
        return;
    qDebug() << QString("set %1(%2) meta properties:").arg(obj->metaObject()->className()).arg(obj->objectName());
    if (opt.type() == QVariant::Hash) {
        QVariantHash options(opt.toHash());
        if (options.isEmpty())
            return;
        QHashIterator<QString, QVariant> i(options);
        while (i.hasNext()) {
            i.next();
            if (i.value().type() == QVariant::Hash) // for example "vaapi": {...}
                continue;
            obj->setProperty(i.key().toUtf8().constData(), i.value());
            qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        }
    }
    if (opt.type() != QVariant::Map)
        return;
    QVariantMap options(opt.toMap());
    if (options.isEmpty())
        return;
    QMapIterator<QString, QVariant> i(options);
    while (i.hasNext()) {
        i.next();
        if (i.value().type() == QVariant::Map) // for example "vaapi": {...}
            continue;
        obj->setProperty(i.key().toUtf8().constData(), i.value());
        qDebug("%s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

} //namespace Internal
} //namespace QtAV
