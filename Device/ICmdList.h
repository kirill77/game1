#pragma once
#include <memory>

enum eBarrier
{
    eBarrierStateCommon = 1,
    eBarrierStateCopyDst = 2,
    eBarrierStateCopySrc = 3
};

struct IResource;

struct ICmdList : public std::enable_shared_from_this<ICmdList>
{
    virtual void barrier(IResource* pResource, eBarrier eStateBefore, eBarrier eStateAfter) = 0;
    virtual void copyFromStaging(IResource* pDstTexture2D, IResource* pSrcBuffer, uint32_t nSrcBytesPerRow) = 0;
    virtual void copy(IResource* pDst, IResource* pSrc) = 0;
};
