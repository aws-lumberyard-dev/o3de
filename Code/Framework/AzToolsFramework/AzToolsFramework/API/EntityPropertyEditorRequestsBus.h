/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    class ComponentEditor;

    //! Requests to be made of all EntityPropertyEditors
    //! Beware, there may be more than one EntityPropertyEditor that can respond
    //! Broadcast should be used for accessing these functions
    class EntityPropertyEditorRequests
        : public AZ::EBusTraits
    {
    public:
        using VisitComponentEditorsCallback = AZStd::function<bool(const ComponentEditor*)>;

        //! Returns the list of selected entities or if in a pinned window, the list of entities in that window
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedAndPinnedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Returns the list of selected entities
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Returns the list of selected components
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedComponents(AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedComponentEntityIds) = 0;

        //! Explicitly sets a component as having been the most recently added.
        //! This means that the next time the UI refreshes, that component will be ensured to be visible.
        virtual void SetNewComponentId(AZ::ComponentId componentId) = 0;

        //! Visits the component editors in an EntityPropertyEditor via a callback.
        //! @param callback The callback that iterates over all the component editors within an EntityPropertyEditor.
        virtual void VisitComponentEditors(const VisitComponentEditorsCallback& callback) const = 0;

        //! Open the Add Components panel for the Inspector that contains a selected component.
        virtual void OpenAddComponentPanel() = 0;

        //! Delete selected components.
        virtual void DeleteSelectedComponents() = 0;

        //! Cut selected components.
        virtual void CutSelectedComponents() = 0;

        //! Copy selected components.
        virtual void CopySelectedComponents() = 0;

        //! Paste components.
        virtual void PasteComponents() = 0;

        //! Enable selected components.
        virtual void EnableSelectedComponents() = 0;

        //! Disable selected components.
        virtual void DisableSelectedComponents() = 0;

        //! Move selected component editors up one element.
        virtual void MoveUpSelectedComponents() = 0;

        //! Move selected component editors down one element.
        virtual void MoveDownSelectedComponents() = 0;

        //! Move selected component editors to the top.
        virtual void MoveSelectedComponentsToTop() = 0;

        //! Move selected component editors to the bottom.
        virtual void MoveSelectedComponentsToBottom() = 0;

        //! Queries whether selected components can be removed from the entity.
        virtual void CanRemoveSelectedComponents(bool& result) = 0;

        //! Queries whether selected components can be copied.
        virtual void CanCopySelectedComponents(bool& result) = 0;

        //! Queries whether it is possible to paste in the current selection.
        virtual void CanPasteOnSelection(bool& result) = 0;

        //! Queries whether it is possible to move the component selection up.
        virtual void CanMoveComponentSelectionUp(bool& result) = 0;

        //! Queries whether it is possible to move the component selection down.
        virtual void CanMoveComponentSelectionDown(bool& result) = 0;

        //! Queries whether it is possible to enable the selected components.
        virtual void CanEnabledSelectedComponents(bool& result) = 0;

        //! Queries whether it is possible to disable the selected components.
        virtual void CanDisableSelectedComponents(bool& result) = 0;

        //! Queries whether it is possible to add components.
        virtual void CanAddComponents(bool& result) = 0;
    };

    using EntityPropertyEditorRequestBus = AZ::EBus<EntityPropertyEditorRequests>;
} // namespace AzToolsFramework
