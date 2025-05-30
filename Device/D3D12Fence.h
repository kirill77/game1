#pragma once

#include "IFence.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Queue;

class D3D12Fence : public IFence
{
public:
    D3D12Fence(ComPtr<ID3D12Fence> fence) : m_pFence(fence) {}
    ID3D12Fence* getFence() const { return m_pFence.Get(); }

private:
    // IFence interface implementation
    virtual void signalGpuFenceImpl(IQueue* pQueue, uint64_t value) override;
    virtual void waitGpuFenceImpl(IQueue* pQueue, uint64_t value) override;
    virtual uint64_t getLastLandedValueImpl() override;
    virtual void waitCpuFenceImpl(uint64_t value) override;

    ComPtr<ID3D12Fence> m_pFence;
};

