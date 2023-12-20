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

import QtQuick 2

Rectangle {
    id: actionButton
    width: actionImage.width + actionText.width + 4 * 2 + 2
    height: actionImage.height + 4 * 2
    radius: 2
    border {
        width: 1
        color: "gray"
    }
    color: {
        if (mouseArea.pressed)
            return "gray";
        else
            return "white";
    }

    property string buttonText: ""
    property url buttonImage: ""

    signal clicked

    Image {
        id: actionImage
        source: buttonImage
        width: 16
        height: 16
        anchors.left: actionButton.left
        anchors.leftMargin: 4
        anchors.top: actionButton.top
        anchors.topMargin: 4
    }

    Text {
        id: actionText
        text: buttonText
        anchors.left: actionImage.right
        anchors.leftMargin: 2
        anchors.top: actionImage.top
        color: {
            if (mouseArea.pressed)
                return "white";
            else
                return "black";
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: actionButton
        onClicked: actionButton.clicked()
    }
}
