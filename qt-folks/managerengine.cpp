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

#include <folks/folks.h>
#include <QFile>
#include <QContactAddress>
#include <QContactAvatar>
#include <QContactBirthday>
#include <QContactEmailAddress>
#include <QContactFavorite>
#include <QContactGender>
#include <QContactGlobalPresence>
#include <QContactName>
#include <QContactNickname>
#include <QContactNote>
#include <QContactOnlineAccount>
#include <QContactOrganization>
#include <QContactPhoneNumber>
#include <QContactUrl>
#include <QContactDisplayLabel>
#include <QContactIntersectionFilter>
#include <QContactDetailFilter>
#include <QContactCollectionFilter>
#include "managerengine.h"
#include "debug.h"
#include "utils.h"

QTCONTACTS_USE_NAMESPACE

namespace
{

static void setFieldDetailsFromContexts(FolksAbstractFieldDetails *details,
                                        const QString &parameter,
                                        const QStringList &contexts)
{
    Q_FOREACH (const QString &value, contexts) {
        folks_abstract_field_details_add_parameter (details,
                                                    parameter.toUtf8().data(),
                                                    value.toUtf8().data());
    }
}

static void setFieldDetailsFromContexts(FolksAbstractFieldDetails *details,
                                        const QContactDetail &contactDetail)
{
    setFieldDetailsFromContexts(details, "type", Utils::contextsFromEnums(contactDetail.contexts()));
}

static QStringList contextsFromFieldDetails(QContactDetail &contactDetail,
                                        FolksAbstractFieldDetails *details)
{
    GeeMultiMap *map = folks_abstract_field_details_get_parameters (details);
    GeeSet *keys = gee_multi_map_get_keys (map);
    GeeIterator *siter = gee_iterable_iterator (GEE_ITERABLE (keys));

    QStringList contexts;

    while (gee_iterator_next (siter)) {
        char *parameter = (char*) gee_iterator_get (siter);
        if (QString(parameter) != "type") {
            // FIXME: check what to do with other parameters
            continue;
        }

        GeeCollection *args = folks_abstract_field_details_get_parameter_values(
                details, parameter);

        GeeIterator *iter = gee_iterable_iterator (GEE_ITERABLE (args));

        while (gee_iterator_next (iter)) {
            char *type = (char*) gee_iterator_get (iter);
            QString context(type);
            if (!contexts.contains(context)) {
                contexts << type;
            }
            g_free (type);
        }
    }
    return contexts;

}

static void setContextsFromFieldDetails(QContactDetail &contactDetail,
                                        FolksAbstractFieldDetails *details)
{
    QStringList contexts = contextsFromFieldDetails(contactDetail, details);
    contactDetail.setContexts(Utils::contextsFromStrings(contexts));
}

} //namespace

