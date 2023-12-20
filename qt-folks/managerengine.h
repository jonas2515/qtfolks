/*
 * Copyright (C) 2010-2011 Collabora Ltd.
 *   @author Marco Barisione <marco.barisione@collabora.co.uk>
 *   @author Travis Reitter <travis.reitter@collabora.co.uk>
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

#include <telepathy-glib/account-manager.h>
#include "glib-utils.h"
#include <folks/folks-telepathy.h>
#include <QObject>
#include <QContactManagerEngine>
#include <QContactManagerEngineFactoryInterface>
#include <QContactPresence>

#define protected _protected
#include <folks/folks.h>
#undef protected

#ifndef MANAGER_ENGINE_H
#define MANAGER_ENGINE_H

#define FOLKS_MANAGER_NAME "folks"

QTCONTACTS_USE_NAMESPACE

// This is a very hackish way of making GObject signal handling less painful
// from C++.
// FIXME: We should really disconnect the handlers when the manager engine
// is destroyed...

#define STATIC_C_HANDLER_NAME(cb) \
    _static_callback_ ## cb
#define C_CONNECT(instance, detailed_signal, cb) \
    g_signal_connect(instance, detailed_signal, \
            G_CALLBACK(STATIC_C_HANDLER_NAME(cb)), this)

#define STATIC_C_NOTIFY_HANDLER_NAME(cb) \
    _static_notify_callback_ ## cb
#define DEFINE_C_NOTIFICATION_HANDLER(cb, sourceType) \
    static void STATIC_C_NOTIFY_HANDLER_NAME(cb) ( \
            sourceType *source, \
            GParamSpec *pspec, \
            ManagerEngine *this_) \
    { \
        this_->cb(source); \
    } \
    \
    void cb(sourceType *source);
#define C_NOTIFY_CONNECT(instance, propertyName, cb) \
    g_signal_connect(instance, "notify::" propertyName, \
            G_CALLBACK(STATIC_C_NOTIFY_HANDLER_NAME(cb)), this)

namespace Folks
{

typedef struct {
    QContact contact;
    QContact storedContact;
    FolksPersonaStore *store;
} CallbackData;

void addressDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void avatarDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void birthdayDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void calendarEventIdDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void favoriteDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void fullNameDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void aliasDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void structuredNameDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void noteDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void phoneNumberDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void imDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void roleDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void urlDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void emailDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);
void genderDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata);

class ManagerEngine : public QContactManagerEngine
{
    Q_OBJECT

public:
    ManagerEngine(const QMap<QString, QString>& parameters,
            QContactManager::Error* error);
    ~ManagerEngine();

    QString managerName() const { return QLatin1String(FOLKS_MANAGER_NAME); }
    int managerVersion() const { return 1; }

    QList<QContactType::TypeValues> supportedContactTypes() const {
        return QList<QContactType::TypeValues>() << QContactType::TypeContact;
    }
    QList<QVariant::Type> supportedDataTypes() const {
        return QList<QVariant::Type>() << QVariant::String;
    }

    QList<QContactId> contactIds(const QContactFilter& filter,
            const QList<QContactSortOrder>& sortOrders,
            QContactManager::Error *error) const;
    virtual QContact contact(
            const QContactId& contactId,
            const QContactFetchHint& fetchHint,
            QContactManager::Error* error) const;
    QList<QContact> contacts(const QContactFilter& filter,
            const QList<QContactSortOrder>& sortOrders,
            const QContactFetchHint& fetchHint,
            QContactManager::Error *error) const;
    virtual QContact compatibleContact (const QContact & original,
            QContactManager::Error * error ) const;

private:
    QContactPresence::PresenceState folksToQtPresence(FolksPresenceType fp);
    QContactId addIndividual(FolksIndividual *individual);
    QContactId removeIndividual(FolksIndividual *individual);
    FolksPersona* getPrimaryPersona(FolksIndividual *individual);

    class ContactPair {
    public:
        ContactPair()
            : individual(0) {}
        ContactPair(QContact& c, FolksIndividual *i)
            : contact(c)
            , individual(g_object_ref(i)) {}
        ContactPair(const ContactPair& other)
            : contact(other.contact)
            , individual(g_object_ref(other.individual)) {}
        ContactPair& operator=(const ContactPair& other)
        {
            contact = other.contact;
            g_object_unref(individual);
            individual = g_object_ref(other.individual);
            return *this;
        }
        ~ContactPair() { if (individual) g_object_unref(individual); }

        QContact contact;
        FolksIndividual *individual;
    };

    FolksIndividualAggregator *m_aggregator;

    QMap<QContactId, ContactPair> m_allContacts;
    QMap<FolksIndividual *, QContactId> m_individualsToIds;
    QMultiMap<FolksPersona *, FolksIndividual *> m_personasToIndividuals;
    QMap<QPair<FolksIndividual *, FolksPersona *>, gulong>
        m_personasSignalHandlerIds;

#define ARGS \
    FolksIndividualAggregator *aggregator, GeeSet *added, GeeSet *removed, \
    gchar *message, FolksPersona *actor, FolksGroupDetailsChangeReason reason
    void individualsChangedCb(ARGS);
    static void STATIC_C_HANDLER_NAME(individualsChangedCb)(
            ARGS, ManagerEngine *this_)
    {
        this_->individualsChangedCb(aggregator, added, removed, message,
                actor, reason);
    }
#undef ARGS

    void aggregatorPrepareCb();
    static void STATIC_C_HANDLER_NAME(aggregatorPrepareCb)(
            ManagerEngine *this_)
    {
        this_->aggregatorPrepareCb();
    }

typedef struct
{
    ManagerEngine* this_;
    QContactSaveRequest* request;
    QContact contact;
} AddPersonaFromDetailsClosure;

#define ARGS_CORE \
    GObject *source, GAsyncResult *result
#define ARGS \
    ARGS_CORE, AddPersonaFromDetailsClosure *closure
    void aggregatorAddPersonaFromDetailsCb(ARGS_CORE,
        QContactSaveRequest *request, QContact &contact);
    static void STATIC_C_HANDLER_NAME(aggregatorAddPersonaFromDetailsCb)(
            ARGS)
    {
        ManagerEngine *this_ = closure->this_;
        QContactSaveRequest* request = closure->request;

        this_->aggregatorAddPersonaFromDetailsCb(source, result, request,
                closure->contact);

        delete closure;
    }
#undef ARGS
#undef ARGS_CORE

typedef struct
{
    ManagerEngine* this_;
    QContactRemoveRequest* request;
} RemoveIndividualClosure;

#define ARGS_CORE \
    GObject *source, GAsyncResult *result
#define ARGS \
    ARGS_CORE, RemoveIndividualClosure *closure
    void aggregatorRemoveIndividualCb(ARGS_CORE,
        QContactRemoveRequest *request);
    static void STATIC_C_HANDLER_NAME(aggregatorRemoveIndividualCb)(
            ARGS)
    {
        ManagerEngine *this_ = closure->this_;
        QContactRemoveRequest* request = closure->request;

        this_->aggregatorRemoveIndividualCb(source, result, request);

        delete closure;
    }
#undef ARGS
#undef ARGS_CORE
        typedef struct
        {
            ManagerEngine* this_;
            QUrl url;
            QContactId contactId;
            FolksIndividual *individual;
        } AvatarLoadData;
        static void avatarReadyCB(GObject *source, GAsyncResult *res, gpointer user_data);

    template<typename DetailType>
    void removeOldDetails(QContact& contact)
    {
        foreach(DetailType oldDetail, contact.details<DetailType>())
            contact.removeDetail(&oldDetail);
    }
    void updateGuidFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateAliasFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateStructuredNameFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateFullNameFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateNicknameFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updatePresenceFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updatePresenceFromPersona(QContact& contact,
            FolksIndividual *individual, FolksPersona *persona);
    void updateAvatarFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateBirthdayFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateEmailAddressesFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateImAddressesFromIndividual(QContact &contact,
            FolksIndividual *individual);
    void updateFavoriteFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateGenderFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateNotesFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateOrganizationFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updatePhoneNumbersFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateAddressesFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateUrlsFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updatePersonas(QContact& contact, FolksIndividual *individual,
            GeeSet *added, GeeSet *removed);
    void addAccountDetails(QContact& contact, TpfPersona *persona);
    void removeAccountDetails(QContact& contact, TpfPersona *persona);

    void updateDisplayLabelFromIndividual(QContact& contact,
            FolksIndividual *individual);
    void updateNameFromIndividual(QContact& contact,
            FolksIndividual *individual);

    QString getPersonaAccountUri(FolksPersona *persona);
    QString getPersonaPresenceUri(FolksPersona *persona);
    QContactPresence getPresenceForPersona(QContact& contact,
            FolksPersona *persona);

    static void managerReadyCb(GObject *sourceObject, GAsyncResult *result,
            gpointer userData);
    TpAccount *getAccountForTpContact(TpContact *tpContact);

    template<typename DetailType, typename FolkType>
    bool setPresenceDetail(DetailType& detail, FolkType *folk);

#define ARGS \
    FolksIndividual *individual, GeeSet *added, GeeSet *removed
    void personasChangedCb(ARGS);
    static void STATIC_C_HANDLER_NAME(personasChangedCb)(
            ARGS, ManagerEngine *this_)
    {
        this_->personasChangedCb(individual, added, removed);
    }
#undef ARGS

    DEFINE_C_NOTIFICATION_HANDLER(aliasChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(structuredNameChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(fullNameChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(nicknameChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(presenceChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(avatarChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(birthdayChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(emailAddressesChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(imAddressesChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(favouriteChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(genderChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(notesChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(rolesChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(phoneNumbersChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(postalAddressesChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(urlsChangedCb, FolksIndividual);
    DEFINE_C_NOTIFICATION_HANDLER(personaPresenceChangedCb, FolksPersona);

    // async API
    virtual bool startRequest(QContactAbstractRequest* req);

    // saving changes in Folks
    bool contactSaveChangesToFolks(const QContact& contact);
};

class ManagerEngineFactory : public QContactManagerEngineFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QContactManagerEngineFactoryInterface" FILE "folks.json")

public:
    QContactManagerEngine* engine(const QMap<QString, QString>& parameters,
            QContactManager::Error* error);
    QString managerName() const;
    QContactEngineId *createContactEngineId(const QMap<QString, QString> &parameters, const QString &idString) const;
};

} // namespace Folks

#endif // MANAGER_ENGINE_H
