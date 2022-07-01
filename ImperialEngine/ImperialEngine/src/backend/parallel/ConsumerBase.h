#pragma once
#include <functional>
#include <memory>
#include <utility>

namespace prl
{
	template <typename T>
	class WorkQ;

	template <typename T> class ConsumerBase
	{
	public:
		ConsumerBase(T& functionOwner, WorkQ<T>& workQ)
			: m_FunctionOwner(functionOwner), m_Q(workQ), m_Alive(true)
		{
		}

		// Signal Consumer to not expect anymore work
		virtual void End() = 0;

		// Join when work is done
		virtual void Join() = 0;

	protected:

		T& m_FunctionOwner;
		WorkQ<T>& m_Q;
		bool m_Alive;
	};
}

