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

import Qt 4.7

Item {
    id: detail
    height: 16

    Image {
        id: detailPresenceIcon
        y: 3
        width: source != "" ? 12: 0
        height: 12
        smooth: true
    }

    Text {
        id: detailLabel
        anchors.left: detailPresenceIcon.right
        anchors.leftMargin: 2
        font.bold: true
    }

    Text {
        id: detailText
        font.bold: detailAction.visible
        anchors.left: {
            if (detailLabel.text != "")
                return detailLabel.right;
            else
                return detailPresenceIcon.right;
        }
        anchors.leftMargin: detailPresenceIcon.visible ? 2 : 0
        color: detailAction.visible ? "#2020aa" : "black";
    }

    MouseArea {
        id: detailAction
        property string action: ""
        visible: action != ""
        anchors.fill: detailText
        onClicked: {
            contactModel.startAction(detailAction.action);
        }
    }

    function populate(label, text, icon, action, rows) {
        detailLabel.text = label == "" ? "" : label + ": ";
        detailText.text = text;
        detailPresenceIcon.source = icon;
        detailAction.action = action;
        detail.height = rows * 17;
    }

    function destroy() {
        // If there is no delay the item is not always deleted...
        detail.destroy(1);
    }
}
