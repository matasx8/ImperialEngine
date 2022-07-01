#pragma once
#include <functional>
#include <vector>
#include <list>
#include <mutex>
#include <memory>
#include <utility>

namespace prl
{
	struct CmdRsc // Base command resource
	{	};

	template <typename T> class WorkQ
	{
	public:
		inline WorkQ() :m_Mutex(), m_CV(), m_Q()
		{
		}

		virtual inline void add(std::_Mem_fn<void (T::*)(std::shared_ptr<CmdRsc>)> item, std::shared_ptr<CmdRsc> rsc) {
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Q.emplace_back(item, rsc);
			m_CV.notify_one();
		}

		virtual inline std::pair<std::_Mem_fn<void (T::*)(std::shared_ptr<CmdRsc>)>, std::shared_ptr<CmdRsc>> remove() {
			std::unique_lock<std::mutex> lock(m_Mutex);
			if (m_Q.size() == 0)
				m_CV.wait(lock);

			auto item = m_Q.front();
			m_Q.pop_front();
			return item;
		}

		virtual inline size_t size() {
			std::unique_lock<std::mutex> lock(m_Mutex);
			return m_Q.size();
		}
	protected:
		std::mutex m_Mutex;
		std::condition_variable m_CV;
		std::list<std::pair<std::_Mem_fn<void (T::*)(std::shared_ptr<CmdRsc>)>, std::shared_ptr<CmdRsc>>> m_Q;
	};
}

