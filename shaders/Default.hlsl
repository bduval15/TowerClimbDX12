// Default.hlsl
struct Light {
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

cbuffer cbPass : register(b0)
{
    float4x4 gViewProj;
    float4x4 gView;
    float4x4 gProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float4 gAmbientLight;
    Light gLights[48];
    float gTotalTime;
};

float3 ComputePointLight(Light L, float3 pos, float3 normal)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);
    if (d > L.FalloffEnd)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    lightVec /= d;
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    float att = saturate((L.FalloffEnd - d) / (L.FalloffEnd - L.FalloffStart));
    return lightStrength * att;
}

float3 ApplyHeightFog(float3 color, float3 pos, float densityScale)
{
    float dist = distance(gEyePosW, pos);
    float distanceFog = saturate((dist - 70.0f) / 480.0f);
    float lowHeightFog = saturate((260.0f - pos.y) / 260.0f);
    float towerMist = saturate((pos.y - 780.0f) / 360.0f) * 0.16f;
    float fogAmount = saturate((distanceFog * (0.52f + 0.58f * lowHeightFog) + towerMist) * densityScale);

    float3 deepFog = float3(0.02f, 0.05f, 0.06f);
    float3 upperFog = float3(0.08f, 0.10f, 0.13f);
    float3 fogColor = lerp(upperFog, deepFog, lowHeightFog);

    return lerp(color, fogColor, fogAmount);
}

cbuffer cbObject : register(b1)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint gMaterialIndex;
    float3 gObjPad;
};

