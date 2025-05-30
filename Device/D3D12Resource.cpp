#include "framework.h"
#include "D3D12Resource.h"
#include "D3D12Queue.h"
#include "IResource.h"
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

D3D12Resource::D3D12Resource(ComPtr<ID3D12Resource> resource)
    : m_resource(resource)
{
}

void D3D12Resource::loadFromFile(const std::filesystem::path& sPath, IQueue* pQueue)
{
    // Load image using STB Image
    int width, height, channels;
    unsigned char* imageData = stbi_load(sPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData)
    {
        assert(false && "Failed to load image");
        return;
    }

    // Create staging buffer using IDevice interface
    IResource::ResDesc stagingDesc;
    stagingDesc.m_format = IResource::eFormatUnknown;  // Use unknown format for staging buffer
    stagingDesc.m_nDims = 1;  // Buffer
    stagingDesc.m_res[0] = width * height * 4;  // RGBA format
    stagingDesc.m_res[1] = 1;
    stagingDesc.m_res[2] = 1;
    stagingDesc.m_isStaging = true;  // Mark as staging resource

    IDevice* pDevice = pQueue->getDevice();
    auto pStagingResource = pDevice->createResource(stagingDesc);
    assert(pStagingResource && "Failed to create staging resource");

    pStagingResource->writeTo((const char *)imageData, width * height * 4);

    // Free the loaded image data
    stbi_image_free(imageData);

    // Get command list from queue
    auto pCmdList = pQueue->startRecording();
    assert(pCmdList && "Failed to get command list from queue");

    // Transition resource to copy destination using ICmdList interface
    pCmdList->barrier(this, eBarrierStateCommon, eBarrierStateCopyDst);

    // Copy data from staging buffer to resource using ICmdList interface
    pCmdList->copyFromStaging(this, pStagingResource.get(), width * 4);

    // Transition resource back to common state using ICmdList interface
    pCmdList->barrier(this, eBarrierStateCopyDst, eBarrierStateCommon);

    // Execute command list
    pQueue->execute(pCmdList);

    pQueue->flush();
}

void D3D12Resource::getDesc(IResource::ResDesc& outDesc)
{
    D3D12_RESOURCE_DESC d3dDesc = m_resource->GetDesc();
    
    // Set format
    switch (d3dDesc.Format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        outDesc.m_format = IResource::eFormatRGBA8;
        break;
    case DXGI_FORMAT_UNKNOWN:
        outDesc.m_format = IResource::eFormatUnknown;
        break;
    default:
        assert(false && "Unsupported format");
        break;
    }

    // Set dimensions
    outDesc.m_nDims = d3dDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? 1 : 
                      d3dDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D ? 1 :
                      d3dDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ? 2 : 3;

    // Set resolution
    outDesc.m_res[0] = static_cast<uint32_t>(d3dDesc.Width);
    outDesc.m_res[1] = d3dDesc.Height;
    outDesc.m_res[2] = d3dDesc.DepthOrArraySize;
}

void D3D12Resource::writeTo(const char* pData, uint32_t nBytes)
{
    // Get resource description to determine size
    D3D12_RESOURCE_DESC desc = m_resource->GetDesc();

    // Calculate resource size based on dimensions
    assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

    // Check if we're trying to write more data than the resource can hold
    if (nBytes > desc.Width)
    {
        assert(false && "Attempting to write more data than the resource can hold");
        return;
    }

    // Map the resource
    void* mappedData = nullptr;
    HRESULT hr = m_resource->Map(0, nullptr, &mappedData);
    if (FAILED(hr))
    {
        assert(false && "Failed to map resource for writing");
        return;
    }

    // Copy all data
    memcpy(mappedData, pData, nBytes);

    // Unmap the resource
    m_resource->Unmap(0, nullptr);
}