namespace Folks
{


static guint _folks_abstract_field_details_hash_data_func (gconstpointer v, gpointer self)
{
    const FolksAbstractFieldDetails *constDetails = static_cast<const FolksAbstractFieldDetails*>(v);
    return folks_abstract_field_details_hash_static (const_cast<FolksAbstractFieldDetails*>(constDetails));
}

static int _folks_abstract_field_details_equal_data_func (gconstpointer a, gconstpointer b, gpointer self)
{
    const FolksAbstractFieldDetails *constDetailsA = static_cast<const FolksAbstractFieldDetails*>(a);
    const FolksAbstractFieldDetails *constDetailsB = static_cast<const FolksAbstractFieldDetails*>(b);
    return folks_abstract_field_details_equal_static (const_cast<FolksAbstractFieldDetails*>(constDetailsA), const_cast<FolksAbstractFieldDetails*>(constDetailsB));
}

#define SET_AFD_NEW() \
    GEE_SET(gee_hash_set_new(FOLKS_TYPE_ABSTRACT_FIELD_DETAILS, \
                             (GBoxedCopyFunc) g_object_ref, g_object_unref, \
                             _folks_abstract_field_details_hash_data_func, \
                             NULL, \
                             NULL, \
                             _folks_abstract_field_details_equal_data_func, \
                             NULL, \
                             NULL))


template <class T>
bool checkDetailsChanged(QContact originalContact, QContact modifiedContact)
{
    QList<T> originalDetails = originalContact.details<T>();
    QList<T> modifiedDetails = modifiedContact.details<T>();

    // this is not really a smart way of checking this, but QContactDetail doesn't
    // have any operator== so that's how we can do
    typename QList<T>::iterator it = modifiedDetails.begin();
    while (it != modifiedDetails.end()) {
        // iterate over the original details to check if this detail is in there
        typename QList<T>::iterator originalIt = originalDetails.begin();
        bool remove = false;
        while (originalIt != originalDetails.end()) {
            QContactDetail modifiedDetail = *it;
            QContactDetail originalDetail = *originalIt;


            bool match = true;
            // get the list of fields from both details
            QList<int> fields = modifiedDetail.values().keys();
            fields.append(originalDetail.values().keys());

            Q_FOREACH(int field, fields) {
                if (!originalDetail.hasValue(field) || !modifiedDetail.hasValue(field)) {
                    match = false;
                    break;
                }

                QVariant originalValue = originalDetail.value(field);
                QVariant modifiedValue = modifiedDetail.value(field);

                // QVariant::operator== doesn't work for QList<int>, so compare it manually
                static const QString intListType("QList<int>");
                if (originalValue.typeName() == intListType && modifiedValue.typeName() == intListType) {
                    if (originalValue.value<QList<int> >() != modifiedValue.value<QList<int> >()) {
                        match = false;
                        break;
                    }
                } else if (originalValue != modifiedValue) {
                    match = false;
                    break;
                }
            }
            if (match) {
                remove = true;
                originalIt = originalDetails.erase(originalIt);
                break;
            }
            ++originalIt;
        }
        if (remove) {
            it = modifiedDetails.erase(it);
        } else {
            ++it;
        }
    }

    return (originalDetails.count() > 0 || modifiedDetails.count() > 0);
}

void addressDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksPostalAddressDetails *postalDetails = FOLKS_POSTAL_ADDRESS_DETAILS(detail);
        folks_postal_address_details_change_postal_addresses_finish(postalDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }
    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Avatar
     */
    if(FOLKS_IS_AVATAR_DETAILS(persona) && checkDetailsChanged<QContactAvatar>(data->storedContact, data->contact)) {
        FolksAvatarDetails *avatarDetails = FOLKS_AVATAR_DETAILS(persona);

        QContactAvatar avatar = data->contact.detail<QContactAvatar>();
        QUrl avatarUri = avatar.imageUrl();
        GFileIcon *avatarFileIcon = NULL;
        if(!avatarUri.isEmpty()) {
            QString formattedUri = avatarUri.toString(QUrl::RemoveUserInfo);
            if(!formattedUri.isEmpty()) {
                GFile *avatarFile =
                    g_file_new_for_uri(formattedUri.toUtf8().data());
                avatarFileIcon = G_FILE_ICON(g_file_icon_new(avatarFile));

                gObjectClear((GObject**) &avatarFile);
            }
        }

        folks_avatar_details_change_avatar(avatarDetails,
                G_LOADABLE_ICON(avatarFileIcon), avatarDetailChangeCb, data);
        gObjectClear((GObject**) &avatarFileIcon);
    } else {
        // trigger the callback
        avatarDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void avatarDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksAvatarDetails *avatarDetails = FOLKS_AVATAR_DETAILS(detail);
        folks_avatar_details_change_avatar_finish(avatarDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Birthday
     */
    bool updated = false;
    if(FOLKS_IS_BIRTHDAY_DETAILS(persona) && checkDetailsChanged<QContactBirthday>(data->storedContact, data->contact)) {
        FolksBirthdayDetails *birthdayDetails = FOLKS_BIRTHDAY_DETAILS(persona);

        QContactBirthday birthday = data->contact.detail<QContactBirthday>();
        if(!birthday.isEmpty()) {
            updated = true;
            GDateTime *dateTime = g_date_time_new_from_unix_utc(
                    birthday.dateTime().toMSecsSinceEpoch() / 1000);
            folks_birthday_details_change_birthday(birthdayDetails, dateTime, birthdayDetailChangeCb, data);
            g_date_time_unref(dateTime);
        }
    }

    if(!updated) {
        birthdayDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void birthdayDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksBirthdayDetails *birthdayDetails = FOLKS_BIRTHDAY_DETAILS(detail);
        folks_birthday_details_change_birthday_finish(birthdayDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Calendar Event Id
     */
    bool updated = false;
    if(FOLKS_IS_BIRTHDAY_DETAILS(persona) && checkDetailsChanged<QContactBirthday>(data->storedContact, data->contact)) {
        FolksBirthdayDetails *birthdayDetails = FOLKS_BIRTHDAY_DETAILS(persona);

        QContactBirthday birthday = data->contact.detail<QContactBirthday>();
        if(!birthday.isEmpty()) {
            updated = true;
            GDateTime *dateTime = g_date_time_new_from_unix_utc(
                    birthday.dateTime().toMSecsSinceEpoch() / 1000);
            folks_birthday_details_change_calendar_event_id(birthdayDetails,
                    birthday.calendarId().toUtf8().data(), calendarEventIdDetailChangeCb, data);
            g_date_time_unref(dateTime);
        }
    }

    if (!updated) {
        calendarEventIdDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void calendarEventIdDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksBirthdayDetails *birthdayDetails = FOLKS_BIRTHDAY_DETAILS(detail);
        folks_birthday_details_change_birthday_finish(birthdayDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Favorite
     */
    if(FOLKS_IS_FAVOURITE_DETAILS(persona) && checkDetailsChanged<QContactFavorite>(data->storedContact, data->contact)) {
        FolksFavouriteDetails *favouriteDetails =
            FOLKS_FAVOURITE_DETAILS(persona);

        QContactFavorite favorite = data->contact.detail<QContactFavorite>();

        folks_favourite_details_change_is_favourite(favouriteDetails,
                favorite.isFavorite(), favoriteDetailChangeCb, data);
    } else {
        favoriteDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void favoriteDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksFavouriteDetails *favoriteDetails = FOLKS_FAVOURITE_DETAILS(detail);
        folks_favourite_details_change_is_favourite_finish(favoriteDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Full name
     */
    bool updated = false;
    if(FOLKS_IS_NAME_DETAILS(persona) && checkDetailsChanged<QContactDisplayLabel>(data->storedContact, data->contact)) {
        FolksNameDetails *nameDetails = FOLKS_NAME_DETAILS(persona);

        QContactDisplayLabel displayLabel = data->contact.detail<QContactDisplayLabel>();
        if (!displayLabel.label().isEmpty()) {
            updated = true;
            folks_name_details_change_full_name(nameDetails, displayLabel.label().toUtf8().data(), fullNameDetailChangeCb, data);
        }
    }

    if (!updated) {
        fullNameDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void fullNameDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksNameDetails *nameDetails = FOLKS_NAME_DETAILS(detail);
        folks_name_details_change_full_name_finish(nameDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Alias
     */
    bool updated = false;
    if(FOLKS_IS_ALIAS_DETAILS(persona) && checkDetailsChanged<QContactDisplayLabel>(data->storedContact, data->contact)) {
        FolksAliasDetails *alias_details = FOLKS_ALIAS_DETAILS(persona);
        QContactDisplayLabel displayLabel = data->contact.detail<QContactDisplayLabel>();
        if (!displayLabel.label().isEmpty()) {

            /* XXX: this is a slight conflation, since Folks treats Nickname as
             * contact-set and alias as user-set (but QtContacts doesn't treat them
             * differently) */
            folks_alias_details_change_alias(alias_details,
                    displayLabel.label().toUtf8().data(), aliasDetailChangeCb, data);
        }
    }

    if (!updated) {
        aliasDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void aliasDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksAliasDetails *aliasDetails = FOLKS_ALIAS_DETAILS(detail);
        folks_alias_details_change_alias_finish(aliasDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Structured Name
     */
    if(FOLKS_IS_NAME_DETAILS(persona) && checkDetailsChanged<QContactName>(data->storedContact, data->contact)) {
        FolksNameDetails *nameDetails = FOLKS_NAME_DETAILS(persona);
        FolksStructuredName *sn = NULL;

        QContactName name = data->contact.detail<QContactName>();
        if(!name.isEmpty()) {
            sn = folks_structured_name_new(
                    name.lastName().toUtf8().data(),
                    name.firstName().toUtf8().data(),
                    name.middleName().toUtf8().data(),
                    name.prefix().toUtf8().data(),
                    name.suffix().toUtf8().data());
        }

        folks_name_details_change_structured_name(nameDetails, sn, structuredNameDetailChangeCb, data);
        gObjectClear((GObject**) &sn);
    } else {
        structuredNameDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void structuredNameDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksNameDetails *nameDetails = FOLKS_NAME_DETAILS(detail);
        folks_name_details_change_structured_name_finish(nameDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Notes
     */
    if(FOLKS_IS_NOTE_DETAILS(persona) && checkDetailsChanged<QContactNote>(data->storedContact, data->contact)) {
        FolksNoteDetails *noteDetails = FOLKS_NOTE_DETAILS(persona);

        QList<QContactNote> notes = data->contact.details<QContactNote>();

        GeeSet *noteSet = SET_AFD_NEW();
        foreach(const QContactNote& note, notes) {
            if(!note.isEmpty()) {
                FolksAbstractFieldDetails *fieldDetails =
                    FOLKS_ABSTRACT_FIELD_DETAILS (folks_note_field_details_new(
                        note.note().toUtf8().data(), NULL, NULL));
                setFieldDetailsFromContexts (fieldDetails, note);
                gee_collection_add(GEE_COLLECTION(noteSet), fieldDetails);
                g_object_unref(fieldDetails);
            }
        }

        folks_note_details_change_notes(noteDetails, noteSet, noteDetailChangeCb, data);

        g_object_unref(noteSet);
    } else {
        noteDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void noteDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksNoteDetails *noteDetails = FOLKS_NOTE_DETAILS(detail);
        folks_note_details_change_notes_finish(noteDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Phone numbers
     */
    if(FOLKS_IS_PHONE_DETAILS(persona) && checkDetailsChanged<QContactPhoneNumber>(data->storedContact, data->contact)) {
        FolksPhoneDetails *phoneDetails = FOLKS_PHONE_DETAILS(persona);

        QList<QContactPhoneNumber> numbers =
            data->contact.details<QContactPhoneNumber>();
        GeeSet *numberSet = SET_AFD_NEW();

        foreach(const QContactPhoneNumber& number, numbers) {
            if(!number.isEmpty()) {
                FolksAbstractFieldDetails *fieldDetails =
                    FOLKS_ABSTRACT_FIELD_DETAILS (folks_phone_field_details_new(
                        number.number().toUtf8().data(), NULL));

                QStringList contexts = Utils::contextsFromEnums(number.contexts());
                contexts << Utils::phoneSubTypesFromEnums(number.subTypes());
                setFieldDetailsFromContexts(fieldDetails, "type", contexts);

                gee_collection_add(GEE_COLLECTION(numberSet), fieldDetails);
                g_object_unref(fieldDetails);
            }
        }

        folks_phone_details_change_phone_numbers(phoneDetails, numberSet, phoneNumberDetailChangeCb, data);

        g_object_unref(numberSet);
    } else {
        phoneNumberDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

static guint my_g_str_hash (gconstpointer v, gpointer self)
{
    return g_str_hash (v);
}

static int my_g_str_equal (gconstpointer a, gconstpointer b, gpointer self)
{
    return g_str_equal (a, b);
}

void phoneNumberDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksPhoneDetails *phoneDetails = FOLKS_PHONE_DETAILS(detail);
        folks_phone_details_change_phone_numbers_finish(phoneDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * OnlineAccount
     */
    if(FOLKS_IS_IM_DETAILS(persona) && checkDetailsChanged<QContactOnlineAccount>(data->storedContact, data->contact)) {
        FolksImDetails *imDetails = FOLKS_IM_DETAILS(persona);

        QList<QContactOnlineAccount> accounts =
            data->contact.details<QContactOnlineAccount>();

GeeMultiMap *imAddressHash =
    GEE_MULTI_MAP(gee_hash_multi_map_new(G_TYPE_STRING,\
                                         (GBoxedCopyFunc) g_strdup, g_free, \
                                         FOLKS_TYPE_IM_FIELD_DETAILS, \
                                         (GBoxedCopyFunc)g_object_ref, g_object_unref, \
                                         my_g_str_hash, \
                                         NULL, \
                                         NULL, \
                                         my_g_str_equal, \
                                         NULL, \
                                         NULL, \
                                         _folks_abstract_field_details_hash_data_func, \
                                         NULL, \
                                         NULL, \
                                         _folks_abstract_field_details_equal_data_func, \
                                         NULL, \
                                         NULL));
/*
        GeeMultiMap *imAddressHash = GEE_MULTI_MAP(
                gee_hash_multi_map_new(G_TYPE_STRING, (GBoxedCopyFunc) g_strdup,
                        g_free,
                        FOLKS_TYPE_IM_FIELD_DETAILS,
                        g_object_ref, g_object_unref,
                        g_str_hash, g_str_equal,
                        (GHashFunc) folks_abstract_field_details_hash,
                        (GEqualFunc) folks_abstract_field_details_equal));
*/

        foreach(const QContactOnlineAccount& account, accounts) {
            if(!account.isEmpty()) {
                QString accountUri = account.accountUri();
                if(account.protocol()!= QContactOnlineAccount::ProtocolUnknown) {
                    if(!accountUri.isEmpty()) {
                        FolksImFieldDetails *imfd =
                            folks_im_field_details_new (
                                    accountUri.toUtf8().data(), NULL);

                        QStringList contexts = Utils::contextsFromEnums(account.contexts());
                        contexts << Utils::onlineAccountSubTypesFromEnums(account.subTypes());
                        setFieldDetailsFromContexts(FOLKS_ABSTRACT_FIELD_DETAILS (imfd), "type", contexts);

                        gee_multi_map_set(imAddressHash,
                                Utils::onlineAccountProtocolFromEnum(account.protocol()).toUtf8().data(),
                                imfd);

                        g_object_unref(imfd);
                    }
                }
            }
        }

        folks_im_details_change_im_addresses(imDetails, imAddressHash, imDetailChangeCb, data);

        g_object_unref(imAddressHash);
    } else {
        imDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void imDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksImDetails *imDetails = FOLKS_IM_DETAILS(detail);
        folks_im_details_change_im_addresses_finish(imDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Organization
     */
    if(FOLKS_IS_ROLE_DETAILS(persona) && checkDetailsChanged<QContactOrganization>(data->storedContact, data->contact)) {
        FolksRoleDetails *roleDetails = FOLKS_ROLE_DETAILS(persona);

        QList<QContactOrganization> orgs =
            data->contact.details<QContactOrganization>();

        GeeSet *roleSet = SET_AFD_NEW();
        foreach(const QContactOrganization& org, orgs) {
            if(!org.isEmpty()) {


                // The role values can not be NULL
                const gchar* title = org.title().toUtf8().data();
                const gchar* name = org.name().toUtf8().data();
                const gchar* roleName = org.role().toUtf8().data();

                FolksRole *role = folks_role_new(title ? title : "",
                        name ? name : "", "");
                folks_role_set_role (role, roleName ? roleName : "");

                FolksRoleFieldDetails *fieldDetails = folks_role_field_details_new(
                        role, NULL);

                setFieldDetailsFromContexts (FOLKS_ABSTRACT_FIELD_DETAILS (fieldDetails),
                        org);
                gee_collection_add(GEE_COLLECTION(roleSet), fieldDetails);

                g_object_unref(role);
                g_object_unref(fieldDetails);
            }
        }

        folks_role_details_change_roles(roleDetails, roleSet, roleDetailChangeCb, data);
        g_object_unref(roleSet);
    } else {
        roleDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void roleDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksRoleDetails *roleDetails = FOLKS_ROLE_DETAILS(detail);
        folks_role_details_change_roles_finish(roleDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * URLs
     */
    if(FOLKS_IS_URL_DETAILS(persona) && checkDetailsChanged<QContactUrl>(data->storedContact, data->contact)) {
        FolksUrlDetails *urlDetails = FOLKS_URL_DETAILS(persona);

        QList<QContactUrl> urls = data->contact.details<QContactUrl>();
        GeeSet *urlSet = SET_AFD_NEW();
        foreach(const QContactUrl& url, urls) {
            if(!url.isEmpty()) {
                FolksAbstractFieldDetails *fieldDetails =
                    FOLKS_ABSTRACT_FIELD_DETAILS (folks_url_field_details_new(
                        url.url().toUtf8().data(), NULL));
                gee_collection_add(GEE_COLLECTION(urlSet), fieldDetails);
                setFieldDetailsFromContexts (fieldDetails, url);
                g_object_unref(fieldDetails);
            }
        }

        folks_url_details_change_urls(urlDetails, urlSet, urlDetailChangeCb, data);

        g_object_unref(urlSet);
    } else {
        urlDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void urlDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksUrlDetails *urlDetails = FOLKS_URL_DETAILS(detail);
        folks_url_details_change_urls_finish(urlDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Email addresses
     */
    if(FOLKS_IS_EMAIL_DETAILS(persona) && checkDetailsChanged<QContactEmailAddress>(data->storedContact, data->contact)) {
        FolksEmailDetails *emailDetails = FOLKS_EMAIL_DETAILS(persona);

        QList<QContactEmailAddress> addresses =
            data->contact.details<QContactEmailAddress>();
        GeeSet *addressSet = SET_AFD_NEW();
        foreach(const QContactEmailAddress& address, addresses) {
            if(!address.isEmpty()) {
                FolksAbstractFieldDetails *fieldDetails =
                    FOLKS_ABSTRACT_FIELD_DETAILS (folks_email_field_details_new(
                        address.emailAddress().toUtf8().data(), NULL));
                setFieldDetailsFromContexts (fieldDetails, address);
                gee_collection_add(GEE_COLLECTION(addressSet), fieldDetails);
            }
        }

        folks_email_details_change_email_addresses(emailDetails, addressSet, emailDetailChangeCb, data);

        g_object_unref(addressSet);
    } else {
        emailDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void emailDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksEmailDetails *emailDetails = FOLKS_EMAIL_DETAILS(detail);
        folks_email_details_change_email_addresses_finish(emailDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    /*
     * Gender
     */
    if(FOLKS_IS_GENDER_DETAILS(persona) && checkDetailsChanged<QContactGender>(data->storedContact, data->contact)) {
        FolksGenderDetails *genderDetails = FOLKS_GENDER_DETAILS(persona);

        QContactGender gender = data->contact.detail<QContactGender>();
        FolksGender genderEnum = FOLKS_GENDER_UNSPECIFIED;
        if(gender.gender() == QContactGender::GenderMale) {
            genderEnum = FOLKS_GENDER_MALE;
        } else if(gender.gender() == QContactGender::GenderFemale) {
            genderEnum = FOLKS_GENDER_FEMALE;
        }

        folks_gender_details_change_gender(genderDetails, genderEnum, genderDetailChangeCb, data);
    } else {
        genderDetailChangeCb(G_OBJECT(persona), NULL, data);
    }
}

void genderDetailChangeCb(GObject *detail, GAsyncResult *result, gpointer userdata)
{
    if (result) {
        GError *error = NULL;
        FolksGenderDetails *genderDetails = FOLKS_GENDER_DETAILS(detail);
        folks_gender_details_change_gender_finish(genderDetails, result, &error);

        if (error) {
            qDebug() << "ERROR:" << error->message;
        }
    }

    FolksPersona *persona = FOLKS_PERSONA(detail);
    CallbackData *data = static_cast<CallbackData*>(userdata);

    // Flush the store.
    FolksPersonaStore *primaryStore = data->store;
    if(primaryStore == NULL) {
        qWarning() << "Cannot save Contact changes: Failed determine primary "
            "data store";
    } else {
        folks_persona_store_flush(
                primaryStore,
                0, 0);
    }

    gObjectClear((GObject**) &persona);
    delete data;
}

ManagerEngine::ManagerEngine(
        const QMap<QString, QString>& parameters,
        QContactManager::Error* error)
{
        qWarning() << "contacts new engine creating";

    m_aggregator = folks_individual_aggregator_dup();
    C_CONNECT(m_aggregator, "individuals-changed", individualsChangedCb);
    folks_individual_aggregator_prepare(
            m_aggregator,
            (GAsyncReadyCallback) STATIC_C_HANDLER_NAME(aggregatorPrepareCb),
            this);

  m_notifier = new ContactNotifier(false);
          /*      notifier->connect("collectionsAdded", "au", this, SLOT(_q_collectionsAdded(QVector<quint32>)));
                notifier->connect("collectionsChanged", "au", this, SLOT(_q_collectionsChanged(QVector<quint32>)));
                notifier->connect("collectionsRemoved", "au", this, SLOT(_q_collectionsRemoved(QVector<quint32>)));
                notifier->connect("collectionContactsChanged", "au", this, SLOT(_q_collectionContactsChanged(QVector<quint32>)));

                notifier->connect("contactsAdded", "au", this, SLOT(_q_contactsAdded(QVector<quint32>)));
                notifier->connect("contactsChanged", "au", this, SLOT(_q_contactsChanged(QVector<quint32>)));
                notifier->connect("contactsPresenceChanged", "au", this, SLOT(_q_contactsPresenceChanged(QVector<quint32>)));
                notifier->connect("contactsRemoved", "au", this, SLOT(_q_contactsRemoved(QVector<quint32>)));
                notifier->connect("selfContactIdChanged", "uu", this, SLOT(_q_selfContactIdChanged(quint32,quint32)));
                notifier->connect("relationshipsAdded", "au", this, SLOT(_q_relationshipsAdded(QVector<quint32>)));
                notifier->connect("relationshipsRemoved", "au", this, SLOT(_q_relationshipsRemoved(QVector<quint32>)));
                notifier->connect("displayLabelGroupsChanged", "", this, SLOT(_q_displayLabelGroupsChanged()));
*/

  while (!m_initialIndividualsAdded)
    g_main_context_iteration (g_main_context_default(), TRUE);

        qWarning() << "contacts new engine creation finished";
}

ManagerEngine::~ManagerEngine()
{
    gObjectClear((GObject**) &m_aggregator);
}

void ManagerEngine::aggregatorPrepareCb()
{
qWarning("contacts aggregator has prepared");

//  if (error)
  //  g_warning ("Failed to load Folks contacts: %s", error->message);
}

void ManagerEngine::updateDisplayLabelFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    const char *displayLabel = folks_name_details_get_nickname(
            FOLKS_NAME_DETAILS(individual));
    if(!displayLabel || displayLabel[0] == '\0') {
        displayLabel = folks_alias_details_get_alias(
                FOLKS_ALIAS_DETAILS(individual));

        if(!displayLabel || displayLabel[0] == '\0') {
            return;
        }
    }

    QContactDisplayLabel displayLabelDetail;
    if (!contact.details<QContactDisplayLabel>().isEmpty()) {
        displayLabelDetail = contact.detail<QContactDisplayLabel>();
    }

    displayLabelDetail.setLabel(QString::fromUtf8(displayLabel));
    contact.saveDetail(&displayLabelDetail);
}

void ManagerEngine::updateAliasFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    updateDisplayLabelFromIndividual(contact, individual);
}

void ManagerEngine::updateNameFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    FolksStructuredName *sn = folks_name_details_get_structured_name(
            FOLKS_NAME_DETAILS(individual));
    const char *fullName = folks_name_details_get_full_name(
            FOLKS_NAME_DETAILS(individual));

    removeOldDetails<QContactName>(contact);

    if(sn || fullName) {
        QContactName name;

        if(fullName) {
            QContactDisplayLabel displayLabelDetail;
            if (!contact.details<QContactDisplayLabel>().isEmpty()) {
                displayLabelDetail = contact.detail<QContactDisplayLabel>();
            }
            displayLabelDetail.setLabel(QString::fromUtf8(fullName));
            contact.saveDetail(&displayLabelDetail);
        }

        // yay, gnome-contacts only sets the structured name once on contact creation,
        // then never updates it (instead only updating the full name)
        // until that's fixed, let's always go with the full name as QContactName
        if(fullName) {
            QStringList names = QString::fromUtf8(fullName).split(' ');
            if (names.length() == 2) {
                name.setFirstName(names.at(0));
                name.setLastName(names.at(1));
            } else {
                name.setFirstName(QString::fromUtf8(fullName));
            }
            contact.saveDetail(&name);
        }
        /*
        if(sn) {
            name.setFirstName(QString::fromUtf8(folks_structured_name_get_given_name(sn)));
            name.setLastName(QString::fromUtf8(folks_structured_name_get_family_name(sn)));
            name.setMiddleName(QString::fromUtf8(folks_structured_name_get_additional_names(sn)));
            name.setPrefix(QString::fromUtf8(folks_structured_name_get_prefixes(sn)));
            name.setSuffix(QString::fromUtf8(folks_structured_name_get_suffixes(sn)));
            contact.saveDetail(&name);
        }
        */
    }
}

void ManagerEngine::updateStructuredNameFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    updateNameFromIndividual(contact, individual);
}

void ManagerEngine::updateFullNameFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    updateNameFromIndividual(contact, individual);
}

void ManagerEngine::updateNicknameFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactNickname>(contact);

    const char *nickname = folks_name_details_get_nickname(
            FOLKS_NAME_DETAILS(individual));
    if(nickname && nickname[0] != '\0') {
        QContactNickname detail;
        detail.setNickname(QString::fromUtf8(nickname));
        contact.saveDetail(&detail);
    }
}

void ManagerEngine::updatePresenceFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactGlobalPresence>(contact);

    QContactGlobalPresence presence;
    if(setPresenceDetail(presence, individual))
        contact.saveDetail(&presence);
}

QContactPresence ManagerEngine::getPresenceForPersona(
        QContact& contact,
        FolksPersona *persona)
{
/*
    if(TPF_IS_PERSONA(persona)) {
        QString presenceUri = getPersonaPresenceUri(persona);
        foreach(const QContactPresence& presence,
                contact.details<QContactPresence>()) {
            if(presence.detailUri() == presenceUri)
                return presence;
        }
    }
*/
    return QContactPresence();
}

void ManagerEngine::updatePresenceFromPersona(
        QContact& contact,
        FolksIndividual *individual,
        FolksPersona *persona)
{
    QContactPresence presence = getPresenceForPersona(contact, persona);
    if(!presence.isEmpty()) {
        if(setPresenceDetail(presence, persona))
            contact.saveDetail(&presence);
    }
}

void ManagerEngine::avatarReadyCB(GObject *source, GAsyncResult *res, gpointer user_data)
{
    AvatarLoadData *data = reinterpret_cast<AvatarLoadData*>(user_data);
    if(data->this_->m_allContacts.contains(data->contactId)) {
        ContactPair &pair = data->this_->m_allContacts[data->contactId];

        QContactAvatar avatar;
        avatar.setImageUrl(data->url);
        pair.contact.saveDetail(&avatar);
        debug() << "AvatarImage (UPDATE):" << data->url;
    // TODO: also emit the detail types..
        emit data->this_->contactsChanged(QList<QContactId>() << data->contactId, QList<QContactDetail::DetailType>());
    }

    g_free(data);
}

void ManagerEngine::updateAvatarFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactAvatar>(contact);
    GLoadableIcon *avatarIcon = folks_avatar_details_get_avatar(
            FOLKS_AVATAR_DETAILS(individual));
    if(avatarIcon) {
        gchar *uri;
        if(G_IS_FILE_ICON(avatarIcon)) {
            GFile *avatarFile = g_file_icon_get_file(G_FILE_ICON(avatarIcon));
            uri = g_file_get_uri(avatarFile);

            QContactAvatar avatar;
            avatar.setImageUrl(QUrl(QLatin1String(uri)));
            contact.saveDetail(&avatar);
            g_free(uri);
        } else {
            FolksAvatarCache *cache = folks_avatar_cache_dup();
            const char *contactId = folks_individual_get_id(individual);
            uri = folks_avatar_cache_build_uri_for_avatar(cache, contactId);
            QUrl url = QUrl(QLatin1String(uri));
            if (QFile::exists(url.toLocalFile())) {
                QContactAvatar avatar;
                avatar.setImageUrl(url);
                contact.saveDetail(&avatar);
            } else {
                AvatarLoadData *data = new AvatarLoadData;
                data->url = url;
                data->contactId = contact.id();
                data->this_ = this;
                folks_avatar_cache_store_avatar(cache, contactId, avatarIcon, ManagerEngine::avatarReadyCB, data);
            }
            g_free(uri);
        }
    }
}

void ManagerEngine::updateBirthdayFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactBirthday>(contact);

    FolksBirthdayDetails *birthday_details = FOLKS_BIRTHDAY_DETAILS(individual);
    GDateTime *folks_birthday = folks_birthday_details_get_birthday(
            birthday_details);
    if(folks_birthday != NULL) {
        QContactBirthday birthday;
        QDateTime dt;
        dt.setTime_t(g_date_time_to_unix(folks_birthday));
        birthday.setDateTime(dt);
        contact.saveDetail(&birthday);
    }
}

void ManagerEngine::updateEmailAddressesFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactEmailAddress>(contact);

    GeeSet *addresses =
        folks_email_details_get_email_addresses(
                FOLKS_EMAIL_DETAILS(individual));
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(addresses));

    while(gee_iterator_next(iter)) {
        FolksEmailFieldDetails *email_fd =
            FOLKS_EMAIL_FIELD_DETAILS(gee_iterator_get(iter));
        QContactEmailAddress addr;
        addr.setEmailAddress(
                QString::fromUtf8(
                    (const char*) folks_abstract_field_details_get_value(
                        FOLKS_ABSTRACT_FIELD_DETAILS(email_fd))));
        setContextsFromFieldDetails(addr,
                FOLKS_ABSTRACT_FIELD_DETAILS(email_fd));
        contact.saveDetail(&addr);
        g_object_unref(email_fd);
    }
    g_object_unref (iter);
}

void ManagerEngine::updateImAddressesFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactOnlineAccount>(contact);

    GeeMultiMap *address =
        folks_im_details_get_im_addresses (
                FOLKS_IM_DETAILS(individual));
    GeeSet *keys = gee_multi_map_get_keys (address);
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(keys));

    while(gee_iterator_next(iter)) {
        const gchar *key = (const gchar*) gee_iterator_get(iter);

        GeeCollection *values = gee_multi_map_get(address, key);
        GeeIterator *iterValues = gee_iterable_iterator(GEE_ITERABLE(values));
        while(gee_iterator_next(iterValues)) {
            QContactOnlineAccount addr;
            FolksAbstractFieldDetails *fd =
                FOLKS_ABSTRACT_FIELD_DETAILS(gee_iterator_get(iterValues));

            addr.setAccountUri(QString::fromUtf8(
                    (const char*) folks_abstract_field_details_get_value(fd)));
            addr.setProtocol(Utils::onlineAccountProtocolFromString(QString::fromUtf8(key)));

            // Set Im type and contexts
            QStringList contexts = contextsFromFieldDetails(addr, FOLKS_ABSTRACT_FIELD_DETAILS(fd));
            addr.setContexts(Utils::contextsFromStrings(contexts));
            addr.setSubTypes(Utils::onlineAccountSubTypesFromStrings(contexts));

            g_object_unref(fd);

            contact.saveDetail(&addr);
        }
        g_object_unref (iterValues);
    }
    g_object_unref (iter);

}

void ManagerEngine::updateFavoriteFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactFavorite>(contact);

    QContactFavorite favorite;
    favorite.setFavorite(folks_favourite_details_get_is_favourite(
                FOLKS_FAVOURITE_DETAILS(individual)));

    contact.saveDetail(&favorite);
}

void ManagerEngine::updateGenderFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactGender>(contact);

    QContactGender gender;
    switch(folks_gender_details_get_gender(FOLKS_GENDER_DETAILS(individual))) {
    case FOLKS_GENDER_FEMALE:
        gender.setGender(QContactGender::GenderFemale);
        break;
    case FOLKS_GENDER_MALE:
        gender.setGender(QContactGender::GenderMale);
        break;
    default:
        // What's the difference between not having this field or having it
        // set to GenderUnspecified? Let's just not save the detail at all.
        return;
    }

    setContextsFromFieldDetails(gender,
            FOLKS_ABSTRACT_FIELD_DETAILS(individual));
    contact.saveDetail(&gender);
}

void ManagerEngine::updateNotesFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactNote>(contact);

    FolksNoteDetails *note_details = FOLKS_NOTE_DETAILS(individual);
    GeeSet *notes = folks_note_details_get_notes(note_details);
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(notes));

    while(gee_iterator_next(iter)) {
        FolksAbstractFieldDetails *fd =
            FOLKS_ABSTRACT_FIELD_DETAILS(gee_iterator_get(iter));
        QContactNote note;
        note.setNote(
                QString::fromUtf8(
                    (const char*) folks_abstract_field_details_get_value(fd)));
        setContextsFromFieldDetails(note, fd);
        contact.saveDetail(&note);

        g_object_unref(fd);
    }
}

void ManagerEngine::updateOrganizationFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactOrganization>(contact);

    FolksRoleDetails *role_details = FOLKS_ROLE_DETAILS(individual);
    GeeSet *roles = folks_role_details_get_roles(role_details);
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(roles));

    while(gee_iterator_next(iter)) {
        FolksAbstractFieldDetails *fd =
            FOLKS_ABSTRACT_FIELD_DETAILS(gee_iterator_get(iter));
        FolksRole *role =
            FOLKS_ROLE(folks_abstract_field_details_get_value(fd));

        QContactOrganization org;
        org.setName(QString::fromUtf8(folks_role_get_organisation_name(role)));
        org.setTitle(QString::fromUtf8(folks_role_get_title(role)));

        setContextsFromFieldDetails(org,
                FOLKS_ABSTRACT_FIELD_DETAILS(fd));
        contact.saveDetail(&org);
    }
}

void ManagerEngine::updatePhoneNumbersFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactPhoneNumber>(contact);

    GeeSet *numbers =
        folks_phone_details_get_phone_numbers(FOLKS_PHONE_DETAILS(individual));
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(numbers));

    while(gee_iterator_next(iter)) {
        FolksAbstractFieldDetails *fd =
            FOLKS_ABSTRACT_FIELD_DETAILS(gee_iterator_get(iter));
        QContactPhoneNumber number;
        number.setNumber(
                QString::fromUtf8(
                    (const char*) folks_abstract_field_details_get_value(fd)));

        // Set phone type
        QStringList contexts = contextsFromFieldDetails(number, fd);
        number.setContexts(Utils::contextsFromStrings(contexts));
        number.setSubTypes(Utils::phoneSubTypesFromStrings(contexts));

        contact.saveDetail(&number);
        g_object_unref(fd);
    }
    g_object_unref (iter);
}

void ManagerEngine::updateAddressesFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactAddress>(contact);

    GeeSet *addresses = folks_postal_address_details_get_postal_addresses(
            FOLKS_POSTAL_ADDRESS_DETAILS(individual));
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(addresses));

    while(gee_iterator_next(iter)) {
        FolksPostalAddressFieldDetails *addrFd =
            FOLKS_POSTAL_ADDRESS_FIELD_DETAILS(gee_iterator_get(iter));
        FolksPostalAddress *addr = FOLKS_POSTAL_ADDRESS(
                folks_abstract_field_details_get_value (
                        FOLKS_ABSTRACT_FIELD_DETAILS(addrFd)));
        QContactAddress address;
        address.setCountry(
                QString::fromUtf8(folks_postal_address_get_country(addr)));
        address.setLocality(
                QString::fromUtf8(folks_postal_address_get_locality(addr)));
        address.setPostOfficeBox(
                QString::fromUtf8(folks_postal_address_get_po_box(addr)));
        address.setPostcode(
                QString::fromUtf8(folks_postal_address_get_postal_code(addr)));
        address.setRegion(
                QString::fromUtf8(folks_postal_address_get_region(addr)));
        address.setStreet(
                QString::fromUtf8(folks_postal_address_get_street(addr)));

        QStringList contexts = contextsFromFieldDetails(address,
                FOLKS_ABSTRACT_FIELD_DETAILS(addrFd));
        address.setContexts(Utils::contextsFromStrings(contexts));
        address.setSubTypes(Utils::addressSubTypesFromStrings(contexts));

        contact.saveDetail(&address);

        g_object_unref(addrFd);
    }
    g_object_unref (iter);
}

void ManagerEngine::updateUrlsFromIndividual(
        QContact &contact,
        FolksIndividual *individual)
{
    removeOldDetails<QContactUrl>(contact);

    GeeSet *urls = folks_url_details_get_urls(FOLKS_URL_DETAILS(individual));
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(urls));

    while(gee_iterator_next(iter)) {
        FolksAbstractFieldDetails *fd =
            FOLKS_ABSTRACT_FIELD_DETAILS(gee_iterator_get(iter));
        QContactUrl url;
        url.setUrl(
                QString::fromUtf8(
                    (const char*) folks_abstract_field_details_get_value(fd)));
        setContextsFromFieldDetails(url, fd);
        contact.saveDetail(&url);

        g_object_unref(fd);
    }
    g_object_unref (iter);
}

void ManagerEngine::updatePersonas(
        QContact &contact,
        FolksIndividual *individual,
        GeeSet *added,
        GeeSet *removed)
{
#define CONNECT_PERSONA_NOTIFY(propertyName) \
    signalHandlerId = C_NOTIFY_CONNECT(persona, propertyName, \
            personaPresenceChangedCb); \
    m_personasSignalHandlerIds.insert( \
            QPair<FolksIndividual *, FolksPersona *>(individual, persona), \
            signalHandlerId);

    /* this will be used throughout this function */
    GeeIterator *iter;

    iter = gee_iterable_iterator(GEE_ITERABLE(added));
    while(gee_iterator_next(iter)) {
        FolksPersona *persona = FOLKS_PERSONA(gee_iterator_get(iter));
#if 0
        if(TPF_IS_PERSONA(persona)) {
            m_personasToIndividuals.insertMulti(persona, individual);

            gulong signalHandlerId = 0;

            if(FOLKS_IS_PRESENCE_DETAILS(persona)) {
                CONNECT_PERSONA_NOTIFY("presence-type");
                CONNECT_PERSONA_NOTIFY("presence-message");
            }

            if(FOLKS_IS_ALIAS_DETAILS(persona))
                /* The alias for a single account detail is set in the
                 * presence too */
                CONNECT_PERSONA_NOTIFY("alias");

            //addAccountDetails(contact, TPF_PERSONA(persona));
        }
#endif
        g_object_unref(persona);
    }
    g_object_unref (iter);

#undef CONNECT_PERSONA_NOTIFY

    iter = gee_iterable_iterator(GEE_ITERABLE(removed));
    while(gee_iterator_next(iter)) {
        FolksPersona *persona = FOLKS_PERSONA(gee_iterator_get(iter));
#if 0
        if(TPF_IS_PERSONA(persona)) {
            // TODO: no need to do this anymore, use the folks id
            if(m_individualsToIds.contains(individual)) {
                QContactId contactId = m_individualsToIds[individual];
                ContactPair& contactPair = m_allContacts[contactId];
                //removeAccountDetails(contactPair.contact, TPF_PERSONA(persona));

                QPair<FolksIndividual *, FolksPersona *> pair(individual,
                        persona);
                gulong signalHandlerId = m_personasSignalHandlerIds[pair];
                if(signalHandlerId)
                    g_signal_handler_disconnect(persona, signalHandlerId);
                m_personasSignalHandlerIds.remove(pair);

                m_personasToIndividuals.remove(persona, individual);
            }
        }
#endif

        g_object_unref(persona);
    }
    g_object_unref (iter);
}
#if 0
void ManagerEngine::addAccountDetails(
        QContact& contact,
        TpfPersona *persona)
{
    TpfPersonaStore *store = TPF_PERSONA_STORE(
            folks_persona_get_store(FOLKS_PERSONA(persona)));
    TpAccount *tpAccount = tpf_persona_store_get_account(store);
    if(!tpAccount)
        return;

    QContactOnlineAccount account;
    account.setAccountUri(
            QString::fromUtf8(folks_persona_get_display_id(FOLKS_PERSONA(persona))));
    // FIXME: detail fields are no longer represented by strings, so this needs to be done
    // in a different way.
    /*account.setValue("TelepathyAccountPath",
            QString::fromUtf8(tp_proxy_get_object_path(TP_PROXY(tpAccount))));*/

    QRegExp lettersRegexp(QLatin1String("^[a-z]+$"));

    QString protocol = QString::fromUtf8(tp_account_get_protocol(tpAccount));
    account.setProtocol(Utils::onlineAccountProtocolFromString(protocol));

    QList<int> subTypes;
    if(protocol == QLatin1String("sip"))
        subTypes << QContactOnlineAccount::SubTypeSip
                 << QContactOnlineAccount::SubTypeSipVoip;
    else if(protocol == QLatin1String("jabber"))
        subTypes << QContactOnlineAccount::SubTypeImpp;
    account.setSubTypes(subTypes);

    account.setDetailUri(getPersonaAccountUri(FOLKS_PERSONA(persona)));

    // FIXME: We should set the capabilities here, but from what I understood
    // the current capabilities functions could be deprecated and replaced by
    // QContactAction and related classes (that don't exist yet in the most
    // recently released QtContacts).

    QContactPresence presence;
    if(setPresenceDetail(presence, persona)) {
        QString presenceUri = getPersonaPresenceUri(FOLKS_PERSONA(persona));
        presence.setDetailUri(presenceUri);
        contact.saveDetail(&presence);
        account.setLinkedDetailUris(presenceUri);
    }

    contact.saveDetail(&account);
}

void ManagerEngine::removeAccountDetails(
        QContact& contact,
        TpfPersona *persona)
{
    QList<QContactOnlineAccount> accounts =
        contact.details<QContactOnlineAccount>();
    QString accountUri = getPersonaAccountUri(FOLKS_PERSONA(persona));
    foreach(QContactOnlineAccount account, accounts) {
        if(account.detailUri() == accountUri) {
            contact.removeDetail(&account);
            break;
        }
    }

    QContactPresence presence = getPresenceForPersona(contact,
            FOLKS_PERSONA(persona));
    if(!presence.isEmpty())
        contact.removeDetail(&presence);
}
#endif

QString ManagerEngine::getPersonaPresenceUri(
        FolksPersona *persona)
{
    return QLatin1String("presence:") +
        QString::fromUtf8(folks_persona_get_uid(persona));
}

QString ManagerEngine::getPersonaAccountUri(
        FolksPersona *persona)
{
    return QLatin1String("account:") +
        QString::fromUtf8(folks_persona_get_uid(persona));
}

void ManagerEngine::individualsChangedCb(
        FolksIndividualAggregator *aggregator,
        GeeSet *added,
        GeeSet *removed,
        gchar *message,
        FolksPersona *actor,
        FolksGroupDetailsChangeReason reason)
{
qWarning("contacts manager got individualsChangedCb");
    QList<QContactId> removedIds;
    QList<QContactId> addedIds;

    /* this will be used throughout this function */
    GeeIterator *iter;

    iter = gee_iterable_iterator(GEE_ITERABLE(removed));
    while(gee_iterator_next(iter)) {
        FolksIndividual *individual = FOLKS_INDIVIDUAL(gee_iterator_get(iter));
        QContactId id = removeIndividual(individual);
qWarning("contact got removed id : %s", qPrintable(id.toString()));
        if(!id.isNull())
            removedIds << id;

        g_object_unref(individual);
    }
    g_object_unref (iter);

    iter = gee_iterable_iterator(GEE_ITERABLE(added));
    while(gee_iterator_next(iter)) {
        FolksIndividual *individual = FOLKS_INDIVIDUAL(gee_iterator_get(iter));
        QContactId id = addIndividual(individual);
qWarning("contact got added id : %s", qPrintable(id.toString()));

        if(!id.isNull())
            addedIds << id;

        g_object_unref(individual);
    }
    g_object_unref (iter);

    if(!removedIds.isEmpty()) {
qWarning("contacts manager emitting contacts removed");
        m_notifier->contactsRemoved(removedIds);
        emit contactsRemoved(removedIds);
}

    if(!addedIds.isEmpty()) {
qWarning("contacts manager emitting contacts added");
        m_notifier->contactsAdded(addedIds);
        emit contactsAdded(addedIds);
//        emit dataChanged(); // big hammer
}

  m_initialIndividualsAdded = true;
}

QContactPresence::PresenceState ManagerEngine::folksToQtPresence(
        FolksPresenceType fp)
{
    switch(fp) {
    case FOLKS_PRESENCE_TYPE_OFFLINE:
        return QContactPresence::PresenceOffline;
    case FOLKS_PRESENCE_TYPE_AVAILABLE:
        return QContactPresence::PresenceAvailable;
    case FOLKS_PRESENCE_TYPE_AWAY:
        return QContactPresence::PresenceAway;
    case FOLKS_PRESENCE_TYPE_EXTENDED_AWAY:
        return QContactPresence::PresenceExtendedAway;
    case FOLKS_PRESENCE_TYPE_HIDDEN:
        return QContactPresence::PresenceHidden;
    case FOLKS_PRESENCE_TYPE_BUSY:
        return QContactPresence::PresenceBusy;
    case FOLKS_PRESENCE_TYPE_UNSET:
        // This should not happen as we don't set any presence in this
        // case
    case FOLKS_PRESENCE_TYPE_UNKNOWN:
    case FOLKS_PRESENCE_TYPE_ERROR:
    default:
        return QContactPresence::PresenceUnknown;
    }
}
QByteArray dbIdToByteArray(quint32 dbId, bool isCollection = false)
{
    return isCollection ? (QByteArrayLiteral("col-") + QByteArray::number(dbId))
                        : (QByteArrayLiteral("sql-") + QByteArray::number(dbId));
}

QContactId ManagerEngine::addIndividual(
        FolksIndividual *individual)
{
    QContact contact;
//    EngineId *engineId = new EngineId(QString::fromUtf8(folks_individual_get_id(individual)), managerUri());
//    QContactId contactId(engineId);
//    contact.setId(contactId);

  /* alien tooling needs contacts to be in this collection and of this type because
   * 1) it applies a filter for these two things when it calls contacts() and
   * 2) it won't pick up any changes to contacts signalled via dbus otherwise
   */
QContactCollectionId colId = QContactCollectionId(managerUri(), dbIdToByteArray(1, true));

    contact.setId(QContactId(managerUri(), dbIdToByteArray(qHash(QString::fromUtf8(folks_individual_get_id(individual))))));

        QContactType type;
        type.setType(QContactType::TypeContact);
        contact.saveDetail(&type);


        contact.setCollectionId(colId);

    if(folks_individual_get_is_user(individual)) {
        debug() << "Skipping self contact:"
            << folks_name_details_get_full_name(FOLKS_NAME_DETAILS(individual))
            << individual;
        return QContactId();
    }

    debug() << "Added:"
        << folks_name_details_get_full_name(FOLKS_NAME_DETAILS(individual))
        << individual
        << qPrintable(QString::fromUtf8(folks_individual_get_id(individual)))
        << qPrintable(colId.toString());

    C_NOTIFY_CONNECT(individual, "alias", aliasChangedCb);
    updateAliasFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "structured-name", structuredNameChangedCb);
    updateStructuredNameFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "full-name", fullNameChangedCb);
    updateFullNameFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "nickname", nicknameChangedCb);
    updateNicknameFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "presence-type", presenceChangedCb);
    C_NOTIFY_CONNECT(individual, "presence-message", presenceChangedCb);
    updatePresenceFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "birthday", birthdayChangedCb);
    updateBirthdayFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "email-addresses", emailAddressesChangedCb);
    updateEmailAddressesFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "im-addresses", imAddressesChangedCb);
    updateImAddressesFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "favourite", favouriteChangedCb);
    updateFavoriteFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "gender", genderChangedCb);
    updateGenderFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "notes", notesChangedCb);
    updateNotesFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "roles", rolesChangedCb);
    updateOrganizationFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "phone-numbers", phoneNumbersChangedCb);
    updatePhoneNumbersFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "postal-addresses", postalAddressesChangedCb);
    updateAddressesFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "urls", urlsChangedCb);
    updateUrlsFromIndividual(contact, individual);

    C_NOTIFY_CONNECT(individual, "avatar", avatarChangedCb);
    updateAvatarFromIndividual(contact, individual);

    GeeSet *empty_set = gee_set_empty(G_TYPE_NONE, NULL, NULL);
    C_CONNECT(individual, "personas-changed", personasChangedCb);
    updatePersonas(contact, individual,
            folks_individual_get_personas(individual), empty_set);
    g_object_unref(empty_set);

    // Store the contact
    ContactPair pair(contact, individual);
    m_allContacts.insert(contact.id(), pair);

    m_individualsToIds.insert(individual, contact.id());

    return contact.id();
}

