/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>

namespace AZ::DocumentPropertyEditor
{
    PropertyEditorSystem::PropertyEditorSystem()
    {
        RegisterDefaultNodes(this);

        AZ::Interface<PropertyEditorSystemInterface>::Register(this);
    }

    PropertyEditorSystem::~PropertyEditorSystem()
    {
        AZ::Interface<PropertyEditorSystemInterface>::Unregister(this);
    }

    void PropertyEditorSystem::RegisterNode(NodeMetadata metadata)
    {
        AddNameToCrcTable(metadata.m_name);
        m_nodeMetadata[metadata.m_name] = AZStd::move(metadata);
    }

    void PropertyEditorSystem::RegisterPropertyEditor(PropertyEditorMetadata metadata)
    {
        AddNameToCrcTable(metadata.m_name);
        m_propertyEditorMetadata[metadata.m_name] = AZStd::move(metadata);
    }

    void PropertyEditorSystem::RegisterAttribute(AttributeMetadata metadata)
    {
        AddNameToCrcTable(metadata.m_name);
        AZ::Name parentNodeName;
        if (metadata.m_node != nullptr)
        {
            parentNodeName = metadata.m_node->m_name;
        }
        m_attributeMetadata[metadata.m_name][parentNodeName] = AZStd::move(metadata);
    }

    const NodeMetadata* PropertyEditorSystem::FindNode(AZ::Name name) const
    {
        if (auto nodeIt = m_nodeMetadata.find(name); nodeIt != m_nodeMetadata.end())
        {
            return &nodeIt->second;
        }
        return nullptr;
    }

    const PropertyEditorMetadata* PropertyEditorSystem::FindPropertyEditor(AZ::Name name) const
    {
        if (auto propertyEditorIt = m_propertyEditorMetadata.find(name); propertyEditorIt != m_propertyEditorMetadata.end())
        {
            return &propertyEditorIt->second;
        }
        return nullptr;
    }

    const AttributeMetadata* PropertyEditorSystem::FindAttribute(AZ::Name name, const PropertyEditorMetadata* parent) const
    {
        if (auto attributeContainerIt = m_attributeMetadata.find(name); attributeContainerIt != m_attributeMetadata.end())
        {
            while (parent != nullptr)
            {
                if (auto attributeIt = attributeContainerIt->second.find(parent->m_name); attributeIt != attributeContainerIt->second.end())
                {
                    return &attributeIt->second;
                }
                parent = parent->m_parent;
            }
        }
        return nullptr;
    }

    void PropertyEditorSystem::RegisterDefaultNodes(PropertyEditorSystemInterface* system)
    {
        system->RegisterNode<Nodes::Adapter>();
        system->RegisterNode<Nodes::Row>();
        system->RegisterNode<Nodes::Label>();
        system->RegisterNode<Nodes::PropertyEditor>();

        const AZ::Name numericEditorName = AZ_NAME_LITERAL("NumericEditor");
        const PropertyEditorMetadata* propertyEditorNode = system->FindNode(GetNodeName<Nodes::PropertyEditor>());
        {
            PropertyEditorMetadata numericEditor;
            numericEditor.m_name = numericEditorName;
            numericEditor.m_parent = propertyEditorNode;
            system->RegisterNode(AZStd::move(numericEditor));
        }

        const PropertyEditorMetadata* numericEditorNode = system->FindNode(numericEditorName);
        {
            PropertyEditorMetadata slider;
            slider.m_name = GetNodeName<Nodes::IntSlider>();
            slider.m_parent = numericEditorNode;
            system->RegisterNode(AZStd::move(slider));
        }
        {
            PropertyEditorMetadata spinBox;
            spinBox.m_name = GetNodeName<Nodes::IntSpinBox>();
            spinBox.m_parent = numericEditorNode;
            system->RegisterNode(AZStd::move(spinBox));
        }

        system->RegisterPropertyEditor<Nodes::Button>();
        system->RegisterPropertyEditor<Nodes::CheckBox>();
        system->RegisterPropertyEditor<Nodes::Color>();
        system->RegisterPropertyEditor<Nodes::ComboBox>();
        system->RegisterPropertyEditor<Nodes::RadioButton>();
        system->RegisterPropertyEditor<Nodes::EntityId>();
        system->RegisterPropertyEditor<Nodes::LayoutPadding>();
        system->RegisterPropertyEditor<Nodes::LineEdit>();
        system->RegisterPropertyEditor<Nodes::MultiLineEdit>();
        system->RegisterPropertyEditor<Nodes::Quaternion>();
        system->RegisterPropertyEditor<Nodes::Crc>();
        system->RegisterPropertyEditor<Nodes::Vector2>();
        system->RegisterPropertyEditor<Nodes::Vector3>();
        system->RegisterPropertyEditor<Nodes::Vector4>();
    }

    void PropertyEditorSystem::AddNameToCrcTable(AZ::Name name)
    {
        m_crcToName[AZ::Crc32(name.GetStringView())] = AZStd::move(name);
    }

    AZ::Name PropertyEditorSystem::LookupNameFromCrc(AZ::Crc32 crc) const
    {
        auto crcIt = m_crcToName.find(crc);
        if (crcIt != m_crcToName.end())
        {
            return crcIt->second;
        }
        AZ_Assert(false, "No name found for CRC: " PRIu32, static_cast<AZ::u32>(crc));
        return {};
    }
} // namespace AZ::DocumentPropertyEditor
