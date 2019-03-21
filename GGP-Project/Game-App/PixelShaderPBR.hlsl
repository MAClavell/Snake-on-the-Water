
#include "Lighting.hlsli"

// How many lights could we handle?

//Data that changes once per MatMesh combo
cbuffer perCombo : register(b0)
{
	Light Lights[MAX_LIGHTS]; //array of lights
	int LightCount; //amount of lights
	float3 CameraPosition;
	AmbientLight AmbLight;
}


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float3 worldPos		: POSITION; // The world position of this PIXEL
};


// Texture-related variables
Texture2D AlbedoTexture			: register(t0);
Texture2D NormalTexture			: register(t1);
Texture2D RoughnessTexture		: register(t2);
Texture2D MetalTexture			: register(t3);
SamplerState BasicSampler		: register(s0);


// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	// Fix for poor normals: re-normalizing interpolated normals
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// Use normal mapping
	float3 normalMap = NormalMapping(NormalTexture, BasicSampler, input.uv, input.normal, input.tangent);
	input.normal = normalMap;

	// Sample the roughness map
	float roughness = RoughnessTexture.Sample(BasicSampler, input.uv).r;

	// Sample the metal map
	float metal = MetalTexture.Sample(BasicSampler, input.uv).r;

	// Sample texture
	float4 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv);
	surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
	float3 totalColor = float3(0,0,0);

	//Add ambient light
	totalColor += AmbLight.Color * AmbLight.Intensity * surfaceColor.rgb;

	// Loop through all lights this frame
	for (int i = 0; i < LightCount; i++)
	{
		// Which kind of light?
		switch (Lights[i].Type)
		{
		case LIGHT_TYPE_DIRECTIONAL:
			totalColor += DirLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_POINT:
			totalColor += PointLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_SPOT:
			totalColor += SpotLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;
		}
	}

	// Adjust the light color by the light amount
	float3 gammaCorrect = pow(totalColor, 1.0 / 2.2);
	return float4(gammaCorrect, 1);
}