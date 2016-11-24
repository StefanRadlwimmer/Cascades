#version 330 core

out vec4 FragColor;

in VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec2 UV;
	mat3 TBN;
} fs_in;

const int COLOR_ONLY_MODE = -1;
const int TEXTURE_ONLY_MODE = 1;

uniform bool SoftShadows = true;
uniform bool EnableLighting = true;

struct LightComponents
{
	float Ambient;
	float Diffuse;
	float Specular;
};

const int LIGHT_COUNT = 10;
const int DIR_LIGHT = 0;
const int SPOT_LIGHT = 1;
const int POINT_LIGHT = 2;

struct LightSource
{
	vec3 Pos;
	vec3 Color;

	int Type;
	bool IsEnabled;

	mat4 lightSpaceMatrix;

	float near_plane;
	float far_plane;

	samplerCube depthCube;
	sampler2D depthMap;
};

uniform LightSource Lights[LIGHT_COUNT];

uniform int mode;
uniform vec3 objectColor;
uniform sampler2D objectTexture;
uniform sampler2D normalMap;

uniform vec3 viewPos;

bool IsTextureBound(in sampler2D tex)
{
	return textureSize(tex, 0).x > 0;
}

vec3 DetermineFragmentColor(in int mode)
{
	//Check for blending mode
	if (mode == COLOR_ONLY_MODE)
	{
		return objectColor;
	}
	else if (mode == TEXTURE_ONLY_MODE)
	{
		return texture(objectTexture, fs_in.UV).rgb;
	}
	else
	{
		return texture(objectTexture, fs_in.UV).rgb * objectColor;
	}
}

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
	vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
	vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
	vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
	vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
	vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

float CalculatePointShadow(in LightSource light)
{
	// Get vector between fragment position and light position
	vec3 fragToLight = fs_in.FragPos - light.Pos;
	// Get current linear depth as the length between the fragment and light position
	float currentDepth = length(fragToLight);
	// Test for shadows with PCF
	float shadow = 0.0;
	float bias = 0.15;
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

	// return shadow;
	return shadow;
}

LightComponents CalculateLight(in LightSource light, in vec3 lightDir)
{
	LightComponents lighting;

	// Ambient
	float ambientStrength = 0.1f;
	lighting.Ambient = ambientStrength;

	// Diffuse 
	float diffuseStrength = 1.0f;
	vec3 norm = normalize(fs_in.Normal);
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

float CalculateDirShadow(in LightSource light, in float baseBias)
{
	vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fs_in.FragPos, 1.0f);

	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(light.depthMap, projCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// Calculate bias (based on depth map resolution and slope)
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightDir = normalize(light.Pos);

	float bias = max(baseBias / 10 * (1.0 - dot(normal, lightDir)), baseBias);

	// Check whether current frag pos is in shadow
	// float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	// PCF
	float shadow = 0.0;
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

	// Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if (projCoords.z > 1.0)
	shadow = 0.0;

	return shadow;
}

float CalculateCircularShadow(in LightSource light)
{
	vec4 fragPosLightSpace = light.lightSpaceMatrix * vec4(fs_in.FragPos, 1.0f);

	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	//Generate Circular Shadow
	float distanceToCenter = length(projCoords.xy);
	return clamp(distanceToCenter * distanceToCenter, 0, 1);
}

float CalculateDistanceShadow(in LightSource light)
{
	float distance = length(light.Pos - fs_in.FragPos);
	return distance / light.far_plane;
}

vec3 CalculateDirLightSource(in LightSource light)
{
	LightComponents components = CalculateLight(light, normalize(light.Pos));
	float shadow = CalculateDirShadow(light, 0.005);
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}

vec3 CalculateSpotLightSource(in LightSource light)
{
	LightComponents components = CalculateLight(light, normalize(light.Pos - fs_in.FragPos));
	float shadow = CalculateDirShadow(light, 0.001);
	float circular = CalculateCircularShadow(light);
	shadow = clamp(shadow + circular, 0, 1);
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}

vec3 CalculatePointLightSource(in LightSource light)
{
	LightComponents components = CalculateLight(light, normalize(light.Pos - fs_in.FragPos));
	float shadow = CalculatePointShadow(light);
	float distance = CalculateDistanceShadow(light);
	shadow = clamp(shadow + distance, 0, 1);
	return (components.Ambient + (1.0 - shadow) * (components.Diffuse + components.Specular)) * light.Color;
}


void main()
{
	vec3 color = DetermineFragmentColor(mode);

	//No Normals --> no lighting
	if (fs_in.Normal == vec3(0.0f, 0.0f, 0.0f) || !EnableLighting)
	{
		FragColor = vec4(color, 1.0f);
		return;
	}

	if (IsTextureBound(normalMap))
	{
		// Obtain normal from normal map in range [0,1]
		//fs_in.Normal = texture(normalMap, fs_in.UV).rgb;
		// Transform normal vector to range [-1,1]
		//fs_in.Normal = normalize(normal * 2.0 - 1.0);
	}

	vec3 lighting = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		if (!Lights[i].IsEnabled)
			continue;

		switch (Lights[i].Type)
		{
		case DIR_LIGHT:
			lighting += CalculateDirLightSource(Lights[i]);
			break;
		case SPOT_LIGHT:
			lighting += CalculateSpotLightSource(Lights[i]);
			break;
		case POINT_LIGHT:
			lighting += CalculatePointLightSource(Lights[i]);
			break;
		}
	}

	lighting = clamp(lighting, 0, 1);
	lighting *= color;

	FragColor = vec4(lighting, 1.0f);


	//Point
	//vec3 lightDir = normalize(light.Pos - fs_in.FragPos);

	//Dir
	//vec3 lightDir = normalize(light.Pos);
}