#version 430
#define NUMBER_POINT_LIGHTS 6


struct DirectionalLight{
	vec3 dir;
	vec3 color;
};	

// define the light variables
uniform DirectionalLight dirLight;

// toggle light
uniform bool isDay;
uniform bool pointLightsOn;
uniform bool spotLightsOn;
uniform vec4 coneDir;
uniform float spotCosCutOff;

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

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[9];

} DataIn;

void main() {

	vec4 spec = vec4(0.0);
	vec4 colorAux = vec4(0.0);
	vec4 colorPoint = vec4(0.0);
	vec4 colorSpot = vec4(0.0);

	float intSpec = 0.0;

	float att = 0.0;
	float spotExp = 50.0;


	vec3 n = normalize(DataIn.normal);
	vec3 l = normalize(DataIn.lightDir);
	vec3 e = normalize(DataIn.eye);
	vec3 sd = normalize(vec3(-coneDir));

	if (isDay == true) {
		vec3 l = normalize(vec3(-dir_pos));
		float intensity = max(dot(n,l), 0.0);

		if (intensity > 0.0) {
			vec3 h = normalize(l + e);
			float intSpec = max(dot(h, n), 0.0);
			spec = mat.specular * pow(intSpec, mat.shininess);
		}

		colorAux += max(intensity *  mat.diffuse + spec, mat.ambient);
	}

	if (pointLightsOn == true) { // pointlights are on
		for (int i = 0; i < NUMBER_POINT_LIGHTS; i++){
			vec3 l = normalize(DataIn.lightDir[1 + i]);

			float intensity = max(dot(n,l), 0.0);

			if (intensity > 0.0) {

				vec3 h = normalize(l + e);
				float intSpec = max(dot(h,n), 0.0);
				spec = mat.specular * pow(intSpec, mat.shininess);
			}
		colorPoint += intensity * mat.diffuse + spec;
		}
	}
	if (spotLightsOn == true) {
		for (int i = 0; i < 2; i++){
			vec3 l = normalize(DataIn.lightDir[7 + i]);
			float spotCos = dot(l, sd);
			float intensity;

			if(spotCos > spotCosCutOff)  {	//inside cone?
				att = pow(spotCos, spotExp);
				intensity = max(dot(n,l), 0.0) * att;

				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					float intSpec = max(dot(h,n), 0.0);
					spec = mat.specular * pow(intSpec, mat.shininess) * att;
				}
			}
		colorSpot += intensity * mat.diffuse + spec;
		}
	}
	colorOut = clamp(colorAux + colorPoint + colorSpot, 0.0, 1.0);

}