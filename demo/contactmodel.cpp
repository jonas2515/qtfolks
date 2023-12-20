/*
 * Copyright (C) 2010 Collabora Ltd.
 *   @author Marco Barisione <marco.barisione@collabora.co.uk>
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

#include <QContactAddress>
#include <QContactAvatar>
#include <QContactBirthday>
#include <QContactEmailAddress>
#include <QContactFavorite>
#include <QContactGender>
#include <QContactGlobalPresence>
#include <QContactIdFilter>
#include <QContactOnlineAccount>
#include <QContactManager>
#include <QContactName>
#include <QContactNickname>
#include <QContactNote>
#include <QContactOrganization>
#include <QContactPhoneNumber>
#include <QContactDisplayLabel>
#include <QContactUrl>
#include <QDebug>
#include <QDesktopServices>
#include "contactmodel.h"
//#include "_gen/contactmodel.moc.hpp"


ContactModel::ContactModel(QObject *parent)
: QAbstractListModel(parent)
{
    // Contact manager
    QMap<QString, QString> params;
    QString managerUri = QContactManager::buildUri(
            QLatin1String("folks"), params);
    m_manager = QContactManager::fromUri(managerUri);
    Q_ASSERT(m_manager);

    connect(m_manager, SIGNAL(contactsAdded(const QList<QContactId>&)),
            this, SLOT(contactsAdded(const QList<QContactId>&)));
    connect(m_manager, SIGNAL(contactsRemoved(const QList<QContactId>&)),
            this, SLOT(contactsRemoved(const QList<QContactId>&)));
    connect(m_manager, SIGNAL(contactsChanged(const QList<QContactId>&,  const QList<QContactDetail::DetailType>&)),
            this, SLOT(contactsChanged(const QList<QContactId>&, const QList<QContactDetail::DetailType>&)));

    // Model
    m_roles[DisplayNameRole] = "displayName";
    m_roles[PresenceMessageRole] = "presenceMessage";
    m_roles[PresenceIconRole] = "presenceIcon";
    m_roles[AvatarImageRole] = "avatarImage";
}

ContactModel::~ContactModel()
{
}

QHash<int, QByteArray> ContactModel::roleNames() const
{
    return m_roles;
}

QVariant ContactModel::data(
        const QModelIndex& index,
        int role) const
{
    Q_ASSERT(index.isValid());

    if(index.row() < 0 || index.row() > m_contacts.size())
        return QVariant();

    const QContact& contact = m_contacts[index.row()];

    switch(role) {
    case DisplayNameRole:
        return contact.detail<QContactDisplayLabel>().label();

    case PresenceMessageRole: {
        QContactGlobalPresence presence =
            contact.detail<QContactGlobalPresence>();
        QString msg = presence.customMessage();
        if(msg.isEmpty()) {
            switch(presence.presenceState()) {
            case QContactPresence::PresenceAvailable:
                msg = tr("Available");
                break;
            case QContactPresence::PresenceBusy:
                msg = tr("Busy");
                break;
            case QContactPresence::PresenceAway:
            case QContactPresence::PresenceExtendedAway:
                msg = tr("Away");
                break;
            case QContactPresence::PresenceUnknown:
                msg = QLatin1String("");
                break;
            default:
                msg = tr("Offline");
            }
        }
        return msg;
    }

    case PresenceIconRole:
        if(contact.details<QContactGlobalPresence>().isEmpty())
            return QUrl();
        else {
            QContactGlobalPresence presence =
                contact.detail<QContactGlobalPresence>();
            return presenceIconForDetail(presence);
        }

    case AvatarImageRole:
        return contact.detail<QContactAvatar>().imageUrl();

    default:
        return QVariant();
    }
}

int ContactModel::rowCount(
        const QModelIndex& parent) const
{
    if(parent.isValid())
        // This is not a tree, this should not happen
        return 0;

    return m_contacts.size();
}

QList<QContact> ContactModel::contactsFromIds(
        const QList<QContactId>& ids)
{
    QContactIdFilter idFilter;
    idFilter.setIds(ids);
    return m_manager->contacts(idFilter);
}

void ContactModel::contactsAdded(
        const QList<QContactId>& ids)
{
        qWarning() << "contactsAdded";

    QList<QContact> newContacts = contactsFromIds(ids);
    if(newContacts.isEmpty())
        return;

    int first = m_contacts.size();
    int last = first + newContacts.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    m_contacts += newContacts;
    endInsertRows();
}

void ContactModel::contactsRemoved(
        const QList<QContactId>& ids)
{
        qWarning() << "contactsRevmoed";
    // FIXME: make this nicer and faster
    foreach(const QContactId& contactId, ids) {
        for(int i = m_contacts.size() - 1; i >= 0; i--) {
            const QContact& contact = m_contacts[i];
            if(contact.id() == contactId) {
                beginRemoveRows(QModelIndex(), i, i);
                m_contacts.removeAt(i);
                endRemoveRows();
                break;
            }
        }
    }
}

void ContactModel::contactsChanged(
        const QList<QContactId>& ids,
const QList<QContactDetail::DetailType>& types)
{
        qWarning() << "contactsChanged";
    // FIXME: make this nicer and faster
    QList<QContact> changedContacts = contactsFromIds(ids);
    foreach(const QContact& newContact, changedContacts) {
        for(int i = 0; i < m_contacts.size(); ++i) {
            if(m_contacts[i].id() == newContact.id()) {
                m_contacts[i] = newContact;
                // FIXME: we could avoid emitting too many signals if the
                // changed contacts are one after each other
                QModelIndex changedIndex = index(i, 0);
                emit dataChanged(changedIndex, changedIndex);
                // This is for QML
                emit rowChanged(i);
            }
        }
    }
}

template<typename PresenceDetail>
QString ContactModel::presenceIconForDetail(
        PresenceDetail& presence) const
{
    switch(presence.presenceState()) {
    case QContactPresence::PresenceAvailable:
        return QLatin1String("presence-available.png");
    case QContactPresence::PresenceBusy:
        return QLatin1String("presence-busy.png");
    case QContactPresence::PresenceAway:
        return QLatin1String("presence-away.png");
    case QContactPresence::PresenceExtendedAway:
        return QLatin1String("presence-xa.png");
    default:
        return QLatin1String("presence-offline.png");
    }
}

QStringList ContactModel::detailsForContact(
        int row)
{
    // We pack multiple info n the same list because QML doesn't seem to
    // support lists of custom objects :(
    QStringList details;

    if(row < 0 || row > m_contacts.size())
        return details;

    const QContact& contact = m_contacts[row];

    QString noLabel = QLatin1String("");
    QString noIcon = QLatin1String("");
    QString noAction = QLatin1String("");

    // Names
    QContactName name = contact.detail<QContactName>();
    if(!name.firstName().isEmpty())
        details << tr("First name") << name.firstName() << noIcon << noAction;
    if(!name.lastName().isEmpty())
        details << tr("Last name") << name.lastName() << noIcon << noAction;
    QContactNickname nickname = contact.detail<QContactNickname>();
    if(!nickname.nickname().isEmpty() && nickname.nickname() != contact.detail<QContactDisplayLabel>().label())
        details << tr("Nickname") << nickname.nickname() << noIcon << noAction;

    // Favorite
    QContactFavorite favorite = contact.detail<QContactFavorite>();
    if(favorite.isFavorite())
        details << tr("Favorite") << tr("true") << noIcon << noAction;

    // Gender
    QContactGender gender = contact.detail<QContactGender>();
    QString genderString;
    if(gender.gender() == QContactGender::GenderMale)
        genderString = "Male";
    else if (gender.gender() == QContactGender::GenderFemale)
        genderString = "Female";

    if (!genderString.isNull())
        details << tr("Gender") <<  genderString << noIcon << noAction;

    // Notes
    QList<QContactNote> notes =
            contact.details<QContactNote>();
    foreach(const QContactNote& note, notes) {
        QString noteStr = note.note();
        if(!noteStr.isEmpty())
            details << tr("Note") << noteStr << noIcon << noAction;
    }

    // Organization
    QContactOrganization org = contact.detail<QContactOrganization>();
    if(!org.name().isEmpty()) {
        QString label(org.name() + QLatin1String(" (") + org.title() +
                QLatin1String(")"));
        details << tr("Organization") <<  label << noIcon << noAction;
    }

    // Phone numbers
    QList<QContactPhoneNumber> phoneNumbers =
            contact.details<QContactPhoneNumber>();
    foreach(const QContactPhoneNumber& phoneNumber, phoneNumbers) {
        /* XXX: should include the context here */
        QString phoneStr = phoneNumber.number();
        if(!phoneStr.isEmpty())
            details << tr("Phone") << phoneStr << noIcon << noAction;
    }

    // Email addresses
    QList<QContactEmailAddress> emailAddresses =
            contact.details<QContactEmailAddress>();
    foreach(const QContactEmailAddress& emailAddress, emailAddresses) {
        /* XXX: should include the context here */
        QString emailStr = emailAddress.emailAddress();
        if(!emailStr.isEmpty())
            details << tr("Email") << emailStr << noIcon << noAction;
    }

    // URL
    QList<QContactUrl> urls = contact.details<QContactUrl>();
    foreach(const QContactUrl& url, urls) {
        details << noLabel;

        // FIXME: subtypes are not strings anymore in Qt5
        #if 0
        if(url.subType() == QLatin1String("x-facebook-profile"))
            details << tr("View Facebook profile") << QLatin1String("facebook.png");
        else
        #endif
            details << tr("View website") << QLatin1String("web.png");

        details << url.url();
    }

    // IM
    QList<QContactOnlineAccount> accounts =
        contact.details<QContactOnlineAccount>();

    foreach(const QContactOnlineAccount& account, accounts) {
        details << noLabel;
        details << account.accountUri();

        QStringList linkedUris = account.linkedDetailUris();
        QString presenceIcon;
        foreach(const QString& uri, linkedUris) {
            QList<QContactPresence> presenceDetails =
                contact.details<QContactPresence>();
            foreach(const QContactPresence& presence, presenceDetails) {
                if(presence.detailUri() == uri) {
                    presenceIcon = presenceIconForDetail(presence);
                    break;
                }
            }
            if(presenceIcon != QLatin1String(""))
                break;
        }

        details << presenceIcon;
        details << noAction;
    }

    // Birthday
    QContactBirthday birthday = contact.detail<QContactBirthday>();
    if(!birthday.isEmpty()) {
        details << tr("Birthday") << birthday.date().toString() << noIcon
                << noAction;
    }

    // Address
    QContactAddress address = contact.detail<QContactAddress>();
    if(!address.isEmpty()) {
        /* XXX: this formatting isn't i10n-friendly. That requires a
         * well-maintained database containing an entry for every country. */
        QString label(address.street() + QLatin1String("\n") +
                address.postOfficeBox() + QLatin1String("\n") +
                address.locality() + QLatin1String(", ") +
                address.region() + QLatin1String("  ") +
                address.postcode() + QLatin1String("\n") +
                address.country());

        details << tr("Address") << label << noIcon << noAction;
    }

    return details;
}

