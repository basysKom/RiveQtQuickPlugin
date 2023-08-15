// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "qqmlcontext.h"
#include <QQuickItem>

#include <rive/core.hpp>
#include <rive/animation/state_machine_instance.hpp>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include <QtQml/qqmlregistration.h>
#endif

/**
 * \class RiveStateMachineInput
 * \brief Interface for binding QML properties to Rive's state machine.
 *
 * The `RiveStateMachineInput` bridges QML and Rive's state machine. This utility
 * enables dynamic interactions with the animation state machine using QML.
 *
 * \section usage Usage
 *
 * \subsection direct Direct Property Bindings
 *
 * Create an instance of `RiveStateMachineInput` and set its properties. These properties
 * will automatically be matched to their corresponding names in the Rive animation
 * state machine, considering exclusions from the reserved word list.
 *
 * \note The property names in QML should be the same as those in the Rive animation,
 * excluding the names listed in `reservedWords`.
 *
 * \code{.qml}
 * RiveQtQuickItem {
 *     stateMachineInterface: RiveStateMachineInput {
 *         signal triggerName()    // Corresponds to Rive's trigger
 *         property real level: slider.value  // Matches Rive's input
 *         property bool onOff: button.checked // Matches Rive's input
 *         property bool status  // Matches Rive's input
 *     }
 * }
 * \endcode
 *
 * To invoke the trigger in the above example from QML, you can call `triggerName` signal:
 * \code{.qml}
 * riveItem.stateMachineInterface.triggerName();
 * \endcode
 *
 * \subsection string_interface String Interface
 *
 * This approach uses string names to access the properties and triggers in Rive's state machine,
 * providing flexibility and dynamism.
 *
 * \code{.qml}
 * Repeater {
 *     model: riveItem.stateMachineInterface.riveInputs
 *     delegate: Button {
 *         id: buttonDelegate
 *         text: {
 *             if (modelData.type === RiveStateMachineInput.RiveTrigger) {
 *                 return modelData.text
 *             }
 *             if (modelData.type === RiveStateMachineInput.RiveBoolean
 *                     || modelData.type === RiveStateMachineInput.RiveNumber) {
 *                 return modelData.text + ": " + riveItem.stateMachineInterface.listenTo(modelData.text).value
 *             }
 *         }
 *
 *         onClicked: {
 *             if (modelData.type === RiveStateMachineInput.RiveTrigger) {
 *                 riveItem.stateMachineInterface.callTrigger(modelData.text)
 *             }
 *             if (modelData.type === RiveStateMachineInput.RiveBoolean) {
 *                 riveItem.stateMachineInterface.setRiveProperty(modelData.text,
 *                     !riveItem.stateMachineInterface.listenTo(modelData.text).value)
 *             }
 *         }
 *
 *         Slider {
 *             id: slider
 *             visible: modelData.type === RiveStateMachineInput.RiveNumber
 *             anchors.fill: parent
 *             from: 1
 *             value: riveItem.stateMachineInterface.listenTo(modelData.text).value
 *             to: 100
 *             stepSize: 1
 *             onValueChanged: riveItem.stateMachineInterface.setRiveProperty(modelData.text, value)
 *         }
 *     }
 * }
 *
 * RiveQtQuickItem {
 *     id: riveItem
 *     currentArtboardIndex: 0
 *     currentStateMachineIndex: 0
 * }
 * \endcode
 *
 * Here, `riveItem.stateMachineInterface.riveInputs` provides an array of JS objects,
 * each having the format `{ text: "propertyname", type: RivePropertyType }`. The possible
 * `RivePropertyType` values are:
 * - RiveNumber
 * - RiveBoolean
 * - RiveTrigger
 *
 * \section reserved_words Reserved Words
 *
 * The following words are reserved and cannot be used as property names in QML for matching
 * with the Rive animation state machine:
 * - await, break, case, catch, class, const, continue, debugger, default, delete, do, else,
 *   export, extends, finally, for, function, if, import, in, instanceof, new, return, super,
 *   switch, this, throw, try, typeof, var, void, while, with, yield, enum, implements,
 *   interface, let, package, private, protected, public, static, riveQtArtboardName, riveInputs.
 */

class DynamicPropertyHolder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)

private:
    QVariant m_value;

    QJSValue m_toString;

public:
    explicit DynamicPropertyHolder(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    QVariant value() const { return m_value; }

    void setValue(const QVariant &value)
    {
        if (m_value != value) {
            m_value = value;
            emit valueChanged();
        }
    }

signals:
    void valueChanged();
};

class QQmlEngine;
class RiveStateMachineInput : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    QML_NAMED_ELEMENT(RiveStateMachineInput)

    Q_PROPERTY(QVariantList riveInputs READ riveInputs NOTIFY riveInputsChanged)
public:
    enum class RivePropertyType : short
    {
        RiveNumber,
        RiveBoolean,
        RiveTrigger
    };
    Q_ENUM(RivePropertyType)

    RiveStateMachineInput(QObject *parent = nullptr);

    void generateStringInterface();
    bool hasDirtyStateMachine() const { return m_dirty; }

    Q_INVOKABLE void setRiveProperty(const QString &propertyName, const QVariant &value);
    Q_INVOKABLE QVariant getRiveProperty(const QString &propertyName) const;
    Q_INVOKABLE void callTrigger(const QString &triggerName);
    Q_INVOKABLE QObject *listenTo(const QString &name);

    void setStateMachineInstance(rive::StateMachineInstance *stateMachineInstance);
    QVariantList riveInputs() const;

    const QString &riveQtArtboardName() const;
    void setRiveQtArtboardName(const QString &newRiveQtArtboardName);

    void initializeInternal();
public slots:
    void updateValues();

signals:
    void riveInputsChanged();

private slots:
    void activateTrigger();
    void handlePropertyChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    QString cleanUpRiveName(const QString &name);

    // we ignore properties with those names as they will
    // conflict with JS/QML environment
    const QStringList reservedWords = { "await",      "break",   "case",     "catch",
                                        "class",      "const",   "continue", "debugger",
                                        "default",    "delete",  "do",       "else",
                                        "export",     "extends", "finally",  "for",
                                        "function",   "if",      "import",   "in",
                                        "instanceof", "new",     "return",   "super",
                                        "switch",     "this",    "throw",    "try",
                                        "typeof",     "var",     "void",     "while",
                                        "with",       "yield",   "enum",     "implements",
                                        "interface",  "let",     "package",  "private",
                                        "protected",  "public",  "static",   "riveQtArtboardName",
                                        "riveInputs" };
    void connectStateMachineToProperties();
    rive::StateMachineInstance *m_stateMachineInstance { nullptr };
    QMap<QString, rive::SMIInput *> m_inputMap;

    bool m_dirty { false };
    bool m_isCompleted { false };
    bool m_hasGeneratedStringInterface { false };

    QVariantMap m_generatedRivePropertyMap;
    QMap<QString, DynamicPropertyHolder *> m_dynamicProperties;

    QPair<bool, QVariant> updateProperty(const QString &propertyName, const QVariant &propertyValue);
};

Q_DECLARE_METATYPE(RiveStateMachineInput::RivePropertyType)
