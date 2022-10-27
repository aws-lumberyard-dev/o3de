/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerRowHandler.h>

namespace AzToolsFramework
{
    OutlinerRowHandler::OutlinerRowHandler()
    {
    }

    void OutlinerRowHandler::SetValueFromDom(const AZ::Dom::Value& node)
    {
        auto entityName = AZ::Dpe::Nodes::OutlinerRow::Value.ExtractFromDomNode(node).value_or("");
        auto visibleState = AZ::Dpe::Nodes::OutlinerRow::Visible.ExtractFromDomNode(node).value_or(true);
        auto lockedState = AZ::Dpe::Nodes::OutlinerRow::Locked.ExtractFromDomNode(node).value_or(false);

        m_entityId = AZ::EntityId(AZ::Dpe::Nodes::OutlinerRow::EntityId.ExtractFromDomNode(node).value_or(AZ::EntityId::InvalidEntityId));

        m_name->setText(QString::fromUtf8(entityName.data(), aznumeric_cast<int>(entityName.size())));
        m_visibilityButton->setChecked(!visibleState);
        m_lockButton->setChecked(lockedState);
    }
} // namespace AzToolsFramework
