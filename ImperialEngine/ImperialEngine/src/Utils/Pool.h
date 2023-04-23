#pragma once
#include <vector>
#include <type_traits>
#include <backend/VulkanResource.h>

namespace imp
{
	// must provide somekind of factory
	// 
	template<typename T, 
		typename FactoryFunction>
	class PrimitivePool
	{
	public:
		PrimitivePool(FactoryFunction ff)
			: m_FacFun(ff), m_Pool()
		{}

		T Get(VkDevice device, uint64_t currFrame)
		{
			if (m_Pool.size() && currFrame - m_Pool.front().GetLastUsed() > 3) // TODO: provide this number
			{
				T item = m_Pool.back();
				m_Pool.pop_back();
				return item;
			}
			T item = m_FacFun(device);
			return item;
		}

		void Return(T& item)
		{
			m_Pool.push_back(item);
		}

		void Return(T&& item)
		{
			m_Pool.push_back(item);
		}

	private:

		FactoryFunction m_FacFun;
		std::vector<T> m_Pool;
	};
}