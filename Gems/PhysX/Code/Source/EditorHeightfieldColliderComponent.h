/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorHeightfieldColliderComponent,
            "{C388C3DB-8D2E-4D26-96D3-198EDC799B77}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

//        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
//        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

//        EditorHeightfieldColliderComponent();

        // EditorComponentBase
//        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
//        void RefreshUiProperties();

        // AZ::Component
//        void Activate() override;
//        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
//        void OnSelected() override;
//        void OnDeselected() override;
    };

} // namespace PhysX
