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


#include "utils.h"
#include <QContactDetail>
#include <QContactAddress>
#include <QContactOnlineAccount>
#include <QContactPhoneNumber>

QTCONTACTS_USE_NAMESPACE

QStringList Utils::contextsFromEnums(const QList<int> &contexts)
{
    QStringList strings;

    Q_FOREACH(int context, contexts) {
        switch (context) {
        case QContactDetail::ContextHome:
            strings << "home";
            break;
        case QContactDetail::ContextWork:
            strings << "work";
            break;
        default:
            strings << "other";
        }
    }

    return strings;
}

QList<int> Utils::contextsFromStrings(const QStringList &contexts)
{
    QList<int> values;

    Q_FOREACH(const QString &context, contexts) {
        if (context == "home") {
            values << QContactDetail::ContextHome;
        } else if (context == "work") {
            values << QContactDetail::ContextWork;
        } else if (context == "other") {
            values << QContactDetail::ContextOther;
        }
    }

    return values;
}

QStringList Utils::addressSubTypesFromEnums(const QList<int> subTypes)
{
    static QMap<int, QString> map;

    // populate the map once
    if (map.isEmpty()) {
        map[QContactAddress::SubTypeParcel] = "parcel";
        map[QContactAddress::SubTypePostal] = "postal";
        map[QContactAddress::SubTypeDomestic] = "domestic";
        map[QContactAddress::SubTypeInternational] = "international";
    }

    QStringList strings;

    Q_FOREACH(int subType, subTypes) {
        if (map.contains(subType)) {
            strings << map[subType];
        }
    }

    return strings;

}

QList<int> Utils::addressSubTypesFromStrings(const QStringList &subTypes)
{
    static QMap<QString, int> map;

    // populate the map once
    if (map.isEmpty()) {
        map["parcel"] = QContactAddress::SubTypeParcel;
        map["postal"] = QContactAddress::SubTypePostal;
        map["domestic"] = QContactAddress::SubTypeDomestic;
        map["international"] = QContactAddress::SubTypeInternational;
    }

    QList<int> values;

    Q_FOREACH(const QString &subType, subTypes) {
        if (map.contains(subType)) {
            values << map[subType];
        }
    }

    return values;
}

QString Utils::onlineAccountProtocolFromEnum(QContactOnlineAccount::Protocol protocol)
{
    static QMap<QContactOnlineAccount::Protocol, QString> map;

    // populate the map once
    if (map.isEmpty()) {
        map[QContactOnlineAccount::ProtocolAim] = "aim";
        map[QContactOnlineAccount::ProtocolIcq] = "icq";
        map[QContactOnlineAccount::ProtocolIrc] = "irc";
        map[QContactOnlineAccount::ProtocolJabber] = "jabber";
        map[QContactOnlineAccount::ProtocolMsn] = "msn";
        map[QContactOnlineAccount::ProtocolQq] = "qq";
        map[QContactOnlineAccount::ProtocolSkype] = "skype";
        map[QContactOnlineAccount::ProtocolYahoo] = "yahoo";
    }

    if (map.contains(protocol)) {
        return map[protocol];
    }

    return QString::null;
}

QContactOnlineAccount::Protocol Utils::onlineAccountProtocolFromString(const QString &protocol)
{
    static QMap<QString, QContactOnlineAccount::Protocol> map;

    // populate the map once
    if (map.isEmpty()) {
        map["aim"] = QContactOnlineAccount::ProtocolAim;
        map["icq"] = QContactOnlineAccount::ProtocolIcq;
        map["irc"] = QContactOnlineAccount::ProtocolIrc;
        map["jabber"] = QContactOnlineAccount::ProtocolJabber;
        map["msn"] = QContactOnlineAccount::ProtocolMsn;
        map["qq"] = QContactOnlineAccount::ProtocolQq;
        map["skype"] = QContactOnlineAccount::ProtocolSkype;
        map["yahoo"] = QContactOnlineAccount::ProtocolYahoo;
    }

    if (map.contains(protocol)) {
        return map[protocol];
    }

    return QContactOnlineAccount::ProtocolUnknown;
}

