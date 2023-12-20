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
    id: delegate

    width: delegate.ListView.view.width - 4
    height: contactItem.height + 8

    MouseArea {
        anchors.fill: delegate
        onClicked: {
            if (delegate.ListView.view.currentIndex == index) {
                mouse.accepted = false;
                return;
            }

            delegate.ListView.view.currentIndex = index;
            if (delegate.ListView.view.previousOpenInfo)
                delegate.ListView.view.previousOpenInfo.collapse();

            contactItem.expand();
            delegate.ListView.view.previousOpenInfo = contactItem;
            delegate.ListView.view.previousOpenInfoIndex = index;
        }
    }

    Rectangle {
        id: selection
        x: 2; y: 2
        width: delegate.width
        height: basicInfo.height + 4
        visible: false
    }

    Column {
        id: contactItem

        width: selection.width - 4
        x: selection.x + 2
        y: selection.y + 2
        spacing: 2

        Item {
            id: basicInfo

            width: delegate.width
            height: 34

            Rectangle {
                id: avatar
                width: 34
                height: 34
                radius: 2
                smooth: true
                color: {
                    if (avatarImage != "")
                        return "white";
                    else
                        return "gray"
                }

                Image {
                    source: avatarImage
                    x: 1
                    y: 1
                    width: 32
                    height: 32
                    smooth: true
                }
            }

            Column {
                id: details
                anchors.left: avatar.right
                anchors.leftMargin: 2

                Text {
                    id: contactName

                    text: displayName
                    font.bold: true
                    color: "black"
                }

                Text {
                    text: presenceMessage
                    font.pixelSize: contactName.font.pixelSize - 2
                    color: "gray"
                }
            }

            Image {
                source: presenceIcon
                x: contactItem.width - 2 - 12
                width: 12
                height: 12
                smooth: true
            }
        }

        Item {
            id: buttonsArea
            visible: false
            x: details.x

            ActionButton {
                id: chatButton
                buttonText: "Chat"
                buttonImage: "action-chat.png"
                anchors.left: buttonsArea.left
                onClicked: contactModel.startChat(index)
            }

            ActionButton {
                id: callButton
                buttonText: "Call"
                buttonImage: "action-call.png"
                anchors.left: chatButton.right
                anchors.leftMargin: 8
                onClicked: contactModel.startCall(index)
            }
        }

        property variant detailItems: null
        property variant previousButtons: null
        property int collapsedHeight: 0

        function expand() {
            var allDetails = contactModel.detailsForContact(index);
            if (allDetails.length % 4 != 0)
                return;

            collapsedHeight = height;

            var detailItems = new Array();
            var previousItem = null;
            var component = Qt.createComponent("detail.qml");

            for (var i = 0; i < allDetails.length; i += 4) {
                var label = allDetails[i];
                var text = allDetails[i+1];
                var icon = allDetails[i+2];
                var action = allDetails[i+3];

                var item = component.createObject(delegate);
                if (label == "Address")
                    item.populate(label, text, icon, action, 4);
                else
                    item.populate(label, text, icon, action, 1);

                item.x = details.x + 4;
                if (previousItem != null)
                    item.y = previousItem.y + previousItem.height + 2;
                else
                    item.y = avatar.y + avatar.height + 4;

                detailItems.push(item);

                previousItem = item;
            }

            if (previousItem != null) {
                buttonsArea.y = previousItem.y + previousItem.height;
                buttonsArea.visible = true;
                height = buttonsArea.y + chatButton.height;
            }

            contactItem.detailItems = detailItems;
            contactItem.previousButtons = buttonsArea;
        }

        function collapse() {
            for (var i = 0; i < detailItems.length; i++)
                detailItems[i].destroy();
            detailItems = null;

            contactItem.previousButtons.visible = false;
            contactItem.previousButtons = null;

            height = collapsedHeight;
        }

        function reload() {
            collapse();
            expand();
        }
    }
}
