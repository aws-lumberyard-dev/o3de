/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
    template<typename t_Class>
    class SerializeContextReadWriteHandler
        : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called right before we start reading from the instance pointed by classPtr.
        void OnReadBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnReadBegin();
        }

        /// Called after we are done reading from the instance pointed by classPtr.
        void OnReadEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnReadEnd();
        }

        /// Called right before we start writing to the instance pointed by classPtr.
        void OnWriteBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteBegin();
        }

        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    template<typename t_Class>
    class SerializeContextOnWriteEndHandler
        : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    template<typename t_Class>
    class SerializeContextOnWriteHandler
        : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called right before we start writing to the instance pointed by classPtr.
        void OnWriteBegin(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteBegin();
        }

        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)
}
