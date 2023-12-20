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

#include <QQuickView>

#ifndef DEMO_WINDOW_H
#define DEMO_WINDOW_H

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

class ContactModel;

QT_USE_NAMESPACE

class DemoWindow : public QQuickView
{
    Q_OBJECT

public:
    DemoWindow(QWindow *parent = 0);
    ~DemoWindow();

private:
    ContactModel *m_model;
};

#endif // DEMO_WINDOW_H
