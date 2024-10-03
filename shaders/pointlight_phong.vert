#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform vec4 point_pos[6];
uniform vec4 spot_pos[2];

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[9];
} DataOut;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.normal = normalize(m_normal * normal.xyz);
	for (int i = 0; i < 6; i++)
		DataOut.lightDir[1 + i] = vec3(point_pos[i] - pos);
	for (int i = 0; i < 2; i++)
		DataOut.lightDir[7 + i] = vec3(spot_pos[i] - pos);
	DataOut.eye = vec3(-pos);

	gl_Position = m_pvm * position;	
}