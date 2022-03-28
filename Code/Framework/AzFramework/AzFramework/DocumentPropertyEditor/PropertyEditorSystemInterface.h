/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AZ::DocumentPropertyEditor
{
    class PropertyEditorSystemInterface
    {
    public:
        virtual ~PropertyEditorSystemInterface() = default;

        virtual void RegisterNode(NodeMetadata metadata) = 0;
        virtual void RegisterPropertyEditor(PropertyEditorMetadata metadata) = 0;
        virtual void RegisterAttribute(AttributeMetadata metadata) = 0;

        virtual const NodeMetadata* FindNode(AZ::Name name) const = 0;
        virtual const PropertyEditorMetadata* FindPropertyEditor(AZ::Name name) const = 0;
        virtual const AttributeMetadata* FindAttribute(AZ::Name name) const = 0;
        virtual AZ::Name LookupNameFromCrc(AZ::Crc32 crc) const = 0;

        template<typename NodeDefinition>
        void RegisterNode()
        {
            RegisterNode(NodeMetadata<NodeDefinition>());
        }

        template<typename NodeDefinition, typename ParentNodeDefinition>
        void RegisterNode()
        {
            const AZ::Name parentName = GetNodeName<ParentNodeDefinition>();
            NodeMetadata* parent = FindNode(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No node definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterNode(NodeMetadata<NodeDefinition>(parent));
        }

        template<typename PropertyEditor>
        void RegisterPropertyEditor()
        {
            RegisterPropertyEditor(PropertyEditorMetadata<PropertyEditor>());
        }

        template<typename PropertyEditor, typename ParentPropertyEditor>
        void RegisterPropertyEditor()
        {
            const AZ::Name parentName = GetNodeName<ParentPropertyEditor>();
            PropertyEditorMetadata* parent = FindPropertyEditor(parentName);
            AZ_Assert(parent != nullptr, "DPE RegisterNode: No property editor definiton found for parent \"%s\"", parentName.GetCStr());
            RegisterPropertyEditor(PropertyEditorMetadata::FromType<PropertyEditor>(parent));
        }

        template<typename AttributeDefinition>
        void RegisterAttributeDefinition(const AttributeDefinition& definition)
        {
            RegisterAttribute(AttributeMetadata(definition));
        }
    };
} // namespace AZ::DocumentPropertyEditor
