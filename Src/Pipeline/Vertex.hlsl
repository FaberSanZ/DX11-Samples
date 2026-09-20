float4 VS(uint vertexID : SV_VertexID) : SV_Position
{
    float4 positions[] =
    {
        float4(0.0f, 0.5f, 0.0f, 1.0f),
        float4(0.5f, -0.5f, 0.0f, 1.0f),
        float4(-0.5f, -0.5f, 0.0f, 1.0f)
    };

    return positions[vertexID];
}