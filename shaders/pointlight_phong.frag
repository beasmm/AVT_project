#version 430
#define NUMBER_POINT_LIGHTS 6


//define the light types
struct PointLight{
	vec4 pos;
	vec3 color;
};


struct DirectionalLight{
	vec3 dir;
	vec3 color;
};	

// define the light variables
uniform PointLight pointLights[NUMBER_POINT_LIGHTS];
uniform DirectionalLight dirLight;

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
	vec3 lightDir[6];
} DataIn;

void main() {

	vec4 spec = vec4(0.0);
	vec4 colorAux = mat.ambient;

	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);

	
	for (int i = 0; i < NUMBER_POINT_LIGHTS; i++){
		vec3 l = normalize(DataIn.lightDir[i]);

		float intensity = max(dot(n,l), 0.0);

	
		if (intensity > 0.0) {

			vec3 h = normalize(l + e);
			float intSpec = max(dot(h,n), 0.0);
			spec = mat.specular * pow(intSpec, mat.shininess);
		}
		colorAux += intensity * mat.diffuse + spec;
	}
	
	colorOut = clamp(colorAux, 0.0, 1.0);
}