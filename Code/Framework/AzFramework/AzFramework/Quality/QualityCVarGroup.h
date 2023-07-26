/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Console/ConsoleFunctor.h>

namespace AzFramework
{
    // QualityCVarGroup wraps an integer CVar for a quality group that, when set,
    // notifies the QualitySystemBus to load all the settings for the group and the
    // requested level
    //
    // Example:
    // A q_shadows group CVar could be created with 4 levels (low=0,medium=1,high=2,veryhigh=3)
    // When the level of q_shadows is set to 2 the QualitySystem will be notified
    // and apply all console settings defined in the SettingsRegistry for that
    // group at level 2 (high).
    //
    class QualityCVarGroup
    {
    public:
        typedef int16_t ValueType;
        static constexpr ValueType UseDeviceAttributeRules = -1;

        QualityCVarGroup(const char* name, const char* description, ValueType value);
        ~QualityCVarGroup();

        inline operator ValueType() const { return m_value; }

        //! required to set the value
        inline void CvarFunctor(const AZ::ConsoleCommandContainer& arguments);

        //! required to set the value
        bool StringToValue(const AZ::ConsoleCommandContainer& arguments);

        //! required to get the current value
        void ValueToString(AZ::CVarFixedString& outString) const;

    private:
        AZ_DISABLE_COPY(QualityCVarGroup);

        AZ::ConsoleFunctor<QualityCVarGroup, /*replicate=*/true> m_functor;
        ValueType m_value = UseDeviceAttributeRules;
    };
} // namespace AzFramework
