#pragma once

#include "Component.hpp"
#include "Actor.hpp"
#include "Global.hpp"
#include "Timer.hpp"

namespace Velvet
{
	class Collider : public Component
	{
	public:
		int sphereOrPlane = 0;
		glm::vec3 lastPos;
		glm::vec3 velocity;
		glm::mat4 curTransform;
		glm::mat4 lastTransform;

		Collider(bool _sphereOrPlane)
		{
			name = __func__;
			sphereOrPlane = _sphereOrPlane;
		}

		void Start() override
		{
			lastPos = actor->transform->position;

			curTransform = actor->transform->matrix();
			lastTransform = curTransform;
		}

		void FixedUpdate() override
		{
			auto curPos = actor->transform->position;
			velocity = (curPos - lastPos) / Timer::fixedDeltaTime();
			lastPos = actor->transform->position;

			lastTransform = curTransform;
			curTransform = actor->transform->matrix();
		}

		virtual glm::vec3 ComputeSDF(glm::vec3 position)
		{
			if (sphereOrPlane)
			{
				return ComputePlaneSDF(position);
			}
			else
			{
				return ComputeSphereSDF(position);
			}
		}

		virtual glm::vec3 ComputePlaneSDF(glm::vec3 position)
		{
			if (position.y < Global::simParams.collisionMargin)
			{
				return glm::vec3(0, Global::simParams.collisionMargin - position.y, 0);
			}
			return glm::vec3(0);
		}

		virtual glm::vec3 ComputeSphereSDF(glm::vec3 position)
		{
			auto mypos = actor->transform->position;
			float radius = actor->transform->scale.x + Global::simParams.collisionMargin;

			auto diff = position - mypos;
			float distance = glm::length(diff);
			if (distance < radius)
			{
				auto direction = diff / distance;
				return (radius - distance) * direction;
			}
			return glm::vec3(0);
		}
	};
}