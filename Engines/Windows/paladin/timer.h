// for portable high res timing (c++ 11)
#include <chrono>

class Timer
{
public:
    void start()
    {
        startTime = std::chrono::high_resolution_clock::now();
    }

    long long stop()
    {
        std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::high_resolution_clock::now() - startTime);
        return time.count();
    }

    long long getElapsedMicroSeconds()
    {
        std::chrono::microseconds time = std::chrono::duration_cast<std::chrono::microseconds> (std::chrono::high_resolution_clock::now() - startTime);
        return time.count();
    }

private:
    std::chrono::high_resolution_clock::time_point startTime;
    
};
