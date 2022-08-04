#pragma once
#include "Utils/NonCopyable.h"


namespace imp
{
	class UI : NonCopyable
	{
	public:
		UI();
		void Initialize();

		void Update();

		void Destroy();

	private:

	};
}