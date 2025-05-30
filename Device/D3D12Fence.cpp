#include "D3D12Fence.h"
#include "D3D12Queue.h"
#include <cassert>

void D3D12Fence::signalGpuFenceImpl(IQueue* pQueue, uint64_t value)
{
    assert(pQueue && "Queue cannot be null");
    D3D12Queue* pD3D12Queue = static_cast<D3D12Queue*>(pQueue);
    HRESULT hr = pD3D12Queue->getQueue12()->Signal(m_pFence.Get(), value);
    assert(SUCCEEDED(hr) && "Failed to signal fence");

#ifndef NDEBUG
    updateLastLandedValue(m_pFence->GetCompletedValue());
#endif
}

void D3D12Fence::waitGpuFenceImpl(IQueue* pQueue, uint64_t value)
{
    assert(pQueue && "Queue cannot be null");
    D3D12Queue* pD3D12Queue = static_cast<D3D12Queue*>(pQueue);
    HRESULT hr = pD3D12Queue->getQueue12()->Wait(m_pFence.Get(), value);
    assert(SUCCEEDED(hr) && "Failed to wait for fence");

#ifndef NDEBUG
    updateLastLandedValue(m_pFence->GetCompletedValue());
#endif
}
uint64_t D3D12Fence::getLastLandedValueImpl()
{
    return m_pFence->GetCompletedValue();
}

void D3D12Fence::waitCpuFenceImpl(uint64_t value)
{
    auto completedValue = m_pFence->GetCompletedValue();
    updateLastLandedValue(completedValue);
    if (value > completedValue)
    {
        // Create event for CPU waiting
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(eventHandle != INVALID_HANDLE_VALUE && "Failed to create event");
        
        HRESULT hr = m_pFence->SetEventOnCompletion(value, eventHandle);
        assert(SUCCEEDED(hr) && "Failed to set event on completion");
        
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}