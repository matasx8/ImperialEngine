#pragma once
#include <stdio.h>
#include <type_traits>
//#include "Representable.h"

namespace Debug
{
	//// Debug Log information about Representable object
	//// T must derive from Representable
	//template <typename T,
	//	typename = typename std::enable_if<std::is_base_of<Representable, T>::value>::type>
	//void Log(T obj)
	//{
	//	size_t size = obj.GetRepresentCstrLen();
	//	char* buff = (char*)alloca(size);
	//	obj.RepresentCstr(buff, size);
	//	printf("[Debug %s]: %s", typeid(obj).name(), buff);
	//}

	// dt - delta time in seconds
	static void FrameInfo(float dt)
	{
		float dtms = dt * 10000;// a bit less than mili
		int fps = 1 / (((dtms > 1) ? floor(dtms) : 1.0f) / 10000);
		printf("[Debug Frame Info] length: %.1fms FPS: %d\n", dtms / 10, fps);
	}

	// Logs a message that is null terminated
	// TODO: make it like printf;
	static void LogMsg(const char* msg)
	{
		printf("[Debug Message] %s\n", msg);
	}

	/*template void Log<Representable>()
	{

	}*/
}