bool ContactModel::setupIM(
        int row,
        QString *pContactId)
{
    Q_ASSERT(pContactId);

    *pContactId = QLatin1String("");

    if(row < 0 || row > m_contacts.size())
        return false;

    const QContact& contact = m_contacts[row];
    QList<QContactOnlineAccount> accounts =
        contact.details<QContactOnlineAccount>();

    // FIXME: Choose the account to use in a better way
    // FIXME: the field names are not strings anymore in Qt5,
    #if 0
    foreach(const QContactOnlineAccount& account, accounts) {
        QString accountPath =  account.value("TelepathyAccountPath");
        if(!accountPath.isEmpty()) {
            *pContactId = account.accountUri();
            // FIXME: We should get the bus name directly from the account
            // detail
            *pAccount = Account::create(TP_QT_ACCOUNT_MANAGER_BUS_NAME, accountPath);
            return true;
        }
    }
    #endif

    return false;
}

void ContactModel::startChat(
        int row)
{
    QString contactId;
    if(setupIM(row, &contactId)) {
        qDebug() << "Starting chat with" << contactId;
    }
}

void ContactModel::startCall(
        int row)
{
    QString contactId;
    if(setupIM(row, &contactId)) {
        qDebug() << "Starting call to" << contactId;
    }
}

void ContactModel::startAction(
        QString action)
{
    QDesktopServices::openUrl(action);
}
