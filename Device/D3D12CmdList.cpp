#include "framework.h"
#include "D3D12CmdList.h"
#include "D3D12Resource.h"
#include "IResource.h"
#include <cassert>

namespace {
    D3D12_RESOURCE_STATES convertState(eBarrier state)
    {
        switch (state)
        {
        case eBarrierStateCommon:
            return D3D12_RESOURCE_STATE_COMMON;
        case eBarrierStateCopyDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case eBarrierStateCopySrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        default:
            assert(false && "Unsupported barrier state");
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }
}

D3D12CmdList::D3D12CmdList(ComPtr<ID3D12GraphicsCommandList> cmdList)
    : m_cmdList(cmdList)
{
}

void D3D12CmdList::barrier(IResource* pResource, eBarrier eStateBefore, eBarrier eStateAfter)
{
    auto pD3D12Resource = dynamic_cast<D3D12Resource*>(pResource);
    assert(pD3D12Resource && "Failed to cast to D3D12 resource");

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = pD3D12Resource->getResource();
    barrier.Transition.StateBefore = convertState(eStateBefore);
    barrier.Transition.StateAfter = convertState(eStateAfter);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);
}

void D3D12CmdList::copy(IResource* pDst, IResource* pSrc)
{
    auto pD3D12Dst = dynamic_cast<D3D12Resource*>(pDst);
    assert(pD3D12Dst && "Failed to cast destination to D3D12 resource");

    auto pD3D12Src = dynamic_cast<D3D12Resource*>(pSrc);
    assert(pD3D12Src && "Failed to cast source to D3D12 resource");

    m_cmdList->CopyResource(pD3D12Dst->getResource(), pD3D12Src->getResource());
}

void D3D12CmdList::copyFromStaging(IResource* pDstTexture2D, IResource* pSrcBuffer, uint32_t nSrcBytesPerRow)
{
    auto pD3D12Texture = dynamic_cast<D3D12Resource*>(pDstTexture2D);
    assert(pD3D12Texture && "Failed to cast texture to D3D12 resource");

    auto pD3D12Buffer = dynamic_cast<D3D12Resource*>(pSrcBuffer);
    assert(pD3D12Buffer && "Failed to cast buffer to D3D12 resource");

    // Get resource descriptions to determine dimensions
    IResource::ResDesc textureDesc, bufferDesc;
    pDstTexture2D->getDesc(textureDesc);
    pSrcBuffer->getDesc(bufferDesc);

    uint32_t nBytesPerPixel = IResource::getBytesPerPixel(textureDesc.m_format);
    uint32_t srcWidth = nSrcBytesPerRow / nBytesPerPixel;
    uint32_t srcHeight = bufferDesc.m_res[0] / nSrcBytesPerRow;

    // Set up copy locations
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = pD3D12Texture->getResource();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = pD3D12Buffer->getResource();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = 0;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = srcWidth;
    src.PlacedFootprint.Footprint.Height = srcHeight;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = nSrcBytesPerRow;  // Use provided bytes per row

    m_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}
