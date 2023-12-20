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

#include <glib.h>
#include <glib-object.h>
//#include <QObject>

#ifndef GLIB_UTILS_H
#define GLIB_UTILS_H

namespace Folks {

template<typename T>
inline T convert_pointer(gpointer data)
{
    return (T) data;
}

#define SPECIALIZE_POINTER_CONVERSION(type, conversion_func) \
    template<> \
    inline type convert_pointer<type>(gpointer data) \
    { \
        return conversion_func(data); \
    }

SPECIALIZE_POINTER_CONVERSION(gint, GPOINTER_TO_INT)
SPECIALIZE_POINTER_CONVERSION(guint, GPOINTER_TO_UINT)

#define foreach_glist(type, item, list) \
    for (bool _done = false; !_done;) \
        for (type item = 0; !_done; _done = true) \
            for (GList *_l_ ## item = list; \
                _l_ ## item && \
                    (item = convert_pointer<type> (_l_ ## item->data), true); \
                _l_ ## item = _l_ ## item->next)

} // namespace Folks

template<typename T>
inline T *g_object_ref_cpp(T *t)
{
    return (T*)g_object_ref((gpointer)t);
}

template<typename T>
void g_object_unref_cpp(T *t)
{
    return g_object_unref((gpointer)t);
}

//#define g_object_ref g_object_ref_cpp
//#define g_object_unref g_object_unref_cpp

inline GValue* gValueSliceNew(GType type)
{
    GValue *retval = g_slice_new0(GValue);
    g_value_init(retval, type);

    return retval;
}

inline void gValueSliceFree(GValue *value)
{
    g_value_unset(value);
    g_slice_free(GValue, value);
}

inline void gObjectClear(GObject **obj)
{
    if (obj != NULL && *obj != NULL) {
        g_object_unref (*obj);
        *obj = NULL;
    }
}

#endif // GLIB_UTILS_H
