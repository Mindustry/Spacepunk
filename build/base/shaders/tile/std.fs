#version 450

in vec3 DiffuseMap;
in vec3 NormalMap;
in vec3 EffectsMap;
in vec3 Normal;
in vec3 WorldPos;
in vec3 Tangent;

out vec4 FragColor;

#define CHUNK_SIZE 2
#define TILE_SIZE 128

uniform bool gActiveLight;
uniform vec3 gCameraPos;
uniform vec3 gLightPos;
uniform vec4 gLightColor;
uniform float gLightIntensity;
uniform float gLightRadius;
uniform vec3 gLightScale;
uniform vec3 gLightDirection;
uniform int gLightShape;
uniform sampler2DArray gDiffuseMap;
uniform sampler2DArray gNormalMap;
uniform sampler2DArray gEffectsMap;
uniform samplerCube gCubeMap;
uniform mat3 gTileColors[CHUNK_SIZE * CHUNK_SIZE];

vec3 CalcBumpedNormal() {
    vec3 lNormal = normalize(Normal);
    vec3 lTangent = normalize(Tangent);
    lTangent = normalize(lTangent - dot(lTangent, lNormal) * lNormal);
    vec3 lBitangent = cross(lTangent, lNormal);

    vec3 lBumpMapNormal = texture(gNormalMap, NormalMap).xyz;
    lBumpMapNormal = 2.0 * lBumpMapNormal - vec3(1.0, 1.0, 1.0);

    mat3 lTBN = mat3(lTangent, lBitangent, lNormal);
    vec3 lNewNormal = lTBN * lBumpMapNormal;
    lNewNormal = normalize(lNewNormal);
    return lNewNormal;
}

vec4 MapDiffuseTexture(vec3 lNormal, float lReflectPower) {
	vec4 lTexColor = texture(gDiffuseMap, DiffuseMap);

	float size  = CHUNK_SIZE * TILE_SIZE;
	int index   = int(mod(floor(WorldPos.z + Normal.z), size) / TILE_SIZE);
	index      += int(mod(floor(WorldPos.x + Normal.x), size) / TILE_SIZE) * CHUNK_SIZE;

	vec4 lFinalColor;
	lFinalColor  = vec4(gTileColors[index][0], 0.0) * lTexColor.r;
	lFinalColor += vec4(gTileColors[index][1], 0.0) * lTexColor.g;
	lFinalColor += vec4(gTileColors[index][2], 0.0) * lTexColor.b;
	lFinalColor += vec4(0.0, 0.0, 0.0, 1.0) * lTexColor.a;
	lFinalColor  = lFinalColor * (1.0 - lReflectPower);

	vec3 cameraDir    = normalize(gCameraPos - WorldPos);
	vec3 lReflection  = reflect(normalize(cameraDir), lNormal) * -1;
	lFinalColor      += texture(gCubeMap, lReflection) * lReflectPower;

	return lFinalColor;
}

vec4 MapDiffuseTextureOLD(vec3 lNormal, float lReflectPower) {
	vec4 lTexColor   = texture(gDiffuseMap, DiffuseMap) * (1.0 - lReflectPower);

	vec3 cameraDir     = normalize(gCameraPos - WorldPos);
	vec3 lReflection   = reflect(normalize(cameraDir), lNormal) * -1;
	vec4 lReflectColor = texture(gCubeMap, lReflection) * lReflectPower;

	return lTexColor + lReflectColor;
}

void main() {
	vec4  lEffectFrag   = texture(gEffectsMap, EffectsMap);
	float lReflectPower = lEffectFrag.a;
	vec3  lNormal       = CalcBumpedNormal();
	if( gActiveLight ) {
		vec3  lLightDirection = normalize(gLightPos - WorldPos);

		// stops stencil shadow z-fighting
		if( dot(Normal, lLightDirection) <= 0.0 ) {
			discard;
		}

		float lDiffuseFactor  = dot(lNormal, lLightDirection);

		vec4 lDiffuseColor  = vec4(0.0);
		vec4 lSpecularColor = vec4(0.0);

		// calculate diffuse color
		if( lDiffuseFactor > 0.0 ) {
			lDiffuseColor = gLightColor * max(gLightIntensity, 1.f) * lDiffuseFactor;

			// calculate specular color
			vec3  lFragToEye       = normalize(WorldPos - gCameraPos);
			vec3  lLightReflect    = normalize(reflect(lLightDirection, lNormal));
			float lSpecularFactor  = dot(lFragToEye, lLightReflect);

			if( lSpecularFactor > 0.0 ) {
				float lSpecularIntensity  = lEffectFrag.r * 255.0;
				float lSpecularPower      = pow(lSpecularFactor, max(lEffectFrag.g * 255.0, 1.f));

				lSpecularColor = gLightColor * lSpecularIntensity * lSpecularPower;
			}

			// apply lighting
			FragColor = MapDiffuseTexture(lNormal, lReflectPower) * (lDiffuseColor + lSpecularColor);

			// apply falloff
			if( gLightShape == 0 ) {
				// sphere
				vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
				float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
				float  lLightRadius    = gLightRadius * gLightRadius;
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.f), 1.f);
			} else if( gLightShape == 1 ) {
				// box
				vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
				float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
				float  lLightRadius    = gLightRadius * gLightRadius;
				float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
				FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.f), 1.f);
			} else if( gLightShape == 2 ) {
				// capsule
			} else if( gLightShape == 3 ) {
				// cylinder
			} else if( gLightShape == 4 ) {
				// cone
				float  lSpotFactor = dot(-lLightDirection, gLightDirection);
				if( lSpotFactor > 0.0 ) {
					lSpotFactor = pow(lSpotFactor, 10);
					vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
					float  lLightFragDist  = lLightFragDiff.x + lLightFragDiff.y + lLightFragDiff.z;
					float  lLightRadius    = gLightRadius * gLightRadius;
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.0) / lLightRadius;
					FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.0), 1.0) * lSpotFactor;
				} else {
					FragColor.a = 0.0;
				}
			} else if( gLightShape == 5 ) {
				// pyramid
				float  lSpotFactor = dot(-lLightDirection, gLightDirection);
				if( lSpotFactor > 0.0 ) {
					lSpotFactor = pow(lSpotFactor, 10);
					vec3   lLightFragDiff  = (WorldPos - gLightPos) * (WorldPos - gLightPos);
					float  lLightFragDist  = max(lLightFragDiff.x, max(lLightFragDiff.y, lLightFragDiff.z));
					float  lLightRadius    = gLightRadius * gLightRadius;
					float  lLightFalloff   = max(lLightRadius - lLightFragDist, 0.f) / lLightRadius;
					FragColor.a = lLightFalloff * min( max(gLightIntensity, 0.0), 1.0) * lSpotFactor;
				} else {
					FragColor.a = 0.0;
				}
			}
		} else {
			FragColor = vec4(0.0);
		}
	} else {
		float lGlowFactor = lEffectFrag.b;
		FragColor = MapDiffuseTexture(lNormal, lReflectPower) * lGlowFactor;
	}
}