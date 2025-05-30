#include "framework.h"
#include "D3D12Device.h"
#include "D3D12Window.h"
#include "D3D12Resource.h"
#include "D3D12Queue.h"
#include "D3D12Fence.h"
#include "IResource.h"
#include <vector>
#include <cassert>

namespace {
    DXGI_FORMAT convertFormat(IResource::eFormat format)
    {
        switch (format)
        {
        case IResource::eFormatUnknown:
            return DXGI_FORMAT_UNKNOWN;
        case IResource::eFormatRGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            assert(false && "Unsupported format");
            return DXGI_FORMAT_UNKNOWN;
        }
    }
}

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

std::shared_ptr<IDevice> IDevice::createD3D12Device(bool bUseIntegratedGpu)
{
    return std::make_shared<D3D12Device>(bUseIntegratedGpu);
}

D3D12Device::D3D12Device(bool bUseIntegratedGpu)
{
    // Create DXGI Factory
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_pDxgiFactory));
    assert(SUCCEEDED(hr) && "Failed to create DXGI Factory");

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> pAdapter;
    ComPtr<IDXGIAdapter1> pSelectedAdapter;
    DXGI_ADAPTER_DESC1 desc{};
    SIZE_T minDedicatedMemory = SIZE_MAX;
    SIZE_T maxDedicatedMemory = 0;

    // First pass: find min and max dedicated memory
    for (UINT i = 0; m_pDxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        pAdapter->GetDesc1(&desc);
        if (desc.DedicatedVideoMemory == 0)
            continue;
        minDedicatedMemory = std::min(minDedicatedMemory, desc.DedicatedVideoMemory);
        maxDedicatedMemory = std::max(maxDedicatedMemory, desc.DedicatedVideoMemory);
        pAdapter = nullptr;
    }

    // Second pass: select the appropriate adapter
    for (UINT i = 0; m_pDxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        pAdapter->GetDesc1(&desc);
        
        // If we want integrated GPU, select the one with least dedicated memory
        // If we want discrete GPU, select the one with most dedicated memory
        if ((bUseIntegratedGpu && desc.DedicatedVideoMemory == minDedicatedMemory) ||
            (!bUseIntegratedGpu && desc.DedicatedVideoMemory == maxDedicatedMemory))
        {
            pSelectedAdapter = pAdapter;
            break;
        }
        
        pAdapter = nullptr;
    }

    if (!pSelectedAdapter)
    {
        printf("Error: Failed to find suitable adapter\n");
        assert(false);
        return;
    }

    pSelectedAdapter->GetDesc1(&desc);

#ifdef _DEBUG
    // Enable the D3D12 debug layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    hr = D3D12CreateDevice(pSelectedAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
    
    if (SUCCEEDED(hr))
    {
        printf("Device created on the %s adapter: %S (Dedicated Memory: %zu MB)\n", 
            bUseIntegratedGpu ? "integrated" : "discrete", 
            desc.Description,
            desc.DedicatedVideoMemory / (1024 * 1024));
    }
    else
    {
        printf("Error: Failed to create device on the adapter: %S\n", desc.Description);
        assert(false);
    }

    m_sDesc = desc.Description;
}

std::shared_ptr<IWindow> D3D12Device::createWindow(uint32_t nSwapChainImages)
{
    return D3D12Window::create(this, nSwapChainImages);
}

std::shared_ptr<IQueue> D3D12Device::createQueue(const std::wstring &sName)
{
    return std::make_shared<D3D12Queue>(this, sName);
}

std::shared_ptr<IResource> D3D12Device::createResource(const IResource::ResDesc& desc)
{
    // Create resource description
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = desc.m_nDims == 1 ? D3D12_RESOURCE_DIMENSION_BUFFER :
                            desc.m_nDims == 2 ? D3D12_RESOURCE_DIMENSION_TEXTURE2D :
                            D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    resourceDesc.Width = desc.m_res[0];
    resourceDesc.Height = desc.m_res[1];
    resourceDesc.DepthOrArraySize = desc.m_res[2];
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = convertFormat(desc.m_format);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    if (desc.m_isShared || desc.m_isStaging)
    {
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    }
    else
    {
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    }
    resourceDesc.Flags = desc.m_isShared ?
        D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER : D3D12_RESOURCE_FLAG_NONE;
    resourceDesc.Alignment = 65536;

    // Create the resource
    ComPtr<ID3D12Resource> resource;
    D3D12_HEAP_PROPERTIES heapProps = {
        desc.m_isStaging ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0, 0
    };

    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
    if (desc.m_isShared)
    {
        heapFlags |= D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED;
    }

    HRESULT hr = m_pDevice->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&resource)
    );

    if (FAILED(hr))
    {
        assert(false);
        return nullptr;
    }

    return std::make_shared<D3D12Resource>(resource);
}

std::shared_ptr<IResource> D3D12Device::createSharedResource(std::shared_ptr<IDevice> pOtherDevice, std::shared_ptr<IResource> pResource)
{
    // Cast to D3D12 types to access underlying objects
    auto pOtherD3D12Device = std::dynamic_pointer_cast<D3D12Device>(pOtherDevice);
    if (!pOtherD3D12Device)
    {
        assert(false && "Invalid device type for sharing");
        return nullptr;
    }
    if (pOtherD3D12Device->m_pDevice == this->m_pDevice)
    {
        // if it's the same device - no need to share
        return pResource;
    }

    auto pD3D12Resource = std::dynamic_pointer_cast<D3D12Resource>(pResource);
    if (!pD3D12Resource)
    {
        assert(false && "Invalid resource type for sharing");
        return nullptr;
    }

    // Create a shared handle from the source resource
    HANDLE sharedHandle = nullptr;
    HRESULT hr = pOtherD3D12Device->getDevice()->CreateSharedHandle(
        pD3D12Resource->getResource(),
        nullptr,  // security attributes
        GENERIC_ALL,  // access
        nullptr,  // name
        &sharedHandle
    );

    if (FAILED(hr))
    {
        assert(false && "Failed to create shared handle");
        return nullptr;
    }

    // Open the shared handle on this device
    ComPtr<ID3D12Resource> sharedResource;
    hr = m_pDevice->OpenSharedHandle(
        sharedHandle,
        IID_PPV_ARGS(&sharedResource)
    );

    // Close the handle as it's no longer needed
    CloseHandle(sharedHandle);

    if (FAILED(hr))
    {
        assert(false && "Failed to open shared handle");
        return nullptr;
    }

    return std::make_shared<D3D12Resource>(sharedResource);
}

std::shared_ptr<IFence> D3D12Device::createFence()
{
    ComPtr<ID3D12Fence> fence;
    HRESULT hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr))
    {
        return nullptr;
    }
    return std::make_shared<D3D12Fence>(fence);
}
