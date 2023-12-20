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

#ifndef UTILS_H
#define UTILS_H

#include <QStringList>
#include <QContactOnlineAccount>

QTCONTACTS_USE_NAMESPACE

class Utils
{
public:
static QStringList contextsFromEnums(const QList<int> &contexts);
static QList<int> contextsFromStrings(const QStringList &contexts);

static QStringList addressSubTypesFromEnums(const QList<int> subTypes);
static QList<int> addressSubTypesFromStrings(const QStringList &subTypes);

static QString onlineAccountProtocolFromEnum(QContactOnlineAccount::Protocol protocol);
static QContactOnlineAccount::Protocol onlineAccountProtocolFromString(const QString &protocol);

static QStringList onlineAccountSubTypesFromEnums(const QList<int> subTypes);
static QList<int> onlineAccountSubTypesFromStrings(const QStringList &subTypes);

static QStringList phoneSubTypesFromEnums(const QList<int> subTypes);
static QList<int> phoneSubTypesFromStrings(const QStringList &subTypes);
};

#endif
