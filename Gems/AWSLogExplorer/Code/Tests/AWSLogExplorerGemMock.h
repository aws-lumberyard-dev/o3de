#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
namespace AWSLogExplorer
{
    class AWSLogExplorerGemAllocatorFixture : public UnitTest::ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            // Set up the file IO and alias
            // m_localFileIO = aznew AZ::IO::LocalFileIO();
            // m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            m_localFileIO->SetAlias("@devroot@", engineRoot.c_str());
        }
        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

    public:
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
    };
} // namespace AWSLogExplorer