Texture2D gDiffuseMap : register(t0);
SamplerState gsamAnisotropicWrap : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 texSample = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

    if (gMaterialIndex == 6)
    {
        float pulse = sin(gTotalTime * 5.0f) * 0.4f + 0.6f;
        float3 N = normalize(pin.NormalW);
        float3 V = normalize(gEyePosW - pin.PosW);
        float rim = 1.0f - saturate(dot(N, V));

        float3 colorA = float3(1.0f, 0.1f, 0.1f);
        float3 colorB = float3(0.1f, 0.5f, 1.0f);
        float3 orbColor = lerp(colorA, colorB, rim) * 3.0f;
        orbColor = ApplyHeightFog(orbColor, pin.PosW, 0.35f);
        return float4(orbColor, pulse);
    }

    if (gMaterialIndex == 3)
    {
        float flicker = 0.92f + 0.08f * sin(gTotalTime * 9.0f + pin.PosW.y * 0.1f);
        return float4(float3(1.0f, 0.95f, 0.55f) * flicker, 1.0f);
    }

    if (gMaterialIndex == 7)
    {
        float4 portalTex = gDiffuseMap.Sample(
            gsamAnisotropicWrap,
            pin.TexC + float2(
                sin(gTotalTime * 1.7f + pin.TexC.y * 11.0f),
                cos(gTotalTime * 1.4f + pin.TexC.x * 9.0f)) * 0.02f);
        float2 centered = pin.TexC * 2.0f - 1.0f;
        float radius = length(centered);
        float angle = atan2(centered.y, centered.x);
        float swirl = 0.5f + 0.5f * sin(16.0f * radius - gTotalTime * 6.0f + angle * 5.0f);
        float ringPulse = 0.5f + 0.5f * sin(gTotalTime * 8.5f - radius * 34.0f);
        float arcPulse = 0.5f + 0.5f * sin(gTotalTime * 4.2f + angle * 8.0f + radius * 9.0f);
        float innerGlow = smoothstep(0.55f, 0.0f, radius);
        float outerRing = smoothstep(1.0f, 0.65f, radius);
        float alpha = saturate(portalTex.a * outerRing + innerGlow * 0.45f + ringPulse * 0.18f * outerRing);

        float3 lowCol = float3(0.05f, 0.35f, 1.0f);
        float3 highCol = float3(0.8f, 1.0f, 0.55f);
        float3 edgeCol = float3(0.18f, 0.85f, 1.0f);
        float3 portalColor = lerp(lowCol, highCol, swirl) * (0.55f + portalTex.rgb * 1.55f);
        portalColor += edgeCol * ringPulse * outerRing * 0.65f;
        portalColor += highCol * arcPulse * innerGlow * 0.25f;
        portalColor = ApplyHeightFog(portalColor * (1.05f + innerGlow), pin.PosW, 0.45f);
        return float4(portalColor, alpha);
    }

    float3 N = normalize(pin.NormalW);
    float3 V = normalize(gEyePosW - pin.PosW);
    if (dot(N, V) < 0.0f)
    {
        N = -N;
    }

    float3 ambient = gAmbientLight.rgb;
    float3 sunDir = normalize(float3(0.577f, 0.577f, 0.577f));
    float sunDiff = max(dot(N, sunDir), 0.0f);
    float3 sunColor = sunDiff * float3(0.28f, 0.30f, 0.24f);

    float3 lightVec = gEyePosW - pin.PosW;
    float d = length(lightVec);
    float atten = 1.0f / (1.0f + 0.1f * d + 0.01f * d * d);
    float diff = max(dot(N, normalize(lightVec)), 0.0f);
    float3 pointColor = diff * atten * float3(0.16f, 0.18f, 0.20f);

    float3 baseColor = texSample.rgb;
    if (gMaterialIndex == 2)
    {
        float accent = 0.65f + 0.35f * sin((pin.TexC.x + pin.TexC.y) * 8.0f + gTotalTime * 2.0f);
        baseColor *= lerp(float3(0.8f, 0.78f, 0.72f), float3(1.12f, 1.02f, 0.65f), accent);
    }

    if (gMaterialIndex == 4)
    {
        baseColor *= float3(1.15f, 0.95f, 0.95f);
    }

    if (gMaterialIndex == 5)
    {
        baseColor *= float3(1.0f, 0.96f, 0.82f);
    }

    if (gMaterialIndex == 8)
    {
        float visorGlow = smoothstep(0.20f, 0.10f, abs(pin.TexC.y - 0.15f)) *
                          smoothstep(0.28f, 0.08f, abs(pin.TexC.x - 0.5f));
        baseColor *= float3(0.92f, 1.02f, 1.08f);
        baseColor += float3(0.04f, 0.12f, 0.16f) * visorGlow;
    }

    float3 finalLight = ambient + sunColor + (pointColor * 0.6f);
    for (int i = 0; i < 48; ++i)
    {
        finalLight += ComputePointLight(gLights[i], pin.PosW, N);
    }

    float3 litColor = ApplyHeightFog(baseColor * finalLight, pin.PosW, 1.0f);
    return float4(litColor, texSample.a);
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1], inout TriangleStream<VertexOut> triStream)
{
    float3 centerPos = gin[0].PosW;
    float3 look = normalize(gEyePosW - centerPos);
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, look));
    up = cross(look, right);

    float halfWidth = 1.0f;
    float halfHeight = 1.0f;

    float4 v[4];
    v[0] = float4(centerPos - halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(centerPos - halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(centerPos + halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(centerPos + halfWidth * right + halfHeight * up, 1.0f);

    float2 uvs[4] = {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    VertexOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosW = v[i].xyz;
        gout.PosH = mul(v[i], gViewProj);
        gout.NormalW = look;
        gout.TexC = uvs[i];
        triStream.Append(gout);
    }
}

struct GS_TORCH_IN
{
    float4 PosW  : POSITION;
    float3 NormW : NORMAL;
    float2 TexC  : TEXCOORD;
};

struct GS_TORCH_OUT
{
    float4 PosH  : SV_POSITION;
    float2 TexC  : TEXCOORD0;
    float  Alpha : TEXCOORD1;
};

GS_TORCH_IN VS_Torch(VertexIn vin)
{
    GS_TORCH_IN vout;
    float4x4 world = gWorld;
    vout.PosW = mul(float4(vin.PosL, 1.0f), world);
    vout.NormW = vin.NormalL;
    vout.TexC = vin.TexC;
    return vout;
}

[maxvertexcount(4)]
void GS_Torch(
    point GS_TORCH_IN gin[1],
    inout TriangleStream<GS_TORCH_OUT> triStream)
{
    float3 right = float3(gViewProj[0][0], gViewProj[1][0], gViewProj[2][0]);
    float3 up = float3(0.0f, 1.0f, 0.0f);
    right = normalize(right);

    float3 toPoint = gin[0].PosW.xyz - gEyePosW;
    float dist = length(toPoint);
    float scale = 12.0f / (1.0f + dist * 0.008f);
    scale = clamp(scale, 0.5f, 12.0f);
    float alpha = saturate(1.0f - (dist - 50.0f) / 500.0f);

    float3 center = gin[0].PosW.xyz + up * (scale * 0.5f);
    float3 corners[4];
    corners[0] = center - right * scale * 0.5f - up * scale * 0.5f;
    corners[1] = center - right * scale * 0.5f + up * scale * 0.5f;
    corners[2] = center + right * scale * 0.5f - up * scale * 0.5f;
    corners[3] = center + right * scale * 0.5f + up * scale * 0.5f;

    float2 uvs[4] = {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        GS_TORCH_OUT gout;
        gout.PosH = mul(float4(corners[i], 1.0f), gViewProj);
        gout.TexC = uvs[i];
        gout.Alpha = alpha;
        triStream.Append(gout);
    }
    triStream.RestartStrip();
}

float4 PS_Torch(GS_TORCH_OUT pin) : SV_Target
{
    float2 uv = pin.TexC;
    float t = gTotalTime;

    float flicker = 0.85f + 0.15f * sin(t * 17.3f + uv.x * 5.0f)
                          + 0.08f * sin(t * 31.7f);

    float xFade = 1.0f - abs(uv.x - 0.5f) * 2.0f;
    float yFade = uv.y;
    float shape = pow(xFade, 1.5f) * pow(yFade, 0.6f);

    float baseAlpha = shape * flicker;
    clip(baseAlpha - 0.05f);

    float3 innerCol = float3(1.0f, 0.95f, 0.6f);
    float3 midCol = float3(1.0f, 0.45f, 0.05f);
    float3 outerCol = float3(0.8f, 0.1f, 0.02f);

    float3 col = lerp(outerCol, midCol, saturate(yFade * 2.0f));
    col = lerp(col, innerCol, saturate(shape * flicker));
    col = floor(col * 8.0f + 0.5f) / 8.0f;

    float finalAlpha = baseAlpha * pin.Alpha;
    return float4(col, finalAlpha);
}
