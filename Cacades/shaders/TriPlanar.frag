#version 330 core

out vec4 FragColor;

#pragma include "EnumLightType.h"

in VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec3 UVW;
	mat3 TBN;
} fs_in;

uniform bool SoftShadows = true;
uniform bool EnableLighting = true;

struct LightComponents
{
	float Ambient;
	float Diffuse;
	float Specular;
};

const int LIGHT_COUNT = 5;

struct LightSource
{
	vec3 Pos;
	vec3 Color;

	int Type;
	bool IsEnabled;
	bool CastShadow;

	mat4 lightSpaceMatrix;

	float near_plane;
	float far_plane;

	samplerCube depthCube;
	sampler2D depthMap;
};

uniform LightSource Lights[LIGHT_COUNT];

uniform sampler2D objectTexture[3];

uniform sampler2D normalMap[3];
uniform float bumbiness = 1f;

uniform sampler2D displacementMap[3];
uniform int displacement_initialSteps = 8;
uniform int displacement_refinementSteps = 8;
uniform float displacement_scale = 0.025f;

uniform vec3 viewPos;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
	vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
	vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
	vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
	vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
	vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

float CalculatePointShadow(in LightSource light, in vec3 normal)
{
	// Get vector between fragment position and light position
	vec3 fragToLight = fs_in.FragPos - light.Pos;
	// Get current linear depth as the length between the fragment and light position
	float currentDepth = length(fragToLight);
	float bias = 0.05;
	float shadow = 0.0;

	if (!SoftShadows)
	{
		// Use the light to fragment vector to sample from the depth map
		float closestDepth = texture(light.depthCube, fragToLight).r;
		// It is currently in linear range between [0,1]. Re-transform back to original value
		closestDepth *= light.far_plane;
		shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	}
	else
	{
		// Test for shadows with PCF
		int samples = 20;
		float viewDistance = length(viewPos - fs_in.FragPos);
		float diskRadius = (1.0 + (viewDistance / light.far_plane)) / 25.0f;
		for (int i = 0; i < samples; ++i)
		{
			float closestDepth = texture(light.depthCube, fragToLight + gridSamplingDisk[i] * diskRadius).r;
			closestDepth *= light.far_plane;   // Undo mapping [0;1]

			float distance = length(light.Pos - fs_in.FragPos);
			float distanceBias = max(bias, bias * distance / 5.0f);
			if (currentDepth - distanceBias > closestDepth)
				shadow += 1.0;
		}
		shadow /= float(samples);
	}

	// return shadow;
	return shadow;
}

LightComponents CalculateLight(in LightSource light, in vec3 normal, in vec3 lightDir)
{
	LightComponents lighting;

	// Ambient
	float ambientStrength = 0.1f;
	lighting.Ambient = ambientStrength;

	// Diffuse
	float diffuseStrength = 1.0f;
	vec3 norm = normalize(normal);
	float diff = max(dot(norm, lightDir), 0.0);
	lighting.Diffuse = diffuseStrength * diff;

	// Specular
	float specularStrength = 0.1f;
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
	lighting.Specular = specularStrength * spec;

	return lighting;
}

float CalculateDirShadow(in LightSource light, in vec3 normal, in float baseBias)
{
	vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fs_in.FragPos, 1.0f);

	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	// Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if (projCoords.z > 1.0)
		return 0.0;

	// Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(light.depthMap, projCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// Calculate bias (based on depth map resolution and slope)
	vec3 lightDir = normalize(light.Pos);

	float bias = max(baseBias / 10 * (1.0 - dot(normal, lightDir)), baseBias);

	float shadow = 0.0;
	// Check whether current frag pos is in shadow
	if (!SoftShadows)
		shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	else
	{
		// PCF
		vec2 texelSize = 1.0 / textureSize(light.depthMap, 0);
		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(light.depthMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;
	}

	return shadow;
}

float CalculateCircularShadow(in LightSource light, in vec3 normal)
{
	vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fs_in.FragPos, 1.0f);

	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	//Generate Circular Shadow
	float distanceToCenter = length(projCoords.xy);
	return clamp(distanceToCenter * distanceToCenter, 0, 1);
}

float CalculateDistanceShadow(in LightSource light, in vec3 normal)
{
	float distance = length(light.Pos - fs_in.FragPos);
	return distance / light.far_plane;
}

