#pragma once
#include <tuple>
#include "EngineStaticConfig.h"
#include <vector>
#include <algorithm>

namespace imp
{
	struct FrameTimeRow
	{
        FrameTimeRow()
            : cull(-1.0)
            , frameMainCPU(-1.0)
            , frameRenderCPU(-1.0)
            , frameGPU(-1.0)
            , frame(-1.0)
            , triangles(-1)
        {}

#if BENCHMARK_MODE
		double cull;
		double frameMainCPU;
		double frameRenderCPU;
		double frameGPU;
		double frame;
		int64_t triangles;
#else
        float cull;
        float frameMainCPU;
        float frameRenderCPU;
        float frameGPU;
        float frame;
        float triangles;
#endif
	};

	struct FrameTimeTable
	{
        FrameTimeTable()
            : table_rows()
        {}

		std::vector<FrameTimeRow> table_rows;
	};

    class CircularFrameTimeRowContainer
    {
    public:
        explicit CircularFrameTimeRowContainer(size_t capacity) :
            m_Capacity(capacity),
            m_Buffer(capacity),
            m_Size(0)
        {}

        FrameTimeRow GetMaximumValues()
        {
            FrameTimeRow maxValues;

            for (size_t i = 0; i < m_Size; ++i) {
                const auto& row = m_Buffer[i];
                maxValues.cull = std::max(maxValues.cull, row.cull);
                maxValues.frameMainCPU = std::max(maxValues.frameMainCPU, row.frameMainCPU);
                maxValues.frameRenderCPU = std::max(maxValues.frameRenderCPU, row.frameRenderCPU);
                maxValues.frameGPU = std::max(maxValues.frameGPU, row.frameGPU);
                maxValues.frame = std::max(maxValues.frame, row.frame);
                maxValues.triangles = std::max(maxValues.triangles, row.triangles);
            }

            return maxValues;
        }

        FrameTimeRow GetAverageValues()
        {
            FrameTimeRow avgValues;

            if (m_Size == 0) {
                return avgValues;
            }

            for (size_t i = 0; i < m_Size; ++i) {
                const auto& row = m_Buffer[i];
                avgValues.cull += row.cull > 0.0f ? row.cull : avgValues.cull / i + 1; // quick hack to not count average for values we dont have yet
                avgValues.frameMainCPU += row.frameMainCPU;
                avgValues.frameRenderCPU += row.frameRenderCPU;
                avgValues.frameGPU += row.frameGPU > 0.0f ? row.frameGPU : avgValues.frameGPU / i + 1;
                avgValues.frame += row.frame;
                avgValues.triangles += row.triangles > 0.0f ? row.triangles : avgValues.triangles / i + 1;
            }

            avgValues.cull /= m_Size;
            avgValues.frameMainCPU /= m_Size;
            avgValues.frameRenderCPU /= m_Size;
            avgValues.frameGPU /= m_Size;
            avgValues.frame /= m_Size;
            avgValues.triangles /= m_Size;

            return avgValues;
        }

        void push_back(const FrameTimeRow& row)
        {
            if (m_Size == m_Capacity)
            {
                // Shift all elements to the left by one, overwriting the first element
                std::rotate(m_Buffer.begin(), m_Buffer.begin() + 1, m_Buffer.end());
                m_Buffer[m_Capacity - 1] = row;
            }
            else
            {
                m_Buffer[m_Size] = row;
                m_Size++;
            }
        }

        void push_back(FrameTimeRow&& row)
        {
            if (m_Size == m_Capacity)
            {
                // Shift all elements to the left by one, overwriting the first element
                std::rotate(m_Buffer.begin(), m_Buffer.begin() + 1, m_Buffer.end());
                m_Buffer[m_Capacity - 1] = std::move(row);
            }
            else
            {
                m_Buffer[m_Size] = std::move(row);
                m_Size++;
            }
        }

        FrameTimeRow& operator[](size_t index)
        {
            return m_Buffer[index];
        }

        const FrameTimeRow& operator[](size_t index) const
        {
            return m_Buffer[index];
        }

        const FrameTimeRow* data() const
        {
            return m_Buffer.data();
        }

        size_t size() const
        {
            return m_Size;
        }

        bool empty() const
        {
            return size() == 0;
        }

        void set_capacity(size_t capacity)
        {
            if (capacity < m_Capacity)
            {
                // If the new capacity is smaller, truncate the buffer
                m_Size = std::min(m_Size, capacity);
                m_Buffer.resize(capacity);
                m_Capacity = capacity;
            }
            else if (capacity > m_Capacity)
            {
                // If the new capacity is larger, resize the buffer and initialize the new elements
                m_Buffer.resize(capacity);
                for (size_t i = m_Capacity; i < capacity; i++)
                {
                    m_Buffer[i] = FrameTimeRow();
                }
                m_Capacity = capacity;
            }
        }

    private:
        size_t m_Capacity;
        std::vector<FrameTimeRow> m_Buffer;
        size_t m_Size;
    };
}