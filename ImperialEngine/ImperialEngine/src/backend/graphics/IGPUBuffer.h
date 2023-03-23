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
		IGPUBuffer() : m_NumElements(), m_MemoryPtr(), m_WritePointer() {}

		// sets the number of elements and moves the write pointer to next write (next index is size)
		void resize(size_t size, size_t elementSize)
		{
			m_NumElements = size;
			m_WritePointer = static_cast<char*>(m_MemoryPtr);
			m_WritePointer += size * elementSize;
		}

		void push_back(const void* data, size_t size)
		{
			assert(data);
			assert(size);
			std::memcpy(m_WritePointer, data, size);
			m_WritePointer += size;
			m_NumElements++;
		}

		void insert(size_t index, const void* data, size_t elementSize)
		{
			assert(data);
			assert(elementSize);
			auto wptr = static_cast<char*>(m_MemoryPtr);
			wptr += index * elementSize;
			std::memcpy(wptr, data, elementSize);
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

		void* GetRawMappedBufferPointer()
		{
			return m_MemoryPtr;
		}

		virtual void MapWholeBuffer(VkDevice device) = 0;

	protected:
		size_t m_NumElements;
		void* m_MemoryPtr;
		char* m_WritePointer;
	};
}
