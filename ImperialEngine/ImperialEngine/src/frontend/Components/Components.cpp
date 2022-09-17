#pragma once
#include "Components.h"

namespace Comp
{
	glm::vec3 Transform::GetPosition() const
	{
		return glm::vec3(transform[3]);
	}

	glm::vec4& Transform::GetPosition()
	{
		return transform[3];
	}
}