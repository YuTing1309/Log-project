#include "Log.h"

int main()
{
    LoggerFunc logtest("test_log.txt");

    logtest.Debug("test message 1");
    logtest.Error("test message 2");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    logtest.Critical("test message 3");
    logtest.Info("test message 4");
}
