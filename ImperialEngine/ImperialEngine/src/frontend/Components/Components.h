#include <GLM/mat4x4.hpp>
#include <ENTT/entt.hpp>

namespace Comp
{
	struct Transform
	{
		glm::mat4x4 transform;
	};

	struct Material
	{

	};

	struct Mesh
	{
		entt::entity e;	// vbo created by backend
	};
}