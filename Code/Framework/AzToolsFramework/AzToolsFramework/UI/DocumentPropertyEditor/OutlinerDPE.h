/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/FilteredDPE.h>

namespace AZ::DocumentPropertyEditor
{
    class OutlinerAdapter;
}

namespace AzToolsFramework
{
    class OutlinerDPE : public FilteredDPE
    {
    public:
        OutlinerDPE(QWidget* parentWidget = nullptr);
        static void RegisterViewClass();

    protected:
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::OutlinerAdapter> m_outlinerAdapter;
    };
} // namespace AzToolsFramework
