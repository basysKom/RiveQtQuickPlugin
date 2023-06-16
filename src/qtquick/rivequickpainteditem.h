// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "rive/artboard.hpp"
#include "riveqtfactory.h"
#include "riveqtrenderer.h"
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QTimer>

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

class RiveQuickItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QList<AnimationInfo> animations READ animations NOTIFY animationsChanged)

public:
    RiveQuickItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QList<AnimationInfo> animations() const;

    Q_INVOKABLE void triggerAnimation(int id);

    rive::Artboard *artboard() { return m_artboard; };
    std::unique_ptr<rive::ArtboardInstance> &artboardInstance() { return m_artboardInstance; };

signals:
    void animationsChanged();

private:
    rive::Artboard *m_artboard;
    std::unique_ptr<rive::ArtboardInstance> m_artboardInstance;
    rive::LinearAnimationInstance *m_animationInstance;

    RiveQtRenderer m_renderer;
    RiveQtFactory customFactory;
    std::unique_ptr<rive::File> riveFile;

    QTimer m_animator;

    const rive::LinearAnimation *m_linearAnimation;
    float m_elapsedTime;
};
