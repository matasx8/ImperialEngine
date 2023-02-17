#pragma once
#include <chrono>

namespace imp
{
#define LOG_TIMINGS 1
#if LOG_TIMINGS
#define AUTO_TIMER(message) SimpleAutoTimer timer(message); 
#else
#define AUTO_TIMER(message)
#endif
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

    class SimpleAutoTimer
    {
    public:
        SimpleAutoTimer(const char* msg) : m_Timer(), m_Msg(msg)
        {
            m_Timer.start();
        }

        ~SimpleAutoTimer()
        {
            m_Timer.stop();
            printf("%s%lli\n", m_Msg, m_Timer.ms());
        }

    private:
        SimpleTimer m_Timer;
        const char* m_Msg;
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