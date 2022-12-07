/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#pragma optimize("", off)
namespace AzToolsFramework
{
    namespace Prefab
    {
        Link::Link()
            : Link(InvalidLinkId)
        {
        }

        Link::Link(LinkId linkId)
        {
            m_id = linkId;

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab System Component Interface could not be found. "
                "It is a requirement for the Link class. Check that it is being correctly initialized.");

            m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
            AZ_Assert(m_instanceToTemplateInterface != nullptr,
                "Instance To Template Interface could not be found. "
                "It is a requirement for the Link class. Check that it is being correctly initialized.");
        }

        Link::Link(Link&& other) noexcept
            : m_id(AZStd::move(other.m_id))
            , m_sourceTemplateId(AZStd::move(other.m_sourceTemplateId))
            , m_targetTemplateId(AZStd::move(other.m_targetTemplateId))
            , m_instanceName(AZStd::move(other.m_instanceName))
            , m_prefabSystemComponentInterface(AZStd::move(other.m_prefabSystemComponentInterface))
            , m_instanceToTemplateInterface(AZStd::move(other.m_instanceToTemplateInterface))
            , m_linkPatchesTree(AZStd::move(other.m_linkPatchesTree))
            // NEW
            , m_linkPrefix(AZStd::move(other.m_linkPrefix))
            , m_nestedLinks(AZStd::move(other.m_nestedLinks))
        {
            other.m_prefabSystemComponentInterface = nullptr;
        }


