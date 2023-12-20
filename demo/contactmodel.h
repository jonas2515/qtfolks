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

#include <QAbstractListModel>
#include <QContact>
#include <TelepathyQt/Account>

#ifndef CONTACT_MODEL_H
#define CONTACT_MODEL_H

QTCONTACTS_BEGIN_NAMESPACE
class QContactManager;
QTCONTACTS_END_NAMESPACE

QT_USE_NAMESPACE
QTCONTACTS_USE_NAMESPACE

class ContactModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        DisplayNameRole = Qt::UserRole + 1,
        PresenceMessageRole,
        PresenceIconRole,
        AvatarImageRole,
    };

    ContactModel(QObject *parent = 0);
    ~ContactModel();

    QContactManager& manager() { return *m_manager; }

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE QStringList detailsForContact(int row);
    Q_INVOKABLE void startChat(int row);
    Q_INVOKABLE void startCall(int row);
    Q_INVOKABLE void startAction(QString action);

Q_SIGNALS:
    void rowChanged(int row);

private:
    QContactManager *m_manager;
    QList<QContact> m_contacts;
    QHash<int, QByteArray> m_roles;

    QList<QContact> contactsFromIds(const QList<QContactId>& ids);

    template<typename PresenceDetail>
    QString presenceIconForDetail(PresenceDetail& presence) const;

    bool setupIM(int row, QString *pContactId, Tp::AccountPtr *pAccount);

private slots:
    void contactsAdded(const QList<QContactId>& ids);
    void contactsRemoved(const QList<QContactId>& ids);
    void contactsChanged(const QList<QContactId>& ids);
};

#endif // CONTACT_MODEL_H