QStringList Utils::onlineAccountSubTypesFromEnums(const QList<int> subTypes)
{
    static QMap<int, QString> map;

    // populate the map once
    if (map.isEmpty()) {
        map[QContactOnlineAccount::SubTypeSip] = "sip";
        map[QContactOnlineAccount::SubTypeSipVoip] = "sipvoip";
        map[QContactOnlineAccount::SubTypeImpp] = "impp";
        map[QContactOnlineAccount::SubTypeVideoShare] = "videoshare";
    }

    QStringList strings;

    Q_FOREACH(int subType, subTypes) {
        if (map.contains(subType)) {
            strings << map[subType];
        }
    }

    return strings;

}

QList<int> Utils::onlineAccountSubTypesFromStrings(const QStringList &subTypes)
{
    static QMap<QString, int> map;

    // populate the map once
    if (map.isEmpty()) {
        map["sip"] = QContactOnlineAccount::SubTypeSip;
        map["sipvoip"] = QContactOnlineAccount::SubTypeSipVoip;
        map["impp"] = QContactOnlineAccount::SubTypeImpp;
        map["videoshare"] = QContactOnlineAccount::SubTypeVideoShare;
    }

    QList<int> values;

    Q_FOREACH(const QString &subType, subTypes) {
        if (map.contains(subType)) {
            values << map[subType];
        }
    }

    return values;
}


QStringList Utils::phoneSubTypesFromEnums(const QList<int> subTypes)
{
    static QMap<int, QString> map;

    // populate the map once
    if (map.isEmpty()) {
        map[QContactPhoneNumber::SubTypeLandline] = "landline";
        map[QContactPhoneNumber::SubTypeMobile] = "mobile";
        map[QContactPhoneNumber::SubTypeFax] = "fax";
        map[QContactPhoneNumber::SubTypePager] = "pager";
        map[QContactPhoneNumber::SubTypeVoice] = "voice";
        map[QContactPhoneNumber::SubTypeModem] = "modem";
        map[QContactPhoneNumber::SubTypeVideo] = "video";
        map[QContactPhoneNumber::SubTypeCar] = "car";
        map[QContactPhoneNumber::SubTypeBulletinBoardSystem] = "bulletinboard";
        map[QContactPhoneNumber::SubTypeMessagingCapable] = "messaging";
        map[QContactPhoneNumber::SubTypeAssistant] = "assistant";
        map[QContactPhoneNumber::SubTypeDtmfMenu] = "dtmfmenu";
    }

    QStringList strings;

    Q_FOREACH(int subType, subTypes) {
        if (map.contains(subType)) {
            strings << map[subType];
        }
    }

    return strings;
}


QList<int> Utils::phoneSubTypesFromStrings(const QStringList &subTypes)
{
    static QMap<QString, int> map;

    // populate the map once
    if (map.isEmpty()) {
        map["landline"] = QContactPhoneNumber::SubTypeLandline;
        map["mobile"] = QContactPhoneNumber::SubTypeMobile;
        map["fax"] = QContactPhoneNumber::SubTypeFax;
        map["pager"] = QContactPhoneNumber::SubTypePager;
        map["voice"] = QContactPhoneNumber::SubTypeVoice;
        map["modem"] = QContactPhoneNumber::SubTypeModem;
        map["video"] = QContactPhoneNumber::SubTypeVideo;
        map["car"] = QContactPhoneNumber::SubTypeCar;
        map["bulletinboard"] = QContactPhoneNumber::SubTypeBulletinBoardSystem;
        map["messaging"] = QContactPhoneNumber::SubTypeMessagingCapable;
        map["assistant"] = QContactPhoneNumber::SubTypeAssistant;
        map["dtmfmenu"] = QContactPhoneNumber::SubTypeDtmfMenu;
    }

    QList<int> values;

    Q_FOREACH(const QString &subType, subTypes) {
        if (map.contains(subType)) {
            values << map[subType];
        }
    }

    return values;
}
