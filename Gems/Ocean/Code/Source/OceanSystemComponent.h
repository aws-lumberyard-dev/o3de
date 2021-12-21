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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>

#include <Ocean/OceanBus.h>

namespace Ocean
{
    class OceanSystemComponent
        : public AZ::Component
        , protected OceanRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(OceanSystemComponent, "{6958061e-301e-427a-8279-bb8c6d153522}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        OceanSystemComponent();
        ~OceanSystemComponent();

        AZ::Data::Instance<AZ::RPI::StreamingImage> GetGaussianNoiseImage() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // OceanRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        
        //! Used for loading the pass templates of the ocean gem.
        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateGaussian256ImageAsset();

        AZ::Data::Instance<AZ::RPI::StreamingImage> m_gaussianNoise256Image;
    };

} // namespace Ocean
