#pragma once

#include "IDevice.h"
#include "ICmdList.h"
#include "IFence.h"
#include <memory>

struct IQueue : public std::enable_shared_from_this<IQueue>
{
public:
    virtual ~IQueue() = default;
    virtual std::shared_ptr<ICmdList> startRecording() = 0;
    virtual void execute(std::shared_ptr<ICmdList> pCmdList) = 0;
    virtual void flush() = 0;

    inline IDevice* getDevice() const { return m_pDevice.get(); }

protected:
    std::shared_ptr<IDevice> m_pDevice;
};