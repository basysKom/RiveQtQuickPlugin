// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>

int main(int argc, char **argv)
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine("qrc:/main.qml");
    return app.exec();
}