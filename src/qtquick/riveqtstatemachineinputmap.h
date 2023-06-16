
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <rive/animation/state_machine_instance.hpp>
#include <rive/core.hpp>
#include <QQmlPropertyMap>

class QQmlEngine;
class RiveQtStateMachineInputMap : public QQmlPropertyMap
{
    Q_OBJECT

public:
    RiveQtStateMachineInputMap(rive::StateMachineInstance *stateMachineInstance, QObject *parent = nullptr);

    Q_INVOKABLE void activateTrigger(const QString &trigger);

    bool hasDirtyStateMachine() const { return m_dirty; }

public slots:
    void updateValues();

private slots:
    void onInputValueChanged(const QString &key, const QVariant &value);

private:
    rive::StateMachineInstance *m_stateMachineInstance;
    bool m_dirty{false};
};
