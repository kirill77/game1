#pragma once

#include "IQueue.hpp"
#include "D3D12Device.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class D3D12Queue : public IQueue
{
public:
    D3D12Queue(D3D12Device* pDevice, const std::wstring &sName);
    virtual std::shared_ptr<ICmdList> startRecording() override;
    virtual void execute(std::shared_ptr<ICmdList> pCmdList) override;
    virtual void flush() override;

    ID3D12CommandQueue* getQueue12() const { return m_pQueue.Get(); }

private:
    ComPtr<ID3D12CommandQueue> m_pQueue;
    ComPtr<ID3D12CommandAllocator> m_pCurAlloc;
    ComPtr<ID3D12CommandAllocator> m_pOtherAlloc;
    std::shared_ptr<IFence> m_pAllocFence;
    uint64_t m_otherAllocLastFenceValue = 0;
}; 