QContactId ManagerEngine::removeIndividual(
        FolksIndividual *individual)
{
    debug() << "Removed:"
        << folks_alias_details_get_alias(FOLKS_ALIAS_DETAILS(individual))
        << individual;

    QContactId id;
    if (m_individualsToIds.contains(individual)) {
        id = m_individualsToIds[individual];
        m_individualsToIds.remove(individual);
        m_allContacts.remove(id);
    }

    return id;
}

// DetailType can be a QContactGlobalPresence or a QContactPresence
// FolkType can be a FolksIndividual or a FolksPersona
template<typename DetailType, typename FolkType>
bool ManagerEngine::setPresenceDetail(
        DetailType& detail,
        FolkType *folk)
{
    Q_ASSERT(FOLKS_IS_PRESENCE_DETAILS(folk) && FOLKS_IS_ALIAS_DETAILS(folk));

    if(folks_presence_details_get_presence_type(FOLKS_PRESENCE_DETAILS(folk))
            != FOLKS_PRESENCE_TYPE_UNSET) {
        detail.setCustomMessage(QString::fromUtf8(
            folks_presence_details_get_presence_message(
                FOLKS_PRESENCE_DETAILS(folk))));
        detail.setPresenceState(folksToQtPresence(
            folks_presence_details_get_presence_type(
                FOLKS_PRESENCE_DETAILS(folk))));
        detail.setNickname(QString::fromUtf8(
            folks_alias_details_get_alias(FOLKS_ALIAS_DETAILS(folk))));
        return true;
    }
    else
        return false;
}

