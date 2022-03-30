#include <iostream>

#include "VtEngine.hpp"
#include "VtClothSolver.hpp"
#include "GameInstance.hpp"
#include "Resource.hpp"
#include "Scene.hpp"
#include "Helper.hpp"

using namespace Velvet;

typedef shared_ptr<Scene> ScenePtr;

class ScenePremitiveRendering : public Scene
{
public:
	ScenePremitiveRendering() { name = "Basic / Premitive Rendering"; }

	void PopulateActors(GameInstance* game) override
	{
		Scene::PopulateCameraAndLight(game);

		//=====================================
		// 3. Objects
		//=====================================

		auto material = Resource::LoadMaterial("_Default");
		{
			material->Use();

			material->SetTexture("material.diffuse", Resource::LoadTexture("wood.png"));
			material->SetTexture("_ShadowTex", game->depthFrameBuffer());
			material->SetBool("material.useTexture", true);
		}

		auto shadowMaterial = Resource::LoadMaterial("_ShadowDepth");

		auto cube1 = game->CreateActor("Sphere");
		{
			auto mesh = Resource::LoadMesh("sphere.obj");
			shared_ptr<MeshRenderer> renderer(new MeshRenderer(mesh, material, shadowMaterial));
			cube1->AddComponent(renderer);
			cube1->transform->position = glm::vec3(0.6f, 2.0f, 0.0);
			cube1->transform->scale = glm::vec3(0.5f);
		}

		auto cube2 = game->CreateActor("Cube2");
		{
			auto mesh = Resource::LoadMesh("cube.obj");
			shared_ptr<MeshRenderer> renderer(new MeshRenderer(mesh, material, shadowMaterial));
			cube2->AddComponent(renderer);
			cube2->transform->position = glm::vec3(2.0f, 0.5, 1.0);
		}

		auto cube3 = game->CreateActor("Cube3");
		{
			auto mesh = Resource::LoadMesh("cube.obj");
			shared_ptr<MeshRenderer> renderer(new MeshRenderer(mesh, material, shadowMaterial));
			cube3->AddComponent(renderer);
			cube3->transform->position = glm::vec3(-1.0f, 0.5, 2.0);
			cube3->transform->scale = glm::vec3(0.5f);
			cube3->transform->rotation = glm::vec3(60, 0, 60);
		}

		game->AddActor(Scene::InfinitePlane(game));
	}
};

class SceneColoredCubes : public Scene
{
public:
	SceneColoredCubes() { name = "Basic / Colored Cubes"; }

	void PopulateActors(GameInstance* game)  override
	{
		Scene::PopulateCameraAndLight(game);

		game->AddActor(Scene::InfinitePlane(game));
		{
			auto whiteCube = Scene::ColoredCube(game, glm::vec3(1.0, 1.0, 1.0));
			whiteCube->Initialize(glm::vec3(0, 0.25, 0),
				glm::vec3(2, 0.5f, 2));
			game->AddActor(whiteCube);
		}

		vector<glm::vec3> colors = {
			glm::vec3(0.0f, 0.5f, 1.0f),
			glm::vec3(0.797f, 0.354f, 0.000f),
			glm::vec3(0.000f, 0.349f, 0.173f),
			glm::vec3(0.875f, 0.782f, 0.051f),
			glm::vec3(0.01f, 0.170f, 0.453f),
			glm::vec3(0.673f, 0.111f, 0.000f),
			glm::vec3(0.612f, 0.194f, 0.394f)
		};

		vector<shared_ptr<Actor>> cubes;
		static vector<glm::vec3> velocities;
		for (int i = 0; i < 50; i++)
		{
			glm::vec3 color = colors[Helper::Random(0, colors.size())];
			auto cube = Scene::ColoredCube(game, color);
			cube->Initialize(glm::vec3(Helper::Random(-3.0f, 3.0f), Helper::Random(0.3f, 0.5f), Helper::Random(-3.0f, 3.0f)), 
				glm::vec3(0.3));
			game->AddActor(cube);
			cubes.push_back(cube);
			velocities.push_back(glm::vec3(0.0));
		}

		game->postUpdate.push_back([cubes, game]() {
			for (int i = 0; i < cubes.size(); i++)
			{
				auto cube = cubes[i];
				//cube->transform->position += Helper::RandomUnitVector() * game->deltaTime * 5.0f;
				velocities[i] = Helper::Lerp(velocities[i], Helper::RandomUnitVector() * 1.0f, game->deltaTime);
				cube->transform->rotation += Helper::RandomUnitVector() * game->deltaTime * 50.0f;
				cube->transform->position += velocities[i] * game->deltaTime * 5.0f;

				if (cube->transform->position.y < 0.07)
				{
					cube->transform->position.y = 0.07;
				}
				if (glm::length(cube->transform->position) > 3)
				{
					cube->transform->position = cube->transform->position / glm::length(cube->transform->position) * 3.0f;
				}
			}
			});
	}
};

