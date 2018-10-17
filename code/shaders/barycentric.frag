#version 330
//Inputs
in vec3 eye_normal;
in vec3 V;
in vec4 light;
in vec2 texture;
in vec3 tangent;
in vec3 bitangent;
in vec4 pos_light_space;

//Outputs
out vec3 output_colour;

//Uniforms
uniform mat4 view; 
uniform vec3 diffuse;
uniform vec3 ambient;
uniform vec3 specular;
uniform float specular_exponent;

uniform sampler2D texture_map;
uniform sampler2DShadow shadow_map;
uniform vec2 imageSize;

uniform sampler2D control0;
uniform sampler2D control1;
uniform sampler2D control2;
uniform sampler2D control3;
uniform sampler2D control4;
uniform sampler2D control5;
uniform sampler2D control6;
uniform sampler2D control7;
uniform sampler2D control8;


void defColour(int factor, vec3 l, vec3 normal, vec3 reflect, vec3 eye, vec3 pos, float bias, out float w, out vec3 c) 
{
	//diffuse
	if (factor==1)
	{
		w = max(dot(normal, l), 0.0);
		c = texture(control4, texture).rgb;
	}
		
	//specular
	if (factor==2)
	{
		w = clamp(pow(max(dot(eye, reflect), 0.0), specular_exponent), 0.0, 1.0);
		c = mix(texture(control8, texture).rgb, vec3(1.0, 1.0, 1.0), 0.2);
	}
			
	//Shadow
	if (factor==3)
	{
		float light_comp = 0.0;
		float samplesize = 10;
		float yOff = 1.0/imageSize.y;
		float xOff = 1.0/imageSize.x;
		
		for (int x = -1; x <= 1; x++) 
		{
			for (int y = -1; y <= 1; y++) 
			{
				vec3 offset = vec3(x * xOff, y * yOff, -bias);
				vec3 UVC = offset + pos;
				light_comp = light_comp + texture(shadow_map, UVC); 
			}
		}
		
		light_comp = (light_comp / 100) * samplesize; 
		w = (1 - light_comp); 
		c = (texture(control1, texture)).rgb;
	}
}

void main()
{
	vec3 sampleColour;
	vec3 pos = pos_light_space.xyz / pos_light_space.w; 
	pos = 0.5 + (pos / 2);
	
	vec3 l_norm = normalize(light.xyz);
	
	float cos = dot(l_norm, eye_normal);
	float bias = tan(acos(cos)) * 0.01; 
	float weight = 0;
	
	//ambient
	vec3 ambientTex = texture(control8, texture).rgb; 
	vec3 colour = ambientTex; 
	
	vec3 E = normalize(-V);
    vec3 reflected = normalize(-reflect(l_norm, eye_normal));
	
	for(int factor = 1; factor <= 3; factor++) 
	{ 
		weight = 0;	
		defColour(factor, l_norm, eye_normal, reflected, E, pos, bias, weight, sampleColour); 
		colour = (1 - weight) * colour + weight*sampleColour; 
	}
	output_colour = colour; 
}
