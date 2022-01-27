/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/base.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ::Debug
{
    struct Magic32
    {
        static const AZ::u32 m_defValue = 0xfeedf00d;
        AZ_FORCE_INLINE Magic32()
        {
            m_value = (m_defValue ^ (AZ::u32)((AZStd::size_t)this));
        }
        AZ_FORCE_INLINE ~Magic32()
        {
            m_value = 0;
        }
        AZ_FORCE_INLINE bool Validate() const
        {
            return m_value == (m_defValue ^ (AZ::u32)((AZStd::size_t)this));
        }

    private:
        AZ::u32 m_value;
    };
}
