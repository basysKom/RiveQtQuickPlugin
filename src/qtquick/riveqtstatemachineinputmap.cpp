/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "riveqtstatemachineinputmap.h"

#include "rive/animation/state_machine_input_instance.hpp"
#include <rive/animation/state_machine_input.hpp>
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"

#include <QJSValue>
#include <QVariant>
#include <QQmlEngine>

RiveQtStateMachineInputMap::RiveQtStateMachineInputMap(rive::StateMachineInstance *stateMachineInstance, QObject *parent)
  : QQmlPropertyMap(this, parent)
  , m_stateMachineInstance(stateMachineInstance)
{
  connect(this, &QQmlPropertyMap::valueChanged, this, &RiveQtStateMachineInputMap::onInputValueChanged);

  if (!stateMachineInstance)
    return;

  QStringList triggerList;
  for (int i = 0; i < stateMachineInstance->stateMachine()->inputCount(); i++) {
    auto input = stateMachineInstance->stateMachine()->input(i);
    QString propertyName = QString::fromStdString(input->name());

    if (input->isTypeOf(rive::StateMachineBool::typeKey)) {
      insert(propertyName, QVariant(dynamic_cast<const rive::SMIBool *>(input)->value()));
    } else if (input->isTypeOf(rive::StateMachineNumber::typeKey)) {
      insert(propertyName, QVariant(static_cast<double>(dynamic_cast<const rive::SMINumber *>(input)->value())));
    } else if (input->isTypeOf(rive::StateMachineTrigger::typeKey)) {
      triggerList << propertyName;
    }
  }

  // save just a list of possible trigger names. maybe we can do some qmetaobject magic to add real functions?
  // not sure if I really want to know...
  insert(QStringLiteral("triggers"), triggerList);
}

void RiveQtStateMachineInputMap::updateValues()
{
  if (!m_stateMachineInstance)
    return;

  for (int i = 0; i < m_stateMachineInstance->stateMachine()->inputCount(); i++) {
    auto input = m_stateMachineInstance->stateMachine()->input(i);
    QString propertyName = QString::fromStdString(input->name());
    propertyName[0] = propertyName[0].toLower();

    if (input->isTypeOf(rive::StateMachineBool::typeKey)) {
      auto *smiBool = dynamic_cast<const rive::SMIBool *>(input);
      insert(propertyName, smiBool->value());
    } else if (input->isTypeOf(rive::StateMachineNumber::typeKey)) {
      auto *smiNumber = dynamic_cast<const rive::SMINumber *>(input);
      insert(propertyName, static_cast<double>(smiNumber->value()));
    }
  }
}

void RiveQtStateMachineInputMap::activateTrigger(const QString &trigger)
{
  if (!m_stateMachineInstance)
    return;
  auto *targetInput = m_stateMachineInstance->getTrigger(trigger.toStdString());
  if (!targetInput)
    return;

  targetInput->fire();
}

void RiveQtStateMachineInputMap::onInputValueChanged(const QString &key, const QVariant &value)
{
  if (!m_stateMachineInstance)
    return;

  switch (value.type()) {
  case QVariant::Type::Bool: {
    auto *targetInput = m_stateMachineInstance->getBool(key.toStdString());
    if (!targetInput)
      return;

    if (value.toBool() != targetInput->value()) {
      targetInput->value(value.toBool());
    }
  }

  case QVariant::Type::Double: {
    auto *targetInput = m_stateMachineInstance->getNumber(key.toStdString());
    if (!targetInput)
      return;

    if (value.toFloat() != targetInput->value()) {
      targetInput->value(value.toFloat());
    }
  }
  default:
    return;
  }
}
