#pragma once

#include "IWindow.h"
#include "D3D12Device.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Window : public IWindow
{
public:
    static std::shared_ptr<IWindow> create(D3D12Device* pDevice, uint32_t nSwapChainImages);
    virtual std::shared_ptr<IResource> getNextImage() override;
    virtual void present() override;
    virtual bool pollEvents() override;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd;
    ComPtr<IDXGISwapChain3> m_swapChain;
    uint32_t m_currentImageIndex;
    bool m_shouldClose;
};

