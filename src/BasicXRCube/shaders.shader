cbuffer TransformBuffer {
	float4x4 world;
	float4x4 viewproj;
};

struct vsIn {
	float4 pos  : SV_POSITION;
	float3 norm : NORMAL;
};

struct psIn {
	float4 pos   : SV_POSITION;
	float4 color : COLOR;
};

psIn VShader(vsIn input) {
	psIn output;
	output.pos = mul(float4(input.pos.xyz, 1), world);
	output.pos = mul(output.pos, viewproj);

	float3 normal = normalize(mul(float4(input.norm, 0), world).xyz);

	output.color = saturate(dot(normal, float3(0, 1, 0))).xxxx;
	return output;
}

float4 PShader(psIn input) : SV_TARGET{
	return input.color;
}