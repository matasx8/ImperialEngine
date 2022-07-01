#pragma once
#include <thread>
#include "ConsumerBase.h"

namespace prl
{
	template <typename T> class ConsumerThread : ConsumerBase<T>
	{
	public:
		ConsumerThread(T& functionOwner, WorkQ<T>& workQ)
			: ConsumerBase<T>(functionOwner, workQ), m_Thread(&ConsumerThread::WorkLoop, this)
		{}

		void End()
		{
			this->m_Alive = false;
		}

		void Join()
		{
			this->m_Thread.join();
		}

	private:
		void WorkLoop()
		{
			while (this->m_Alive || this->m_Q.size())
			{
				auto funcNdata = this->m_Q.remove();
				funcNdata.first(this->m_FunctionOwner, funcNdata.second);
			}
		}

		std::thread m_Thread;
	};
}

