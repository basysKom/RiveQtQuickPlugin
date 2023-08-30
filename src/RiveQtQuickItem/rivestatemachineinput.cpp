// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QJSValue>
#include <QVariant>
#include <QQmlEngine>

#include <QRegularExpression>

#include <rive/animation/state_machine_input_instance.hpp>
#include <rive/animation/state_machine_input.hpp>
#include <rive/animation/state_machine_bool.hpp>
#include <rive/animation/state_machine_number.hpp>
#include <rive/animation/state_machine_trigger.hpp>

#include "rivestatemachineinput.h"
#include "rqqplogging.h"

RiveStateMachineInput::RiveStateMachineInput(QObject *parent)
    : QObject(parent)
{
}

void RiveStateMachineInput::generateStringInterface()
{
    if (!m_isCompleted)
        return;
    if (!m_stateMachineInstance)
        return;

    m_generatedRivePropertyMap.clear();
    emit riveInputsChanged();

    for (int i = 0; i < m_stateMachineInstance->inputCount(); i++) {
        auto input = m_stateMachineInstance->input(i);

        const QString &propertyName = cleanUpRiveName(QString::fromStdString(input->name()));
        if (propertyName.isEmpty())
            continue;

        if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
            qCDebug(rqqpInspection) << "Found the following properties in rive animation" << propertyName << "Type: Number";
            auto numberInput = static_cast<rive::SMINumber *>(input);
            m_inputMap.insert(propertyName, numberInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveNumber));
        }

        if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
            qCDebug(rqqpInspection) << "Found the following properties in rive animation" << propertyName << "Type: Bool";
            auto booleanInput = static_cast<rive::SMIBool *>(input);
            m_inputMap.insert(propertyName, booleanInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveBoolean));
        }

        if (input->inputCoreType() == rive::StateMachineTrigger::typeKey) {
            qCDebug(rqqpInspection) << "Found the following function in rive animation" << propertyName << "Type: Trigger";
            auto tiggerInput = static_cast<rive::SMITrigger *>(input);
            m_inputMap.insert(propertyName, tiggerInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveTrigger));
        }
    }

    emit riveInputsChanged();
}

void RiveStateMachineInput::setRiveProperty(const QString &propertyName, const QVariant &value)
{
    if (!m_isCompleted)
        return;
    if (!m_stateMachineInstance)
        return;

    if (m_generatedRivePropertyMap.contains(propertyName)) {
        const auto &type = m_generatedRivePropertyMap.value(propertyName).value<RiveStateMachineInput::RivePropertyType>();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if ((value.typeId() ==  QMetaType::Type::Int || value.typeId() ==  QMetaType::Type::Double)
            && type == RiveStateMachineInput::RivePropertyType::RiveNumber) {
            auto *input = static_cast<rive::SMINumber *>(m_inputMap.value(propertyName));
            input->value(value.toDouble());
        }
        if (value.typeId() ==  QMetaType::Type::Bool && type == RiveStateMachineInput::RivePropertyType::RiveBoolean) {
            auto *input = static_cast<rive::SMIBool *>(m_inputMap.value(propertyName));
            input->value(value.toBool());
        }
#else
        if ((value.type() == QVariant::Type::Int || value.type() == QVariant::Type::Double)
            && type == RiveStateMachineInput::RivePropertyType::RiveNumber) {
            auto *input = static_cast<rive::SMINumber *>(m_inputMap.value(propertyName));
            input->value(value.toDouble());
        }
        if (value.type() == QVariant::Type::Bool && type == RiveStateMachineInput::RivePropertyType::RiveBoolean) {
            auto *input = static_cast<rive::SMIBool *>(m_inputMap.value(propertyName));
            input->value(value.toBool());
        }
#endif
    }
}

QVariant RiveStateMachineInput::getRiveProperty(const QString &propertyName) const
{
    if (!m_isCompleted)
        return QVariant();
    if (!m_stateMachineInstance)
        return QVariant();

    if (m_generatedRivePropertyMap.contains(propertyName)) {
        if (m_inputMap.contains(propertyName)) {

            auto *input = m_inputMap.value(propertyName);
            // use type to stay compatible to qt5
            if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
                auto numberInput = static_cast<rive::SMINumber *>(input);
                if (numberInput) {
                    return QVariant::fromValue(numberInput->value());
                }
            }
            if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
                auto booleanInput = static_cast<rive::SMIBool *>(input);
                if (booleanInput) {
                    return QVariant::fromValue(booleanInput->value());
                }
            }
        }
    }
    return QVariant();
}

