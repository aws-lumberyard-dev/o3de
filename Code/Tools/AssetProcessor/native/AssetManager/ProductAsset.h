#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/SystemFile.h>

namespace AssetProcessor
{
    //! Represents a single product asset file, either in the cache or the intermediate directory
    class ProductAsset
    {
    public:
        ProductAsset(const AssetBuilderSDK::JobProduct& jobProduct, AZ::IO::Path absolutePath);

        bool IsValid() const;
        AZ::IO::Path GetProperCaseAbsolutePath() const;
        bool ExistsOnDisk(bool printErrorMessage) const;
        bool DeleteFile() const;

    protected:
        const AssetBuilderSDK::JobProduct& m_product;
        const AZ::IO::Path m_absolutePath;
    };

    //! Represents a single job output, which itself can be either a cache product, intermediate product, or both
    class ProductAssetWrapper
    {
    public:
        ProductAssetWrapper(const AssetBuilderSDK::JobProduct& jobProduct, const AssetUtilities::ProductPath& productPath);

        bool IsValid() const;
        bool ExistOnDisk() const;
        bool DeleteFiles() const;

    protected:
        AZStd::fixed_vector<AZStd::unique_ptr<ProductAsset>, 2> m_products;
    };

    inline ProductAsset::ProductAsset(const AssetBuilderSDK::JobProduct& jobProduct, AZ::IO::Path absolutePath)
        : m_product(jobProduct)
        , m_absolutePath(AZStd::move(absolutePath))
    {

    }

    inline bool ProductAsset::IsValid() const
    {
        return AZ::IO::PathView(m_product.m_productFileName).IsRelative() && ExistsOnDisk(false);
    }

    inline bool ProductAsset::ExistsOnDisk(bool printErrorMessage) const
    {
        bool exists = AZ::IO::SystemFile::Exists(m_absolutePath.c_str());

        if(!exists && printErrorMessage)
        {
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel, "Was expecting product asset to exist at `%s` but it was not found\n",
                m_absolutePath.c_str());
        }

        return exists;
    }

    inline bool ProductAsset::DeleteFile() const
    {
        if(!ExistsOnDisk(false))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was expecting to delete product file %s but it already appears to be gone.\n",
                m_absolutePath.c_str());
            return false;
        }
        else if(!AZ::IO::SystemFile::Delete(m_absolutePath.c_str()))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was unable to delete product file %s\n", m_absolutePath.c_str());
            return false;
        }

        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Deleted product file %s\n", m_absolutePath.c_str());
        return true;
    }

    inline ProductAssetWrapper::ProductAssetWrapper(
        const AssetBuilderSDK::JobProduct& jobProduct,
        const AssetUtilities::ProductPath& productPath)
    {
        if((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::ProductAsset) ==
            AssetBuilderSDK::ProductOutputFlags::ProductAsset)
        {
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(jobProduct, productPath.m_cachePath));
        }

        if ((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::IntermediateAsset) ==
            AssetBuilderSDK::ProductOutputFlags::IntermediateAsset)
        {
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(jobProduct, productPath.m_intermediatePath));
        }
    }

    inline bool ProductAssetWrapper::IsValid() const
    {
        return AZStd::all_of(m_products.begin(), m_products.end(), [](const AZStd::unique_ptr<ProductAsset>& productAsset)
        {
            return productAsset && productAsset->IsValid();
        });
    }

    inline bool ProductAssetWrapper::ExistOnDisk() const
    {
        return AZStd::all_of(m_products.begin(), m_products.end(), [](const AZStd::unique_ptr<ProductAsset>& productAsset)
        {
            return productAsset->ExistsOnDisk(true);
        });
    }

    inline bool ProductAssetWrapper::DeleteFiles() const
    {
        return AZStd::all_of(m_products.begin(), m_products.end(), [](const AZStd::unique_ptr<ProductAsset>& productAsset)
        {
            return productAsset->DeleteFile();
        });
    }
}