QList<QContactId> ManagerEngine::contactIds(
        const QContactFilter& filter,
        const QList<QContactSortOrder>& sortOrders,
        QContactManager::Error *error) const
{
qWarning("contacts someone requested IDS");
    QList<QContactId> ids;
    QContactManager::Error tmpError = QContactManager::NoError;
    QList<QContact> cnts = contacts(filter, sortOrders,
            QContactFetchHint(), &tmpError);
    if(tmpError == QContactManager::NoError) {
        foreach(const QContact& contact, cnts)
            ids << contact.id();
    }
    else
        *error = tmpError;

    return ids;
}

QContact ManagerEngine::compatibleContact (const QContact & original,
        QContactManager::Error * error ) const
{
qWarning("contacts compatible contact req"); 
    // Let's keep all information about contact we will need that on EDS
    return original;
}

QContact ManagerEngine::contact(
        const QContactId& contactId,
        const QContactFetchHint& fetchHint,
        QContactManager::Error* error) const
{
    Q_UNUSED(fetchHint);
qWarning("contacts SOMEONE requestioned a contacts");

    QContact contact = QContact();
    if(m_allContacts.contains(contactId)) {
qWarning("contacts found it: id %s", qPrintable(contactId.toString()));

        ContactPair pair = m_allContacts[contactId];
        contact = pair.contact;
        *error = QContactManager::NoError;
    }

    return contact;
}

