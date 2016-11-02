#version 330 core
out vec4 FragColor;

in VS_OUT{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;

uniform int mode;
uniform vec3 objectColor;
uniform sampler2D objectTexture;

uniform vec3 viewPos;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float far_plane;
uniform samplerCube depthMap;

vec3 DetermineFragmentColor(int mode)
{
	//Check for blending mode
	if (mode == -1)
	{
		return objectColor;
	}
	else if (mode == 1)
	{
		return texture(objectTexture, fs_in.TexCoords).rgb;
	}
	else
	{
		return texture(objectTexture, fs_in.TexCoords).rgb * objectColor;
	}
}

float ShadowCalculation(vec3 fragPos)
{
	// Get vector between fragment position and light position
	vec3 fragToLight = fragPos - lightPos;
	// Use the light to fragment vector to sample from the depth map    
	float closestDepth = texture(depthMap, fragToLight).r;
	// It is currently in linear range between [0,1]. Re-transform back to original value
	closestDepth *= far_plane;
	// Now get current linear depth as the length between the fragment and light position
	float currentDepth = length(fragToLight);
	// Now test for shadows
	float bias = 0.05;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
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

float SoftShadowCalculation(vec3 fragPos)
{
	// Get vector between fragment position and light position
	vec3 fragToLight = fragPos - lightPos;
	// Get current linear depth as the length between the fragment and light position
	float currentDepth = length(fragToLight);
	// Test for shadows with PCF
	float shadow = 0.0;
	float bias = 0.15;
	int samples = 20;
	float viewDistance = length(viewPos - fragPos);
	float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
	for (int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
		closestDepth *= far_plane;   // Undo mapping [0;1]
		if (currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);

	// Display closestDepth as debug (to visualize depth cubemap)
	// FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    

	// return shadow;
	return shadow;
}

void main()
{
	vec3 color = DetermineFragmentColor(mode);	

	//No Normals --> no lighting
	if (fs_in.Normal == vec3(0.0f, 0.0f, 0.0f))
	{
		FragColor = vec4(color, 1.0f);
		return;
	}

	// Ambient
	float ambientStrength = 0.1f;
	vec3 ambient = ambientStrength * lightColor;

	// Diffuse 
	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	// Specular
	float specularStrength = 0.1f;
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
	vec3 specular = specularStrength * spec * lightColor;

	// Calculate shadow
	float shadow = SoftShadowCalculation(fs_in.FragPos);
	vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

	FragColor = vec4(lighting, 1.0f);
}