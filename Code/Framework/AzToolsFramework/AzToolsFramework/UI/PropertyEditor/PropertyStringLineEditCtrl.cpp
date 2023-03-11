/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyStringLineEditCtrl.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/StyledLineEdit.h>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QtWidgets/QHBoxLayout>
#include <QFocusEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyStringLineEditCtrl::PropertyStringLineEditCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        m_pLineEdit = new AzQtComponents::StyledLineEdit(this);
        m_pLineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pLineEdit->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pLineEdit->setFixedHeight(PropertyQTConstant_DefaultHeight);
        m_pLineEdit->setFocusPolicy(Qt::StrongFocus);

        setLayout(new QHBoxLayout(this));
        layout()->setSpacing(4);
        layout()->setContentsMargins(1, 0, 1, 0);
        layout()->addWidget(m_pLineEdit);

        setFocusProxy(m_pLineEdit);
        setFocusPolicy(m_pLineEdit->focusPolicy());

        ConnectWidgets();
    };

    void PropertyStringLineEditCtrl::setValue(const AZStd::string& value)
    {
        QSignalBlocker blocker(m_pLineEdit);
        if (m_pLineEdit->text().compare(QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size()))) != 0)
        {
            m_pLineEdit->setText(value.c_str());
        }
    }

    void PropertyStringLineEditCtrl::focusInEvent(QFocusEvent* e)
    {
        m_pLineEdit->event(e);
        m_pLineEdit->selectAll();
    }

    AZStd::string PropertyStringLineEditCtrl::value() const
    {
        return m_pLineEdit->text().toUtf8().constData();
    }

    QLineEdit* PropertyStringLineEditCtrl::GetLineEdit() const
    {
        return m_pLineEdit;
    }

    void PropertyStringLineEditCtrl::setMaxLen(int maxLen)
    {
        QSignalBlocker blocker(m_pLineEdit);
        m_pLineEdit->setMaxLength(maxLen);
    }

    PropertyStringLineEditCtrl::~PropertyStringLineEditCtrl()
    {
    }

    QWidget* PropertyStringLineEditCtrl::GetFirstInTabOrder()
    {
        return m_pLineEdit;
    }

    QWidget* PropertyStringLineEditCtrl::GetLastInTabOrder()
    {
        return m_pLineEdit;
    }

    void PropertyStringLineEditCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    void PropertyStringLineEditCtrl::ConnectWidgets()
    {
        connect(m_pLineEdit, &QLineEdit::editingFinished, this, [this]() {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, this);
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
        });
    }

    QWidget* StringPropertyLineEditHandler::CreateGUI(QWidget* pParent)
    {
        return aznew PropertyStringLineEditCtrl(pParent);
    }

    void StringPropertyLineEditHandler::ConsumeAttribute(
        PropertyStringLineEditCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(debugName);

        QSignalBlocker blocker(GUI);
        if (attrib == AZ::Edit::Attributes::MaxLength)
        {
            AZ::s64 value;
            if (attrValue->Read<int>(value))
            {
                GUI->setMaxLen(static_cast<int>(value));
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'MaxLength' attribute from property '%s' into text field", debugName);
            }
        }
    }

    void StringPropertyLineEditHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyStringLineEditCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        instance = static_cast<property_t>(GUI->value());
    }

    bool StringPropertyLineEditHandler::ReadValuesIntoGUI(
        size_t index, PropertyStringLineEditCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->setValue(instance);
        return false;
    }

    void RegisterStringLineEditHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew StringPropertyLineEditHandler());
    }

} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyStringLineEditCtrl.cpp"
