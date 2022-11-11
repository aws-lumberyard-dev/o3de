/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Data/DataType.h>

namespace ScriptCanvas
{
    class NodePropertyInterfaceListener
    {
    public:
        virtual void OnPropertyChanged() {};
    };
    
    // Dummy Wrapper Class to streamline the interface.
    // Should always be the TypeNodePropertyInterface
    class NodePropertyInterface
    {
    protected:
        NodePropertyInterface() = default;

    public:
        AZ_RTTI(NodePropertyInterface, "{265A2163-D3AE-4C4E-BDCC-37BA0084BF88}");
        virtual ~NodePropertyInterface() = default;

        virtual Data::Type GetDataType() = 0;

        void RegisterListener(NodePropertyInterfaceListener* listener)
        {
            m_listeners.insert(listener);
        }

        void RemoveListener(NodePropertyInterfaceListener* listener)
        {
            m_listeners.erase(listener);
        }

        void SignalDataChanged()
        {
            for (auto listener : m_listeners)
            {
                listener->OnPropertyChanged();
            }
        }

        virtual void ResetToDefault() = 0;

    private:
        AZStd::unordered_set<NodePropertyInterfaceListener*> m_listeners;
    };

    template<typename DataType>
    class TypedNodePropertyInterface
        : public NodePropertyInterface
    {
    public:
        AZ_RTTI((TypedNodePropertyInterface<DataType>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", DataType), NodePropertyInterface);

        TypedNodePropertyInterface() = default;
        virtual ~TypedNodePropertyInterface() = default;

        void SetPropertyReference(DataType* dataReference)
        {
            m_dataType = dataReference;
        }

        Data::Type GetDataType() override
        {
            return Data::FromAZType(azrtti_typeid<DataType>());
        }

        const DataType* GetPropertyData() const
        {
            return m_dataType;
        }

        void SetPropertyData(DataType dataType)
        {
            if ((*m_dataType != dataType))
            {
                (*m_dataType) = dataType;

                SignalDataChanged();
            }
        }

        void ResetToDefault() override
        {
            SetPropertyData(DataType());
        }

    private:
        DataType* m_dataType;
    };

    class ComboBoxPropertyInterface
    {
    public:

        AZ_RTTI(ComboBoxPropertyInterface, "{6CA5B611-59EA-4EAF-8A55-E7E74D7C1E53}");

        virtual int GetSelectedIndex() const = 0;
        virtual void SetSelectedIndex(int index) = 0;
    };

    template<typename DataType>
    class TypedComboBoxNodePropertyInterface
        : public TypedNodePropertyInterface<DataType>
        , public ComboBoxPropertyInterface
        
    {
    public:
        // The this-> method calls are here to deal with clang quirkiness with dependent template classes. Don't remove them.

        AZ_RTTI((TypedComboBoxNodePropertyInterface<DataType>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", DataType), TypedNodePropertyInterface<DataType>, ComboBoxPropertyInterface);

        TypedComboBoxNodePropertyInterface() = default;
        virtual ~TypedComboBoxNodePropertyInterface() = default;

        // TypedNodePropertyInterface
        void ResetToDefault() override
        {
            if (m_displaySet.empty())
            {
                this->SetPropertyData(DataType());
            }
            else
            {
                this->SetPropertyData(m_displaySet.front().second);
            }
        }

        void RegisterValueType(const AZStd::string& displayString, DataType value)
        {
            if (m_keySet.find(displayString) != m_keySet.end())
            {
                return;
            }

            m_displaySet.emplace_back(AZStd::make_pair(displayString, value));
        }

        // ComboBoxPropertyInterface
        int GetSelectedIndex() const override
        {
            int counter = -1;

            const DataType* value = this->GetPropertyData();

            for (int i = 0; i < m_displaySet.size(); ++i)
            {
                if (m_displaySet[i].second == (*value))
                {
                    counter = i;
                    break;
                }
            }

            return counter;
        }

        void SetSelectedIndex(int index) override
        {
            if (index >= 0 || index < m_displaySet.size())
            {
                this->SetPropertyData(m_displaySet[index].second);
            }
        }

        const AZStd::vector<AZStd::pair<AZStd::string, DataType>>& GetValueSet() const
        {
            return m_displaySet;
        }

    private:
        AZStd::unordered_set<AZStd::string> m_keySet;
        AZStd::vector<AZStd::pair<AZStd::string, DataType>> m_displaySet;
    };

    class EnumComboBoxNodePropertyInterface
        : public TypedComboBoxNodePropertyInterface<int>
    {
    public:
        AZ_RTTI(EnumComboBoxNodePropertyInterface, "{7D46B998-9E05-401A-AC92-37A90BAF8F60}", TypedComboBoxNodePropertyInterface<int32_t>);
        virtual ~EnumComboBoxNodePropertyInterface() = default;

        // No way of identifying Enum types properly yet. Going to fake a BCO object type for now.
        static const AZ::Uuid k_EnumUUID;

        Data::Type GetDataType() override
        {
            return ScriptCanvas::Data::Type::BehaviorContextObject(k_EnumUUID);
        }
    };
}
