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
        , m_filterAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::OutlinerFilterAdapter>())
        , m_ui(new Ui::OutlinerDPE())
        //, m_displayOptionsMenu(new EntityOutliner::DisplayOptionsMenu(this))
    {
        m_ui->setupUi(this);
        setWindowTitle(tr("DPE-based Outliner"));

        m_filterAdapter->SetIncludeAllMatchDescendants(false);

        connect(m_ui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
            m_filterAdapter.get(), &AZ::DocumentPropertyEditor::OutlinerFilterAdapter::OnTextFilterChanged);

        connect(m_ui->m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
            m_filterAdapter.get(), &AZ::DocumentPropertyEditor::OutlinerFilterAdapter::SetCriteriaFilter);

        SetAdapter(m_outlinerAdapter);

        //QToolButton* display_options = new QToolButton(this);
        //display_options->setObjectName(QStringLiteral("m_display_options"));
        //display_options->setPopupMode(QToolButton::InstantPopup);
        //display_options->setAutoRaise(true);

        //m_gui->m_searchWidget->AddWidgetToSearchWidget(display_options);

        //// Set the display options menu
        //display_options->setMenu(m_displayOptionsMenu);
        //connect(
        //    m_displayOptionsMenu, &EntityOutliner::DisplayOptionsMenu::OnSortModeChanged, this, &EntityOutlinerWidget::OnSortModeChanged);
        //connect(
        //    m_displayOptionsMenu,
        //    &EntityOutliner::DisplayOptionsMenu::OnOptionToggled,
        //    this,
        //    &EntityOutlinerWidget::OnDisplayOptionChanged);
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
        m_outlinerAdapter = AZStd::static_pointer_cast<AZ::DocumentPropertyEditor::OutlinerAdapter>(sourceAdapter);
        m_filterAdapter->SetSourceAdapter(m_outlinerAdapter);
        m_ui->m_dpe->SetAdapter(m_filterAdapter);
    }

    DocumentPropertyEditor* OutlinerDPE::GetDPE()
    {
        return m_ui->m_dpe;
    }
} // namespace AzToolsFramework
