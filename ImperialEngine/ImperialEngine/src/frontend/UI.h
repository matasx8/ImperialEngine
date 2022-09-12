#pragma once
#include "Utils/NonCopyable.h"
#include <extern/ENTT/entt.hpp>

namespace imp
{
	class UI : NonCopyable
	{
	public:
		UI();
		void Initialize();

		void Update(entt::registry& reg);

		void Destroy();

	private:
		bool m_ShowDefaultWindow;
	};
}