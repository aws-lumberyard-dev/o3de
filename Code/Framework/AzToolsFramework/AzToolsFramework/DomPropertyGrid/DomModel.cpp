/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Reflection/Reflection.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomBuilder.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomDescriber.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelArrayData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelNativeData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelStringData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/Utils.h>

namespace AzToolsFramework
{
    DomModel::DomModel()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_context.m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(m_context.m_serializeContext, "Unable to retrieve serialize context.");

        m_context.m_hiddenElementDescription.m_name = "Hidden";
        DomPropertyGridInternal::AddAttribute(m_context.m_hiddenElementDescription.m_attributes, AZ::Edit::Attributes::AutoExpand, true);
        DomPropertyGridInternal::AddAttribute(
            m_context.m_hiddenElementDescription.m_attributes, AZ::Edit::Attributes::Visibility,
            AZ::Edit::PropertyVisibility::ShowChildrenOnly);
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator)
    {
        SetDom(dom, domAllocator, AZ::TypeId::CreateNull());
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, DomModelEvent eventCallback)
    {
        m_dom = &dom;
        m_context.m_domAllocator = &domAllocator;
        m_context.m_eventCallback = AZStd::move(eventCallback);

        DomModelData root;
        bool result = DomBuilder::BuildFromDom(root, dom, &m_context);
        if (result)
        {
            m_root = AZStd::move(root);
        }
        else
        {
            AZ_TracePrintf("DomModel", "Failed to (fully) load the Dom into the property grid.\n");
        }
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType)
    {
        SetDom(dom, domAllocator, targetType, [](DomModelEventType, const AZStd::string_view&) {});
    }

    void DomModel::SetDom(
        rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType, DomModelEvent eventCallback)
    {
        m_dom = &dom;
        m_context.m_domAllocator = &domAllocator;
        m_context.m_eventCallback = AZStd::move(eventCallback);

        DomModelData root;
        bool result = DomBuilder::BuildFromDom(root, dom, &m_context, targetType);
        if (result)
        {
            m_root = AZStd::move(root);
        }
        else
        {
            AZ_TracePrintf("DomModel", "Failed to (fully) load the Dom into the property grid.\n");
        }
    }

    void DomModel::Reflect(AZ::ReflectContext* context)
    {
        DomModelObjectData::Reflect(context);
        DomModelArrayData::Reflect(context);
        DomModelStringData::Reflect(context);
        DomModelNativeData::Reflect(context);
        DomModelData::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModel>()
                ->Field("Root", &DomModel::m_root);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModel>("DOM model", "Data model use to interact with a DOM.")
                    ->DataElement(0, &DomModel::m_root, "Root", "Data model at the root of the DOM.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }
} // namespace AzToolsFramework
