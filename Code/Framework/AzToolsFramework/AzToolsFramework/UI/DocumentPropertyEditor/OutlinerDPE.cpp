/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OutlinerDPE.h"
#include "OutlinerAdapter.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

// TRYAN - Including this for now; it stinks, but there are probably ways we could address that
#include "../../../../../Editor/LyViewPaneNames.h"

namespace AzToolsFramework
{
    OutlinerDPE::OutlinerDPE(QWidget* parentWidget)
        : FilteredDPE(parentWidget)
        , m_outlinerAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::OutlinerAdapter>())
    {
        setWindowTitle(tr("DPE-based Outliner"));
        SetAdapter(m_outlinerAdapter);
    }

    void OutlinerDPE::RegisterViewClass()
    {
        ViewPaneOptions opts;
        opts.paneRect = QRect(100, 100, 700, 600);
        opts.isDeletable = false;
        RegisterViewPane<OutlinerDPE>(LyViewPane::DpeOutliner, LyViewPane::CategoryOther, opts);
    }
} // namespace AzToolsFramework
