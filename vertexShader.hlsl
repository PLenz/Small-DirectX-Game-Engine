struct  VS_INPUT{
	float3  pos : POSITION;
	float4  color : COLOR;
	float2  texcoord : TEXCOORD;
};

struct  VS_OUTPUT{
	float4  pos: SV_POSITION;
	float4  color: COLOR;
	float2  texcoord : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0) {
	float4 factor;
	float4x4 wvpMat;
}

VS_OUTPUT  main(VS_INPUT  input)
{
	VS_OUTPUT  output;
	output.pos = float4(input.pos , 1.0f);
	output.color = input.color;
	output.texcoord = float2(0.0f, 0.0f);
	return  output;
}

VS_OUTPUT multiplied(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.color = input.color * factor;
	output.texcoord = float2(0.0f, 0.0f);
	return  output;
}

VS_OUTPUT rotate(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = mul(float4(input.pos, 1.0f), wvpMat);
	output.color = input.color * factor;
	output.texcoord = input.texcoord;
	return  output;
}