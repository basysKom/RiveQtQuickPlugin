// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QtQml>
#include <QQuickItem>
#include <QSGRendererInterface>

struct AnimationInfo
{
    Q_GADGET

    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(float duration MEMBER duration CONSTANT)
    Q_PROPERTY(uint32_t fps MEMBER fps CONSTANT)

public:
    int id;
    QString name;
    float duration;
    uint32_t fps;
};

class ArtBoardInfo
{
    Q_GADGET

    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)

public:
    int id;
    QString name;
};

struct StateMachineInfo
{
    Q_GADGET

    Q_PROPERTY(int id MEMBER id CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)

public:
    int id;
    QString name;
};

struct RiveRenderSettings
{
    Q_GADGET

    Q_PROPERTY(RenderQuality renderQuality MEMBER renderQuality)
    Q_PROPERTY(QSGRendererInterface::GraphicsApi graphicsApi MEMBER graphicsApi)
    Q_PROPERTY(FillMode fillMode MEMBER fillMode)

public:
    enum RenderQuality
    {
        Low,
        Medium,
        High
    };
    Q_ENUM(RenderQuality)

    enum FillMode
    {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop
    };
    Q_ENUM(FillMode)

    enum PostprocessingMode
    {
        None = 0x0,
        SMAA = 0x01
    };
    Q_ENUM(PostprocessingMode)

    RenderQuality renderQuality { Medium };
    QSGRendererInterface::GraphicsApi graphicsApi { QSGRendererInterface::GraphicsApi::Software };
    FillMode fillMode { PreserveAspectFit };
};
Q_DECLARE_METATYPE(RiveRenderSettings)
