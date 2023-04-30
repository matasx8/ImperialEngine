#pragma once
#include "Utils/NonCopyable.h"
#include <extern/ENTT/entt.hpp>

namespace imp
{
	class Engine;
	class CircularFrameTimeRowContainer;

	class UI : NonCopyable
	{
	public:
		UI();
		void Initialize();

		void Update(Engine& engine, entt::registry& reg, CircularFrameTimeRowContainer& stats);

		void Destroy();

	private:
		bool m_ShowDefaultWindow;
	};
}