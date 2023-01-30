#pragma once
#include <chrono>

namespace imp
{

	class SimpleTimer
	{
        typedef std::int_fast64_t i64;

    public:
        void start()
        {
            start_time = std::chrono::steady_clock::now();
        }

        void stop()
        {
            elapsed_time = std::chrono::steady_clock::now() - start_time;
        }

        i64 ms() const
        {
            return (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time)).count();
        }

        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_time = std::chrono::duration<double>::zero();

	};

    struct Timings
    {
        SimpleTimer frameWorkTime;
        SimpleTimer waitTime;
        SimpleTimer totalFrameTime;

        void StartAll()
        {
            frameWorkTime.start();
            totalFrameTime.start();
        }
    };
}