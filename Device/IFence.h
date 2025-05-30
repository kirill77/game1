#pragma once
#include <memory>
#include <assert.h>

struct IQueue;

struct IFence : public std::enable_shared_from_this<IFence>
{
    inline void signalGpuFence(IQueue* pQueue, uint64_t value)
    {
        signalGpuFenceImpl(pQueue, value);

        uint64_t prevValue = m_lastSignalledValue.exchange(value);
        assert(value > prevValue); // must be monotonous
    }
    inline uint64_t getLastSignalledValue() const
    {
        return m_lastSignalledValue.load();
    }
    inline void waitGpuFence(IQueue* pQueue, uint64_t value)
    {
        if (value <= m_lastLandedValue)
            return;
        assert(value <= m_lastSignalledValue); // should not wait if not signalled
        waitGpuFenceImpl(pQueue, value);
    }
    inline uint64_t getLastLandedValue()
    {
        uint64_t value = getLastLandedValueImpl();
        updateLastLandedValue(value);
        return value;
    }
    inline void waitCpuFence(uint64_t value)
    {
        if (value <= m_lastLandedValue)
            return;
        assert(value <= m_lastSignalledValue); // should not wait if not signalled
        waitCpuFenceImpl(value);
        updateLastLandedValue(value);
    }

protected:
    virtual void signalGpuFenceImpl(IQueue* pQueue, uint64_t value) = 0;
    virtual void waitGpuFenceImpl(IQueue* pQueue, uint64_t value) = 0;
    virtual uint64_t getLastLandedValueImpl() = 0;
    virtual void waitCpuFenceImpl(uint64_t value) = 0;

    inline void updateLastLandedValue(uint64_t value)
    {
        uint64_t prevValue = m_lastLandedValue.exchange(value);
        assert(value >= prevValue); // must be monotonous
    }

private:
    std::atomic<uint64_t> m_lastSignalledValue = 0, m_lastLandedValue = 0;
};