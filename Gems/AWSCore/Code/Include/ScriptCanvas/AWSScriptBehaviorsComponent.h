/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/AWSScriptBehaviorDynamoDB.h>
#include <ScriptCanvas/AWSScriptBehaviorS3.h>

namespace AWSCore
{
    class AWSScriptBehaviorBase;

    //! Bootstraps and provides AWS ScriptCanvas behaviors
    class AWSScriptBehaviorsComponent
        : public AZ::Component
        , public AWSScriptBehaviorDynamoDBNotificationBus::Handler
        , public AWSScriptBehaviorS3NotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AWSScriptBehaviorsComponent, "{9F37F23F-4229-4A1F-BAA6-4A4AB8422B47}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        void OnGetItemSuccess(const DynamoDBAttributeValueMap& resultBody)
        {
            for (auto it = resultBody.begin(); it != resultBody.end(); it++)
            {
                AZ::Debug::Trace::Instance().Printf(
                    "AWSScriptBehaviorsComponent", "result key: %s, value: %s", it->first.c_str(), it->second.c_str());
            }
        }

        void OnGetItemError(const AZStd::string& errorBody)
        {
            AZ_UNUSED(errorBody);
            AZ::Debug::Trace::Instance().Printf("AWSScriptBehaviorsComponent", "OnGetItemError %s", errorBody.c_str());
        }

        void OnHeadObjectSuccess(const AZStd::string& resultBody)
        {
            AZ_UNUSED(resultBody);
            AZ::Debug::Trace::Instance().Printf("AWSScriptBehaviorsComponent", "OnHeadObjectSuccess %s", resultBody.c_str());
        }

        void OnHeadObjectError(const AZStd::string& errorBody)
        {
            AZ_UNUSED(errorBody);
            AZ::Debug::Trace::Instance().Printf("AWSScriptBehaviorsComponent", "OnHeadObjectError %s", errorBody.c_str());
        }

        void OnGetObjectSuccess(const AZStd::string& resultBody)
        {
            AZ_UNUSED(resultBody);
            AZ::Debug::Trace::Instance().Printf("AWSScriptBehaviorsComponent", "OnHeadObjectSuccess %s", resultBody.c_str());
        }

        void OnGetObjectError(const AZStd::string& errorBody)
        {
            AZ_UNUSED(errorBody);
            AZ::Debug::Trace::Instance().Printf("AWSScriptBehaviorsComponent", "OnHeadObjectError %s", errorBody.c_str());
        }
    };
} // namespace AWSCore
