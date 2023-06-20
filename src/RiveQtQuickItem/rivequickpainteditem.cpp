// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "rivequickitem.h"
#include "rive/file.hpp"
#include "riveqtfactory.h"
#include "riveqtrenderer.h"

#include <rive/node.hpp>
#include <rive/shapes/clipping_shape.hpp>
#include <rive/shapes/rectangle.hpp>
#include <rive/shapes/image.hpp>
#include <rive/assets/image_asset.hpp>
#include <rive/animation/linear_animation_instance.hpp>

RiveQuickItem::RiveQuickItem(QQuickItem* parent)
    : QQuickPaintedItem(parent), m_artboard(nullptr)
    , m_animationInstance(nullptr), m_elapsedTime(0.0f)
{
    m_animator.setInterval(16);
    connect(&m_animator,
            &QTimer::timeout,
            this, [this](){
                if(m_artboard) {
                    float deltaTime = m_animator.interval() / 1000.0f;
                    m_elapsedTime += deltaTime;

                    if (m_animationInstance)
                    {
                        bool shouldContinue = m_animationInstance->advance(deltaTime);
                        if (shouldContinue) {
                            m_animationInstance->apply();
                            m_artboardInstance->updateComponents();
                        } else {
                            m_animator.stop();
                        }

                        m_artboardInstance->advance(deltaTime);
                        m_artboardInstance->update(rive::ComponentDirt::Filthy);
                        this->update();
                    }
                }

            });

    QFile file(":/rive-cpp/test/assets/dependency_test.riv");

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open the file.");
        return;
    }

    QByteArray fileData = file.readAll();

    rive::Span<const uint8_t> dataSpan(reinterpret_cast<const uint8_t*>(fileData.constData()), fileData.size());


    rive::ImportResult importResult;
    riveFile = rive::File::import(dataSpan, &customFactory, &importResult);
    if (importResult == rive::ImportResult::success)
    {
        qDebug("Successfully imported Rive file.");
    }
    else
    {
        qDebug("Failed to import Rive file.");
    }

    // Get the first Artboard
    m_artboard = riveFile->artboard();
    m_artboard->updateComponents();

    emit animationsChanged();

    qDebug() << "Artboard instance valid?:" << m_artboard->instance().get();

    m_artboardInstance = m_artboard->instance();
}

void RiveQuickItem::paint(QPainter* painter)
{
    if (!m_artboardInstance || !painter)
    {
        return;
    }

    m_renderer.setPainter(painter);
    m_artboardInstance->draw(&m_renderer);
}

QList<AnimationInfo> RiveQuickItem::animations() const
{
    QList<AnimationInfo> animationList;
    if (m_artboard)
    {
        for (size_t i = 0; i < m_artboard->animationCount(); i++)
        {
            const auto animation = m_artboard->animation(i);
            if (animation)
            {
                qDebug() << "Animation" << i << "found.";

                qDebug() << "  Duration:" << animation->duration();
                qDebug() << "  FPS:" << animation->fps();

                AnimationInfo info;
                info.id = i;
                info.name = QString::fromStdString(animation->name());
                info.duration = animation->duration();
                info.fps = animation->fps();

                qDebug() << "  Name:" << info.name;
                animationList.append(info);
            }
        }
    }
    return animationList;
}

void RiveQuickItem::triggerAnimation(int id)
{
    if (m_artboard) {
        if (id >= 0 && id < m_artboard->animationCount()) {

            if (m_animationInstance) {
                delete m_animationInstance;
            }
            m_linearAnimation = m_artboard->animation(id);

            if (m_linearAnimation)
            {
                m_animationInstance = new rive::LinearAnimationInstance(m_linearAnimation, m_artboardInstance.get());
            }

            m_animator.start();
        }
    }
}
