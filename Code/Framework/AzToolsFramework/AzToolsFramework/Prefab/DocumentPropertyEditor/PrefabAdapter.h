/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapterInterface.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentEditPatcher.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>

namespace AzToolsFramework::Prefab
{
    class PrefabOverridePublicInterface;

    class PrefabAdapter
        : public AZ::DocumentPropertyEditor::ReflectionAdapter
        , private PrefabAdapterInterface
    {
    public:
        PrefabAdapter();
        ~PrefabAdapter();
    private:
        void AddPropertyHandlerIfOverridden(
            AZ::DocumentPropertyEditor::AdapterBuilder* adapterBuilder, const AZ::Dom::Path& componentPathInEntity, AZ::EntityId entityId) override;

        void GeneratePropertyEditPatch(
            const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo&,
            AZ::EntityId entityId,
            AZ::Dom::Path pathToComponentProperty) override;

        PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
        PrefabComponentEditPatcher m_prefabComponentEditPatcher;
    };

} // namespace AzToolsFramework::Prefab
