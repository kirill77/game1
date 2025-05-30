#include "framework.h"
#include "D3D12Window.h"
#include "D3D12Device.h"
#include "D3D12Queue.h"
#include "D3D12Resource.h"
#include "math/vector.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

std::shared_ptr<IWindow> D3D12Window::create(D3D12Device* pDevice, uint32_t nSwapChainImages)
{
    // Tell Windows this application is DPI-aware and doesn't need DPI scaling
    SetProcessDPIAware();

    // Get desktop dimensions
    uint2 vRes;
    vRes.x = GetSystemMetrics(SM_CXSCREEN);
    vRes.y = GetSystemMetrics(SM_CYSCREEN);

    // Create window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = D3D12Window::WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"D3D12WindowClass";
    RegisterClassExW(&wc);

    // Create borderless window
    HWND hwnd = CreateWindowExW(
        0,
        L"D3D12WindowClass",
        L"D3D12 Window",
        WS_POPUP,
        0, 0,  // Position at top-left
        vRes.x, vRes.y,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (!hwnd)
    {
        return nullptr;
    }

    // Create swap chain with tearing support
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = vRes.x;
    swapChainDesc.Height = vRes.y;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = nSwapChainImages;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    auto pQueue = pDevice->createQueue(L"PresentQueue");
    auto pQueue12 = dynamic_cast<D3D12Queue*>(pQueue.get());
    assert(pQueue12);

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = pDevice->getFactory()->CreateSwapChainForHwnd(
        pQueue12->getQueue12(),  // Use the D3D12 command queue
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr))
    {
        return nullptr;
    }

    // Create window instance
    auto window = std::make_shared<D3D12Window>();
    window->m_hwnd = hwnd;
    
    // Query for IDXGISwapChain3
    ComPtr<IDXGISwapChain3> swapChain3;
    hr = swapChain1.As(&swapChain3);
    if (FAILED(hr))
    {
        return nullptr;
    }
    window->m_swapChain = swapChain3;
    
    window->m_pDevice = pDevice->shared_from_this();  // Get a safe shared_ptr to the device
    window->m_pQueue = pQueue;  // Store the queue
    window->m_currentImageIndex = 0;  // Initialize the current image index
    window->m_shouldClose = false;

    // Store window pointer in HWND user data for access in WindowProc
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window.get()));

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    return window;
}

std::shared_ptr<IResource> D3D12Window::getNextImage()
{
    // Get the next back buffer index
    m_currentImageIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Get the back buffer resource
    ComPtr<ID3D12Resource> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(m_currentImageIndex, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr))
    {
        return nullptr;
    }

    return std::make_shared<D3D12Resource>(backBuffer);
}

void D3D12Window::present()
{
    // Present the swap chain with no vsync (0) and allow tearing
    HRESULT hr = m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    assert(SUCCEEDED(hr) && "Failed to present swap chain");
}

LRESULT CALLBACK D3D12Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get the window instance from user data
    D3D12Window* pWindow = reinterpret_cast<D3D12Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CLOSE:
        if (pWindow)
        {
            pWindow->m_shouldClose = true;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (pWindow)
            {
                pWindow->m_shouldClose = true;
            }
        }
        return 0;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

bool D3D12Window::pollEvents()
{
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Return true if window should continue running, false if it should close
    return !m_shouldClose;
}

