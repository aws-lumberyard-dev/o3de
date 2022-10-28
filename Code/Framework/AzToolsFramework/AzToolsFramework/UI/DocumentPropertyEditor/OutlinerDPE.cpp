/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerDPE.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerAdapter.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerFilterAdapter.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include "AzToolsFramework/UI/DocumentPropertyEditor/ui_OutlinerDPE.h"

// TRYAN - Including this for now; it stinks, but there are probably ways we could address that
#include "../../../../../Editor/LyViewPaneNames.h"

namespace AzToolsFramework
{
    OutlinerDPE::OutlinerDPE(QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_outlinerAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::OutlinerAdapter>())
    {
        setWindowTitle(tr("DPE-based Outliner"));
        SetAdapter(m_outlinerAdapter);

        connect(m_ui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_filterAdapter, [=](const QString& filterText)
            {
                m_filterAdapter->SetFilterString(filterText.toUtf8().data());
            });
        //connect(m_ui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        //    m_filterAdapter, &AZ::DocumentPropertyEditor::OutlinerFilterAdapter::OnFilterChanged);
    }

    OutlinerDPE::~OutlinerDPE()
    {
        delete m_ui;
    }

    void OutlinerDPE::RegisterViewClass()
    {
        ViewPaneOptions opts;
        opts.paneRect = QRect(100, 100, 700, 600);
        opts.isDeletable = false;
        RegisterViewPane<OutlinerDPE>(LyViewPane::DpeOutliner, LyViewPane::CategoryOther, opts);
    }

    void OutlinerDPE::SetAdapter(AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> sourceAdapter)
    {
        m_outlinerAdapter = sourceAdapter;
        m_filterAdapter->SetSourceAdapter(m_outlinerAdapter);
    }

    DocumentPropertyEditor* OutlinerDPE::GetDPE()
    {
        return m_ui->m_dpe;
    }
} // namespace AzToolsFramework
