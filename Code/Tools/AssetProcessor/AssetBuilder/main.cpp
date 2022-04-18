/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetBuilderApplication.h"
#include "TraceMessageHook.h"
#include "AssetBuilderComponent.h"

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/SystemAllocator.h>

int main(int argc, char** argv)
{
    AssetBuilderApplication app(&argc, &argv);
    AssetBuilder::TraceMessageHook traceMessageHook; // Hook AZ Debug messages and redirect them to stdout
    traceMessageHook.EnableTraceContext(true);
    AZ::Debug::Trace::HandleExceptions(true);

    AZ::ComponentApplication::StartupParameters startupParams;
    startupParams.m_loadDynamicModules = false;
    AzFramework::Application::Descriptor desc;
    desc.m_stackRecordLevels = 25;
    AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
    records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
    app.Start(desc, startupParams);
    AZ::AllocatorManager::Instance().EnterProfilingMode();
    AZ::AllocatorManager::Instance().SetTrackingMode(AZ::Debug::AllocationRecords::RECORD_FULL);
    traceMessageHook.EnableDebugMode(app.IsInDebugMode());

    bool result = false;

    BuilderBus::BroadcastResult(result, &BuilderBus::Events::Run);

    traceMessageHook.EnableTraceContext(false);
    app.Stop();
    
    return result ? 0 : 1;
}
