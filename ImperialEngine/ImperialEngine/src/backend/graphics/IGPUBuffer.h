#pragma once
#include <cassert>
#include <cstring>

namespace imp
{
	// Very simple interface for host-visible gpu buffers
	// Assumes element size is always the same, be careful
	class IGPUBuffer
	{
	public:
		IGPUBuffer() : m_NumElements(), m_MemoryPtr() {}

		void resize(size_t size)
		{
			m_NumElements = size;
		}

		void push_back(const void* data, size_t size)
		{
			assert(data);
			assert(size);
			const auto offset = m_NumElements * size;
			char* tempPtr = reinterpret_cast<char*>(m_MemoryPtr);
			tempPtr += offset;
			std::memcpy(tempPtr, data, size);
			m_NumElements++;
		}

		size_t size() const
		{
			return m_NumElements;
		}

		// We're tracking if this memmory is currently mapped by the fact if it's not pointing to null
		bool IsMemoryMappedByHost() const 
		{ 
			return m_MemoryPtr != nullptr; 
		}

		const void* GetRawMappedBufferPointer() const 
		{ 
			return m_MemoryPtr; 
		}

		virtual void MapWholeBuffer(VkDevice device) = 0;

	protected:
		size_t m_NumElements;
		void* m_MemoryPtr;
	};
}
