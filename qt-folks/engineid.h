/*
 * Copyright (C) 2012-2013 Canonical Ltd.
 *   @author Gustavo Pichorim Boiko <gustavo.boiko@canonical.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ENGINEID_H
#define ENGINEID_H

#include <qcontactengineid.h>
#include <QMap>

QTCONTACTS_USE_NAMESPACE

namespace Folks
{

class EngineId : public QContactEngineId
{
public:
    EngineId(const QString &folksId, const QString &managerUri);
    EngineId(const QMap<QString, QString> &parameters, const QString &folksId);

    bool isEqualTo(const QContactEngineId *other) const;
    bool isLessThan(const QContactEngineId *other) const;

    QString managerUri() const;

    QContactEngineId* clone() const;

    QString toString() const;

#ifndef QT_NO_DEBUG_STREAM
    // NOTE: on platforms where Qt is built without debug streams enabled, vtable will differ!
    QDebug& debugStreamOut(QDebug &dbg) const;
#endif
    uint hash() const;

private:
    QString m_folksId;
    QString m_managerUri;
};

}

#endif
