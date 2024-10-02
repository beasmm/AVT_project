#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform vec4 l_pos[6];

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[6];
} DataOut;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.normal = normalize(m_normal * normal.xyz);
	for (int i = 0; i < 6; i++)
		DataOut.lightDir[i] = vec3(l_pos[i] - pos);
	DataOut.eye = vec3(-pos);

	gl_Position = m_pvm * position;	
}