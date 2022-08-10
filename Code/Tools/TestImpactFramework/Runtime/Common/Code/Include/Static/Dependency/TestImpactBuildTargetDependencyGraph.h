/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTargetList.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    //!
    template<typename ProductionTarget, typename TestTarget>
    struct BuildTargetDependencyGraphNode;
    
    //!
    template<typename ProductionTarget, typename TestTarget>
    using BuildTargetDependencyList = AZStd::vector<const BuildTargetDependencyGraphNode<ProductionTarget, TestTarget>*>;
    
    //!
    template<typename ProductionTarget, typename TestTarget>
    struct BuildTargetDependencies
    {
        BuildTargetDependencyList<ProductionTarget, TestTarget> m_build; //!<
        BuildTargetDependencyList<ProductionTarget, TestTarget> m_runtime; //!<
    };
    
    //!
    template<typename ProductionTarget, typename TestTarget>
    struct BuildTargetDependencyGraphNode
    {
        BuildTargetDependencyGraphNode(BuildTarget<ProductionTarget, TestTarget> buildTarget)
            : m_buildTarget(buildTarget)
        {
        }

        BuildTarget<ProductionTarget, TestTarget> m_buildTarget; //!<
        BuildTargetDependencies<ProductionTarget, TestTarget> m_dependencies; //!<
    };
    
    //!
    template<typename ProductionTarget, typename TestTarget>
    class BuildTargetDependencyGraph
    {
    public:
    
        //!
        BuildTargetDependencyGraph(const BuildTargetList<ProductionTarget, TestTarget>& buildTargetList);
    
        //!
        //const BuildTargetDependencies<ProductionTarget, TestTarget> GetBuildTargetDependencies(
        //    const BuildTarget<ProductionTarget, TestTarget>& buildTarget) const;
    
    private:
        AZStd::unordered_map<BuildTarget<ProductionTarget, TestTarget>, BuildTargetDependencyGraphNode<ProductionTarget, TestTarget>>
            m_buildTargetDependencyMap;
    };
    
    template<typename ProductionTarget, typename TestTarget>
    BuildTargetDependencyGraph<ProductionTarget, TestTarget>::BuildTargetDependencyGraph(
        [[maybe_unused]] const BuildTargetList<ProductionTarget, TestTarget>& buildTargetList)
    {
        for (const auto& buildTarget : buildTargetList.GetBuildTargets())
        {
            const auto addOrRetrieveNode = [this](const BuildTarget<ProductionTarget, TestTarget>& buildTarget)
            {
                auto it = m_buildTargetDependencyMap.find(buildTarget);
                if (it == m_buildTargetDependencyMap.end())
                {
                    return &m_buildTargetDependencyMap
                                .emplace(
                                    AZStd::piecewise_construct, AZStd::forward_as_tuple(buildTarget), AZStd::forward_as_tuple(buildTarget))
                                .first->second;
                }
                else
                {
                    return &it->second;
                }
            };

            auto* node = addOrRetrieveNode(buildTarget);
            for (const auto& buildDependency : buildTarget.GetTarget()->GetDependencies().m_build)
            {
                const auto buildDependencyTarget = buildTargetList.GetBuildTarget(buildDependency);
                if (!buildDependencyTarget.has_value())
                {
                    AZ_Warning(
                        "BuildTargetDependencyGraph",
                        false,
                        "Couldn't find build dependency '%s' for build target '%s'",
                        buildDependency.c_str(),
                        buildTarget.GetTarget()->GetName().c_str());
                    continue;
                }

                const auto buildDependencyNode = addOrRetrieveNode(buildDependencyTarget.value());
                node->m_dependencies.m_build.push_back(buildDependencyNode);
            }
        }
    }
} // namespace TestImpact

//namespace AZStd
//{
//    //! Hash function for BuildTarget types for use in maps and sets.
//    template<typename ProductionTarget, typename TestTarget>
//    struct hash<TestImpact::BuildTargetDependencyGraphNode<ProductionTarget, TestTarget>>
//    {
//        size_t operator()(
//            const TestImpact::BuildTargetDependencyGraphNode<ProductionTarget, TestTarget>& node) const noexcept
//        {
//            return reinterpret_cast<size_t>(node.m_buildTarget.GetTarget());
//        }
//    };
//} // namespace AZStd
