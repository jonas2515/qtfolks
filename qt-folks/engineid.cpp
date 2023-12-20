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

#include "engineid.h"
#include <QContactManager>

namespace Folks
{

EngineId::EngineId(const QString &folksId, const QString &managerUri)
: m_folksId(folksId), m_managerUri(managerUri)
{
}

EngineId::EngineId(const QMap<QString, QString> &parameters, const QString &folksId)
: m_folksId(folksId)
{
    m_managerUri = QContactManager::buildUri("folks", parameters);
}

bool EngineId::isEqualTo(const QContactEngineId *other) const
{
    const EngineId *otherId = dynamic_cast<const EngineId*>(other);
    if (!otherId) {
        return false;
    }

    return otherId->m_folksId == m_folksId;
}


bool EngineId::isLessThan(const QContactEngineId *other) const
{
    const EngineId *otherId = dynamic_cast<const EngineId*>(other);
    if (!otherId) {
        return false;
    }

    return m_folksId < otherId->m_folksId;
}


QString EngineId::managerUri() const
{
    return m_managerUri;
}

QContactEngineId* EngineId::clone() const
{
    return new EngineId(m_folksId, m_managerUri);
}

QString EngineId::toString() const
{
    return m_folksId;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug& EngineId::debugStreamOut(QDebug &dbg) const
{
    dbg.nospace() << "Folks::EngineId(" << m_folksId << "," << m_managerUri << ")";
    return dbg.maybeSpace();
}
#endif

uint EngineId::hash() const
{
    return qHash(m_folksId);
}

}