vec3 CalculateDirLightSource(in LightSource light, in vec3 normal)
{
	LightComponents components = CalculateLight(light, normal, normalize(light.Pos));
	float shadow = 0;
	if (light.CastShadow)
		shadow = CalculateDirShadow(light, normal, 0.005);
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}

vec3 CalculateSpotLightSource(in LightSource light, in vec3 normal)
{
	LightComponents components = CalculateLight(light, normal, normalize(light.Pos - fs_in.FragPos));
	float shadow = 0;
	if (light.CastShadow)
	{
		shadow = CalculateDirShadow(light, normal, 0.002);
		float circular = CalculateCircularShadow(light, normal);
		shadow = clamp(shadow + circular, 0, 1);
	}
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}

vec3 CalculatePointLightSource(in LightSource light, in vec3 normal)
{
	LightComponents components = CalculateLight(light, normal, normalize(light.Pos - fs_in.FragPos));
	float shadow = 0;
	if (light.CastShadow)
	{
		shadow = CalculatePointShadow(light, normal);
		float distance = CalculateDistanceShadow(light, normal);
		shadow = clamp(shadow + distance, 0, 1);
	}
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}

vec2 Parallax(sampler2D map, vec2 texCoords, vec3 viewDir)
{
    return texCoords;

    // calculate the size of each layer
    float layerDepth = 1.0 / displacement_initialSteps;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = vec2(viewDir.x, -viewDir.y) / viewDir.z * displacement_scale;
    vec2 deltaTexCoords = P / displacement_initialSteps;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(map, currentTexCoords).r;

 	while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(map, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    currentTexCoords += deltaTexCoords;
    currentDepthMapValue = texture(map, currentTexCoords).r;
    currentLayerDepth -= layerDepth;
	currentLayerDepth -= 0.085f; //reduces artifacts

	// decrease the step size as we do the refinement steps
	deltaTexCoords /= displacement_refinementSteps;
	layerDepth /= displacement_refinementSteps;

 	while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(map, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	float prevLayerDepth = currentLayerDepth - layerDepth;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(map, prevTexCoords).r - prevLayerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec3 NormalizeNormal(vec3 tmpNormal)
{
  // Transform normal vector to range [-1,1]
  tmpNormal = normalize(tmpNormal * 2.0 - 1.0);
  // Apply "Bumpiness"
  return normalize(tmpNormal * vec3(bumbiness, bumbiness, 1.0f));
}

void main()
{
    vec3 blending = abs( fs_in.Normal );
    blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
    float b = (blending.x + blending.y + blending.z);
    blending /= vec3(b, b, b);

		mat3 AntiTBN = transpose(fs_in.TBN);
		vec3 tangentViewDir = normalize((AntiTBN * viewPos) - (AntiTBN * fs_in.FragPos));

    vec2[3] uvs;
    uvs[0] = fs_in.UVW.yz;
    uvs[1] = fs_in.UVW.xz;
    uvs[2] = fs_in.UVW.xy;

    vec3[3] colors;
    vec3[3] normals;
    for (int i = 0; i < 3; ++i)
    {
        uvs[i] = Parallax(displacementMap[i], uvs[i], tangentViewDir);
        colors[i] = texture(objectTexture[i], uvs[i]).xyz;
        normals[i] = NormalizeNormal(texture(normalMap[i], uvs[i]).xyz);
    }

    vec3 color = colors[0] * blending[0] + colors[1] * blending[1] + colors[2] * blending[2];
    vec3 normal = fs_in.Normal;//normals[0] * blending[0] + normals[1] * blending[1] + normals[2] * blending[2];

    //normal = transpose(fs_in.TBN) * normal;

	if (!EnableLighting || normal == vec3(0.0f))
	{
		FragColor = vec4(color, 1.0f);
		return;
	}

	vec3 lighting = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		if (!Lights[i].IsEnabled)
			continue;

		switch (Lights[i].Type)
		{
		case DIR_LIGHT:
			lighting += CalculateDirLightSource(Lights[i], normal);
			break;
		case SPOT_LIGHT:
			lighting += CalculateSpotLightSource(Lights[i], normal);
			break;
		case POINT_LIGHT:
			lighting += CalculatePointLightSource(Lights[i], normal);
			break;
		}
	}

	lighting = clamp(lighting, 0, 1);
	lighting *= color;

	FragColor = vec4(lighting, 1.0f);
}
