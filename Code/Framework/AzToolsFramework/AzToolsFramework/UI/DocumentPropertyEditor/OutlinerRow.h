/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>

#include "AzToolsFramework/UI/DocumentPropertyEditor/ui_OutlinerRow.h"

namespace AzQtComponents
{
    class OutlinerRow
        : public QWidget
        , public Ui::OutlinerRow
    {
        Q_OBJECT
    public:
        OutlinerRow(QWidget* parent);

    protected slots:
        void onVisibilityToggled(bool checked);
        void onLockedToggled(bool checked);

    protected:
        AZ::EntityId m_entityId;
    };
} // namespace AzQtComponents
