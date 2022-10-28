/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerFilterAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class OutlinerAdapter;
}

namespace Ui
{
    class OutlinerDPE;
}

namespace AzToolsFramework
{
    class DocumentPropertyEditor;

    class OutlinerDPE
        : public QWidget
    {
    public:
        OutlinerDPE(QWidget* parentWidget = nullptr);
        virtual ~OutlinerDPE();

        static void RegisterViewClass();

        void SetAdapter(AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> sourceAdapter);
        DocumentPropertyEditor* GetDPE();

    protected:
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::OutlinerAdapter> m_outlinerAdapter;
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::OutlinerFilterAdapter> m_filterAdapter;

        Ui::OutlinerDPE* m_ui;
    };
} // namespace AzToolsFramework
