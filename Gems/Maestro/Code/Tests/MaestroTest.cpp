/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <AzCore/Memory/OSAllocator.h>

#include <ISystem.h>

using ::testing::NiceMock;

class MaestroTestEnvironment
    : public AZ::Test::ITestEnvironment
    , public UnitTest::TraceBusRedirector
{
public:
    AZ_TEST_CLASS_ALLOCATOR(MaestroTestEnvironment);

    virtual ~MaestroTestEnvironment()
    {}

protected:

    void SetupEnvironment() override
    {
        m_stubEnv.pCryPak = &pak;
        m_stubEnv.pConsole = &console;
        gEnv = &m_stubEnv;

        BusConnect();
    }

    void TeardownEnvironment() override
    {
        BusDisconnect();
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    NiceMock<CryPakMock> pak;
    NiceMock<ConsoleMock> console;
};

AZ_UNIT_TEST_HOOK(new MaestroTestEnvironment)