QList<QContact> ManagerEngine::contacts(
        const QContactFilter& filter,
        const QList<QContactSortOrder>& sortOrders,
        const QContactFetchHint& fetchHint,
        QContactManager::Error *error) const
{
qWarning("contacts SOMEONE requestioned contacts, filter type %d", filter.type());
QList<QContactFilter> filters = ((QContactIntersectionFilter& )filter).filters();
    foreach(QContactFilter f, filters) {
qWarning("contacts internal filter: type %d", f.type());
      if (f.type() == 1) {
        const QVariant& v = ((QContactDetailFilter& )f).value();
qWarning("contacts detail filter: detailType %d detailField %d, variant t %s", ((QContactDetailFilter& )f).detailType(), ((QContactDetailFilter& )f).detailField(), qPrintable(v.typeName()));
        int url = v.toInt();
qWarning("contacts url int: %d", url);
  }

      if (f.type() == 10) {
        const QSet<QContactCollectionId> ids = ((QContactCollectionFilter& )f).collectionIds();
    foreach(QContactCollectionId id, ids) {
qWarning("contacts collection ID filter, id: %s", qPrintable(id.toString()));

        }
      }
    }

    QList<QContact> cnts;

    foreach(const ContactPair& pair, m_allContacts) {
qWarning("contacts adding one to list");
        /* no clue what that filter set by sailfish is, all we know is that ours don't pass it */
        if(QContactManagerEngine::testFilter(filter, pair.contact)) {
qWarning("contact passed the filter");
            QContactManagerEngine::addSorted(&cnts, pair.contact, sortOrders);
}
    }

qWarning("contacts done adding");
/*
    QContact contact;
//    EngineId *engineId = new EngineId(QString::fromUtf8(folks_individual_get_id(individual)), managerUri());
//    QContactId contactId(engineId);
//    contact.setId(contactId);
QContactId contactId = QContactId(managerUri(), QByteArrayLiteral("sql-") + QByteArray::number(0));
    contact.setId(contactId);
qWarning("contact added default test contact with id id : %s", qPrintable(contactId.toString()));
        QContactPhoneNumber number;
        number.setNumber(
                QString::fromLatin1("004915234742285"));

        contact.saveDetail(&number);

        QContactName name;

            name.setFirstName(QString::fromLatin1("tester"));
            name.setLastName(QString::fromLatin1("testwssises"));
            contact.saveDetail(&name);

            QContactManagerEngine::addSorted(&cnts, contact, sortOrders);
*/
    return cnts;
}
    QList<QContact> ManagerEngine::contacts(
                const QList<QContactId> &localIds,
                const QContactFetchHint &fetchHint,
                QMap<int, QContactManager::Error> *errorMap,
                QContactManager::Error *error) const
{
qWarning("contacts local ids not implemented");
    QList<QContact> cnts;

    return cnts;

}

