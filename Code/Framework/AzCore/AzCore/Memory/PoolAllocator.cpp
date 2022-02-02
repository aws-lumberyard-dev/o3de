/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/AllocatorDebug.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/parallel/containers/lock_free_intrusive_stack.h>
#include <AzCore/std/typetraits/alignment_of.h>

#define POOL_ALLOCATION_PAGE_SIZE (4 * 1024)
#define POOL_ALLOCATION_MIN_ALLOCATION_SIZE 8
#define POOL_ALLOCATION_MAX_ALLOCATION_SIZE 512

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Pool Allocation algorithm
    /**
     * Pool Allocation algorithm implementation. Used in both PoolAllocator and ThreadPoolAllocator.
     */
    template<class Allocator>
    class PoolAllocation
    {
    public:
        using PageType = typename Allocator::Page;
        using BucketType = typename Allocator::Bucket;
        using value_type = typename Allocator::value_type;
        using pointer = typename Allocator::pointer;
        using size_type = typename Allocator::size_type;
        using difference_type = typename Allocator::difference_type;
        using align_type = typename Allocator::align_type;
        using propagate_on_container_copy_assignment = typename Allocator::propagate_on_container_copy_assignment;
        using propagate_on_container_move_assignment = typename Allocator::propagate_on_container_move_assignment;

        PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        virtual ~PoolAllocation();

        pointer allocate(size_type byteSize, align_type alignment = 1);
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0);
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const;

        void Merge([[maybe_unused]] IAllocator* aOther)
        {
        }

        // if isForceFreeAllPages is true we will free all pages even if they have allocations in them.
        void GarbageCollect(bool isForceFreeAllPages = false);

        Allocator* m_allocator;
        size_t m_pageSize;
        size_t m_minAllocationShift;
        size_t m_minAllocationSize;
        size_t m_maxAllocationSize;
        size_t m_numBuckets;
        BucketType* m_buckets;
        size_t m_numBytesAllocated;
    };

    template<class Allocator>
    PoolAllocation<Allocator>::PoolAllocation(Allocator* allocator, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
        : m_allocator(allocator)
        , m_pageSize(pageSize)
        , m_numBytesAllocated(0)
    {
        AZ_Assert(m_allocator->m_pageAllocator, "We need the page allocator setup!");
        AZ_Assert(
            pageSize >= maxAllocationSize * 4,
            "We need to fit at least 4 objects in a pool! Increase your page size! Page %d MaxAllocationSize %d", pageSize,
            maxAllocationSize);
        AZ_Assert(
            minAllocationSize == maxAllocationSize || ((minAllocationSize) & (minAllocationSize - 1)) == 0,
            "Min allocation should be either equal to max allocation size or power of two");

        m_minAllocationSize = AZ::GetMax(minAllocationSize, size_t(8));
        m_maxAllocationSize = AZ::GetMax(maxAllocationSize, minAllocationSize);

        m_minAllocationShift = 0;
        for (size_t i = 1; i < sizeof(unsigned int) * 8; i++)
        {
            if (m_minAllocationSize >> i == 0)
            {
                m_minAllocationShift = i - 1;
                break;
            }
        }

        AZ_Assert(
            m_maxAllocationSize % m_minAllocationSize == 0,
            "You need to be able to divide m_maxAllocationSize (%d) / m_minAllocationSize (%d) without fraction!", m_maxAllocationSize,
            m_minAllocationSize);
        m_numBuckets = m_maxAllocationSize / m_minAllocationSize;
        AZ_Assert(m_numBuckets <= 0xffff, "You can't have more than 65535 number of buckets! We need to increase the index size!");
        m_buckets = reinterpret_cast<BucketType*>(
            m_allocator->m_pageAllocator->allocate(sizeof(BucketType) * m_numBuckets,  alignof(BucketType)));
        for (size_t i = 0; i < m_numBuckets; ++i)
        {
            new (m_buckets + i) BucketType();
        }
    }

    template<class Allocator>
    PoolAllocation<Allocator>::~PoolAllocation()
    {
        GarbageCollect(true);

        for (size_t i = 0; i < m_numBuckets; ++i)
        {
            m_buckets[i].~BucketType();
        }
        m_allocator->m_pageAllocator->deallocate(m_buckets, sizeof(BucketType) * m_numBuckets);
    }

    template<class Allocator>
    typename PoolAllocation<Allocator>::pointer PoolAllocation<Allocator>::allocate(size_type byteSize, align_type alignment)
    {
        AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
        AZ_Assert(
            static_cast<size_t>(alignment) > 0 && (static_cast<size_t>(alignment) & (static_cast<size_t>(alignment) - 1)) == 0,
            "Alignment must be >0 and power of 2!");

        // pad the size to the min allocation size.
        byteSize = AZ::SizeAlignUp(byteSize, m_minAllocationSize);
        byteSize = AZ::SizeAlignUp(byteSize, alignment);

        if (byteSize > m_maxAllocationSize)
        {
            AZ_Assert(false, "Allocation size (%d) is too big (max: %d) for pools!", byteSize, m_maxAllocationSize);
            return nullptr;
        }

        u32 bucketIndex = static_cast<u32>((byteSize >> m_minAllocationShift) - 1);
        BucketType& bucket = m_buckets[bucketIndex];
        PageType* page = nullptr;
        if (!bucket.m_pages.empty())
        {
            page = &bucket.m_pages.front();

            // check if we have free slot in the page
            if (page->m_freeList.empty())
            {
                page = nullptr;
            }
            else if (page->m_freeList.size() == 1)
            {
                // if we have only 1 free slot this allocation will
                // fill the page, so put in on the back
                bucket.m_pages.pop_front();
                bucket.m_pages.push_back(*page);
            }
        }
        if (!page)
        {
            page = m_allocator->PopFreePage();
            if (page)
            {
                // We have any pages available on free page stack.
                if (page->m_bin != bucketIndex) // if this page was used the same bucket we are ready to roll.
                {
                    size_t elementSize = byteSize;
                    size_t pageDataSize = m_pageSize - sizeof(PageType);
                    page->SetupFreeList(elementSize, pageDataSize);
                    page->m_bin = bucketIndex;
                    page->m_elementSize = static_cast<u32>(elementSize);
                    page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
                }
            }
            else
            {
                // We need to align each page on it's size, this way we can quickly find which page the pointer belongs to.
                page = m_allocator->ConstructPage(byteSize);
                page->m_bin = bucketIndex;
            }
            bucket.m_pages.push_front(*page);
        }

        // The data address and the fake node address are shared.
        void* address = &page->m_freeList.front();
        page->m_freeList.pop_front();

        m_numBytesAllocated += byteSize;

        return address;
    }

    //=========================================================================
    // DeAllocate
    // [9/09/2009]
    //=========================================================================
    template<class Allocator>
    void PoolAllocation<Allocator>::deallocate(pointer ptr, [[maybe_unused]] size_type byteSize, [[maybe_unused]] align_type alignment)
    {
        PageType* page = m_allocator->PageFromAddress(ptr);
        if (page == nullptr)
        {
            AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
            return;
        }

        // (pageSize - info struct at the end) / (element size)
        size_t maxElementsPerBucket = page->m_maxNumElements;

        size_t numFreeNodes = page->m_freeList.size();
        typename PageType::FakeNode* node = new (ptr) typename PageType::FakeNode();
        page->m_freeList.push_front(*node);

        if (numFreeNodes == 0)
        {
            // if the page was full before sort at the front
            BucketType& bucket = m_buckets[page->m_bin];
            bucket.m_pages.erase(*page);
            bucket.m_pages.push_front(*page);
        }
        else if (numFreeNodes == maxElementsPerBucket - 1)
        {
            // push to the list of free pages
            BucketType& bucket = m_buckets[page->m_bin];
            PageType* frontPage = &bucket.m_pages.front();
            if (frontPage != page)
            {
                bucket.m_pages.erase(*page);
                // check if the front page is full if so push the free page to the front otherwise push
                // push it on the free pages list so it can be reused by other bins.
                if (frontPage->m_freeList.empty())
                {
                    bucket.m_pages.push_front(*page);
                }
                else
                {
                    m_allocator->PushFreePage(page);
                }
            }
            else if (frontPage->m_next != nullptr)
            {
                // if the next page has free slots free the current page
                if (frontPage->m_next->m_freeList.size() < maxElementsPerBucket)
                {
                    bucket.m_pages.erase(*page);
                    m_allocator->PushFreePage(page);
                }
            }
        }

        m_numBytesAllocated -= page->m_elementSize;
    }

    template<class Allocator>
    typename PoolAllocation<Allocator>::pointer PoolAllocation<Allocator>::reallocate([[maybe_unused]] pointer ptr, [[maybe_unused]] size_type newSize, [[maybe_unused]] align_type alignment)
    {
        return nullptr;
    }

    template<class Allocator>
    typename PoolAllocation<Allocator>::size_type PoolAllocation<Allocator>::get_allocated_size(pointer ptr, [[maybe_unused]] align_type alignment) const
    {
        PageType* page = m_allocator->PageFromAddress(ptr);
        return page->m_elementSize;
    }

    template<class Allocator>
    void PoolAllocation<Allocator>::GarbageCollect(bool isForceFreeAllPages)
    {
        // Free empty pages in the buckets (or better be empty)
        for (unsigned int i = 0; i < (unsigned int)m_numBuckets; ++i)
        {
            // (pageSize - info struct at the end) / (element size)
            size_t maxElementsPerBucket = (m_pageSize - sizeof(PageType)) / ((i + 1) << m_minAllocationShift);

            typename BucketType::PageListType& pages = m_buckets[i].m_pages;
            while (!pages.empty())
            {
                PageType& page = pages.front();
                pages.pop_front();
                if (page.m_freeList.size() == maxElementsPerBucket || isForceFreeAllPages)
                {
                    m_allocator->FreePage(&page);
                }
            }
        }
    }

    /**
     * PoolSchema Private Implementation... to keep the header clean.
     */
    class PoolSchemaPimpl : public IAllocatorWithTracking
    {
    public:
        AZ_RTTI(OSAllocator, "{21B5B147-0FD8-4A67-9A29-9AEAA6763740}", IAllocatorWithTracking)

        PoolSchemaPimpl(IAllocator* subAllocator);
        ~PoolSchemaPimpl() override;

        pointer allocate(size_type byteSize, align_type alignment = 1) override;
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        void Merge([[maybe_unused]] IAllocator* aOther) override
        {
        }

        /**
         * We allocate memory for pools in pages. Page is a information struct
         * located at the end of the allocated page. When it's in the at the end
         * we can usually hide it's size in the free bytes left from the pagesize/poolsize.
         * \note IMPORTANT pages are aligned on the page size, this way can find quickly which
         * pool the pointer belongs to.
         */
        struct Page : public AZStd::intrusive_list_node<Page>
        {
            struct FakeNode : public AZStd::intrusive_slist_node<FakeNode>
            {
            };

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            u32 m_bin;
            Debug::Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
         * A bucket has a list of pages used with the specific pool size.
         */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType m_pages;
        };

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void PushFreePage(Page* page);
        void GarbageCollect();
        inline Page* ConstructPage(size_t elementSize)
        {
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->allocate(m_pageSize, static_cast<align_type>(m_pageSize)));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new (memBlock + pageDataSize) Page();
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->deallocate(memBlock, m_pageSize);
        }

        inline Page* PageFromAddress(void* address)
        {
            if (!address)
            {
                return nullptr;
            }
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~(m_pageSize - 1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        using AllocatorType = PoolAllocation<PoolSchemaPimpl>;
        IAllocator* m_pageAllocator;
        AllocatorType m_allocator;
        size_t m_pageSize;
        Bucket::PageListType m_freePages;
    };

    PoolSchemaPimpl::PoolSchemaPimpl(IAllocator* subAllocator)
        : m_pageAllocator(subAllocator)
        , m_allocator(this, POOL_ALLOCATION_PAGE_SIZE, POOL_ALLOCATION_MIN_ALLOCATION_SIZE, POOL_ALLOCATION_MAX_ALLOCATION_SIZE)
        , m_pageSize(POOL_ALLOCATION_PAGE_SIZE)
    {
    }

    PoolSchemaPimpl::~PoolSchemaPimpl()
    {
        // Force free all pages
        m_allocator.GarbageCollect(true);

        // Free all unused memory
        GarbageCollect();
    }

    PoolSchemaPimpl::pointer PoolSchemaPimpl::allocate(size_type byteSize, align_type alignment)
    {
        return m_allocator.allocate(byteSize, alignment);
    }

    void PoolSchemaPimpl::deallocate(pointer ptr, size_type byteSize, align_type alignment)
    {
        m_allocator.deallocate(ptr, byteSize, alignment);
    }

    PoolSchemaPimpl::pointer PoolSchemaPimpl::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
        return m_allocator.reallocate(ptr, newSize, alignment);
    }

    PoolSchemaPimpl::size_type PoolSchemaPimpl::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return m_allocator.get_allocated_size(ptr, alignment);
    }

    AZ_FORCE_INLINE PoolSchemaPimpl::Page* PoolSchemaPimpl::PopFreePage()
    {
        Page* page = nullptr;
        if (!m_freePages.empty())
        {
            page = &m_freePages.front();
            m_freePages.pop_front();
        }
        return page;
    }

    AZ_INLINE void PoolSchemaPimpl::PushFreePage(Page* page)
    {
        m_freePages.push_front(*page);
    }

    void PoolSchemaPimpl::GarbageCollect()
    {
        // if( m_ownerThread == AZStd::this_thread::get_id() )
        {
            m_allocator.GarbageCollect();

            while (!m_freePages.empty())
            {
                Page* page = &m_freePages.front();
                m_freePages.pop_front();
                FreePage(page);
            }
        }
    }

    AZ_FORCE_INLINE void PoolSchemaPimpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
    {
        char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
        m_freeList.clear();
        // setup free list
        size_t numElements = pageDataBlockSize / elementSize;
        for (unsigned int i = 0; i < numElements; ++i)
        {
            char* address = pageData + i * elementSize;
            Page::FakeNode* node = new (address) Page::FakeNode();
            m_freeList.push_back(*node);
        }
    }

    IAllocator* CreatePoolAllocatorPimpl(IAllocator& subAllocator)
    {
        PoolSchemaPimpl* allocatorMemory =
            reinterpret_cast<PoolSchemaPimpl*>(subAllocator.allocate(sizeof(PoolSchemaPimpl), alignof(PoolSchemaPimpl)));
        return new (allocatorMemory) PoolSchemaPimpl(&subAllocator);
    }

    void DestroyPoolAllocatorPimpl(IAllocator& subAllocator, IAllocator* allocator)
    {
        PoolSchemaPimpl* allocatorImpl = azrtti_cast<PoolSchemaPimpl*>(allocator);
        allocatorImpl->~PoolSchemaPimpl();
        subAllocator.deallocate(allocatorImpl, sizeof(PoolSchemaPimpl));
    }

    struct ThreadPoolData;

    /**
     * Thread safe pool allocator.
     */
    class ThreadPoolSchemaPimpl : public IAllocator
    {
    public:
        AZ_RTTI(ThreadPoolSchemaPimpl, "{DCC37F0A-7D5D-410E-BF83-74912D8CE6A6}", IAllocator)

        /**
         * Specialized \ref PoolAllocator::Page page for lock free allocator.
         */
        struct Page : public AZStd::intrusive_list_node<Page>
        {
            Page(ThreadPoolData* threadData)
                : m_threadData(threadData)
            {
            }

            struct FakeNode : public AZStd::intrusive_slist_node<FakeNode>
            {
            };
            // Fake Lock Free node used when we delete an element from another thread.
            struct FakeNodeLF : public AZStd::lock_free_intrusive_stack_node<FakeNodeLF>
            {
            };

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            using FreeListType = AZStd::intrusive_slist<FakeNode, AZStd::slist_base_hook<FakeNode>>;

            FreeListType m_freeList;
            AZStd::lock_free_intrusive_stack_node<Page> m_lfStack; ///< Lock Free stack node
            ThreadPoolData* m_threadData; ///< The thread data that own's the page.
            u32 m_bin;
            Debug::Magic32 m_magic;
            u32 m_elementSize;
            u32 m_maxNumElements;
        };

        /**
         * A bucket has a list of pages used with the specific pool size.
         */
        struct Bucket
        {
            using PageListType = AZStd::intrusive_list<Page, AZStd::list_base_hook<Page>>;
            PageListType m_pages;
        };

        ThreadPoolSchemaPimpl(IAllocator* subAllocator);
        ~ThreadPoolSchemaPimpl();

        pointer allocate(size_type byteSize, align_type alignment = 1) override;
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        void Merge([[maybe_unused]] IAllocator* aOther) override
        {
        }

        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect();
        //////////////////////////////////////////////////////////////////////////

        // Functions used by PoolAllocation template
        AZ_INLINE Page* PopFreePage();
        AZ_INLINE void PushFreePage(Page* page);
        inline Page* ConstructPage(size_t elementSize)
        {
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->allocate(m_pageSize, static_cast<align_type>(m_pageSize)));
            size_t pageDataSize = m_pageSize - sizeof(Page);
            Page* page = new (memBlock + pageDataSize) Page(m_threadData);
            page->SetupFreeList(elementSize, pageDataSize);
            page->m_elementSize = static_cast<u32>(elementSize);
            page->m_maxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - m_pageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->deallocate(memBlock, m_pageSize);
        }

        inline Page* PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~static_cast<size_t>(m_pageSize - 1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        inline const Page* PageFromAddress(void* address) const
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~static_cast<size_t>(m_pageSize - 1));
            memBlock += m_pageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->m_magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        // NOTE we allow only one instance of a allocator, you need to subclass it for different versions.
        // so for now it's safe to use static. We use TLS on a static because on some platforms set thread key is available
        // only for pthread lib and we don't use it. I can't find other way to it, otherwise please switch this to
        // use TlsAlloc/TlsFree etc.
        static AZ_THREAD_LOCAL ThreadPoolData* m_threadData;

        // Fox X64 we push/pop pages using the m_mutex to sync. Pages are
        using FreePagesType = Bucket::PageListType;
        FreePagesType m_freePages;
        AZStd::vector<ThreadPoolData*> m_threads; ///< Array with all separate thread data. Used to traverse end free elements.

        IAllocator* m_pageAllocator;
        size_t m_pageSize;
        size_t m_minAllocationSize;
        size_t m_maxAllocationSize;
        // TODO rbbaklov Changed to recursive_mutex from mutex for Linux support.
        AZStd::recursive_mutex m_mutex;
    };

    AZ_THREAD_LOCAL ThreadPoolData* ThreadPoolSchemaPimpl::m_threadData = nullptr;

    struct ThreadPoolData
    {
        ThreadPoolData(ThreadPoolSchemaPimpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        ~ThreadPoolData();

        using AllocatorType = PoolAllocation<ThreadPoolSchemaPimpl>;
        /**
         * Stack with freed elements from other threads. We don't need stamped stack since the ABA problem can not
         * happen here. We push from many threads and pop from only one (we don't push from it).
         */
        using FreedElementsStack = AZStd::lock_free_intrusive_stack<
            ThreadPoolSchemaPimpl::Page::FakeNodeLF,
            AZStd::lock_free_intrusive_stack_base_hook<ThreadPoolSchemaPimpl::Page::FakeNodeLF>>;

        AllocatorType m_allocator;
        FreedElementsStack m_freedElements;
    };
   
    ThreadPoolSchemaPimpl::ThreadPoolSchemaPimpl(IAllocator* subAllocator)
        : m_pageAllocator(subAllocator)
        , m_pageSize(POOL_ALLOCATION_PAGE_SIZE)
        , m_minAllocationSize(POOL_ALLOCATION_MIN_ALLOCATION_SIZE)
        , m_maxAllocationSize(POOL_ALLOCATION_MAX_ALLOCATION_SIZE)
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
        // In memory allocation case (usually tools) we might have high contention,
        // using spin lock will improve performance.
        SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#endif
    }

    ThreadPoolSchemaPimpl::~ThreadPoolSchemaPimpl()
    {
        // clean up all the thread data.
        // IMPORTANT: We assume/rely that all threads (except the calling one) are or will
        // destroyed before you create another instance of the pool allocation.
        // This should generally be ok since the all allocators are singletons.
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            if (!m_threads.empty())
            {
                for (size_t i = 0; i < m_threads.size(); ++i)
                {
                    if (m_threads[i])
                    {
                        // Force free all pages
                        ThreadPoolData* threadData = m_threads[i];
                        threadData->~ThreadPoolData();
                        m_pageAllocator->deallocate(threadData, sizeof(ThreadPoolData), static_cast<align_type>(AZStd::alignment_of<ThreadPoolData>::value));
                    }
                }

                /// reset the variable for the owner thread.
                m_threadData = nullptr;
            }
        }

        GarbageCollect();
    }

    ThreadPoolSchemaPimpl::pointer ThreadPoolSchemaPimpl::allocate(size_type byteSize, align_type alignment)
    {
        ThreadPoolData* threadData = m_threadData;

        if (threadData == nullptr)
        {
            void* threadPoolData = m_pageAllocator->allocate(sizeof(ThreadPoolData), static_cast<align_type>(AZStd::alignment_of<ThreadPoolData>::value));
            threadData = new(threadPoolData) ThreadPoolData(this, m_pageSize, m_minAllocationSize, m_maxAllocationSize);
            m_threadData = threadData;
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                m_threads.push_back(threadData);
            }
        }
        else
        {
            // deallocate elements if they were freed from other threads
            Page::FakeNodeLF* fakeLFNode;
            while ((fakeLFNode = threadData->m_freedElements.pop()) != nullptr)
            {
                threadData->m_allocator.deallocate(fakeLFNode, 0);
            }
        }

        return threadData->m_allocator.allocate(byteSize, alignment);
    }

    void ThreadPoolSchemaPimpl::deallocate(pointer ptr, size_type byteSize, align_type alignment)
    {
        Page* page = PageFromAddress(ptr);
        if (page == nullptr)
        {
            AZ_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
            return;
        }
        AZ_Assert(page->m_threadData != nullptr, ("We must have valid page thread data for the page!"));
        ThreadPoolData* threadData = m_threadData;
        if (threadData == page->m_threadData)
        {
            // we can free here
            threadData->m_allocator.deallocate(ptr, byteSize, alignment);
        }
        else
        {
            // push this element to be deleted from it's own thread!
            // cast the pointer to a fake lock free node
            Page::FakeNodeLF* fakeLFNode = reinterpret_cast<Page::FakeNodeLF*>(ptr);
#ifdef AZ_DEBUG_BUILD
            // we need to reset the fakeLFNode because we share the memory.
            // otherwise we will assert the node is in the list
            fakeLFNode->m_next = 0;
#endif
            page->m_threadData->m_freedElements.push(*fakeLFNode);
        }
    }

    ThreadPoolSchemaPimpl::pointer ThreadPoolSchemaPimpl::reallocate([[maybe_unused]] pointer ptr, [[maybe_unused]] size_type newSize, [[maybe_unused]] align_type alignment)
    {
        return nullptr;
    }

    ThreadPoolSchemaPimpl::size_type ThreadPoolSchemaPimpl::get_allocated_size(pointer ptr, [[maybe_unused]] align_type alignment) const
    {
        const Page* page = PageFromAddress(ptr);
        return page->m_elementSize;
    }

    AZ_INLINE ThreadPoolSchemaPimpl::Page* ThreadPoolSchemaPimpl::PopFreePage()
    {
        Page* page;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            if (m_freePages.empty())
            {
                page = nullptr;
            }
            else
            {
                page = &m_freePages.front();
                m_freePages.pop_front();
            }
        }
        if (page)
        {
#ifdef AZ_DEBUG_BUILD
            AZ_Assert(page->m_threadData == 0, "If we stored the free page properly we should have null here!");
#endif
            // store the current thread data, used when we free elements
            page->m_threadData = m_threadData;
        }
        return page;
    }

    AZ_INLINE void ThreadPoolSchemaPimpl::PushFreePage(Page* page)
    {
#ifdef AZ_DEBUG_BUILD
        page->m_threadData = 0;
#endif
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_freePages.push_front(*page);
        }
    }

    void ThreadPoolSchemaPimpl::GarbageCollect()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
        while (!m_freePages.empty())
        {
            Page* page = &m_freePages.front();
            m_freePages.pop_front();
            FreePage(page);
        }
    }

    inline void ThreadPoolSchemaPimpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
    {
        char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
        m_freeList.clear();
        // setup free list
        size_t numElements = pageDataBlockSize / elementSize;
        for (size_t i = 0; i < numElements; ++i)
        {
            char* address = pageData + i * elementSize;
            Page::FakeNode* node = new (address) Page::FakeNode();
            m_freeList.push_back(*node);
        }
    }

    ThreadPoolData::ThreadPoolData(ThreadPoolSchemaPimpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
        : m_allocator(alloc, pageSize, minAllocationSize, maxAllocationSize)
    {
    }

    //=========================================================================
    // ThreadPoolData::~ThreadPoolData
    // [9/15/2009]
    //=========================================================================
    ThreadPoolData::~ThreadPoolData()
    {
        // deallocate elements if they were freed from other threads
        ThreadPoolSchemaPimpl::Page::FakeNodeLF* fakeLFNode;
        while ((fakeLFNode = m_freedElements.pop()) != nullptr)
        {
            m_allocator.deallocate(fakeLFNode, 0);
        }
    }

    IAllocator* CreateThreadPoolAllocatorPimpl(IAllocator& subAllocator)
    {
        ThreadPoolSchemaPimpl* allocatorMemory = reinterpret_cast<ThreadPoolSchemaPimpl*>(
            subAllocator.allocate(sizeof(ThreadPoolSchemaPimpl), AZStd::alignment_of<ThreadPoolSchemaPimpl>::value));
        return new (allocatorMemory) ThreadPoolSchemaPimpl(&subAllocator);
    }

    void DestroyThreadPoolAllocatorPimpl(IAllocator& subAllocator, IAllocator* allocator)
    {
        ThreadPoolSchemaPimpl* allocatorImpl = azrtti_cast<ThreadPoolSchemaPimpl*>(allocator);
        allocatorImpl->~ThreadPoolSchemaPimpl();
        subAllocator.deallocate(allocatorImpl, sizeof(ThreadPoolSchemaPimpl));
    }

} // namespace AZ
