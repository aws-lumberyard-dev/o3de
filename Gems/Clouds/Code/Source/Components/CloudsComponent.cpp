/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Components/CloudsComponent.h>

namespace Clouds
{
    CloudsComponent::CloudsComponent(const CloudsComponentConfig& config)
        : BaseClass(config)
    {
    }

    void CloudsComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CloudsComponent, BaseClass>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<CloudsComponent>()->RequestBus("CloudsRequestsBus");

            behaviorContext->ConstantProperty("CloudsComponentTypeId", BehaviorConstant(AZ::Uuid(Clouds::CloudsComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
        }
    }

    void CloudsComponent::Activate()
    {
        BaseClass::Activate();
    }

    void CloudsComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

} // namespace Clouds


