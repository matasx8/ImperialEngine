#pragma once
#include <vector>
#include "Utils/NonCopyable.h"

namespace imp
{
	template<typename T>
	class DrawData
	{
	public:

		inline DrawData()
			: m_Size(0), m_Idx(0), m_Storage()
		{
		}

		inline void Add(const T& item)
		{
			if (m_Idx >= m_Storage.capacity())
			{
				m_Storage.push_back(item);
			}
			else
			{
				m_Storage[m_Idx] = item;
			}
			m_Idx++;
			m_Size++;
		}

		inline void Add(T&& item)
		{
			if (m_Idx >= m_Storage.capacity())
			{
				m_Storage.emplace_back(std::move(item));
			}
			else
			{
				m_Storage[m_Idx] = std::move(item);
			}
			m_Idx++;
			m_Size++;
		}

		inline void Reset()
		{
			m_Idx = 0;
			m_Size = 0;
		}

	private:
		uint32_t m_Size;
		uint32_t m_Idx;
		std::vector<T> m_Storage;
	};


}