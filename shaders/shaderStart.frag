#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

uniform vec3 fogColor; // Fog color
uniform float fogStart; // Fog start distance
uniform float fogEnd;   // Fog end distance

//point light
uniform vec3 lightPos;
uniform bool enablePosLight;
float constant=1.0f;
float linear=0.35f;
float quadratic=0.44f;


void computeLightComponents()
{
	vec3 cameraPosEye = vec3(0.0f); // In eye coordinates, the viewer is situated at the origin

	// Transform normal
	vec3 normalEye = normalize(fNormal);

	// Compute light direction
	vec3 lightDirN = normalize(lightDir);

	// Compute view direction
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

	// Compute ambient light
	ambient = ambientStrength * lightColor;

	// Compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

	// Compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

float computeShadow()
{
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	normalizedCoords = normalizedCoords * 0.5f + 0.5f;

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	if (normalizedCoords.z > 1.0f) {
		return 0.0f;
	}

	return shadow;
}

vec3 CalcPointLight()
{

	if(!enablePosLight) {
		return vec3(0.0f);
	}
	//return vec3(1.0f,0.0f,0.0f);
	vec3 cameraPosEye=vec3(0.0f);
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
	vec3 normalEye = normalize(fNormal);
	vec3 lightDirN = normalize(lightPos - fPosEye.xyz);

	// diffuse shading
	float diff = max(dot(normalEye, lightDirN), 0.0);

	// specular shading
	vec3 reflectDir = reflect(-lightDirN, normalEye);
	float spec = pow(max(dot(viewDirN, reflectDir), 0.0), shininess);

	// attenuation
	float distance    = length(lightPos - fPosEye.xyz);
	float attenuation = 1.0 / (constant + linear * distance +
	quadratic * (distance * distance));

	// combine results
	vec3 ambientPoint  = ambientStrength  * lightColor * vec3(texture(diffuseTexture, fTexCoords));
	vec3 diffusePoint  =  diff * lightColor * vec3(texture(diffuseTexture, fTexCoords));
	vec3 specularPoint = specularStrength * spec * vec3(texture(specularTexture, fTexCoords));
	ambientPoint  *= attenuation;
	diffusePoint  *= attenuation;
	specularPoint *= attenuation;
	return (ambientPoint + diffusePoint + specularPoint);
}


void main()
{
	computeLightComponents();

	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	float shadow = computeShadow();
	vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);

	// Fog calculation
	float distance = length(fPosEye.xyz); // Distance from the camera in eye space
	float fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);

	// Blend final color with fog color
	vec3 finalColor = mix(fogColor, color, fogFactor);
	vec3 colorFin=CalcPointLight();
	fColor = vec4(finalColor,1.0f);

	if(enablePosLight){
		fColor = vec4(colorFin, 1.0f);
	}
}
