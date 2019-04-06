Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct  VS_OUTPUT
{
	float4  pos: SV_POSITION;
	float4  color: COLOR;
	float2  texcoord : TEXCOORD;
};

float4  main(VS_OUTPUT  input) : SV_TARGET
{
	return  input.color;
}

float4  textured(VS_OUTPUT  input) : SV_TARGET
{
	return t1.Sample(s1, input.texcoord);
}