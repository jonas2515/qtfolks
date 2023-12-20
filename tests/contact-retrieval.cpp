/*
 * Copyright (C) 2011 Collabora Ltd.
 *   @author Travis Reitter <travis.reitter@collabora.co.uk>
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

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QContactLocalIdFilter>
#include <QContactManager>
#include <glib-object.h>
#include "contact-retrieval.h"
#include "_gen/contact-retrieval.moc.hpp"
#include "utils.h"

#define TIMEOUT_S 3

TestContactRetrieval::TestContactRetrieval(QCoreApplication *app)
    : mApp(app)
{
    QMap<QString, QString> params;
    QString managerUri = QContactManager::buildUri(
            QLatin1String("folks"), params);
    mManager = QContactManager::fromUri(managerUri);
    Q_ASSERT(mManager);

    mExpectedContacts[QLatin1String("0")] = QLatin1String("user1@localhost");
    mExpectedContacts[QLatin1String("A Classic")] =
        QLatin1String("foo@localhost");
    mExpectedContacts[QLatin1String("Palm Tree Squirrel")] =
        QLatin1String("palm.tree.squirrel@localhost");

    connect(mManager, SIGNAL(contactsAdded(const QList<QContactLocalId>&)),
            this, SLOT(contactsAdded(const QList<QContactLocalId>&)));
    connect(mManager, SIGNAL(contactsRemoved(const QList<QContactLocalId>&)),
            this, SLOT(contactsRemoved(const QList<QContactLocalId>&)));
    connect(mManager, SIGNAL(contactsChanged(const QList<QContactLocalId>&)),
            this, SLOT(contactsChanged(const QList<QContactLocalId>&)));

    /* XXX: but we want to eventually fail if the test doesn't complete by some
     * amount of time (to prevent hanging in case of failure) */
    QTimer::singleShot(TIMEOUT_S * 1000, this, SLOT(timeout()));
}

void TestContactRetrieval::timeout()
{
    qCritical("timed out before test completed");
    this->app().exit(1);
}

void TestContactRetrieval::testContactRetrieval()
{
    qDebug("Running %s\n===============================", __PRETTY_FUNCTION__);
}

QList<QContact> TestContactRetrieval::contactsFromIds(
        const QList<QContactLocalId>& ids)
{
    QContactLocalIdFilter idFilter;
    idFilter.setIds(ids);
    return mManager->contacts(idFilter);
}

void TestContactRetrieval::contactsAdded(
        const QList<QContactLocalId>& ids)
{
    QList<QContact> newContacts = contactsFromIds(ids);
    foreach(const QContact& i, newContacts) {
        int removeCount = mExpectedContacts.remove(i.displayLabel());
        Q_ASSERT(removeCount == 1);
        
        if(mExpectedContacts.size() == 0)
            this->app().quit();
    }
}

void TestContactRetrieval::contactsRemoved(
        const QList<QContactLocalId>& ids)
{
    qDebug("** ** contactsRemoved");

    ASSERT_NOT_REACHED();
}

void TestContactRetrieval::contactsChanged(
        const QList<QContactLocalId>& ids)
{
    ASSERT_NOT_REACHED();
}

int main(int argc, char **argv)
{
    // required for Folks to be properly initialized
    g_type_init();

    QCoreApplication app(argc, argv);
    TestContactRetrieval test(&app);
    test.testContactRetrieval();

    return app.exec();
}
