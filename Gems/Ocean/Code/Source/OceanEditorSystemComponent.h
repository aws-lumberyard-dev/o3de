// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 // {END_LICENSE}

#pragma once

#include <OceanSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace Ocean
{
    /// System component for Ocean editor
    class OceanEditorSystemComponent
        : public OceanSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = OceanSystemComponent;
    public:
        AZ_COMPONENT(OceanEditorSystemComponent, "{0e61e3de-333c-43bb-bd4f-f7a11d8008ff}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        OceanEditorSystemComponent();
        ~OceanEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace Ocean
