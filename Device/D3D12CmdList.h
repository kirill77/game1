#pragma once

#include "ICmdList.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12CmdList : public ICmdList
{
public:
    D3D12CmdList(ComPtr<ID3D12GraphicsCommandList> cmdList);

    // ICmdList interface
    virtual void barrier(IResource* pResource, eBarrier eStateBefore, eBarrier eStateAfter) override;
    virtual void copy(IResource* pDst, IResource* pSrc) override;
    virtual void copyFromStaging(IResource* pDstTexture2D, IResource* pSrcBuffer, uint32_t nSrcBytesPerRow) override;

    // Getter for the underlying D3D12 command list
    ID3D12GraphicsCommandList* getCmdList() const { return m_cmdList.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList> m_cmdList;
};