QList<QContact> ManagerEngine::contacts(
        const QContactFilter &filter,
        const QList<QContactSortOrder> &sortOrders,
        const QContactFetchHint &fetchHint,
        QMap<int, QContactManager::Error> *errorMap,
        QContactManager::Error *error) const
{
    Q_UNUSED(errorMap);
qWarning("contacts contacts fetch order not implemented");

    return contacts(filter, sortOrders, fetchHint, error);
}

bool ManagerEngine::saveContacts(
            QList<QContact> *contacts,
            QMap<int, QContactManager::Error> *errorMap,
            QContactManager::Error *error)
{
qWarning("contacts safeContacts not implemented");

    return false;
}


bool ManagerEngine::removeContact(const QContactId &contactId, QContactManager::Error* error)
{
    QMap<int, QContactManager::Error> errorMap;
qWarning("contacts remove contacts implemented");

return false;
}

bool ManagerEngine::removeContacts(
            const QList<QContactId> &contactIds,
            QMap<int, QContactManager::Error> *errorMap,
            QContactManager::Error* error)
{
qWarning("contacts removed contacts 2 implemented");
return false;
}

QContactId ManagerEngine::selfContactId(QContactManager::Error* error) const
{
qWarning("contacts self contact id implemented");
QContactId contactId;
    return contactId;
}

bool ManagerEngine::setSelfContactId(
        const QContactId&, QContactManager::Error* error)
{
qWarning("contacts set self contact id implemented");

    return false;
}

QContactCollectionId ManagerEngine::defaultCollectionId() const
{
qWarning("contacts default col id implemented");

    return QContactCollectionId();
}

QContactCollection ManagerEngine::collection(
        const QContactCollectionId &collectionId,
        QContactManager::Error *error) const
{
qWarning("contacts default col() id implemented");

    return QContactCollection();
}

QList<QContactCollection> ManagerEngine::collections(
        QContactManager::Error *error) const
{
    QList<QContactCollection> collections;
qWarning("contacts default cols() id implemented");

    return collections;
}


void ManagerEngine::personasChangedCb(
        FolksIndividual *individual,
        GeeSet *added,
        GeeSet *removed)
{
qWarning("contacts got personas changed for an individual");
    if(!added && !removed)
        return;

    if (!m_individualsToIds.contains(individual))
        return;

    QContactId contactId = m_individualsToIds[individual];

    ContactPair& pair = m_allContacts[contactId];
    updatePersonas(pair.contact, individual, added, removed);

        m_notifier->contactsChanged(QList<QContactId>() << contactId);
    emit contactsChanged(QList<QContactId>() << contactId, QList<QContactDetail::DetailType>());
}

#define IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(cb, updateFunction) \
    void ManagerEngine::cb( \
            FolksIndividual *individual) \
    { \
        if (!m_individualsToIds.contains(individual)) \
            return; \
        \
        QContactId contactId = m_individualsToIds[individual]; \
        \
        ContactPair& pair = m_allContacts[contactId]; \
        updateFunction(pair.contact, individual); \
        \
        m_notifier->contactsChanged(QList<QContactId>() << contactId); \
        emit contactsChanged(QList<QContactId>() << contactId, QList<QContactDetail::DetailType>()); \
    }

IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        aliasChangedCb,
        updateAliasFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        structuredNameChangedCb,
        updateStructuredNameFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        fullNameChangedCb,
        updateFullNameFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        nicknameChangedCb,
        updateNicknameFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        presenceChangedCb,
        updatePresenceFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        birthdayChangedCb,
        updateBirthdayFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        avatarChangedCb,
        updateAvatarFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        emailAddressesChangedCb,
        updateEmailAddressesFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        imAddressesChangedCb,
        updateImAddressesFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        favouriteChangedCb,
        updateFavoriteFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        genderChangedCb,
        updateGenderFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        notesChangedCb,
        updateNotesFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        rolesChangedCb,
        updateOrganizationFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        phoneNumbersChangedCb,
        updatePhoneNumbersFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        postalAddressesChangedCb,
        updateAddressesFromIndividual)
IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK(
        urlsChangedCb,
        updateUrlsFromIndividual)

#undef IMPLEMENT_INDIVIDUAL_NOTIFY_CALLBACK

#define IMPLEMENT_PERSONA_NOTIFY_CALLBACK(cb, updateFunction) \
    void ManagerEngine::cb( \
            FolksPersona *persona) \
    { \
        QList<FolksIndividual *> individuals = \
            m_personasToIndividuals.values(persona); \
        QList<QContactId> changedIds; \
        foreach(FolksIndividual * individual, individuals) { \
            if (!m_individualsToIds.contains(individual)) \
                return; \
            \
            QContactId contactId = m_individualsToIds[individual]; \
            \
            ContactPair& pair = m_allContacts[contactId]; \
            updateFunction(pair.contact, individual, persona); \
            \
            changedIds << contactId; \
        } \
        \
        if(!changedIds.isEmpty()) { \
            m_notifier->contactsChanged(changedIds); \
            emit contactsChanged(changedIds, QList<QContactDetail::DetailType>()); \
        } \
    }

IMPLEMENT_PERSONA_NOTIFY_CALLBACK(
        personaPresenceChangedCb,
        updatePresenceFromPersona)

#undef IMPLEMENT_PERSONA_NOTIFY_CALLBACK

static GValue* asvSetStrNew(QMultiMap<QString, QString> providerUidMap)
{
GeeMultiMap *hashSet =
    GEE_MULTI_MAP(gee_hash_multi_map_new(G_TYPE_STRING,\
                                         (GBoxedCopyFunc) g_strdup, g_free, \
                                         FOLKS_TYPE_IM_FIELD_DETAILS, \
                                         (GBoxedCopyFunc)g_object_ref, g_object_unref, \
                                         my_g_str_hash, \
                                         NULL, \
                                         NULL, \
                                         my_g_str_equal, \
                                         NULL, \
                                         NULL, \
                                         _folks_abstract_field_details_hash_data_func, \
                                         NULL, \
                                         NULL, \
                                         _folks_abstract_field_details_equal_data_func, \
                                         NULL, \
                                         NULL));
/*
    GeeMultiMap *hashSet = GEE_MULTI_MAP(
            gee_hash_multi_map_new(G_TYPE_STRING, (GBoxedCopyFunc) g_strdup,
                    g_free,
                    FOLKS_TYPE_IM_FIELD_DETAILS,
                    g_object_ref, g_object_unref,
                    g_str_hash, g_str_equal,
                    (GHashFunc) folks_abstract_field_details_hash,
                    (GEqualFunc) folks_abstract_field_details_equal));
*/
    GValue *retval = gValueSliceNew (G_TYPE_OBJECT);
    g_value_take_object (retval, hashSet);

    QList<QString> keys = providerUidMap.keys();
    foreach(const QString& key, keys) {
        QList<QString> values = providerUidMap.values(key);
        foreach(const QString& value, values) {
            FolksImFieldDetails *imfd;

            imfd = folks_im_field_details_new (value.toUtf8().data(), NULL);

            gee_multi_map_set(hashSet,
                              key.toUtf8().data(),
                              imfd);
            g_object_unref(imfd);
        }
    }

    return retval;
}

