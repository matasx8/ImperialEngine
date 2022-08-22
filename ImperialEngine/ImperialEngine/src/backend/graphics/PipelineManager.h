#pragma once
#include "Utils/NonCopyable.h"

namespace imp
{
	class PipelineManager : NonCopyable
	{
	public: 
		PipelineManager();
		void Initialize();



		void Destroy();
	};
}