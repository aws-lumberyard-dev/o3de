/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::Reflection
{
    namespace LegacyReflectionInternal
    {
        struct InstanceVisitor : IObjectAccess, IAttributes
        {
            IReadWrite* m_visitor;
            void* m_instance;
            AZ::TypeId m_typeId;
            SerializeContext* m_serializeContext;
            const SerializeContext::ClassData* m_classData = nullptr;
            const SerializeContext::ClassElement* m_classElement = nullptr;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            virtual ~InstanceVisitor() = default;

            InstanceVisitor(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, SerializeContext* serializeContext)
                : m_visitor(visitor)
                , m_instance(instance)
                , m_typeId(typeId)
                , m_serializeContext(serializeContext)
            {
                RegisterPrimitiveHandlers<
                    bool,
                    uint8_t,
                    uint16_t,
                    uint32_t,
                    uint64_t,
                    int8_t,
                    int16_t,
                    int32_t,
                    int64_t,
                    float,
                    double
                >();
            }

            template <typename T>
            void RegisterHandler(AZStd::function<bool(T&)> handler)
            {
                m_handlers[azrtti_typeid<T>()] = [this, handler]() -> bool
                {
                    return handler(*reinterpret_cast<T*>(m_instance));
                };
            }

            template <typename T>
            void RegisterPrimitiveHandlers()
            {
                RegisterHandler<T>([this](T& value) -> bool
                {
                    m_visitor->Visit(value, *this);
                    return false;
                });
            }

            template <typename T1, typename T2, typename... Rest>
            void RegisterPrimitiveHandlers()
            {
                RegisterPrimitiveHandlers<T1>();
                RegisterPrimitiveHandlers<T2, Rest...>();
            }

            void Visit()
            {
                SerializeContext::EnumerateInstanceCallContext context(
                    [this](
                        void* instance, const AZ::SerializeContext::ClassData* classData,
                        const AZ::SerializeContext::ClassElement* classElement)
                    {
                        return BeginNode(instance, classData, classElement);
                    },
                    [this]()
                    {
                        return EndNode();
                    },
                    m_serializeContext, SerializeContext::EnumerationAccessFlags::ENUM_ACCESS_FOR_WRITE, nullptr);

                m_serializeContext->EnumerateInstance(&context, m_instance, m_typeId, nullptr, nullptr);
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                m_instance = instance;
                m_typeId = classData ? classData->m_typeId : Uuid::CreateNull();
                m_classData = classData;
                m_classElement = classElement;

                if (auto handlerIt = m_handlers.find(m_typeId); handlerIt != m_handlers.end())
                {
                    return handlerIt->second();
                }

                m_visitor->VisitObjectBegin(*this, *this);
                return true;
            }

            bool EndNode()
            {
                if (auto handlerIt = m_handlers.find(m_typeId); handlerIt != m_handlers.end())
                {
                    return true;
                }

                m_visitor->VisitObjectEnd();
                return true;
            }

            const AZ::TypeId& GetType() const override
            {
                return m_typeId;
            }

            AZStd::string_view GetTypeName() const override
            {
                const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(m_typeId);
                return classData ? classData->m_name : "<unregistered type>";
            }

            void* Get() override
            {
                return m_instance;
            }

            const void* Get() const override
            {
                return m_instance;
            }

            void ListAttributes(const IterationCallback& callback) const override
            {
                // Legacy reflection doesn't have groups, so they're in the root "" group
                // Name group;
                (void) callback;
            }
        };
    } // namespace LegacyReflectionInternal

    void VisitLegacyInMemoryInstance(IRead* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext)
    {
        IReadWriteToRead helper(visitor);
        VisitLegacyInMemoryInstance(&helper, instance, typeId, serializeContext);
    }

    void VisitLegacyInMemoryInstance(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext)
    {
        if (serializeContext == nullptr)
        {
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext != nullptr, "Unable to retreive a SerializeContext");
        }

        LegacyReflectionInternal::InstanceVisitor helper(visitor, instance, typeId, serializeContext);
        helper.Visit();
    }
} // namespace AZ::Reflection
