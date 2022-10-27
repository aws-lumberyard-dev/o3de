/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OutlinerRow.h"
#include <Entity/EditorEntityHelpers.h>

namespace AzQtComponents
{
    OutlinerRow::OutlinerRow(QWidget* parent)
        : QWidget(parent)
    {
        setupUi(this);
        QObject::connect(m_visibilityButton, &QCheckBox::toggled, this, &OutlinerRow::onVisibilityToggled);
        QObject::connect(m_lockButton, &QCheckBox::toggled, this, &OutlinerRow::onLockedToggled);
    }

    void OutlinerRow::onVisibilityToggled(bool checked)
    {
        AzToolsFramework::SetEntityVisibility(m_entityId, !checked);
    }

    void OutlinerRow::onLockedToggled(bool checked)
    {
        AzToolsFramework::SetEntityLockState(m_entityId, checked);
    }
} // namespace AzQtComponents
