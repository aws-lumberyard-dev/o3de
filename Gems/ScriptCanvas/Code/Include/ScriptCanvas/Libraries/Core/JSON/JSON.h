#pragma once

#include <AzCore/EBus/Ebus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <aws/core/utils/json/JsonSerializer.h>

namespace ScriptCanvas
{
    class JSONView
    {
    public:
        AZ_TYPE_INFO(JSONView, "{EF943E37-6831-4767-896F-046EBFD17258}");

        JSONView() = default;
        JSONView(const Aws::Utils::Json::JsonView& jsonView)
        : m_jsonView(jsonView)
        {}

        template <typename T>
        ::AZStd::tuple<bool, T> KeyNotFoundError(const AZStd::string& key, T t)
        {
            AZ_Error("SC", false, AZStd::string::format("Key: %s not found in JSON data", key.c_str()).c_str());
            return ::AZStd::make_tuple(false, t);
        }

        template <typename T>
        ::AZStd::tuple<bool, T> ValueNotFoundError(const AZStd::string& key, T t)
        {
            AZ_Error("SC", false, AZStd::string::format("Value not found for key: %s in JSON data", key.c_str()).c_str());
            return ::AZStd::make_tuple(false, t);
        }

        ::AZStd::tuple<bool, ::AZStd::string> GetString(const ::AZStd::string& key)
        {
            AZStd::string defaultStr;
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, defaultStr);
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, defaultStr);
            }

            auto jsonStr = m_jsonView.GetString(key.c_str());

            AZStd::string str = jsonStr.c_str();

            return ::AZStd::make_tuple(true, str.c_str());
        }

        ::AZStd::tuple<bool, double> GetDouble(const ::AZStd::string& key)
        {
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, 0.0);
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, 0.0);
            }

            if (m_jsonView.IsFloatingPointType())
            {
                return ::AZStd::make_tuple(true, m_jsonView.GetDouble(key.c_str()));
            }

            return ::AZStd::make_tuple(false, 0.0);
        }

        ::AZStd::tuple<bool, bool> GetBool(const ::AZStd::string& key)
        {
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, false);
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, false);
            }

            if (m_jsonView.IsBool())
            {
                return ::AZStd::make_tuple(true, m_jsonView.GetBool(key.c_str()));
            }

            return ::AZStd::make_tuple(false, false);
        }

        ::AZStd::tuple<bool, int> GetInteger(const ::AZStd::string& key)
        {
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, 0);
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, 0);
            }

            if (m_jsonView.IsIntegerType())
            {
                return ::AZStd::make_tuple(true, m_jsonView.GetInteger(key.c_str()));
            }

            return ::AZStd::make_tuple(false, 0);
        }

        ::AZStd::tuple<bool, JSONView> GetObject(const ::AZStd::string& key)
        {
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, JSONView());
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, JSONView());
            }

            if (m_jsonView.IsObject())
            {
                return ::AZStd::make_tuple(true, m_jsonView.GetObject(key.c_str()));
            }

            return ::AZStd::make_tuple(false, JSONView());
        }

        ::AZStd::tuple<bool, AZStd::vector<JSONView>> GetArray(const ::AZStd::string& key)
        {
            if (!m_jsonView.KeyExists(key.c_str()))
            {
                return KeyNotFoundError(key, AZStd::vector<JSONView>());
            }

            if (!m_jsonView.ValueExists(key.c_str()))
            {
                return ValueNotFoundError(key, AZStd::vector<JSONView>());
            }

            if (m_jsonView.IsListType())
            {
                AZStd::vector<JSONView> jvArray;
                for (size_t i = 0; i < m_jsonView.GetArray(key.c_str()).GetLength(); ++i)
                {
                    jvArray.push_back(m_jsonView.GetArray(key.c_str()).GetItem(i));
                }
                return ::AZStd::make_tuple(true, jvArray);
            }

            return ::AZStd::make_tuple(false, AZStd::vector<JSONView>());
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<JSONView>();
                serializeContext->RegisterGenericType<JSONView>();

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<JSONView>("JSONView", "Provides access to a JSON object")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_Hide"))
                    ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<JSONView>("JSONView")
                ->Attribute(AZ::Script::Attributes::Category, "Core/JSON")
                ->Method("GetString", &JSONView::GetString)
                ->Method("GetDouble", &JSONView::GetDouble)
                ->Method("GetBool", &JSONView::GetBool)
                ->Method("GetInteger", &JSONView::GetInteger)
                ->Method("GetArray", &JSONView::GetArray)
                ->Method("GetObject", &JSONView::GetObject)
                ;
            }
        }

    private:

        Aws::Utils::Json::JsonView m_jsonView;

        AZStd::vector<JSONView> m_onDemandReflection;


    };
}
