#include "Scene.h"

Scene::Scene() {}
Scene::~Scene() {}

void Scene::LoadModel(ID3D11Device* device, const char* filename)
{
	Model model;
	model.Initialize(m_nextUID);
	model.Load(device, filename);
	m_models[m_nextUID++] = std::make_unique<Model>(std::move(model));
}

void Scene::DeleteModel(uint64_t uid)
{
	m_models.erase(uid);
}

const std::unordered_map<uint64_t, std::unique_ptr<Model>>& Scene::GetModels() const
{
	return m_models;
}