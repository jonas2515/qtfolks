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

#include <QQmlContext>
#include "demowindow.h"
#include "contactmodel.h"

DemoWindow::DemoWindow(QWindow *parent)
: QQuickView(parent)
{
    QQmlContext *context = rootContext();
    m_model = new ContactModel(this);
    context->setContextProperty(QLatin1String("contactModel"), m_model);

    QString sourcePath = QLatin1String("contactlist.qml");
    setSource(QUrl::fromLocalFile(sourcePath));
    setResizeMode(QQuickView::SizeRootObjectToView);
}

DemoWindow::~DemoWindow()
{
}
