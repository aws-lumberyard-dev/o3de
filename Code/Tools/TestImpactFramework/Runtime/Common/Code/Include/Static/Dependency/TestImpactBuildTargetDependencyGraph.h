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
    template<typename ProductionTarget, typename TestTarget>
    class BuildTargetDependencyGraph
    {
    public:
        struct BuildTargetDependencyList
        {
        };

        BuildTargetDependencyGraph(const BuildTargetList<ProductionTarget, TestTarget>* buildTargetList);

    private:
    };

    template<typename ProductionTarget, typename TestTarget>
    BuildTargetDependencyGraph<ProductionTarget, TestTarget>::BuildTargetDependencyGraph(
        const BuildTargetList<ProductionTarget, TestTarget>* buildTargets)
    {
    }
} // namespace TestImpact
