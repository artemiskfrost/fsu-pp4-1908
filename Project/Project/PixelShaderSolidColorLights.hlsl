#define MAX_INSTANCES 5
#define MAX_DIRECTIONAL_LIGHTS 3
#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS 3


struct DirectionalLight
{
	float4 Color;
	float4 Direction;
};

struct PointLight
{
	float4 Color;
	float4 Position;
	float Range;
	float3 Attenuation;
};

struct SpotLight
{
	float4 Color;
	float4 Position;
	float4 Direction;
	float Range;
	float3 Attenuation;
	float Cone;
};


struct InputData
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float3 Texel : TEXCOORD;
	float4 Color : COLOR;
	uint InstanceId : SV_INSTANCEID;
	float4 WorldPosition : WORLDPOSITION;
};


cbuffer ConstantBuffer : register(b1)
{
	float4 AmbientColor;
	float4 InstanceColors[MAX_INSTANCES];
	DirectionalLight DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	PointLight PointLights[MAX_POINT_LIGHTS];
	//SpotLight SpotLights[MAX_SPOT_LIGHTS];
	float Time;
	float3 Padding;
}


float4 main(InputData input) : SV_TARGET
{
	input.Normal = normalize(input.Normal);
	float4 finalColor = float4(0, 0, 0, 0);

	for (unsigned int i = 0; i < MAX_POINT_LIGHTS; ++i)
	{
		float3 lightToPixelVector = PointLights[i].Position.xyz - input.WorldPosition.xyz;
		float distance = length(lightToPixelVector);
		if (distance <= PointLights[i].Range)
		{
			lightToPixelVector /= distance;
			float lightIntensity = dot(lightToPixelVector, input.Normal);
			if (lightIntensity > 0)
			{
				finalColor += lightIntensity * PointLights[i].Color;
				finalColor /= PointLights[i].Attenuation[0] + (PointLights[i].Attenuation[1] * distance) + (PointLights[i].Attenuation[2] * (distance * distance));
			}
		}
	}

	for (unsigned int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
	{
		finalColor += saturate(dot((float3) DirectionalLights[i].Direction, input.Normal) * DirectionalLights[i].Color);
	}

	finalColor = saturate(finalColor + AmbientColor);
	finalColor.a = 1;
	return finalColor;
}