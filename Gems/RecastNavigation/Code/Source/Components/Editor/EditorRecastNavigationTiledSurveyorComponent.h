/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Components/RecastNavigationTiledSurveyorCommon.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationTiledSurveyorComponent
    class EditorRecastNavigationTiledSurveyorComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , public RecastNavigationTiledSurveyorCommon
        , public RecastNavigationSurveyorRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationTiledSurveyorComponent,
            "{F1E57D0B-11A1-46C2-876D-720DD40CB14D}", AzToolsFramework::Components::EditorComponentBase);
        EditorRecastNavigationTiledSurveyorComponent();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! EditorComponentBase interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

        //! RecastNavigationSurveyorRequestBus interface implementation
        //! @{
        void CollectGeometryAsync(float tileSize, float borderSize, AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback) override;
        //! Not implemented on purpose to avoid blocking the Editor.
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        int GetNumberOfTiles(float tileSize) const override;
        AZ::Aabb GetWorldBounds() const override;
        //! @}

    private:
        bool m_debugDrawInputData = false;
    };
} // namespace RecastNavigation