void RiveStateMachineInput::callTrigger(const QString &triggerName)
{
    if (m_generatedRivePropertyMap.contains(triggerName)) {
        if (m_inputMap.contains(triggerName)) {
            auto *input = m_inputMap.value(triggerName);

            if (input->inputCoreType() == rive::StateMachineTrigger::typeKey) {
                auto trigger = static_cast<rive::SMITrigger *>(input);
                trigger->fire();
            }
        }
    }
}

QObject *RiveStateMachineInput::listenTo(const QString &name)
{
    if (!m_dynamicProperties.contains(name)) {
        m_dynamicProperties[name] = new DynamicPropertyHolder(this);
        m_dynamicProperties[name]->setValue(getRiveProperty(name));
    }

    return m_dynamicProperties[name];
}

QPair<bool, QVariant> RiveStateMachineInput::updateProperty(const QString &propertyName, const QVariant &propertyValue)
{
    auto *input = m_inputMap.value(propertyName);
    if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
        auto numberInput = static_cast<rive::SMINumber *>(input);
        if (numberInput) {
            auto v = QVariant::fromValue(numberInput->value());
            if (propertyValue != v) {
                return { true, v };
            }
        }
    }
    if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
        auto booleanInput = static_cast<rive::SMIBool *>(input);
        if (booleanInput) {
            auto v = QVariant::fromValue(booleanInput->value());
            if (propertyValue != v) {
                return { true, v };
            }
        }
    }

    return { false, QVariant() };
}

void RiveStateMachineInput::updateValues()
{
    if (!m_isCompleted)
        return;
    if (!m_stateMachineInstance)
        return;

    const QMetaObject *metaObject = this->metaObject();

    QList<QByteArray> dynamicProps = this->dynamicPropertyNames();

    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        QString propertyName = QString(property.name()); // convert property name to lower case to match inputMap key
        QVariant propertyValue = this->property(propertyName.toUtf8());

        // connect change signals of custom qml properties to our handle method where we will match them to rive inputs
        if (m_inputMap.contains(propertyName) && property.hasNotifySignal()) {
            const auto &result = updateProperty(propertyName, propertyValue);
            if (result.first) {
                this->setProperty(propertyName.toLatin1(), result.second);
            }
        }
    }

    for (const auto &propertyName : m_dynamicProperties.keys()) {
        QVariant propertyValue = this->property(propertyName.toUtf8());
        // connect change signals of custom qml properties to our handle method where we will match them to rive inputs
        if (m_inputMap.contains(propertyName)) {
            const auto &result = updateProperty(propertyName, propertyValue);
            if (result.first) {
                auto *listenToProperty = m_dynamicProperties.value(propertyName, nullptr);
                if (listenToProperty) {
                    listenToProperty->setValue(result.second);
                }
            }
        }
    }
}

void RiveStateMachineInput::activateTrigger()
{
    QObject *senderObj = sender();
    if (senderObj) {
        QByteArray triggerName = senderSignalIndex() != -1 ? senderObj->metaObject()->method(senderSignalIndex()).name() : QByteArray();
        if (m_inputMap.contains(triggerName)) {
            auto *input = m_inputMap.value(triggerName);

            if (input->inputCoreType() == rive::StateMachineTrigger::typeKey) {
                auto trigger = static_cast<rive::SMITrigger *>(input);
                trigger->fire();
            }
        }
    }
}

void RiveStateMachineInput::setStateMachineInstance(rive::StateMachineInstance *stateMachineInstance)
{
    m_stateMachineInstance = stateMachineInstance;

    connectStateMachineToProperties();
    generateStringInterface();
}

void RiveStateMachineInput::classBegin() { }

void RiveStateMachineInput::componentComplete()
{
    m_isCompleted = true;
    connectStateMachineToProperties();
}

QString RiveStateMachineInput::cleanUpRiveName(const QString &name)
{
    QString cleanName = name;
    cleanName[0] = cleanName[0].toLower();
    cleanName.replace(" ", "_");

    //
    QRegularExpression regExp("^[a-z_][0-9a-zA-Z_$]*$");

    QRegularExpressionMatch match = regExp.match(cleanName);
    if (match.hasMatch() && !reservedWords.contains(cleanName)) {
        return cleanName;
    }

    qCDebug(rqqpInspection) << name << "is not a valid property name and can not be adjusted to be one please change in the animation.";
    return "";
}

