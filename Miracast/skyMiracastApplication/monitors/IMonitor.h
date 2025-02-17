#ifndef __IMONITOR_H__
#define __IMONITOR_H__

#include <memory>
#include <thread>

class IMonitor
{
public:
    virtual ~IMonitor()
    {
        stop();
    }

    virtual bool start() = 0;
    virtual void stop() {};

protected:
    virtual void run() = 0;

    void startWorkerThread()
    {
        mThread = std::make_shared<std::thread>(&IMonitor::run, this);
    }

    void waitForWorkerThread()
    {
        if (mThread)
        {
            mThread->join();
        }
    }

private:
    std::shared_ptr<std::thread> mThread;
};

#endif // __IMONITOR_H__