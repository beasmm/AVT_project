#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform vec4 l_pos;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};

uniform Materials mat;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec4 color;
} DataOut;

void main () {

	vec4 pos = m_viewModel * position;

	gl_Position = m_pvm * position;

	vec4 spec = vec4(0.0);

	vec3 n = normalize(m_normal * vec3(normal));
	vec3 l = normalize(vec3(l_pos - pos));
	vec3 e = normalize(vec3(-pos));

	float intensity = max(dot(n, l), 0.0);

	if (intensity > 0.0) {
		
		vec3 h = normalize(l + e);
		float intSpec = max(dot(h, n), 0.0);
		spec = mat.specular * pow(intSpec, mat.shininess);
	}

	DataOut.color = max(intensity*2 * mat.diffuse + spec, mat.ambient);


}