#pragma once

#include "Node.h"
#include <glm/mat4x4.hpp>

class GeometryNode : public Node
{
public:
	GeometryNode() {};
	~GeometryNode() {};

private:
	bool InitializeInputLayout(ID3D11Device* device, ID3D10Blob* vertexShaderBuffer) override;
};