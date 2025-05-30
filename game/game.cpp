#include "Device/IDevice.h"
#include "Device/IResource.h"
#include "Device/IWindow.h"
#include "math/vector.h"
#include "fileUtils/fileUtils.h"
#include <memory>
#include <cstdio>
#include <filesystem>

static bool areSame(IResource* p1, IResource* p2)
{
    if (p1 == nullptr && p2 == nullptr)
        return true;
    if (p1 == nullptr || p2 == nullptr)
        return false;
    IResource::ResDesc d1, d2;
    p1->getDesc(d1);
    p2->getDesc(d2);
    return d1 == d2;
}

int main()
{
    std::shared_ptr<IDevice> pRenderGPU, pPresentGPU;

    pPresentGPU = IDevice::createD3D12Device(false);  // Use integrated GPU for presentation
    if (!pPresentGPU)
    {
        printf("Failed to create Present device\n");
        return 1;
    }

    pRenderGPU = IDevice::createD3D12Device(false);  // Use discrete GPU for rendering
    if (!pRenderGPU)
    {
        printf("Failed to create Render device\n");
        return 1;
    }
    auto pRenderQueue = pRenderGPU->createQueue();

    uint32_t nSwapChainImages = 4;
    uint2 vPresentRes(3840, 2160);
    auto pWindow = pPresentGPU->createWindow(nSwapChainImages, vPresentRes);
    if (!pWindow)
    {
        printf("Failed to create window\n.");
        return 1;
    }

    auto pSwapChainQueue = pWindow->getQueue();

    std::vector<std::shared_ptr<IResource>> pSrcFrames(nSwapChainImages);

    for (uint32_t uFrame = 0; ; ++uFrame)
    {
        if (!pWindow->pollEvents())
            break;

        auto pDstFrame = pWindow->getNextImage();

        uint32_t uSrcFrame = uFrame % pSrcFrames.size();
        auto& pSrcFrame = pSrcFrames[uSrcFrame];
        if (!areSame(pSrcFrame.get(), pDstFrame.get()))
        {
            IResource::ResDesc desc;
            pDstFrame->getDesc(desc);
            pSrcFrame = pRenderGPU->createResource(desc);

            // load image from file
            char buffer[32];
            sprintf_s(buffer, sizeof(buffer), "media/%d.jpg", uFrame + 1);
            std::filesystem::path sPath;
            if (FileUtils::findTheFileOrFolder(buffer, sPath))
            {
                pSrcFrame->loadFromFile(sPath, pRenderQueue.get());
            }
        }

        auto pCmdList = pSwapChainQueue->startRecording();
        pCmdList->copy(pDstFrame.get(), pSrcFrame.get());

        pWindow->present();
    }

    pRenderQueue->flush();
    pWindow->getQueue()->flush();

    return 0;
}

