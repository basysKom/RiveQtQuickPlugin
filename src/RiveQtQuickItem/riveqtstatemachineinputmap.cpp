// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <QJSValue>
#include <QVariant>
#include <QQmlEngine>

#include <rive/animation/state_machine_input_instance.hpp>
#include <rive/animation/state_machine_input.hpp>
#include <rive/animation/state_machine_bool.hpp>
#include <rive/animation/state_machine_number.hpp>
#include <rive/animation/state_machine_trigger.hpp>

#include "riveqtstatemachineinputmap.h"

RiveQtStateMachineInputMap::RiveQtStateMachineInputMap(std::weak_ptr<rive::StateMachineInstance> stateMachineInstance, QObject *parent)
    : QQmlPropertyMap(this, parent)
    , m_stateMachineInstance(stateMachineInstance)
{
    connect(this, &QQmlPropertyMap::valueChanged, this, &RiveQtStateMachineInputMap::onInputValueChanged);

    if (m_stateMachineInstance.expired())
        return;

    const auto stateMachineInstanceShared = m_stateMachineInstance.lock();

    QStringList triggerList;

    for (int i = 0; i < stateMachineInstanceShared->inputCount(); i++) {
        auto input = stateMachineInstanceShared->input(i);

        const QString &propertyName = QString::fromStdString(input->name());

        if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
            auto numberInput = static_cast<const rive::SMINumber *>(input);
            if (numberInput) {
                auto v = numberInput->value();
                insert(propertyName, QVariant(static_cast<float>(v)));
            }
        }

        if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
            auto numberInput = static_cast<const rive::SMIBool *>(input);
            if (numberInput) {
                auto v = numberInput->value();
                insert(propertyName, QVariant(static_cast<bool>(v)));
            }
        }

        if (input->inputCoreType() == rive::StateMachineTrigger::typeKey) {
            triggerList << propertyName;
        }
    }

    // save just a list of possible trigger names. maybe we can do some qmetaobject magic to add real functions?
    // not sure if I really want to know...
    insert(QStringLiteral("triggers"), triggerList);
}

void RiveQtStateMachineInputMap::updateValues()
{
    if (m_stateMachineInstance.expired())
        return;

    const auto stateMachineInstanceShared = m_stateMachineInstance.lock();

    for (int i = 0; i < stateMachineInstanceShared->inputCount(); i++) {
        auto input = stateMachineInstanceShared->input(i);
        QString propertyName = QString::fromStdString(input->name());

        if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
            auto numberInput = static_cast<const rive::SMINumber *>(input);
            if (numberInput) {
                auto v = numberInput->value();
                insert(propertyName, QVariant(static_cast<float>(v)));
            }
        }

        if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
            auto numberInput = static_cast<const rive::SMIBool *>(input);
            if (numberInput) {
                auto v = numberInput->value();
                insert(propertyName, QVariant(static_cast<bool>(v)));
            }
        }
    }
}

void RiveQtStateMachineInputMap::activateTrigger(const QString &trigger)
{
    if (m_stateMachineInstance.expired())
        return;

    const auto stateMachineInstanceShared = m_stateMachineInstance.lock();

    auto *targetInput = stateMachineInstanceShared->getTrigger(trigger.toStdString());

    if (!targetInput)
        return;

    targetInput->fire();
}

void RiveQtStateMachineInputMap::onInputValueChanged(const QString &key, const QVariant &value)
{
    if (m_stateMachineInstance.expired())
        return;

    const auto stateMachineInstanceShared = m_stateMachineInstance.lock();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    switch (value.typeId()) {
    case QMetaType::Type::Bool: {
#else
    switch (value.type()) {
    case QVariant::Type::Bool: {
#endif
        auto *targetInput = stateMachineInstanceShared->getBool(key.toStdString());
        if (!targetInput)
            return;

        if (value.toBool() != targetInput->value()) {
            targetInput->value(value.toBool());
            stateMachineInstanceShared->needsAdvance();
            insert(key, value);
        }
        break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QMetaType::Type::Int:
    case QMetaType::Type::Double: {
#else
    case QVariant::Type::Int:
    case QVariant::Type::Double: {
#endif
        auto *targetInput = stateMachineInstanceShared->getNumber(key.toStdString());
        if (!targetInput) {
            return;
        }

        if (value.toFloat() != targetInput->value()) {
            targetInput->value(value.toFloat());
            insert(key, value);
        }
        break;
    }
    default:
        return;
    }
}
