/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerRowHandler.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QSignalBlocker>

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
        auto selectedState = AZ::Dpe::Nodes::OutlinerRow::Selected.ExtractFromDomNode(node).value_or(false);

        m_entityId = AZ::EntityId(AZ::Dpe::Nodes::OutlinerRow::EntityId.ExtractFromDomNode(node).value_or(AZ::EntityId::InvalidEntityId));

        m_name->setText(QString::fromUtf8(entityName.data(), aznumeric_cast<int>(entityName.size())));

        // set the check state on the checkboxes, but don't let them emit their toggled signals or we'll loop back on ourselves
        QSignalBlocker blockVisibleSignals(m_visibilityButton);
        QSignalBlocker blockLockSignals(m_lockButton);
        m_visibilityButton->setChecked(!visibleState);
        m_lockButton->setChecked(lockedState);

        if (selectedState)
        {
            setStyleSheet("border: 2px solid #0000ff;");
        }
        else
        {
            setStyleSheet("");
        }
    }

    void OutlinerRowHandler::mouseReleaseEvent(QMouseEvent* event)
    {
        (void)event;
        const AzToolsFramework::EntityIdList selection = { m_entityId };
        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selection);
    }

} // namespace AzToolsFramework