        Link& Link::operator=(Link&& other) noexcept
        {
            if (this != &other)
            {
                m_id = AZStd::move(other.m_id);
                m_sourceTemplateId = AZStd::move(other.m_targetTemplateId);
                m_targetTemplateId = AZStd::move(other.m_sourceTemplateId);
                m_instanceName = AZStd::move(other.m_instanceName);
                m_prefabSystemComponentInterface = AZStd::move(other.m_prefabSystemComponentInterface);
                AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                    "Prefab System Component Interface could not be found. "
                    "It is a requirement for the Link class. Check that it is being correctly initialized.");
                m_instanceToTemplateInterface = AZ::Interface<InstanceToTemplateInterface>::Get();
                AZ_Assert(m_instanceToTemplateInterface != nullptr,
                    "Instance To Template Interface could not be found. "
                    "It is a requirement for the Link class. Check that it is being correctly initialized.");
                other.m_prefabSystemComponentInterface = nullptr;
                m_linkPatchesTree = AZStd::move(other.m_linkPatchesTree);
                // NEW
                m_linkPrefix = AZStd::move(other.m_linkPrefix);
                m_nestedLinks = AZStd::move(other.m_nestedLinks);
            }
            return *this;
        }

        void Link::SetSourceTemplateId(TemplateId id)
        {
            m_sourceTemplateId = id;
        }

        void Link::SetTargetTemplateId(TemplateId id)
        {
            m_targetTemplateId = id;
        }

        void Link::SetLinkDom(const PrefabDomValue& linkDom)
        {
            AZ_PROFILE_FUNCTION(PrefabSystem);
            m_linkPatchesTree.Clear();
            PrefabDomValueConstReference patchesReference = PrefabDomUtils::FindPrefabDomValue(linkDom, PrefabDomUtils::PatchesName);
            if (patchesReference.has_value())
            {
                RebuildLinkPatchesTree(patchesReference->get());
            }
        }

        void Link::AddPatchesToLink(const PrefabDom& patches)
        {
            RebuildLinkPatchesTree(patches);
        }

        void Link::SetInstanceName(const char* instanceName)
        {
            m_instanceName = instanceName;
        }

        bool Link::IsValid() const
        {
            return
                m_targetTemplateId != InvalidTemplateId &&
                m_sourceTemplateId != InvalidTemplateId &&
                !m_instanceName.empty();
        }

        TemplateId Link::GetSourceTemplateId() const
        {
            return m_sourceTemplateId;
        }

        TemplateId Link::GetTargetTemplateId() const
        {
            return m_targetTemplateId;
        }

        LinkId Link::GetId() const
        {
            return m_id;
        }

        void Link::GetLinkDom(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const
        {
            AZ_PROFILE_FUNCTION(PrefabSystem);
            ConstructLinkDomFromPatches(linkDom, allocator, false, false);
        }

        void Link::GetLinkDomWithNestedLinkDoms(PrefabDomValue& linkDom, PrefabDomAllocator& allocator, bool expandNestedLinkDoms) const
        {
            AZ_PROFILE_FUNCTION(PrefabSystem);
            ConstructLinkDomFromPatches(linkDom, allocator, true, expandNestedLinkDoms);
        }

        bool Link::AreOverridesPresent(AZ::Dom::Path path, AZ::Dom::PrefixTreeTraversalFlags prefixTreeTraversalFlags)
        {
            bool areOverridesPresent = false;
            auto visitorFn = [&areOverridesPresent](AZ::Dom::Path, const PrefabOverrideMetadata&)
            {
                areOverridesPresent = true;
                // We just need to check if at least one override is present at the path.
                // Return false here so that we don't keep looking for all patches at the path.
                return false;
            };

            m_linkPatchesTree.VisitPath(path, visitorFn, prefixTreeTraversalFlags);
            return areOverridesPresent;
        }

        PrefabOverridePrefixTree Link::RemoveOverrides(AZ::Dom::Path path)
        {
            return m_linkPatchesTree.DetachSubTree(path);
        }

        bool Link::AddOverrides(const AZ::Dom::Path& path, AZ::Dom::DomPrefixTree<PrefabOverrideMetadata>&& subTree)
        {
            return m_linkPatchesTree.AttachSubTree(path, AZStd::move(subTree));
        }

        PrefabDomPath Link::GetInstancePath() const
        {
            //return PrefabDomUtils::GetPrefabDomInstancePath(m_instanceName.c_str());
            return PrefabDomPath(GetInstancePathString().c_str());
        }

        AZStd::string Link::GetInstancePathString() const
        {
            //AZStd::string::format("%s/Instances/%s", m_linkPrefix, m_instanceName);
            return m_linkPrefix + AZStd::string("/Instances/") + m_instanceName;
        }

        const AZStd::string& Link::GetInstanceName() const
        {
            return m_instanceName;
        }

        void Link::ExpandLinkDom(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const
        {
            if (!linkDom.IsObject())
            {
                return;
            }

            if (!linkDom.HasMember("Source"))
            {
                // It is not a link DOM.
                return;
            }

            if (!linkDom.HasMember("Patches"))
            {
                // Already expanded. No need to expand.
                return;
            }

            // Collapse
            const PrefabDomValue& theSourceValue = linkDom["Source"];
            AZStd::string theSourceTemplatePath(theSourceValue.GetString(), theSourceValue.GetStringLength());
            TemplateId theSourceTemplateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(theSourceTemplatePath.c_str());
            TemplateReference theSourceTemplateReference = m_prefabSystemComponentInterface->FindTemplate(theSourceTemplateId);

            PrefabDom& sourceDomDom = theSourceTemplateReference->get().GetPrefabDom();
            PrefabDomValue sourceTemplateDomCopy(sourceDomDom, allocator);

            // Apply patches
            AZ::JsonSerializationResult::ResultCode applyPatchResult =
                PrefabDomUtils::ApplyPatches(sourceTemplateDomCopy, allocator, linkDom["Patches"]);

            linkDom.CopyFrom(sourceTemplateDomCopy, allocator);
        }

        void Link::CollapseLinkDom(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const
        {
            if (!linkDom.IsObject())
            {
                return;
            }

            if (!linkDom.HasMember("Source"))
            {
                // It is not a nested link DOM.
                return;
            }

            if (linkDom.HasMember("Patches"))
            {
                // Already collapsed. No need to collapse.
                return;
            }

            // Collapse
            const PrefabDomValue& theSourceValue = linkDom["Source"];
            AZStd::string theSourceTemplatePath(theSourceValue.GetString(), theSourceValue.GetStringLength());
            TemplateId theSourceTemplateId = m_prefabSystemComponentInterface->GetTemplateIdFromFilePath(theSourceTemplatePath.c_str());
            TemplateReference theSourceTemplateReference = m_prefabSystemComponentInterface->FindTemplate(theSourceTemplateId);

            PrefabDom& sourceDomDom = theSourceTemplateReference->get().GetPrefabDom();
            if (sourceDomDom.HasMember(PrefabDomUtils::LinkIdName))
            {
                sourceDomDom.RemoveMember("LinkId");
            }
            if (linkDom.HasMember(PrefabDomUtils::LinkIdName))
            {
                linkDom.RemoveMember("LinkId");
            }

            // Generate the link patch.
            PrefabDom newLinkDomPatches(&allocator);
            m_instanceToTemplateInterface->GeneratePatch(newLinkDomPatches, sourceDomDom, linkDom);

            linkDom.RemoveMember("ContainerEntity");
            linkDom.RemoveMember("Entities");
            linkDom.RemoveMember("Instances");
            linkDom.AddMember(rapidjson::GenericStringRef(PrefabDomUtils::PatchesName), newLinkDomPatches.Move(), allocator);
        }

        bool Link::UpdateTarget()
        {
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            PrefabDom& sourceTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_sourceTemplateId);

            // Copy the source template dom so that the actual template DOM does not change and only the linked instance DOM does.
            PrefabDom sourceTemplateDomCopy;
            sourceTemplateDomCopy.CopyFrom(sourceTemplatePrefabDom, sourceTemplatePrefabDom.GetAllocator());

            PrefabDomValueReference linkedInstanceDomRef = GetLinkedInstanceDom();

            // Create new one.
            if (!linkedInstanceDomRef.has_value())
            {
                PrefabDomPath instancePath = GetInstancePath();

                //PrefabDomValue newLinkedInstanceDom(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator());

                instancePath.Set(targetTemplatePrefabDom, PrefabDomValue(rapidjson::kObjectType).Move());
                linkedInstanceDomRef = GetLinkedInstanceDom();
            }

            PrefabDomValue& linkedInstanceDom = linkedInstanceDomRef->get();

            PrefabDom linkDom;
            GetLinkDom(linkDom, linkDom.GetAllocator()); // without nested link DOMs, they will be process in the end.
            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(linkDom, PrefabDomUtils::PatchesName);

            if (!patchesReference.has_value())
            {
                if (AZ::JsonSerialization::Compare(linkedInstanceDom, sourceTemplateDomCopy) != AZ::JsonSerializerCompareResult::Equal)
                {
                    linkedInstanceDom.CopyFrom(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator());
                }
                // Should shoot a warning...
            }
            else
            {
                AZ::JsonSerializationResult::ResultCode applyPatchResult = PrefabDomUtils::ApplyPatches(
                    sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator(), patchesReference->get());
                linkedInstanceDom.CopyFrom(sourceTemplateDomCopy, targetTemplatePrefabDom.GetAllocator());

                [[maybe_unused]] PrefabDomValueReference sourceTemplateName =
                    PrefabDomUtils::FindPrefabDomValue(sourceTemplateDomCopy, PrefabDomUtils::SourceName);
                AZ_Assert(sourceTemplateName && sourceTemplateName->get().IsString(), "A valid source template name couldn't be found");
                [[maybe_unused]] PrefabDomValueReference targetTemplateName =
                    PrefabDomUtils::FindPrefabDomValue(targetTemplatePrefabDom, PrefabDomUtils::SourceName);
                AZ_Assert(targetTemplateName && targetTemplateName->get().IsString(), "A valid target template name couldn't be found");

                if (applyPatchResult.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    AZ_Error(
                        "Prefab",
                        false,
                        "Link::UpdateTarget - ApplyPatches failed for Prefab DOM from source Template '%u' and target Template '%u'.",
                        m_sourceTemplateId,
                        m_targetTemplateId);
                    return false;
                }
                if (applyPatchResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::PartialSkip ||
                    applyPatchResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Skipped)
                {
                    AZ_Warning(
                        "Prefab",
                        false,
                        "Link::UpdateTarget - Some of the patches couldn't be applied on the source template '%s' present under the  "
                        "target Template '%s'.",
                        sourceTemplateName->get().GetString(),
                        targetTemplateName->get().GetString());
                }
            }

            // This is a guardrail to ensure the linked instance dom always has the LinkId value
            // in case the template copy or the patch application removed it.
            AddLinkIdToInstanceDom(linkedInstanceDom, targetTemplatePrefabDom.GetAllocator());

            // Process nested links if any.
            bool isUpdateTargetSuccessful = true;
            for (auto& [patchPath, linkId] : m_nestedLinks)
            {
                LinkReference nestedLink = m_prefabSystemComponentInterface->FindLink(linkId);
                if (nestedLink.has_value())
                {
                    if (!nestedLink->get().UpdateTarget())
                    {
                        isUpdateTargetSuccessful = false;
                    }
                }
                else
                {
                    isUpdateTargetSuccessful = false;
                }
            }

            return isUpdateTargetSuccessful;
            //return true;
        }

        PrefabDomValueReference Link::GetLinkedInstanceDom()
        {
            AZ_Assert(IsValid(), "Link::GetLinkedInstanceDom - Trying to get DOM of an invalid link.");
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            PrefabDomPath instancePath = GetInstancePath();
            PrefabDomValue* instanceValue = instancePath.Get(targetTemplatePrefabDom);
            if (instanceValue)
            {
                return *instanceValue;
            }
            /*AZ_Assert(instanceValue,"Link::GetLinkedInstanceDom - Invalid value for instance pointed by the link in template with id '%u'.",
                    m_targetTemplateId);*/
            return AZStd::nullopt;
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom) const
        {
            PrefabDom& targetTemplatePrefabDom = m_prefabSystemComponentInterface->FindTemplateDom(m_targetTemplateId);
            AddLinkIdToInstanceDom(instanceDom, targetTemplatePrefabDom.GetAllocator());
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom, PrefabDomAllocator& allocator) const
        {
            PrefabDomValueReference linkIdReference = PrefabDomUtils::FindPrefabDomValue(instanceDom, PrefabDomUtils::LinkIdName);
            if (!linkIdReference.has_value())
            {
                AZ_Assert(instanceDom.IsObject(), "Link Id '%u' cannot be added because the DOM of the instance is not an object.", m_id);
                instanceDom.AddMember(rapidjson::StringRef(PrefabDomUtils::LinkIdName), rapidjson::Value().SetUint64(m_id), allocator);
            }
            else
            {
                linkIdReference->get().SetUint64(m_id);
            }
        }

        void Link::AddLinkIdToInstanceDom(PrefabDomValue& instanceDom, LinkId linkId, PrefabDomAllocator& allocator) const
        {
            PrefabDomValueReference linkIdReference = PrefabDomUtils::FindPrefabDomValue(instanceDom, PrefabDomUtils::LinkIdName);
            if (!linkIdReference.has_value())
            {
                AZ_Assert(instanceDom.IsObject(), "Link Id '%u' cannot be added because the DOM of the instance is not an object.", m_id);
                instanceDom.AddMember(rapidjson::StringRef(PrefabDomUtils::LinkIdName), rapidjson::Value().SetUint64(linkId), allocator);
            }
            else
            {
                linkIdReference->get().SetUint64(linkId);
            }
        }

        void Link::ConstructLinkDomFromPatches(PrefabDomValue& linkDom, PrefabDomAllocator& allocator, bool includeNestedLinkDoms, bool expanded) const
        {
            linkDom.SetObject();

            TemplateReference sourceTemplate = m_prefabSystemComponentInterface->FindTemplate(m_sourceTemplateId);
            if (!sourceTemplate.has_value())
            {
                AZ_Assert(false, "Failed to fetch source template from link");
                return;
            }

            linkDom.AddMember(
                rapidjson::StringRef(PrefabDomUtils::SourceName),
                PrefabDomValue(sourceTemplate->get().GetFilePath().c_str(), allocator),
                allocator);

            PrefabDomValue patchesArray;

            GetLinkPatches(patchesArray, allocator, includeNestedLinkDoms, expanded);

            if (patchesArray.Size() != 0)
            {
                linkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), AZStd::move(patchesArray), allocator);
            }
        }

        void Link::RebuildLinkPatchesTree(const PrefabDomValue& patches)
        {
            m_linkPatchesTree.Clear();
            if (patches.IsArray())
            {
                rapidjson::GenericArray patchesArray = patches.GetArray();
                for (rapidjson::SizeType i = 0; i < patchesArray.Size(); i++)
                {
                    const PrefabDomValue& patch = patchesArray[i];

                    // Nested link DOM.
                    if (patch.HasMember("value") && patch["value"].IsObject() && patch["value"].HasMember("Source"))
                    {
                        // OKAY. This is a nested link DOM.

                        PrefabDom patchEntry;
                        patchEntry.CopyFrom(patch, patchEntry.GetAllocator());

                        // Needs to collapse it at its first level.
                        CollapseLinkDom(patchEntry["value"], patchEntry.GetAllocator());

                        // Locates the nested link.
                        const PrefabDomValue& thePatchPathValue = patchEntry["path"];
                        const AZStd::string thePatchPath(thePatchPathValue.GetString(), thePatchPathValue.GetStringLength());
                        LinkId nestedLinkId = m_nestedLinks[thePatchPath];
                        LinkReference nestedLink = m_prefabSystemComponentInterface->FindLink(nestedLinkId);
                        if (!nestedLink.has_value())
                        {
                            AZ_Error("Prefab", nestedLink.has_value(), "NO WAY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                        }

                        nestedLink->get().SetLinkDom(patchEntry["value"]); // recursive!!!

                        // Do not push into this prefix tree...
                        /*
                        patchEntry["value"].SetObject(); // or null.

                        auto path = patchEntry.FindMember("path");
                        if (path != patchEntry.MemberEnd())
                        {
                            AZ::Dom::Path domPath(path->value.GetString());
                            PrefabOverrideMetadata overrideMetadata(AZStd::move(patchEntry), i);
                            m_linkPatchesTree.SetValue(domPath, AZStd::move(overrideMetadata));
                        }
                        */
                    }
                    else  // regular patch
                    {
                        PrefabDom patchEntry;
                        patchEntry.CopyFrom(patchesArray[i], patchEntry.GetAllocator());

                        auto path = patchEntry.FindMember("path");
                        if (path != patchEntry.MemberEnd())
                        {
                            AZ::Dom::Path domPath(path->value.GetString());
                            PrefabOverrideMetadata overrideMetadata(AZStd::move(patchEntry), i);
                            m_linkPatchesTree.SetValue(domPath, AZStd::move(overrideMetadata));
                        }
                    }
                }
            }
        }

        void Link::GetLinkPatches(PrefabDomValue& patchesDom, PrefabDomAllocator& allocator, bool includeNestedLinkDoms, bool expanded) const
        {
            auto cmp = [](const PrefabOverrideMetadata* a, const PrefabOverrideMetadata* b)
            {
                return (a->m_patchIndex < b->m_patchIndex);
            };

            // Use a set to sort the patches based on their patch indices. This will make sure that entities are
            // retrieved from the tree in the same order as they are inserted in.
            AZStd::set<const PrefabOverrideMetadata*, decltype(cmp)> patchesSet(cmp);

            auto visitorFn = [&patchesSet](const AZ::Dom::Path&, const PrefabOverrideMetadata& overrideMetadata)
            {
                patchesSet.emplace(&overrideMetadata);
                return true;
            };

            patchesDom.SetArray();
            m_linkPatchesTree.VisitPath(AZ::Dom::Path(), visitorFn, AZ::Dom::PrefixTreeTraversalFlags::ExcludeExactPath);

            for (auto patchesSetIterator = patchesSet.begin(); patchesSetIterator != patchesSet.end(); ++patchesSetIterator)
            {
                PrefabDomValue patch((*patchesSetIterator)->m_patch, allocator);

                patchesDom.PushBack(patch.Move(), allocator);
            }

            // Process nested link DOMs if needed.
            if (includeNestedLinkDoms)
            {
                for (auto& [patchPath, nestedLinkId] : m_nestedLinks)
                {
                    LinkReference nestedLink = m_prefabSystemComponentInterface->FindLink(nestedLinkId);

                    PrefabDomValue nestedLinkDom;

                    nestedLink->get().GetLinkDomWithNestedLinkDoms(nestedLinkDom, allocator, expanded);  // recursively

                    if (expanded)
                    {
                        ExpandLinkDom(nestedLinkDom, allocator);

                        // Adds the nested link id.
                        AddLinkIdToInstanceDom(nestedLinkDom, nestedLink->get().GetId(), allocator);
                    }

                    PrefabDomValue patch(rapidjson::kObjectType);
                    patch.AddMember(rapidjson::StringRef("op"), rapidjson::StringRef("add"), allocator);
                    rapidjson::Value pathDataValue = rapidjson::Value(patchPath.data(), aznumeric_caster(patchPath.length()), allocator);
                    patch.AddMember(rapidjson::StringRef("path"), pathDataValue.Move(), allocator);
                    patch.AddMember(rapidjson::StringRef("value"), nestedLinkDom.Move(), allocator);

                    patchesDom.PushBack(patch.Move(), allocator);
                }
            }
        }


        PrefabDomConstReference Link::FindOverridePatch(AZ::Dom::Path path, AZ::Dom::PrefixTreeTraversalFlags prefixTreeTraversalFlags)
        {
            PrefabDomConstReference overridePatch = {};
            auto visitorFn = [&overridePatch](AZ::Dom::Path, const PrefabOverrideMetadata& overrideData)
            {
                overridePatch = overrideData.m_patch;

                // We just need to get one override at the provided path.
                // Return false here so that we don't keep looking for all patches at the path.
                return false;
            };

            m_linkPatchesTree.VisitPath(path, visitorFn, prefixTreeTraversalFlags);
            return overridePatch;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
#pragma optimize("", on)
