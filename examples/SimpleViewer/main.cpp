
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

int main(int argc, char *argv[])
{

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Force OpenGL
    // QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    const auto &pathList = engine.importPathList();
    for (const QString &path : pathList) {
        qDebug() << "  " << path;
    }

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
