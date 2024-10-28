#version 430
#define NUMBER_POINT_LIGHTS 6


uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform sampler2D tex_flare;
uniform	sampler2D texUnitDiff;
uniform	sampler2D texUnitDiff1;
uniform	sampler2D texUnitSpec;
uniform	sampler2D texUnitNormalMap;

uniform int texMode;

// toggle light
uniform bool isDay;
uniform bool pointLightsOn;
uniform bool spotLightsOn;
uniform bool shadowMode;
uniform vec4 coneDir;
uniform float spotCosCutOff;
uniform bool fogEffectOn;

uniform vec4 dir_pos;

out vec4 colorOut;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};
uniform Materials mat;

uniform bool specularMap;
uniform uint diffMapCount;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[8];
	vec2 tex_coord;
} DataIn;

vec4 diff, auxSpec;

void main() {
	
	vec4 texel, texel1, texel2; 

	vec4 spec = vec4(0.0);
	vec4 colorAux = mat.ambient;
	vec4 colorPoint = vec4(0.0);
	vec4 colorSpot = vec4(0.0);

	float intSpec = 0.0;

	float att = 0.0;
	float spotExp = 50.0;

	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);
	vec3 sd = normalize(vec3(-coneDir));

	if (shadowMode == true) {
		colorOut = vec4(0.5, 0.5, 0.5, 1.0);
	}
	else {
		if (mat.texCount == 0) {
			diff = mat.diffuse;
			auxSpec = mat.specular;
		}
		else {
			if(diffMapCount == 0)
				diff = mat.diffuse;
			else if(diffMapCount == 1)
				diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord);
			else
				diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord) * texture(texUnitDiff1, DataIn.tex_coord);

			if(specularMap) 
				auxSpec = mat.specular * texture(texUnitSpec, DataIn.tex_coord);
			else
				auxSpec = mat.specular;
		}

		if (isDay == true) {
			vec3 l = normalize(vec3(-dir_pos));
			float intensity = max(dot(n,l), 0.0);

			if (intensity > 0.0) {
				vec3 h = normalize(l + e);
				float intSpec = max(dot(h, n), 0.0);
				spec = auxSpec * pow(intSpec, mat.shininess);
			}

			if(texMode == 0) {
				colorAux += max(intensity *  diff + spec, mat.ambient);
			}
			else if (texMode == 1) {
				texel = texture(texmap, DataIn.tex_coord); 
				texel1 = texture(texmap1, DataIn.tex_coord);
				colorAux += vec4(max(intensity*texel*texel1 + spec, 0.07*texel*texel1).rgb, diff.a);
			}
			else if (texMode == 3) {
				texel2 = texture(texmap2, DataIn.tex_coord); 
				if(texel2.a == 0.0) discard;
				else
					colorAux = vec4(max((intensity*texel2 + spec).rgb, 0.1*texel2.rgb), texel2.a);
			}
		} else {
			if (texMode == 1) {
				texel = texture(texmap, DataIn.tex_coord); 
				texel1 = texture(texmap1, DataIn.tex_coord);
				colorAux += vec4((0.07*texel*texel1).rgb, diff.a);
			}
			else if (texMode == 3) {
				texel2 = texture(texmap2, DataIn.tex_coord); 
				if(texel2.a == 0.0) discard;
				else
					colorAux = vec4(texel2.rgb*0.1, 0.8);
			}

		}

		if (pointLightsOn == true) { // pointlights are on
			for (int i = 0; i < 6; i++){
				vec3 l = normalize(DataIn.lightDir[i]);
				float intensity = max(dot(n,l), 0.0);
				spec = vec4(0.0);
				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					float intSpec = max(dot(h,n), 0.0);
					spec = auxSpec * pow(intSpec, mat.shininess);
				}
				if(texMode == 0) {
					colorPoint += intensity * diff * 0.5 + spec;
				}
				else if (texMode == 1) {
					texel = texture(texmap, DataIn.tex_coord);
					texel1 = texture(texmap1, DataIn.tex_coord);
					colorPoint += max(intensity*texel*texel1 + spec, 0.07*texel*texel1);
					colorPoint = vec4(colorPoint.rgb, diff.a);
				}
				else if (texMode == 3) {
					texel2 = texture(texmap2, DataIn.tex_coord); 
					if(texel2.a == 0.0) discard;
					else
						colorPoint = vec4(max((intensity*texel2 + spec).rgb, 0.1*texel2.rgb), texel2.a);
				}
			}
		}

		if (spotLightsOn == true) {
			for (int i = 0; i < 2; i++){
				vec3 l = normalize(DataIn.lightDir[6+i]);
				float spotCos = dot(l, sd);
				float intensity;
				spec = vec4(0.0);

				if(spotCos > spotCosCutOff)  {	//inside cone?
					att = pow(spotCos, spotExp);
					intensity = max(dot(n,l), 0.0) * att;

					if (intensity > 0.0) {
						vec3 h = normalize(l + e);
						float intSpec = max(dot(h,n), 0.0);
						spec = auxSpec * pow(intSpec, mat.shininess) * att;
					}
					if(texMode == 0) {
						colorSpot += intensity * diff + spec;
					}
					else if (texMode == 1) {
						texel = texture(texmap, DataIn.tex_coord);  
						texel1 = texture(texmap1, DataIn.tex_coord); 
						colorSpot += max(intensity*texel*texel1 + spec, 0.07*texel*texel1);
						colorSpot = vec4(colorSpot.rgb, diff.a);
					}
					else if (texMode == 3) {
						texel2 = texture(texmap2, DataIn.tex_coord); 
						if(texel2.a == 0.0) discard;
						else
							colorSpot = vec4(max((intensity*texel2 + spec).rgb, 0.1*texel2.rgb), texel2.a);
					}
				}
			}
		}
		colorOut = clamp(colorAux + colorPoint + colorSpot, 0.0f, 1.0f);
		if (fogEffectOn == true) {
			float dist = length(DataIn.eye);

			float fogAmount = exp(-dist*0.05);
			vec3 fogColor = vec3( 0.5, 0.6, 0.7);
			vec3 finalColor = mix(colorOut.rgb, fogColor, fogAmount);
			colorOut = vec4(finalColor, 1);
		}
	
		if (texMode == 2) {
			texel = texture(texmap, DataIn.tex_coord);  //texel from element flare texture
			if(texel.a == 0.0) discard;
			else
				colorOut = vec4((diff * texel).rgb, 0.4);
		}
	}	
}