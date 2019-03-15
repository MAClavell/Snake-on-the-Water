#include "Renderer.h"

using namespace DirectX;

// Initialize values in the renderer
void Renderer::Init()
{
	// Initialize fields
	vertexShader = 0;
	pixelShader = 0;

	//Directional lights
	dLight = new DirectionalLight(XMFLOAT4(0.1f, 0.1f, 0.1f, 1), XMFLOAT4(1, 1, 1, 1), 1);

	//Point light
	pLight = new PointLight(5, XMFLOAT4(0, 1, 0, 1), 1);
	pLight->SetPosition(0, -2, 3);

	//Spot light
	sLight = new SpotLight(5, XMFLOAT4(0, 0, 1, 1), 1);
	sLight->SetPosition(2, 0, -1);
	sLight->SetRotation(0, -90, 0);
}

// Destructor for when the singleton instance is deleted
Renderer::~Renderer()
{ 
	//Release lights
	if (dLight)
		delete dLight;
	if (pLight)
		delete pLight;
	if (sLight)
		delete sLight;
}

// Draw all entities in the render list
void Renderer::Draw(ID3D11DeviceContext* context, Camera* camera)
{
	//TODO: Finish refactoring renderer to sort based on mesh/material combo
	//TODO: Assign lights to entities
	//TODO: Apply attenuation

	//for (size_t j = 0; j < renderMap.; j++)
	for (auto const& x : renderMap)
	{
		if (x.second.size() < 1)
			return;

		//Get material and mesh
		std::vector<Entity*> list = x.second;
		Material* mat = list[0]->GetMaterial();
		Mesh* mesh = list[0]->GetMesh();

		//Get shaders
		pixelShader = mat->GetPixelShader();

		//Set pixel shader variables
		pixelShader->SetData("light1", dLight->GetLightStruct(), sizeof(LightStruct));
		pixelShader->SetData("light2", pLight->GetLightStruct(), sizeof(LightStruct));
		pixelShader->SetData("light3", sLight->GetLightStruct(), sizeof(LightStruct));
		pixelShader->SetFloat4("surfaceColor", mat->GetSurfaceColor());
		pixelShader->SetFloat3("cameraPosition", camera->GetPosition());
		pixelShader->SetFloat("specularity", mat->GetSpecularity());
		pixelShader->SetShaderResourceView("diffuseTexture", mat->GetResourceView());
		pixelShader->SetSamplerState("basicSampler", mat->GetSamplerState());

		//Send data to GPU
		pixelShader->CopyAllBufferData();

		//Set pixel shader
		pixelShader->SetShader();

		// Set buffers in the input assembler
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer();
		ID3D11Buffer* indexBuffer = mesh->GetIndexBuffer();
		context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		//Loop through each entity in the list
		for (size_t i = 0; i < list.size(); i++)
		{
			Entity* ent = list[i];

			vertexShader = ent->GetMaterial()->GetVertexShader();

			// Send data to shader variables
			//  - Do this ONCE PER OBJECT you're drawing
			//  - This is actually a complex process of copying data to a local buffer
			//    and then copying that entire buffer to the GPU.  
			//  - The "SimpleShader" class handles all of that for you.
			vertexShader->SetMatrix4x4("world", ent->GetWorldMatrix());
			vertexShader->SetMatrix4x4("projection", camera->GetProjectionMatrix());
			vertexShader->SetMatrix4x4("worldInvTrans", ent->GetWorldInvTransMatrix());
			vertexShader->SetMatrix4x4("view", camera->GetViewMatrix());

			//Send data to GPU
			vertexShader->CopyAllBufferData();

			//Set vertex shader
			vertexShader->SetShader();

			// Finally do the actual drawing
			//  - Do this ONCE PER OBJECT you intend to draw
			//  - This will use all of the currently set DirectX "stuff" (shaders, buffers, etc)
			//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
			//     vertices in the currently set VERTEX BUFFER
			context->DrawIndexed(
				mesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
				0,     // Offset to the first index we want to use
				0);    // Offset to add to each index when looking up vertices
		}
	}
}

// Add an entity to the render list
void Renderer::AddEntityToRenderList(Entity* e)
{
	bool isInList = IsEntityInRenderList(e);
	if (IsEntityInRenderList(e))
	{
		printf("Cannot add entity because it is already in render list");
		return;
	}

	//Look for existing mesh/material combos
	if (renderMap.find(e->GetMatMeshIdentifier()) != renderMap.end())
	{
		renderMap[e->GetMatMeshIdentifier()].push_back(e);
	}
	//Make a new entry
	else
	{
		std::vector<Entity*> list;
		list.push_back(e);
		renderMap.emplace(e->GetMatMeshIdentifier(), list);
	}
}

// Remove an entity from the render list
void Renderer::RemoveEntityFromRenderList(Entity* e)
{
	//Early return if render list is empty and if the entity is in it
	if (renderMap.find(e->GetMatMeshIdentifier()) == renderMap.end())
		return;

	//Get correct render list
	std::vector<Entity*> list = renderMap[e->GetMatMeshIdentifier()];

	//Get the index of the entity
	size_t* index = nullptr;
	size_t i;
	for (i = 0; i < list.size(); i++)
	{
		if (e == list[i])
		{
			index = &i;
			break;
		}
	}

	//Entity is not in the render list
	if (index == nullptr)
	{
		printf("Cannont remove entity because it is not in render list");
		return;
	}

	//If the entity is not the very last we swap it for the last one
	if (*index != list.size() - 1)
	{
		std::swap(list[*index], list[list.size() - 1]);
	}

	//Pop the last one
	list.pop_back();

	//Check if the list is empty
	if (list.size() == 0)
	{
		//Erase
		renderMap.erase(e->GetMatMeshIdentifier());
	}
}

// Check if an entity is in the render list. O(n) complexity
//	(n is the amount of entities that share the material and mesh)
bool Renderer::IsEntityInRenderList(Entity* e)
{
	//Early return if render list is empty and if the entity is in it
	if (renderMap.find(e->GetMatMeshIdentifier()) == renderMap.end())
		return false;

	//Get render list
	std::vector<Entity*> list = renderMap[e->GetMatMeshIdentifier()];

	//Early return if render list is empty
	if (list.size() == 0)
		return false;

	//Get the index of the entity
	for (size_t i = 0; i < list.size(); i++)
	{
		if (e == list[i])
		{
			return true;
		}
	}

	return false;
}