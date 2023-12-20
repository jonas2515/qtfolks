/*
 * Copyright (C) 2010 Collabora Ltd.
 *   @author Marco Barisione <marco.barisione@collabora.co.uk>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
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

import QtQuick 2

Item {
    id: window
    width: 500
    height: 500

    ListView {
        id: contactList

        x: 2; y: 2
        width: window.width - 4
        height: window.height - 4

        model: contactModel
        delegate: ContactDelegate {}

        Rectangle {
            width: contactList.width - 1
            border {
                width: 1
                color: "gray"
            }
            color: "lightgray"
            radius: 2
        }
        focus: true

        property variant previousOpenInfo: null
        property int previousOpenInfoIndex: -1

        function reload(row) {
            if (row == previousOpenInfoIndex)
                previousOpenInfo.reload();
        }

        Connections {
            target: contactModel
            onRowChanged: contactList.reload(row)
        }
    }
}
