#pragma once
#include "Model.h"
#include <unordered_map>
#include <memory>

class Scene
{
public:
	Scene();
	~Scene();
	
	//void Initialize();

	void LoadModel(ID3D11Device* device, const char* filename);
	void DeleteModel(uint64_t uid);

	const std::unordered_map<uint64_t, std::unique_ptr<Model>>& GetModels() const;
private:
	std::unordered_map<uint64_t, std::unique_ptr<Model>> m_models;
	uint64_t m_nextUID = 0;
};