static void gValueGeeSetAddStringFieldDetails(GValue *value,
        GType g_type,
        const char* v_string,
        QStringList contexts)
{
    GeeCollection *collection = (GeeCollection*) g_value_get_object(value);

    if(collection == NULL) {
        collection = GEE_COLLECTION(SET_AFD_NEW());
        g_value_take_object(value, collection);
    }

    FolksAbstractFieldDetails *fieldDetails = NULL;

    if(FALSE) {
    } else if(g_type == FOLKS_TYPE_EMAIL_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_email_field_details_new(v_string, NULL));
    } else if(g_type == FOLKS_TYPE_IM_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_im_field_details_new(v_string, NULL));
    } else if(g_type == FOLKS_TYPE_NOTE_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_note_field_details_new(v_string, NULL, NULL));
    } else if(g_type == FOLKS_TYPE_PHONE_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_phone_field_details_new(v_string, NULL));
    } else if(g_type == FOLKS_TYPE_URL_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_url_field_details_new(v_string, NULL));
    } else if(g_type == FOLKS_TYPE_WEB_SERVICE_FIELD_DETAILS) {
        fieldDetails = FOLKS_ABSTRACT_FIELD_DETAILS (
                folks_web_service_field_details_new(v_string, NULL));
    }

    if (fieldDetails == NULL) {
        qWarning() << "Invalid fieldDetails type" << g_type;
    } else {
        setFieldDetailsFromContexts (fieldDetails, "type", contexts);
        gee_collection_add(collection, fieldDetails);

        g_object_unref(fieldDetails);
    }
}

static QContactManager::Error managerErrorFromIndividualAggregatorError(
        FolksIndividualAggregatorError errornum)
{
    switch(errornum) {
        // XXX: Add Failed -> Bad Argument isn't a perfect mapping
        case FOLKS_INDIVIDUAL_AGGREGATOR_ERROR_ADD_FAILED:
        {
            return QContactManager::BadArgumentError;
        }
        break;
        break;
        case FOLKS_INDIVIDUAL_AGGREGATOR_ERROR_STORE_OFFLINE:
        {
            return QContactManager::LockedError;
        }
        break;
        case FOLKS_INDIVIDUAL_AGGREGATOR_ERROR_NO_WRITEABLE_STORE:
        {
            return QContactManager::NotSupportedError;
        }
        break;
        default:
        {
            return QContactManager::UnspecifiedError;
        }
        break;
    }
}

void
ManagerEngine::aggregatorAddPersonaFromDetailsCb(GObject *source,
    GAsyncResult *result,
    QContactSaveRequest *request,
    QContact &contact)
{
    FolksIndividualAggregator *aggregator = FOLKS_INDIVIDUAL_AGGREGATOR(source);
    FolksIndividual *individual;
    FolksPersona *persona;
    GError *error = NULL;

    QContactManager::Error opError = QContactManager::NoError;
    QMap<int, QContactManager::Error> contactErrors;

    persona = folks_individual_aggregator_add_persona_from_details_finish(
        aggregator, result, &error);
    if(error != NULL) {
        qWarning() << "Failed to add individual from contact:"
            << error->message;

        opError = managerErrorFromIndividualAggregatorError(
                (FolksIndividualAggregatorError) error->code);

        g_clear_error(&error);
    } else if(persona == NULL) {
        opError = QContactManager::AlreadyExistsError;
        contactErrors.insert(0, opError);
    } else {
        individual = folks_persona_get_individual(persona);
        if (individual) {
//            EngineId *engineId = new EngineId(QString::fromUtf8(folks_individual_get_id(individual)), managerUri());
//            QContactId contactId(engineId);
//            contact.setId(contactId);
    contact.setId(QContactId(managerUri(), dbIdToByteArray(qHash(QString::fromUtf8(folks_individual_get_id(individual))))));
        }
    }

    contactErrors.insert(0, opError);
    QList<QContact> contacts;
    contacts << contact;
    updateContactSaveRequest(request, contacts, opError, contactErrors,
            QContactAbstractRequest::FinishedState);
}

#define PERSONA_DETAILS_INSERT(details, key, value) \
    g_hash_table_insert((details), \
            (gpointer) folks_persona_store_detail_key((key)), (value));

#define PERSONA_DETAILS_INSERT_STRING_FIELD_DETAILS(\
        details, key, value, q_type, g_type, member) \
{ \
    QList<q_type> contactDetails = contact.details<q_type>(); \
    if(contactDetails.size() > 0) { \
        value = gValueSliceNew(G_TYPE_OBJECT); \
        foreach(const q_type& detail, contact.details<q_type>()) { \
            if(!detail.isEmpty()) { \
                gValueGeeSetAddStringFieldDetails(value, (g_type), \
                        detail.member().toUtf8().data(), \
                        Utils::contextsFromEnums(detail.contexts())); \
            } \
        } \
        PERSONA_DETAILS_INSERT((details), (key), (value)); \
    } \
}


static guint my_folks_role_hash (gconstpointer v, gpointer self)
{
    const FolksRole *constDetails = static_cast<const FolksRole*>(v);
    return folks_role_hash (const_cast<FolksRole*>(constDetails));
}

static int my_folks_role_equal (gconstpointer a, gconstpointer b, gpointer self)
{
    const FolksRole *constDetailsA = static_cast<const FolksRole*>(a);
    const FolksRole *constDetailsB = static_cast<const FolksRole*>(b);
    return folks_role_equal (const_cast<FolksRole*>(constDetailsA), const_cast<FolksRole*>(constDetailsB));
}


/* free the returned GHashTable* with g_hash_table_destroy() */
static GHashTable* personaDetailsHashFromQContact(
        const QContact &contact)
{
    GHashTable *details = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
            (GDestroyNotify) gValueSliceFree);
    GValue *value;

    /*
     * Addresses
     */
    QList<QContactAddress> addresses = contact.details<QContactAddress>();
    if(addresses.size() > 0) {
        value = gValueSliceNew(G_TYPE_OBJECT);
        foreach(const QContactAddress& address, addresses) {
            if(!address.isEmpty()) {
                FolksPostalAddress *postalAddress = folks_postal_address_new(
                        address.postOfficeBox().toUtf8().data(),
                        NULL,
                        address.street().toUtf8().data(),
                        address.locality().toUtf8().data(),
                        address.region().toUtf8().data(),
                        address.postcode().toUtf8().data(),
                        address.country().toUtf8().data(),
                        NULL,
                        NULL);

                GeeCollection *collection =
                    (GeeCollection*) g_value_get_object(value);
                if(collection == NULL) {
                    collection = GEE_COLLECTION(SET_AFD_NEW());
                    g_value_take_object(value, collection);
                }

                FolksPostalAddressFieldDetails *pafd =
                    folks_postal_address_field_details_new (postalAddress,
                            NULL);

                // Set contexts and subTypes
                QStringList contexts = Utils::contextsFromEnums(address.contexts());
                contexts << Utils::addressSubTypesFromEnums(address.subTypes());
                setFieldDetailsFromContexts ((FolksAbstractFieldDetails*)pafd, "type", contexts);

                gee_collection_add(collection, pafd);

                g_object_unref(pafd);
                g_object_unref(postalAddress);
            }
        }

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_POSTAL_ADDRESSES,
                value);
    }

    /*
     * Avatar
     */
    QContactAvatar avatar = contact.detail<QContactAvatar>();
    if(!avatar.isEmpty()) {
        value = gValueSliceNew(G_TYPE_FILE_ICON);
        QUrl avatarUri = avatar.imageUrl();
        if(!avatarUri.isEmpty()) {
            QString formattedUri = avatarUri.toString(QUrl::RemoveUserInfo);
            if(!formattedUri.isEmpty()) {
                GFile *avatarFile =
                    g_file_new_for_uri(formattedUri.toUtf8().data());
                GFileIcon *avatarFileIcon = G_FILE_ICON(
                        g_file_icon_new(avatarFile));
                g_value_take_object(value, avatarFileIcon);

                PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_AVATAR,
                        value);

                gObjectClear((GObject**) &avatarFile);
            }
        }
    }

    /*
     * Birthday
     */
    QContactBirthday birthday = contact.detail<QContactBirthday>();
    if(!birthday.isEmpty()) {
        value = gValueSliceNew(G_TYPE_DATE_TIME);
        GDateTime *dateTime = g_date_time_new_from_unix_utc(
                birthday.dateTime().toMSecsSinceEpoch() / 1000);
        g_value_set_boxed(value, dateTime);

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_BIRTHDAY, value);

        g_date_time_unref(dateTime);
    }

    /*
     * Email addresses
     */
    PERSONA_DETAILS_INSERT_STRING_FIELD_DETAILS(details,
        FOLKS_PERSONA_DETAIL_EMAIL_ADDRESSES, value, QContactEmailAddress,
        FOLKS_TYPE_EMAIL_FIELD_DETAILS, emailAddress);


    /*
     * Favorite
     */
    QContactFavorite favorite = contact.detail<QContactFavorite>();
    if(!favorite.isEmpty()) {
        value = gValueSliceNew(G_TYPE_BOOLEAN);
        g_value_set_boolean(value, favorite.isFavorite());

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_IS_FAVOURITE,
                value);
    }

    /*
     * Gender
     */
    QContactGender gender = contact.detail<QContactGender>();
    if(!gender.isEmpty()) {
        value = gValueSliceNew(FOLKS_TYPE_GENDER);
        FolksGender genderEnum = FOLKS_GENDER_UNSPECIFIED;
        if(gender.gender() == QContactGender::GenderMale) {
            genderEnum = FOLKS_GENDER_MALE;
        } else if(gender.gender() == QContactGender::GenderFemale) {
            genderEnum = FOLKS_GENDER_FEMALE;
        }
        g_value_set_enum(value, genderEnum);

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_GENDER, value);
    }

    /*
     * Names
     */
    QContactName name = contact.detail<QContactName>();
    if(!name.isEmpty()) {
        value = gValueSliceNew(FOLKS_TYPE_STRUCTURED_NAME);
        FolksStructuredName *sn = folks_structured_name_new(
                name.lastName().toUtf8().data(),
                name.firstName().toUtf8().data(),
                name.middleName().toUtf8().data(),
                name.prefix().toUtf8().data(),
                name.suffix().toUtf8().data());
        g_value_take_object(value, sn);

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_STRUCTURED_NAME,
                value);

    }

    QContactDisplayLabel displayLabel = contact.detail<QContactDisplayLabel>();
    if(!displayLabel.label().isEmpty()) {
        value = gValueSliceNew(G_TYPE_STRING);
        g_value_set_string(value, displayLabel.label().toUtf8().data());
        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_FULL_NAME, value);
        // FIXME: check if those values should all be set to the same thing
        value = gValueSliceNew(G_TYPE_STRING);
        g_value_set_string(value, displayLabel.label().toUtf8().data());
        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_ALIAS, value);
    }

    /*
     * Notes
     */
    PERSONA_DETAILS_INSERT_STRING_FIELD_DETAILS(details,
        FOLKS_PERSONA_DETAIL_NOTES, value, QContactNote,
        FOLKS_TYPE_NOTE_FIELD_DETAILS, note);

    /*
     * OnlineAccounts
     */
    QList<QContactOnlineAccount> accounts =
        contact.details<QContactOnlineAccount>();
    if(accounts.size() > 0) {
        QMultiMap<QString, QString> providerUidMap;

        foreach(const QContactOnlineAccount& account, accounts) {
            if (!account.isEmpty()) {
                providerUidMap.insert(Utils::onlineAccountProtocolFromEnum(account.protocol()),
                                      account.accountUri());
            }
        }

        if(!providerUidMap.isEmpty()) {
            value = asvSetStrNew(providerUidMap);
            PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_IM_ADDRESSES,
                    value);
        }
    }

    /*
     * Organization
     */
    QList<QContactOrganization> orgs = contact.details<QContactOrganization>();
    if(orgs.size() > 0) {
        value = gValueSliceNew(G_TYPE_OBJECT);
        GeeHashSet *hashSet = gee_hash_set_new(FOLKS_TYPE_ROLE,
            (GBoxedCopyFunc) g_object_ref, g_object_unref,
            my_folks_role_hash, NULL, NULL,
            my_folks_role_equal, NULL, NULL);
        g_value_take_object(value, hashSet);
        foreach(const QContactOrganization& org, orgs) {
            if(!org.isEmpty()) {
                FolksRole *role = folks_role_new(org.title().toUtf8().data(),
                        org.name().toUtf8().data(), NULL);

                gee_collection_add(GEE_COLLECTION(hashSet), role);
            }
        }

        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_ROLES, value);
    }

    /*
     * Phone Numbers
     */
    QList<QContactPhoneNumber> phones = contact.details<QContactPhoneNumber>();
    if (phones.size() > 0) {
        value = gValueSliceNew(G_TYPE_OBJECT);
        foreach(QContactPhoneNumber detail, phones) {
            if(!detail.isEmpty()) {
                QStringList contexts = Utils::contextsFromEnums(detail.contexts());
                QList<int> subTypes = detail.subTypes();

                if (subTypes.size() > 0) {
                    contexts << Utils::phoneSubTypesFromEnums(subTypes);
                }

                gValueGeeSetAddStringFieldDetails(value,
                                                  FOLKS_TYPE_PHONE_FIELD_DETAILS,
                                                  detail.number().toUtf8().data(),
                                                  contexts);
            }
        }
        PERSONA_DETAILS_INSERT(details, FOLKS_PERSONA_DETAIL_PHONE_NUMBERS, value);
    }

    /*
     * URLs
     */
    PERSONA_DETAILS_INSERT_STRING_FIELD_DETAILS(details,
        FOLKS_PERSONA_DETAIL_URLS, value, QContactUrl,
        FOLKS_TYPE_URL_FIELD_DETAILS, url);

    return details;
}

