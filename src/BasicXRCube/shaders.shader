cbuffer TransformBuffer {
	float4x4 world;
	float4x4 view_projection;
	float4x4 rotation;
	float4 light_vector;
	float4 light_color;
	float4 ambient_color;
};

struct vsIn {
	float4 position  : SV_POSITION;
	float4 normal : NORMAL;
};

struct psIn {
	float4 pos   : SV_POSITION;
	float4 color : COLOR;
};

psIn VShader(vsIn input) {
	psIn output;

	// Calculate the position
	output.pos = mul(mul(input.position, world), view_projection);

	// Calculate the color
	output.color = ambient_color;
	float4 norm = normalize(mul(rotation, input.normal));
	float diffuse_brightness = saturate(dot(norm, light_vector));
	output.color += light_color * diffuse_brightness;

	return output;
}

float4 PShader(psIn input) : SV_TARGET{
	return input.color;
}