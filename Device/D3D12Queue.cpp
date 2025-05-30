#include "framework.h"
#include "D3D12Queue.h"
#include "D3D12CmdList.h"
#include <cassert>

D3D12Queue::D3D12Queue(D3D12Device* pDevice, const std::wstring &sName)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr = pDevice->getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pQueue));
    assert(SUCCEEDED(hr) && "Failed to create command queue");

    // Set the name on the D3D12 command queue
    if (!sName.empty())
    {
        m_pQueue->SetName(sName.c_str());
    }

    // Create command allocators
    hr = pDevice->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCurAlloc));
    assert(SUCCEEDED(hr) && "Failed to create current command allocator");

    hr = pDevice->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pOtherAlloc));
    assert(SUCCEEDED(hr) && "Failed to create other command allocator");

    // Create fence for allocator tracking
    m_pAllocFence = pDevice->createFence();
    assert(m_pAllocFence && "Failed to create allocator fence");

    m_pDevice = pDevice->shared_from_this();
}

std::shared_ptr<ICmdList> D3D12Queue::startRecording()
{
    // if the other alloc is not used anymore - we can make it current
    if (m_pAllocFence->getLastLandedValue() >= m_otherAllocLastFenceValue)
    {
        std::swap(m_pCurAlloc, m_pOtherAlloc);
        m_pCurAlloc->Reset();
        m_otherAllocLastFenceValue = m_pAllocFence->getLastSignalledValue();
    }

    // Create new command list using m_pCurAlloc
    ComPtr<ID3D12GraphicsCommandList> pCmdList;
    HRESULT hr = static_cast<D3D12Device*>(m_pDevice.get())->getDevice()->CreateCommandList(
        0,                          // node mask
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pCurAlloc.Get(),          // command allocator
        nullptr,                    // initial pipeline state
        IID_PPV_ARGS(&pCmdList)
    );
    assert(SUCCEEDED(hr) && "Failed to create command list");

    return std::make_shared<D3D12CmdList>(pCmdList);
}

void D3D12Queue::execute(std::shared_ptr<ICmdList> pCmdList)
{
    assert(pCmdList && "Command list cannot be null");
    D3D12CmdList* pD3D12CmdList = static_cast<D3D12CmdList*>(pCmdList.get());

    // Close the command list
    HRESULT hr = pD3D12CmdList->getCmdList()->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list");

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = { pD3D12CmdList->getCmdList() };
    m_pQueue->ExecuteCommandLists(1, ppCommandLists);

    // Signal the fence to track this command list's completion
    m_pAllocFence->signalGpuFence(this, m_pAllocFence->getLastSignalledValue() + 1);
}

void D3D12Queue::flush()
{
    // Wait for all GPU work to complete by waiting for the fence
    m_pAllocFence->waitCpuFence(m_pAllocFence->getLastSignalledValue());
} 