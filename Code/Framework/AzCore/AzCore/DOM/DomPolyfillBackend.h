/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/DomPolyfillVisitor.h>

namespace AZ::Dom
{
    template<typename BackendToProxy>
    class DomPolyfillBackend : public Backend
    {
    public:
        template<typename... Args>
        DomPolyfillBackend(Args... args)
            : m_backendToProxy(AZStd::make_unique<BackendToProxy>(args...))
        {
        }

        Visitor::Result ReadFromBuffer(const char* buffer, size_t size, AZ::Dom::Lifetime lifetime, Visitor& visitor) override
        {
            DomPolyfillVisitor visitorWrapper(visitor);
            return m_backendToProxy->ReadFromBuffer(buffer, size, lifetime, visitorWrapper);
        }

        Visitor::Result ReadFromBufferInPlace(char* buffer, AZStd::optional<size_t> size, Visitor& visitor) override
        {
            DomPolyfillVisitor visitorWrapper(visitor);
            return m_backendToProxy->ReadFromBufferInPlace(buffer, size, visitorWrapper);
        }

        Visitor::Result WriteToBuffer(AZStd::string& buffer, WriteCallback callback) override
        {
            WriteCallback callbackWrapper = [&](Visitor& visitor)
            {
                DomPolyfillVisitor visitorWrapper(visitor);
                return callback(visitorWrapper);
            };
            return m_backendToProxy->WriteToBuffer(buffer, callbackWrapper);
        }

    private:
        AZStd::unique_ptr<BackendToProxy> m_backendToProxy;
    };
} // namespace AZ::Dom
