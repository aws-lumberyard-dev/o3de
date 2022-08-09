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
    using BuildTargetDependencyList = AZStd::vector<BuildTargetDependencyGraphNode<ProductionTarget*, TestTarget>>;

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
        const BuildTargetList<ProductionTarget, TestTarget>& buildTargetList)
    {
        for (const auto& buildTarget : buildTargetList.GetBuildTargets())
        {
            auto& node = m_buildTargetDependencyMap[buildTarget];
            for (const auto& buildDependency : buildTarget.GetTarget().GetDependencies().m_build)
            {

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
