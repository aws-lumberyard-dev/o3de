/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualitySystemComponent.h>
#include <AzFramework/Quality/QualityCVarGroup.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>


namespace AzFramework
{
    inline constexpr const char* QualitySettingsGroupsRootKey = "/O3DE/Quality/Groups";
    inline constexpr const char* QualitySettingsDefaultGroupKey = "/O3DE/Quality/DefaultGroup";
    inline constexpr const char* QualitySettingsDefaultLevelKey = "Default";
    inline constexpr const char* QualitySettingsDescriptionKey = "Description";
    inline constexpr const char* QualitySettingsCVarSettingsKey = "Settings";
    inline constexpr const char* QualitySettingsGroupLevelsKey = "Levels";

    // constructor and destructor defined here to prevent compiler errors
    // if default constructor/destructor is defined in header because of
    // the vector of unique_ptrs
    QualitySystemComponent::QualitySystemComponent() = default;
    QualitySystemComponent::~QualitySystemComponent() = default;

    void QualitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QualitySystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<QualitySystemComponent>(
                    "AzFramework Quality Component", "System component responsible for quality settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    void QualitySystemComponent::Activate()
    {
        m_settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(m_settingsRegistry, "QualitySystemComponent requires a SettingsRegistry but no instance has been created.");

        m_console = AZ::Interface<AZ::IConsole>::Get();
        AZ_Assert(m_console, "QualitySystemComponent requires an IConsole interface but no instance has been created.");

        if (m_settingsRegistry && m_console)
        {
            QualitySystemEvents::Bus::Handler::BusConnect();
            m_settingsRegistry->Get(m_defaultGroupName, QualitySettingsDefaultGroupKey);
            RegisterCvars();
        }
    }

    void QualitySystemComponent::Deactivate()
    {
        QualitySystemEvents::Bus::Handler::BusDisconnect();
        m_settingsGroupCVars.clear();
        m_qualityGroupStack.clear();
    }

    void PerformConsoleCommand(AZ::IConsole& console, const AZ::SettingsRegistryInterface& registry, AZStd::string_view command, AZStd::string_view key)
    {
        AZStd::string value;
        switch (registry.GetType(key))
        {
        case AZ::SettingsRegistryInterface::Type::FloatingPoint:
            {
                double floatingPointValue;
                if (registry.Get(floatingPointValue, key))
                {
                    value = AZStd::to_string(floatingPointValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Boolean:
            {
                bool boolValue;
                if (registry.Get(boolValue, key))
                {
                    value = boolValue ? "True" : "False";
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Integer:
            {
                AZ::s64 intValue;
                if (registry.Get(intValue, key))
                {
                    value = AZStd::to_string(intValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::String:
        default:
            {
                registry.Get(value, key);
            }
            break;
        }

        if (!value.empty())
        {
            console.PerformCommand(command, { value }, AZ::ConsoleSilentMode::Silent);
        }
        else
        {
            AZ_Warning("QualitySystemComponent", false, "Failed to convert the value for %.*s to a string, or the value is empty.", AZ_STRING_ARG(command));
        }
    }

    void QualitySystemComponent::LoadGroupQualityLevel(AZStd::string_view group, int level)
    {
        AZStd::string_view qualityGroup = group.empty() ? AZStd::string_view(m_defaultGroupName) : group;
        if (qualityGroup.empty())
        {
            AZ_Warning("QualitySystemComponent", false, "No quality group settings will be loaded because no default group name was defined at %s", QualitySettingsDefaultGroupKey);
            return;
        }

        // check for cycles in case someone puts an ancestor group inside this group's settings
        if (AZStd::find(m_qualityGroupStack.cbegin(), m_qualityGroupStack.cend(), qualityGroup) != m_qualityGroupStack.cend())
        {
            AZ_Warning("QualitySystemComponent", false, "Cycle detected when applying group level for %.*s", AZ_STRING_ARG(qualityGroup));
            return;
        }
        m_qualityGroupStack.push_back(qualityGroup);

        int qualityLevel = level < 0 ? GetGroupQualityLevel(qualityGroup) : level;
        if (qualityLevel < 0)
        {
            AZ_Warning("QualitySystemComponent", false, "Failed to automatically determine the quality level to use for %.*s, defaulting to level 0.", AZ_STRING_ARG(qualityGroup));
            qualityLevel = 0;
        }

        // TODO verify the level range is valid for this group

        auto key = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s/%s",
            QualitySettingsGroupsRootKey, AZ_STRING_ARG(qualityGroup), QualitySettingsCVarSettingsKey);

        auto settingsVisitorCallback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type == AZ::SettingsRegistryInterface::Type::Object ||
                visitArgs.m_type == AZ::SettingsRegistryInterface::Type::Null ||
                visitArgs.m_type == AZ::SettingsRegistryInterface::Type::NoType)
            {
                AZ_Warning("QualitySystemComponent", false, "Invalid setting value type for %.*s, objects and null are not supported, only arrays, bool, string, or numbers.", AZ_STRING_ARG(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            AZStd::string_view command = visitArgs.m_fieldName;
            AZStd::string_view valueKey = visitArgs.m_jsonKeyPath;
            if (visitArgs.m_type == AZ::SettingsRegistryInterface::Type::Array)
            {
                valueKey = AZ::SettingsRegistryInterface::FixedValueString::format("%.*s/%d", AZ_STRING_ARG(valueKey), qualityLevel);
                // TODO if the array index doesn't exist, use the value for the highest index
            }

            PerformConsoleCommand(*m_console, *m_settingsRegistry, command, valueKey);

            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitObject(*m_settingsRegistry, settingsVisitorCallback, key);

        m_qualityGroupStack.pop_back();
    }

    AZStd::string GetGroupDescription(const AZ::SettingsRegistryInterface& registry, AZStd::string_view group)
    {
        AZStd::string description;
        auto key = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s/%s",
            QualitySettingsGroupsRootKey, AZ_STRING_ARG(group), QualitySettingsDescriptionKey);
        if (!registry.Get(description, key))
        {
            // no custom description was found
            return AZStd::string::format("%.*s quality settings group", AZ_STRING_ARG(group));
        }

        return description;
    }


    AZ::s64 GetLevelFromName(AZStd::string_view levelName, AZ::SettingsRegistryInterface& registry, AZStd::string_view group)
    {
        AZ::s64 levelIndex = aznumeric_cast<AZ::s64>(QualityCVarGroup::UseDeviceAttributeRules);
        AZ::s64 currentIndex = 0;
        AZStd::string level;

        // walk the quality group levels and find the one that matches
        auto levelVisitorCallback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_registry.Get(level, visitArgs.m_jsonKeyPath))
            {
                if (levelName.compare(level.c_str()) == 0)
                {
                    levelIndex = currentIndex;
                    return AZ::SettingsRegistryInterface::VisitResponse::Done;
                }
            }

            currentIndex++;

            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        auto key = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s/%s",
            QualitySettingsGroupsRootKey, AZ_STRING_ARG(group), QualitySettingsGroupLevelsKey);
        AZ::SettingsRegistryVisitorUtils::VisitObject(registry, levelVisitorCallback, key);
        return levelIndex;
    }

    int16_t GetGroupDefaultLevel(AZ::SettingsRegistryInterface& registry, AZStd::string_view group)
    {
        AZ::s64 defaultLevel = aznumeric_cast<AZ::s64>(QualityCVarGroup::UseDeviceAttributeRules);

        auto key = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s/%s",
            QualitySettingsGroupsRootKey, AZ_STRING_ARG(group), QualitySettingsDefaultLevelKey);
        auto valueType = registry.GetType(key);
        if (valueType == AZ::SettingsRegistryInterface::Type::NoType)
        {
            // NoType means the key does not exist, use device attribute rules
        }
        else if (valueType == AZ::SettingsRegistryInterface::Type::String)
        {
            AZStd::string levelName;
            if (registry.Get(levelName, key))
            {
                defaultLevel = GetLevelFromName(levelName, registry, group);
            }
        }
        else if (valueType == AZ::SettingsRegistryInterface::Type::Integer)
        {
            registry.Get(defaultLevel, key);
        }
        else
        {
            AZ_Warning(
                "QualitySystemComponent",
                false,
                "Invalid quality level type found for %.*s, only string and integer values are supported.",
                AZ_STRING_ARG(key));
        }

        return aznumeric_cast<int16_t>(defaultLevel);
    }

    int QualitySystemComponent::GetGroupQualityLevel(AZStd::string_view group) const
    {
        AZStd::string_view qualityGroup = group.empty() ? AZStd::string_view(m_defaultGroupName) : group;
        if (qualityGroup.empty())
        {
            AZ_Warning("QualitySystemComponent", false, "No group quality level can be found for the default group because no default group name was defined at %s", QualitySettingsDefaultGroupKey);
            return QualitySystemEvents::LevelFromDeviceRules;
        }

        AZ::s64 level = aznumeric_cast<AZ::s64>(QualitySystemEvents::LevelFromDeviceRules);
        m_console->GetCvarValue(qualityGroup, level);

        if (level < 0)
        {
            level = GetGroupDefaultLevel(*m_settingsRegistry, qualityGroup);
            if (level < 0)
            {
                // we may not want this, -1 may be intentionaly set, or no default can be detected
                level = 0;
                AZ_Warning(
                    "QualitySystemComponent",
                    false,
                    "Failed to automatically determine the quality level to use for the group '%.*s', defaulting to 0.",
                    AZ_STRING_ARG(qualityGroup));

            }
        }
        return aznumeric_cast<int>(level);
    }

    void QualitySystemComponent::RegisterCvars()
    {
        // walk the quality groups in the settings registry and create cvars for every group
        auto groupVisitorCallback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::Object)
            {
                AZ_Warning(
                    "QualitySystemComponent",
                    false,
                    "Skipping quality setting group entry '%.*s' that is not an object type.",
                    AZ_STRING_ARG(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            int16_t level = GetGroupDefaultLevel(*m_settingsRegistry, visitArgs.m_fieldName);
            AZStd::string description = GetGroupDescription(*m_settingsRegistry, visitArgs.m_fieldName);
            m_settingsGroupCVars.push_back(AZStd::make_unique<QualityCVarGroup>(visitArgs.m_fieldName.data(), description.c_str(), level));
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitObject(*m_settingsRegistry, groupVisitorCallback, QualitySettingsGroupsRootKey);
        
        if (!m_defaultGroupName.empty() && !m_console->HasCommand(m_defaultGroupName))
        {
            // create a default quality group cvar
            m_settingsGroupCVars.push_back(AZStd::make_unique<QualityCVarGroup>(m_defaultGroupName.c_str(), "General quality settings cvar", QualityCVarGroup::UseDeviceAttributeRules));
        }
    }

    void QualitySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("QualitySystemComponentService"));
    }

    void QualitySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("QualitySystemComponentService"));
    }

    void QualitySystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

} // AzFramework