void RiveStateMachineInput::handlePropertyChanged()
{
    QString changedSignalPostfix = "Changed";

    // sender should be 'this' object
    QObject *senderObj = sender();
    if (senderObj) {
        QByteArray propertyName = senderSignalIndex() != -1 ? senderObj->metaObject()->method(senderSignalIndex()).name() : QByteArray();
        if (!propertyName.isEmpty()) {
            propertyName.chop(changedSignalPostfix.size()); // remove 'Changed' postfix from signal name
            QVariant propertyValue = senderObj->property(propertyName);
            if (propertyValue.isValid() && m_inputMap.contains(propertyName)) {
                rive::SMIInput *input = m_inputMap[propertyName];
                if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
                    rive::SMINumber *numberInput = static_cast<rive::SMINumber *>(input);
                    if (propertyValue.canConvert<float>()) {
                        numberInput->value(propertyValue.toDouble());
                    }
                } else if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
                    rive::SMIBool *boolInput = static_cast<rive::SMIBool *>(input);
                    if (propertyValue.canConvert<bool>()) {
                        boolInput->value(propertyValue.toBool());
                    }
                }
            }
        }
    }
}

void RiveStateMachineInput::connectStateMachineToProperties()
{

    // reset
    m_inputMap.clear();
    m_generatedRivePropertyMap.clear();

    // return if empty state machine has been set
    if (!m_isCompleted)
        return;
    if (!m_stateMachineInstance)
        return;

    for (int i = 0; i < m_stateMachineInstance->inputCount(); i++) {
        auto input = m_stateMachineInstance->input(i);

        const QString &propertyName = cleanUpRiveName(QString::fromStdString(input->name()));
        if (propertyName.isEmpty())
            continue;

        if (input->inputCoreType() == rive::StateMachineNumber::typeKey) {
            qCDebug(rqqpInspection) << "Found the following properties in rive animation" << propertyName << "Type: Number";
            auto numberInput = static_cast<rive::SMINumber *>(input);
            m_inputMap.insert(propertyName, numberInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveNumber));
        }

        if (input->inputCoreType() == rive::StateMachineBool::typeKey) {
            qCDebug(rqqpInspection) << "Found the following properties in rive animation" << propertyName << "Type: Bool";
            auto booleanInput = static_cast<rive::SMIBool *>(input);
            m_inputMap.insert(propertyName, booleanInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveBoolean));
        }

        if (input->inputCoreType() == rive::StateMachineTrigger::typeKey) {
            qCDebug(rqqpInspection) << "Found the following function in rive animation" << propertyName << "Type: Trigger";
            auto triggerInput = static_cast<rive::SMITrigger *>(input);
            m_inputMap.insert(propertyName, triggerInput);
            m_generatedRivePropertyMap.insert(propertyName, QVariant::fromValue(RiveStateMachineInput::RivePropertyType::RiveTrigger));
        }
    }

    emit riveInputsChanged();

    const QMetaObject *metaObject = this->metaObject();

    QMetaMethod handlePropertyChangedMethod;
    int handlePropertyChangedMethodIndex = metaObject->indexOfSlot(QMetaObject::normalizedSignature("handlePropertyChanged()"));
    if (handlePropertyChangedMethodIndex != -1) { // -1 means the slot was not found
        handlePropertyChangedMethod = metaObject->method(handlePropertyChangedMethodIndex);
    }

    // properties
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        QString propertyName = QString(property.name()).toLower(); // convert property name to lower case to match inputMap key
        QVariant propertyValue = this->property(propertyName.toUtf8());

        // connect change signals of custom qml properties to our handle method where we will match them to rive inputs
        if (m_inputMap.contains(propertyName) && property.hasNotifySignal()) {
            qCDebug(rqqpInspection) << "map properties" << propertyName << "connected by" << property.notifySignal().methodSignature();
            connect(this, property.notifySignal(), this, handlePropertyChangedMethod);
        } else {
            qCDebug(rqqpInspection) << "property" << propertyName << "was not found in rive animation. Not connected";
        }
    }

    QMetaMethod triggerActivatedMethod;
    int triggerActivatedMethodIndex = this->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("activateTrigger()"));
    if (triggerActivatedMethodIndex != -1) { // -1 means the slot was not found
        triggerActivatedMethod = this->metaObject()->method(triggerActivatedMethodIndex);
    }

    // trigger
    for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            if (m_inputMap.contains(method.name().toLower())) {
                QObject::connect(this, method, this, triggerActivatedMethod);
            }
        }
    }
}

QVariantList RiveStateMachineInput::riveInputs() const
{
    QVariantList outputList;

    for (auto it = m_generatedRivePropertyMap.constBegin(); it != m_generatedRivePropertyMap.constEnd(); ++it) {
        QVariantMap newEntry;
        newEntry["text"] = it.key();
        newEntry["type"] = it.value();
        outputList.append(newEntry);
    }
    return outputList;
}

void RiveStateMachineInput::initializeInternal()
{
    m_isCompleted = true;
}
