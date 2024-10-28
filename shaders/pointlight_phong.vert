#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform bool normalMap;

uniform vec4 light_pos[8];
uniform vec4 point_pos[6];
uniform vec4 spot_pos[2];

in vec4 position;
in vec4 normal, tangent, bitangent;    //por causa do gerador de geometria
in vec4 texCoord;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[8];
	vec2 tex_coord;
} DataOut;

void main () {
	vec3 n, t, b;
	vec3 lightDir, eyeDir;
	vec3 aux;

	vec4 pos = m_viewModel * position;
	n = normalize(m_normal * normal.xyz);
	eyeDir =  vec3(-pos);

	for (int i = 0; i < 6; i++){
		lightDir = vec3(point_pos[i] - pos);
		if(normalMap)  {  //transform eye and light vectors by tangent basis
			t = normalize(m_normal * tangent.xyz);
			b = normalize(m_normal * bitangent.xyz);

			aux.x = dot(lightDir, t);
			aux.y = dot(lightDir, b);
			aux.z = dot(lightDir, n);
			DataOut.lightDir[i] = normalize(aux);

			aux.x = dot(eyeDir, t);
			aux.y = dot(eyeDir, b);
			aux.z = dot(eyeDir, n);
			eyeDir = normalize(aux);
		}
		else DataOut.lightDir[i] = lightDir;
	}
	for (int i = 0; i < 2; i++) {
		lightDir = vec3(spot_pos[i] - pos);
		if(normalMap)  {  //transform eye and light vectors by tangent basis
			t = normalize(m_normal * tangent.xyz);
			b = normalize(m_normal * bitangent.xyz);

			aux.x = dot(lightDir, t);
			aux.y = dot(lightDir, b);
			aux.z = dot(lightDir, n);
			DataOut.lightDir[6 + i] = normalize(aux);

			aux.x = dot(eyeDir, t);
			aux.y = dot(eyeDir, b);
			aux.z = dot(eyeDir, n);
			eyeDir = normalize(aux);
		}
		else DataOut.lightDir[6 + i] = lightDir;
	}
	DataOut.eye = eyeDir;
	DataOut.tex_coord = texCoord.st;
	DataOut.normal = n;
	gl_Position = m_pvm * position;	
}