void
ManagerEngine::aggregatorRemoveIndividualCb(GObject *source,
    GAsyncResult *result,
    QContactRemoveRequest *request)
{
    FolksIndividualAggregator *aggregator = FOLKS_INDIVIDUAL_AGGREGATOR(source);
    GError *error = NULL;

    QContactManager::Error opError = QContactManager::NoError;
    QMap<int, QContactManager::Error> contactErrors;

    folks_individual_aggregator_remove_individual_finish(aggregator, result,
            &error);
    if(error != NULL) {
        qWarning() << "Failed to remove an individual from contact:"
            << error->message;

        opError = managerErrorFromIndividualAggregatorError(
                (FolksIndividualAggregatorError) error->code);
        contactErrors.insert(0, opError);

        g_clear_error(&error);
    }

    updateContactRemoveRequest(request, opError, contactErrors,
            QContactAbstractRequest::FinishedState);
}

FolksPersona* ManagerEngine::getPrimaryPersona(FolksIndividual *individual)
{
    if(individual == NULL) {
        return NULL;
    }

    FolksPersona *retval = NULL;

    GeeSet *personas = folks_individual_get_personas(individual);
    GeeIterator *iter = gee_iterable_iterator(GEE_ITERABLE(personas));

    while(retval == NULL && gee_iterator_next(iter)) {
        FolksPersona *persona = FOLKS_PERSONA(gee_iterator_get(iter));
        FolksPersonaStore *primaryStore =
            folks_individual_aggregator_get_primary_store(m_aggregator);
        if(folks_persona_get_store(persona) == primaryStore) {
            retval = persona;
            g_object_ref(retval);
        }

        g_object_unref(persona);
    }
    g_object_unref (iter);

    return retval;
}

bool ManagerEngine::contactSaveChangesToFolks(const QContact& contact)
{
    ContactPair pair = m_allContacts[contact.id()];
    FolksIndividual *ind = pair.individual;

    if(ind == NULL) {
        qWarning() << "Failed to save changes to contact" << contact.id()
                   << ": no known corresponding FolksIndividual";
        return false;
    }

    FolksPersona *persona = getPrimaryPersona(ind);
    CallbackData *data = new CallbackData();
    data->contact = contact;
    data->storedContact = pair.contact;
    data->store = folks_individual_aggregator_get_primary_store(m_aggregator);

    /*
     * Addresses
     */
    if(FOLKS_IS_POSTAL_ADDRESS_DETAILS(persona) && checkDetailsChanged<QContactAddress>(data->storedContact, data->contact)) {
        FolksPostalAddressDetails *postalAddressDetails =
            FOLKS_POSTAL_ADDRESS_DETAILS(persona);

        QList<QContactAddress> addresses = contact.details<QContactAddress>();
        GeeSet *addressSet = SET_AFD_NEW();
        foreach(const QContactAddress& address, addresses) {
            if(!address.isEmpty()) {
                FolksPostalAddress *postalAddress = folks_postal_address_new(
                        address.postOfficeBox().toUtf8().data(),
                        NULL,
                        address.street().toUtf8().data(),
                        address.locality().toUtf8().data(),
                        address.region().toUtf8().data(),
                        address.postcode().toUtf8().data(),
                        address.country().toUtf8().data(),
                        NULL,
                        NULL);
                FolksPostalAddressFieldDetails *pafd =
                    folks_postal_address_field_details_new (postalAddress,
                            NULL);

                QStringList contexts = Utils::contextsFromEnums(address.contexts());
                contexts << Utils::addressSubTypesFromEnums(address.subTypes());
                setFieldDetailsFromContexts((FolksAbstractFieldDetails*)pafd, "type", contexts);
                gee_collection_add(GEE_COLLECTION(addressSet), pafd);

                g_object_unref(pafd);
                g_object_unref(postalAddress);
            }
        }

        folks_postal_address_details_change_postal_addresses(postalAddressDetails,
                addressSet, addressDetailChangeCb, data);

        g_object_unref(addressSet);
    } else {
        // if the detail is not address, just trigger the callback to make sure other
        // details are saved.
        addressDetailChangeCb(G_OBJECT(persona), NULL, data);
    }

    // the other details will be saved one by one in the callbacks
    return true;
}

bool ManagerEngine::startRequest(QContactAbstractRequest* request)
{
    if(request == NULL)
        return false;

    qDebug() << "START REQUEST" << request->type();
    updateRequestState(request, QContactAbstractRequest::ActiveState);
    switch(request->type()) {
        case QContactAbstractRequest::ContactSaveRequest:
        {
            QContactSaveRequest *save_request =
                    qobject_cast<QContactSaveRequest*>(request);

            if((save_request == 0) || (save_request->contacts().size() < 1)) {
                qWarning() << "Contact save request NULL or has zero contacts";
                break;
            }

            FolksPersonaStore *primaryStore =
                folks_individual_aggregator_get_primary_store(m_aggregator);
            if(primaryStore == NULL) {
                qWarning() << "Failed to add individual from contact: "
                        "couldn't get the only persona store";
            } else {
                qDebug () << "Save contact" << save_request->contacts();
                /* guaranteed to have >= 1 contacts above */
                foreach(const QContact& contact, save_request->contacts()) {
                    // TODO: check if it is really null or if it has no managerUri
                    if(contact.id().isNull()) {
                        AddPersonaFromDetailsClosure *closure =
                            new AddPersonaFromDetailsClosure;
                        closure->this_ = this;
                        closure->request = save_request;
                        closure->contact = contact;

                        GHashTable *details = personaDetailsHashFromQContact(
                                contact);

                        folks_individual_aggregator_add_persona_from_details(
                                m_aggregator, NULL, primaryStore, details,
                                (GAsyncReadyCallback)
                                    STATIC_C_HANDLER_NAME(
                                        aggregatorAddPersonaFromDetailsCb),
                                closure);

                        g_hash_table_destroy(details);
                    } else {
                        contactSaveChangesToFolks(contact);
                    }
                }
            }
        }
        break;

        case QContactAbstractRequest::ContactRemoveRequest:
        {
            QContactRemoveRequest *remove_request =
                    qobject_cast<QContactRemoveRequest*>(request);

            if((remove_request == 0) ||
                    (remove_request->contactIds().size() < 1)) {
                qWarning() << "Contact remove request NULL or has zero "
                    "contacts";
                break;
            }

            foreach(const QContactId& contactId,
                    remove_request->contactIds()) {
                RemoveIndividualClosure *closure = new RemoveIndividualClosure;
                closure->this_ = this;
                closure->request = remove_request;

                if(!m_allContacts.contains(contactId)) {
                    qWarning() << "Attempted to remove unknown Contact";
                    continue;
                }

                ContactPair pair = m_allContacts[contactId];
                FolksIndividual *individual = pair.individual;

                folks_individual_aggregator_remove_individual(
                        m_aggregator, individual,
                        (GAsyncReadyCallback)
                            STATIC_C_HANDLER_NAME(
                                aggregatorRemoveIndividualCb),
                        closure);
            }
        }
        break;
        case QContactAbstractRequest::ContactFetchRequest:
        {
            QContactFetchRequest* fetchRequest = qobject_cast<QContactFetchRequest*>(request);
            QContactFilter fetchFilter = fetchRequest->filter();
            QList<QContactSortOrder> fetchSorting = fetchRequest->sorting();
            QContactFetchHint fetchHint = fetchRequest->fetchHint();

            QContactManager::Error error = QContactManager::NoError;
            QList<QContact> allContacts = contacts(fetchFilter, fetchSorting, fetchHint, &error);

            if (!allContacts.isEmpty() || error != QContactManager::NoError)
                updateContactFetchRequest(fetchRequest, allContacts, error, QContactAbstractRequest::FinishedState);
            else
                updateRequestState(request, QContactAbstractRequest::FinishedState);
        }
        break;

        default:
        {
            updateRequestState(request, QContactAbstractRequest::CanceledState);
            return false;
        }
        break;
    }

    return true;
}

// Here is the factory used to allocate new manager engines.

QContactManagerEngine* ManagerEngineFactory::engine(
        const QMap<QString, QString>& parameters,
        QContactManager::Error *error)
{
    if(debugEnabled()) {
        debug() << "Creating a new folks engine";
        foreach(const QString& key, parameters)
            debug() << "    " << key << ": " << parameters[key];
    }

    return new ManagerEngine(parameters, error);
}

QString ManagerEngineFactory::managerName() const
{

    return QLatin1String(FOLKS_MANAGER_NAME);
}
/*
QContactEngineId *ManagerEngineFactory::createContactEngineId(const QMap<QString, QString> &parameters, const QString &idString) const
{
    return new EngineId(parameters, idString);
}
*/

} // namespace Folks
