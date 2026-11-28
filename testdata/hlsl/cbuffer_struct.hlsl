// compile: main vs_5_0

cbuffer C
{
	struct SamplerMetadata
	{
		int2 padding;
		int4 intBorderColor;
	};
	SamplerMetadata samplerMetadata[2] : packoffset(c5);
	uint dx_VertexID : packoffset(c4.x);
};

float4 main() : SV_Position
{
	return float4(samplerMetadata[0].intBorderColor);
}
