/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualityCVarGroup.h>
#include <AzFramework/Quality/QualitySystemBus.h>

namespace AzFramework
{
    QualityCVarGroup::QualityCVarGroup(const char* name, const char* description, ValueType value)
        : m_value(value)
        , m_functor(
              name,
              description,
              AZ::ConsoleFunctorFlags::DontReplicate,
              AZ::AzTypeInfo<ValueType>::Uuid(),
              *this,
              &QualityCVarGroup::CvarFunctor)
    {
    }

    QualityCVarGroup::~QualityCVarGroup()
    {
    }

    inline void QualityCVarGroup::CvarFunctor(const AZ::ConsoleCommandContainer& arguments)
    {
        StringToValue(arguments);
    }

    bool QualityCVarGroup::StringToValue(const AZ::ConsoleCommandContainer& arguments)
    {
        if (!arguments.empty())
        {
            AZ::CVarFixedString convertCandidate{ arguments.front() };
            char* endPtr = nullptr;

            // always set the value, even if it is the same
            // to make sure we apply all settings in the group
            m_value = aznumeric_cast<ValueType>(strtoll(convertCandidate.c_str(), &endPtr, 0));

            AzFramework::QualitySystemEvents::Bus::Broadcast(
                &AzFramework::QualitySystemEvents::LoadGroupQualityLevel, m_functor.GetName(), m_value);

            return true;
        }

        return false;
    }

    void QualityCVarGroup::ValueToString(AZ::CVarFixedString& outString) const
    {
        AZStd::to_string(outString, m_value);
    }
} // namespace AzFramework