class SceneSimpleCloth : public Scene
{
public:
	SceneSimpleCloth() { name = "Cloth / Simple"; }

	void PopulateActors(GameInstance* game)  override
	{
		PopulateCameraAndLight(game);

		game->AddActor(Scene::InfinitePlane(game));

		//auto cube = Scene::ColoredCube(game);
		//cube->transform->scale = glm::vec3(2);

		{
			MaterialProperty materialProperty;
			materialProperty.preRendering = [](Material* mat) {
				mat->SetVec3("material.tint", glm::vec3(1.0));
				mat->SetBool("material.useTexture", false);
			};

			auto material = Resource::LoadMaterial("_Default");
			{
				material->Use();
				material->SetTexture("_ShadowTex", game->depthFrameBuffer());
			}
			auto shadowMaterial = Resource::LoadMaterial("_ShadowDepth");

			auto sphere = game->CreateActor("Sphere");
			auto mesh = Resource::LoadMesh("sphere.obj");
			auto renderer = make_shared<MeshRenderer>(mesh, material, shadowMaterial);
			renderer->SetMaterialProperty(materialProperty);
			sphere->AddComponent(renderer);
			auto collider = make_shared<Collider>();
			sphere->AddComponent(collider);
			float radius = 0.6;
			sphere->Initialize(glm::vec3(0, radius, -2), glm::vec3(radius));

			game->postUpdate.push_back([sphere, game, radius, material]() {
				sphere->transform->position = glm::vec3(0, radius, -cos(game->elapsedTime * 2));
				});
		}

		PopulateCloth(game);
	}

	void PopulateCloth(GameInstance* game, int resolution = 16)
	{
		glm::vec3 color = glm::vec3(0.0f, 0.5f, 1.0f);

		auto material = Resource::LoadMaterial("_Default");
		material->Use();
		material->SetTexture("_ShadowTex", game->depthFrameBuffer());
		material->doubleSided = true;

		MaterialProperty materialProperty;
		materialProperty.preRendering = [color](Material* mat) {
			mat->SetVec3("material.tint", color);
			mat->SetBool("material.useTexture", true);
			mat->SetTexture("material.diffuse", Resource::LoadTexture("fabric.jpg"));
		};


		auto shadowMaterial = Resource::LoadMaterial("_ShadowDepth");

		{
			auto cloth = game->CreateActor("Cloth Generated");
			vector<glm::vec3> vertices;
			vector<glm::vec3> normals;
			vector<glm::vec2> uvs;
			vector<unsigned int> indices;
			const float clothSize = 2.0f;

			for (int y = 0; y <= resolution; y++)
			{
				for (int x = 0; x <= resolution; x++)
				{
					vertices.push_back(clothSize * glm::vec3((float)x / (float)resolution - 0.5f, -(float)y / (float)resolution, 0));
					normals.push_back(glm::vec3(0, 0, 1));
					uvs.push_back(glm::vec2((float)x / (float)resolution, (float)y / (float)resolution));
				}
			}

			auto VertexIndexAt = [resolution](int x, int y) {
				return x * (resolution+1) + y;
			};

			for (int x = 0; x < resolution; x++)
			{
				for (int y = 0; y < resolution; y++)
				{
					indices.push_back(VertexIndexAt(x, y));
					indices.push_back(VertexIndexAt(x + 1, y));
					indices.push_back(VertexIndexAt(x, y + 1));

					indices.push_back(VertexIndexAt(x, y + 1));
					indices.push_back(VertexIndexAt(x + 1, y));
					indices.push_back(VertexIndexAt(x + 1, y + 1));
				}
			}
			auto mesh = make_shared<Mesh>(vertices, normals, uvs, indices);

			auto renderer = make_shared<MeshRenderer>(mesh, material, shadowMaterial);
			renderer->SetMaterialProperty(materialProperty);
			cloth->AddComponent(renderer);

			auto solver = make_shared<VtClothSolver>(resolution);
			cloth->AddComponent(solver);

			cloth->Initialize(glm::vec3(0, 2.5f, 0), glm::vec3(1.0));
		}
	}
};

int main()
{
	//=====================================
	// 1. Create graphics
	//=====================================
	//shared_ptr<GameInstance> graphics(new GameInstance());
	//graphics.skyColor = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);
	auto engine = make_shared<VtEngine>();

	//=====================================
	// 2. Instantiate actors
	//=====================================
	
	vector<ScenePtr> scenes = {
		ScenePtr(new SceneSimpleCloth()),
		ScenePtr(new SceneColoredCubes()),
		ScenePtr(new ScenePremitiveRendering()),
	};
	engine->SetScenes(scenes);

	//=====================================
	// 3. Run graphics
	//=====================================
	return engine->Run();
}