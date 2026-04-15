#include "TowerGameApp.h"
#include "Common/GeometryGenerator.h"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace
{
    constexpr float kTwoPi = 6.28318530718f;

    constexpr int kWallTextureIndex = 0;
    constexpr int kFloorTextureIndex = 1;
    constexpr int kPlatformTextureIndex = 2;
    constexpr int kCarTextureIndex = 3;
    constexpr int kPedestalTextureIndex = 4;
    constexpr int kPortalTextureIndex = 5;
    constexpr int kCharacterTextureIndex = 6;
    constexpr int kTextureCount = 7;

    constexpr UINT kMaterialWall = 0;
    constexpr UINT kMaterialFloor = 1;
    constexpr UINT kMaterialPlatform = 2;
    constexpr UINT kMaterialLight = 3;
    constexpr UINT kMaterialCar = 4;
    constexpr UINT kMaterialPedestal = 5;
    constexpr UINT kMaterialOrb = 6;
    constexpr UINT kMaterialPortal = 7;
    constexpr UINT kMaterialCharacter = 8;

    struct GeneratedTextureData
    {
        UINT Width = 0;
        UINT Height = 0;
        std::vector<std::uint32_t> Pixels;
    };

    struct ManualMeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<std::uint32_t> Indices;
    };

    struct TextModelData
    {
        std::vector<Vertex> Vertices;
        std::vector<std::uint32_t> Indices;
    };

    std::filesystem::path GetExecutableDirectory()
    {
        std::array<wchar_t, MAX_PATH> buffer{};
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0 || length == buffer.size())
        {
            return std::filesystem::current_path();
        }

        return std::filesystem::path(buffer.data()).parent_path();
    }

    std::filesystem::path ResolveAssetPath(std::initializer_list<std::filesystem::path> relativePaths)
    {
        const auto cwd = std::filesystem::current_path();
        const auto exeDir = GetExecutableDirectory();
        const std::array<std::filesystem::path, 4> roots = {
            cwd,
            exeDir,
            exeDir.parent_path(),
            exeDir.parent_path().parent_path()
        };

        for (const auto& relativePath : relativePaths)
        {
            if (relativePath.is_absolute() && std::filesystem::exists(relativePath))
            {
                return relativePath;
            }

            for (const auto& root : roots)
            {
                const auto candidate = root / relativePath;
                if (std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }
        }

        return {};
    }

    std::filesystem::path ResolveModelsDirectory()
    {
        const auto cwd = std::filesystem::current_path();
        const auto exeDir = GetExecutableDirectory();
        const std::array<std::filesystem::path, 4> roots = {
            cwd,
            exeDir,
            exeDir.parent_path(),
            exeDir.parent_path().parent_path()
        };

        for (const auto& root : roots)
        {
            const auto candidate = root / L"models";
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return cwd / L"models";
    }

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers()
    {
        const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
            0,
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            8);

        return { anisotropicWrap };
    }

    std::uint32_t PackColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
    {
        return static_cast<std::uint32_t>(r)
            | (static_cast<std::uint32_t>(g) << 8)
            | (static_cast<std::uint32_t>(b) << 16)
            | (static_cast<std::uint32_t>(a) << 24);
    }

    float Frac(float value)
    {
        return value - std::floor(value);
    }

    float Hash2D(int x, int y)
    {
        std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13u)) * 1274126177u;
        return static_cast<float>(h & 0x00ffffffu) / static_cast<float>(0x00ffffffu);
    }

    void AppendQuad(TextModelData& model, const XMFLOAT3& normal, const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT3& p3)
    {
        const std::uint32_t baseIndex = static_cast<std::uint32_t>(model.Vertices.size());

        model.Vertices.push_back({ p0, normal, { 0.0f, 1.0f } });
        model.Vertices.push_back({ p1, normal, { 1.0f, 1.0f } });
        model.Vertices.push_back({ p2, normal, { 1.0f, 0.0f } });
        model.Vertices.push_back({ p3, normal, { 0.0f, 0.0f } });

        model.Indices.push_back(baseIndex + 0);
        model.Indices.push_back(baseIndex + 1);
        model.Indices.push_back(baseIndex + 2);
        model.Indices.push_back(baseIndex + 0);
        model.Indices.push_back(baseIndex + 2);
        model.Indices.push_back(baseIndex + 3);
    }

    void AppendBox(TextModelData& model, const XMFLOAT3& center, const XMFLOAT3& extents)
    {
        const float minX = center.x - extents.x;
        const float maxX = center.x + extents.x;
        const float minY = center.y - extents.y;
        const float maxY = center.y + extents.y;
        const float minZ = center.z - extents.z;
        const float maxZ = center.z + extents.z;

        AppendQuad(model, { 0.0f, 0.0f, -1.0f },
            { minX, minY, minZ }, { maxX, minY, minZ }, { maxX, maxY, minZ }, { minX, maxY, minZ });
        AppendQuad(model, { 0.0f, 0.0f, 1.0f },
            { maxX, minY, maxZ }, { minX, minY, maxZ }, { minX, maxY, maxZ }, { maxX, maxY, maxZ });
        AppendQuad(model, { -1.0f, 0.0f, 0.0f },
            { minX, minY, maxZ }, { minX, minY, minZ }, { minX, maxY, minZ }, { minX, maxY, maxZ });
        AppendQuad(model, { 1.0f, 0.0f, 0.0f },
            { maxX, minY, minZ }, { maxX, minY, maxZ }, { maxX, maxY, maxZ }, { maxX, maxY, minZ });
        AppendQuad(model, { 0.0f, 1.0f, 0.0f },
            { minX, maxY, minZ }, { maxX, maxY, minZ }, { maxX, maxY, maxZ }, { minX, maxY, maxZ });
        AppendQuad(model, { 0.0f, -1.0f, 0.0f },
            { minX, minY, maxZ }, { maxX, minY, maxZ }, { maxX, minY, minZ }, { minX, minY, minZ });
    }

    std::filesystem::path EnsureCharacterModelPath()
    {
        const auto existingPath = ResolveAssetPath({ L"models\\guardian.txt" });
        if (!existingPath.empty())
        {
            return existingPath;
        }

        const auto modelsDir = ResolveModelsDirectory();
        std::filesystem::create_directories(modelsDir);
        const auto characterPath = modelsDir / L"guardian.txt";

        TextModelData model;
        AppendBox(model, { 0.0f, 7.0f, 0.0f }, { 1.7f, 2.6f, 0.9f });
        AppendBox(model, { 0.0f, 11.0f, 0.0f }, { 1.1f, 1.1f, 1.0f });
        AppendBox(model, { 0.0f, 4.0f, 0.0f }, { 1.4f, 1.2f, 0.8f });
        AppendBox(model, { -2.5f, 7.1f, 0.0f }, { 0.55f, 2.0f, 0.55f });
        AppendBox(model, { 2.5f, 7.1f, 0.0f }, { 0.55f, 2.0f, 0.55f });
        AppendBox(model, { -0.9f, 1.8f, 0.0f }, { 0.7f, 1.8f, 0.7f });
        AppendBox(model, { 0.9f, 1.8f, 0.0f }, { 0.7f, 1.8f, 0.7f });
        AppendBox(model, { 0.0f, 9.0f, -1.15f }, { 0.9f, 1.2f, 0.35f });

        std::ofstream fout(characterPath);
        if (!fout)
        {
            throw DxException(E_FAIL, L"std::ofstream(models/guardian.txt)", L"TowerGameApp.cpp", __LINE__);
        }

        fout << std::fixed << std::setprecision(6);
        fout << "VertexCount: " << model.Vertices.size() << "\n";
        fout << "TriangleCount: " << model.Indices.size() / 3 << "\n";
        fout << "VertexList (pos, normal)\n";
        fout << "{\n";
        for (const auto& vertex : model.Vertices)
        {
            fout << "\t"
                 << vertex.Pos.x << " " << vertex.Pos.y << " " << vertex.Pos.z << " "
                 << vertex.Normal.x << " " << vertex.Normal.y << " " << vertex.Normal.z << "\n";
        }
        fout << "}\n";
        fout << "TriangleList\n";
        fout << "{\n";
        for (size_t i = 0; i < model.Indices.size(); i += 3)
        {
            fout << "\t"
                 << model.Indices[i + 0] << " "
                 << model.Indices[i + 1] << " "
                 << model.Indices[i + 2] << "\n";
        }
        fout << "}\n";

        return characterPath;
    }

    GeneratedTextureData CreateStoneTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float row = std::floor(v * 8.0f);
                const float brickU = Frac(u * 8.0f + std::fmod(row, 2.0f) * 0.5f);
                const float brickV = Frac(v * 8.0f);
                const bool mortar = brickU < 0.08f || brickV < 0.08f;
                const float noise = Hash2D(static_cast<int>(x / 4), static_cast<int>(y / 4));
                const float shade = 0.55f + 0.35f * noise;

                std::uint8_t r = static_cast<std::uint8_t>(120.0f * shade);
                std::uint8_t g = static_cast<std::uint8_t>(130.0f * shade);
                std::uint8_t b = static_cast<std::uint8_t>(145.0f * shade);

                if (mortar)
                {
                    r = 60;
                    g = 65;
                    b = 72;
                }

                data.Pixels[y * data.Width + x] = PackColor(r, g, b);
            }
        }

        return data;
    }

    GeneratedTextureData CreateFloorTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float weave = 0.5f + 0.5f * std::sin((u + v) * 40.0f);
                const bool seam = Frac(u * 10.0f) < 0.05f || Frac(v * 10.0f) < 0.05f;
                const float noise = Hash2D(static_cast<int>(x / 3), static_cast<int>(y / 3));

                float r = 25.0f + weave * 12.0f + noise * 6.0f;
                float g = 45.0f + weave * 14.0f + noise * 8.0f;
                float b = 70.0f + weave * 20.0f + noise * 10.0f;

                if (seam)
                {
                    r *= 0.55f;
                    g *= 0.55f;
                    b *= 0.55f;
                }

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)));
            }
        }

        return data;
    }

    GeneratedTextureData CreatePlatformTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const bool stripe = std::fmod(std::floor((u + v) * 7.0f), 2.0f) < 1.0f;
                const bool rivet = std::abs(Frac(u * 4.0f) - 0.5f) < 0.05f && std::abs(Frac(v * 4.0f) - 0.5f) < 0.05f;
                const float noise = Hash2D(static_cast<int>(x / 5), static_cast<int>(y / 5));

                float r = stripe ? 180.0f : 95.0f;
                float g = stripe ? 145.0f : 78.0f;
                float b = stripe ? 60.0f : 55.0f;

                r += noise * 18.0f;
                g += noise * 14.0f;
                b += noise * 8.0f;

                if (rivet)
                {
                    r = 235.0f;
                    g = 210.0f;
                    b = 120.0f;
                }

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)));
            }
        }

        return data;
    }

    GeneratedTextureData CreateCarTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float highlight = 0.75f + 0.25f * std::sin(u * 12.0f) * std::sin(v * 6.0f);
                const bool stripe = std::abs(v - 0.35f) < 0.08f || std::abs(v - 0.65f) < 0.08f;

                float r = 165.0f * highlight;
                float g = 38.0f * highlight;
                float b = 34.0f * highlight;

                if (stripe)
                {
                    r = 210.0f;
                    g = 205.0f;
                    b = 215.0f;
                }

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)));
            }
        }

        return data;
    }

    GeneratedTextureData CreatePedestalTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float rune = 0.5f + 0.5f * std::sin(u * 20.0f) * std::cos(v * 18.0f);
                const bool inlay = rune > 0.82f;
                const float grain = Hash2D(static_cast<int>(x / 4), static_cast<int>(y / 4));

                float r = 90.0f + grain * 24.0f;
                float g = 84.0f + grain * 18.0f;
                float b = 72.0f + grain * 16.0f;

                if (inlay)
                {
                    r = 215.0f;
                    g = 185.0f;
                    b = 92.0f;
                }

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)));
            }
        }

        return data;
    }

    GeneratedTextureData CreatePortalTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float cx = u * 2.0f - 1.0f;
                const float cy = v * 2.0f - 1.0f;
                const float radius = std::sqrt(cx * cx + cy * cy);
                const float angle = std::atan2(cy, cx);
                const float swirl = 0.5f + 0.5f * std::sin(10.0f * radius - angle * 4.0f);
                const float alpha = std::clamp(1.0f - radius, 0.0f, 1.0f);

                float r = 35.0f + swirl * 70.0f;
                float g = 110.0f + swirl * 90.0f;
                float b = 180.0f + swirl * 55.0f;
                const std::uint8_t a = static_cast<std::uint8_t>(alpha * 255.0f);

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)),
                    a);
            }
        }

        return data;
    }

    GeneratedTextureData CreateCharacterTexture()
    {
        GeneratedTextureData data;
        data.Width = 128;
        data.Height = 128;
        data.Pixels.resize(data.Width * data.Height);

        for (UINT y = 0; y < data.Height; ++y)
        {
            for (UINT x = 0; x < data.Width; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(data.Width - 1);
                const float v = static_cast<float>(y) / static_cast<float>(data.Height - 1);
                const float panelNoise = Hash2D(static_cast<int>(x / 6), static_cast<int>(y / 6));
                const bool trim = std::abs(v - 0.18f) < 0.04f || std::abs(v - 0.78f) < 0.035f;
                const bool visor = v > 0.08f && v < 0.22f && u > 0.28f && u < 0.72f;

                float r = 54.0f + panelNoise * 20.0f;
                float g = 86.0f + panelNoise * 28.0f;
                float b = 88.0f + panelNoise * 22.0f;

                if (trim)
                {
                    r = 165.0f;
                    g = 126.0f;
                    b = 72.0f;
                }

                if (visor)
                {
                    r = 90.0f;
                    g = 214.0f;
                    b = 220.0f;
                }

                data.Pixels[y * data.Width + x] = PackColor(
                    static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)));
            }
        }

        return data;
    }

    ManualMeshData BuildPortalMaskMesh(int segments, float radius)
    {
        ManualMeshData mesh;
        mesh.Vertices.push_back({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.5f } });

        for (int i = 0; i <= segments; ++i)
        {
            const float angle = static_cast<float>(i) / static_cast<float>(segments) * kTwoPi;
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            const float u = 0.5f + (x / (radius * 2.0f));
            const float v = 0.5f - (z / (radius * 2.0f));
            mesh.Vertices.push_back({ { x, 0.0f, z }, { 0.0f, 1.0f, 0.0f }, { u, v } });
        }

        for (int i = 1; i <= segments; ++i)
        {
            mesh.Indices.push_back(0);
            mesh.Indices.push_back(i);
            mesh.Indices.push_back(i + 1);
        }

        return mesh;
    }

    ManualMeshData BuildPortalPlaneMesh(float halfExtent)
    {
        ManualMeshData mesh;
        mesh.Vertices = {
            { { -halfExtent, 0.0f, -halfExtent }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
            { { -halfExtent, 0.0f,  halfExtent }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            { {  halfExtent, 0.0f,  halfExtent }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { {  halfExtent, 0.0f, -halfExtent }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } }
        };
        mesh.Indices = { 0, 1, 2, 0, 2, 3 };
        return mesh;
    }

    ManualMeshData BuildRegularPrismMesh(int sides, float radius, float halfHeight)
    {
        ManualMeshData mesh;

        mesh.Vertices.push_back({ { 0.0f, halfHeight, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.5f } });
        for (int i = 0; i < sides; ++i)
        {
            const float angle = static_cast<float>(i) / static_cast<float>(sides) * kTwoPi;
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            mesh.Vertices.push_back({ { x, halfHeight, z }, { 0.0f, 1.0f, 0.0f }, { 0.5f + 0.5f * std::cos(angle), 0.5f - 0.5f * std::sin(angle) } });
        }

        for (int i = 1; i < sides; ++i)
        {
            mesh.Indices.push_back(0);
            mesh.Indices.push_back(i + 1);
            mesh.Indices.push_back(i);
        }
        mesh.Indices.push_back(0);
        mesh.Indices.push_back(1);
        mesh.Indices.push_back(sides);

        const std::uint32_t bottomCenterIndex = static_cast<std::uint32_t>(mesh.Vertices.size());
        mesh.Vertices.push_back({ { 0.0f, -halfHeight, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.5f, 0.5f } });
        for (int i = 0; i < sides; ++i)
        {
            const float angle = static_cast<float>(i) / static_cast<float>(sides) * kTwoPi;
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            mesh.Vertices.push_back({ { x, -halfHeight, z }, { 0.0f, -1.0f, 0.0f }, { 0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle) } });
        }

        for (int i = 1; i < sides; ++i)
        {
            mesh.Indices.push_back(bottomCenterIndex);
            mesh.Indices.push_back(bottomCenterIndex + i);
            mesh.Indices.push_back(bottomCenterIndex + i + 1);
        }
        mesh.Indices.push_back(bottomCenterIndex);
        mesh.Indices.push_back(bottomCenterIndex + sides);
        mesh.Indices.push_back(bottomCenterIndex + 1);

        for (int i = 0; i < sides; ++i)
        {
            const float angle0 = static_cast<float>(i) / static_cast<float>(sides) * kTwoPi;
            const float angle1 = static_cast<float>(i + 1) / static_cast<float>(sides) * kTwoPi;
            const float x0 = radius * std::cos(angle0);
            const float z0 = radius * std::sin(angle0);
            const float x1 = radius * std::cos(angle1);
            const float z1 = radius * std::sin(angle1);
            const float midAngle = (angle0 + angle1) * 0.5f;
            const XMFLOAT3 normal = { std::cos(midAngle), 0.0f, std::sin(midAngle) };

            const std::uint32_t base = static_cast<std::uint32_t>(mesh.Vertices.size());
            const float u0 = static_cast<float>(i) / static_cast<float>(sides);
            const float u1 = static_cast<float>(i + 1) / static_cast<float>(sides);

            mesh.Vertices.push_back({ { x0,  halfHeight, z0 }, normal, { u0, 0.0f } });
            mesh.Vertices.push_back({ { x1,  halfHeight, z1 }, normal, { u1, 0.0f } });
            mesh.Vertices.push_back({ { x0, -halfHeight, z0 }, normal, { u0, 1.0f } });
            mesh.Vertices.push_back({ { x1, -halfHeight, z1 }, normal, { u1, 1.0f } });

            mesh.Indices.push_back(base + 0);
            mesh.Indices.push_back(base + 2);
            mesh.Indices.push_back(base + 1);
            mesh.Indices.push_back(base + 1);
            mesh.Indices.push_back(base + 2);
            mesh.Indices.push_back(base + 3);
        }

        return mesh;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try {
        TowerGameApp theApp(hInstance);
        if (!theApp.Initialize()) return 0;
        return theApp.Run();
    }
    catch (DxException& e) {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TowerGameApp::TowerGameApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    mMainWndCaption = L"DX12 Tower Climber - Inside the Spire";
}

TowerGameApp::~TowerGameApp() {
    if (mControlsFont != nullptr) {
        DeleteObject(mControlsFont);
        mControlsFont = nullptr;
    }

    if (mControlsBrush != nullptr) {
        DeleteObject(mControlsBrush);
        mControlsBrush = nullptr;
    }
}

bool TowerGameApp::Initialize() {
    if (!D3DApp::Initialize()) return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mPlayerPosition = XMFLOAT3(0.0f, eyeLevel, -30.0f);
    UpdateFlyCamera();

    BuildTextures();
    BuildDescriptorHeaps();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildTowerGeometry();
    BuildCustomGeometry();
    BuildCarGeometry();
    BuildCharacterGeometry();
    BuildFrameResources();
    BuildRenderItems();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    CreateControlsOverlay();
    UpdateControlsOverlay();
    UpdateCharacterTransform();

    return true;
}

LRESULT TowerGameApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == mControlsOverlay) {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(228, 233, 240));
            SetBkColor(hdc, RGB(11, 14, 20));
            return reinterpret_cast<LRESULT>(mControlsBrush);
        }
        break;
    }

    return D3DApp::MsgProc(hwnd, msg, wParam, lParam);
}

void TowerGameApp::OnResize() {
    D3DApp::OnResize();
    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 2000.0f);
    if (mControlsOverlay != nullptr) {
        SetWindowPos(mControlsOverlay, HWND_TOP, 12, 12, 360, 92, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void TowerGameApp::CreateControlsOverlay() {
    if (mControlsBrush == nullptr) {
        mControlsBrush = CreateSolidBrush(RGB(11, 14, 20));
    }

    if (mControlsFont == nullptr) {
        mControlsFont = CreateFontW(
            -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }

    if (mControlsOverlay == nullptr) {
        mControlsOverlay = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            12,
            12,
            360,
            92,
            mhMainWnd,
            nullptr,
            mhAppInst,
            nullptr);
    }

    if (mControlsOverlay != nullptr) {
        SendMessageW(mControlsOverlay, WM_SETFONT, reinterpret_cast<WPARAM>(mControlsFont), TRUE);
        SetWindowPos(mControlsOverlay, HWND_TOP, 12, 12, 360, 92, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void TowerGameApp::UpdateControlsOverlay() {
    if (mControlsOverlay == nullptr) {
        return;
    }

    const std::wstring modeLabel = mFreeFlyMode ? L"Mode: Free-Fly Demo" : L"Mode: Third-Person";
    const std::wstring controlsText =
        modeLabel + L"   |   F toggles view\r\n"
        L"Mouse: look around\r\n"
        L"Free-fly: WASD move, Q/E down/up, Shift boost\r\n"
        L"Third-person: WASD move, Space jump, Q/E zoom";

    SetWindowTextW(mControlsOverlay, controlsText.c_str());
}

void TowerGameApp::UpdateCharacterTransform() {
    if (mCharacterRitem == nullptr) {
        return;
    }

    const float characterBaseY = mPlayerPosition.y - eyeLevel;
    const XMMATRIX world =
        XMMatrixScaling(0.90f, 0.90f, 0.90f) *
        XMMatrixRotationY(mYaw) *
        XMMatrixTranslation(mPlayerPosition.x, characterBaseY, mPlayerPosition.z);

    XMStoreFloat4x4(&mCharacterRitem->World, world);
}

void TowerGameApp::UpdateChaseCamera() {
    XMFLOAT3 target = mPlayerPosition;
    target.y += mCameraLookOffset;

    const float pitchCos = std::cos(mPitch);
    const XMFLOAT3 lookDir = {
        std::sin(mYaw) * pitchCos,
        -std::sin(mPitch),
        std::cos(mYaw) * pitchCos
    };

    constexpr float kTowerCameraRadius = 132.0f;
    constexpr float kTowerCameraRadiusSq = kTowerCameraRadius * kTowerCameraRadius;

    XMFLOAT3 eye = target;
    bool foundInteriorPosition = false;

    for (float distance = mCameraDistance; distance >= mMinCameraDistance; distance -= 1.5f) {
        XMFLOAT3 candidate = {
            target.x - lookDir.x * distance,
            target.y - lookDir.y * distance,
            target.z - lookDir.z * distance
        };

        const float radialDistanceSq = candidate.x * candidate.x + candidate.z * candidate.z;
        if (radialDistanceSq <= kTowerCameraRadiusSq) {
            eye = candidate;
            foundInteriorPosition = true;
            break;
        }

        eye = candidate;
    }

    if (!foundInteriorPosition) {
        const float radialDistance = std::sqrt(eye.x * eye.x + eye.z * eye.z);
        if (radialDistance > kTowerCameraRadius) {
            const float scale = kTowerCameraRadius / radialDistance;
            eye.x *= scale;
            eye.z *= scale;
        }
    }

    eye.y = std::clamp(eye.y, eyeLevel + 2.0f, 1200.0f);

    mCamera.LookAt(eye, target, XMFLOAT3(0.0f, 1.0f, 0.0f));
    mCamera.UpdateViewMatrix();
}

void TowerGameApp::UpdateFlyCamera() {
    const float pitchCos = std::cos(mFlyPitch);
    const XMFLOAT3 lookDir = {
        std::sin(mFlyYaw) * pitchCos,
        -std::sin(mFlyPitch),
        std::cos(mFlyYaw) * pitchCos
    };

    const XMFLOAT3 target = {
        mFlyCameraPosition.x + lookDir.x,
        mFlyCameraPosition.y + lookDir.y,
        mFlyCameraPosition.z + lookDir.z
    };

    mCamera.LookAt(mFlyCameraPosition, target, XMFLOAT3(0.0f, 1.0f, 0.0f));
    mCamera.UpdateViewMatrix();
}

void TowerGameApp::Update(const GameTimer& gt) {
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % 3;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    const float dt = gt.DeltaTime();

    const bool togglePressed = (GetAsyncKeyState('F') & 0x8000) != 0;
    if (togglePressed && !mToggleViewPressed) {
        mFreeFlyMode = !mFreeFlyMode;

        if (mFreeFlyMode) {
            const XMFLOAT3 look = mCamera.GetLook3f();
            mFlyCameraPosition = mCamera.GetPosition3f();
            mFlyYaw = std::atan2(look.x, look.z);
            mFlyPitch = std::asin(std::clamp(-look.y, -0.98f, 0.98f));
            UpdateFlyCamera();
        }
        else {
            UpdateChaseCamera();
        }

        UpdateControlsOverlay();
    }
    mToggleViewPressed = togglePressed;

    if (mFreeFlyMode) {
        const float flySpeed = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 110.0f : 65.0f) * dt;
        const float pitchCos = std::cos(mFlyPitch);
        const XMFLOAT3 forward = {
            std::sin(mFlyYaw) * pitchCos,
            -std::sin(mFlyPitch),
            std::cos(mFlyYaw) * pitchCos
        };
        const XMFLOAT3 right = {
            std::cos(mFlyYaw),
            0.0f,
            -std::sin(mFlyYaw)
        };

        if (GetAsyncKeyState('W') & 0x8000) {
            mFlyCameraPosition.x += forward.x * flySpeed;
            mFlyCameraPosition.y += forward.y * flySpeed;
            mFlyCameraPosition.z += forward.z * flySpeed;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            mFlyCameraPosition.x -= forward.x * flySpeed;
            mFlyCameraPosition.y -= forward.y * flySpeed;
            mFlyCameraPosition.z -= forward.z * flySpeed;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            mFlyCameraPosition.x -= right.x * flySpeed;
            mFlyCameraPosition.z -= right.z * flySpeed;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            mFlyCameraPosition.x += right.x * flySpeed;
            mFlyCameraPosition.z += right.z * flySpeed;
        }
        if (GetAsyncKeyState('E') & 0x8000) mFlyCameraPosition.y += flySpeed;
        if (GetAsyncKeyState('Q') & 0x8000) mFlyCameraPosition.y -= flySpeed;

        UpdateFlyCamera();
    }
    else {
        const float moveSpeed = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 64.0f : 46.0f) * dt;
        const float zoomSpeed = 24.0f * dt;

        if (GetAsyncKeyState('Q') & 0x8000) mCameraDistance = std::clamp(mCameraDistance + zoomSpeed, mMinCameraDistance, mMaxCameraDistance);
        if (GetAsyncKeyState('E') & 0x8000) mCameraDistance = std::clamp(mCameraDistance - zoomSpeed, mMinCameraDistance, mMaxCameraDistance);

        XMFLOAT3 pos = mPlayerPosition;
        XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };
        const XMFLOAT3 forward = { std::sin(mYaw), 0.0f, std::cos(mYaw) };
        const XMFLOAT3 right = { std::cos(mYaw), 0.0f, -std::sin(mYaw) };

        if (GetAsyncKeyState('W') & 0x8000) {
            moveDir.x += forward.x;
            moveDir.z += forward.z;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            moveDir.x -= forward.x;
            moveDir.z -= forward.z;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            moveDir.x += right.x;
            moveDir.z += right.z;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            moveDir.x -= right.x;
            moveDir.z -= right.z;
        }

        const float moveLengthSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
        if (moveLengthSq > 0.0f) {
            const float invMoveLength = 1.0f / std::sqrt(moveLengthSq);
            pos.x += moveDir.x * invMoveLength * moveSpeed;
            pos.z += moveDir.z * invMoveLength * moveSpeed;
        }

        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && !mIsJumping) {
            mVerticalVelocity = 90.0f;
            mIsJumping = true;
        }
        mVerticalVelocity -= 150.0f * dt;

        float nextY = pos.y + (mVerticalVelocity * dt);

        if (mCarRitem != nullptr) {
            if (pos.x > 60.0f - 18.0f && pos.x < 60.0f + 18.0f &&
                pos.z > 70.0f - 35.0f && pos.z < 70.0f + 35.0f) {

                if (mVerticalVelocity <= 0 && pos.y >= carHeight + 4.5f && nextY <= carHeight + eyeLevel) {
                    nextY = carHeight + eyeLevel;
                    mVerticalVelocity = 0.0f;
                    mIsJumping = false;
                }
            }
        }

        for (auto* ri : mPlatformRitems) {
            const float time = gt.TotalTime();
            const float angle = 0.4f * time * (ri->ObjCBIndex % 2 == 0 ? 1.0f : -1.0f);
            const XMMATRIX baseWorld = XMLoadFloat4x4(&ri->World);
            const XMMATRIX animatedWorld = baseWorld * XMMatrixRotationY(angle);

            const float px = animatedWorld.r[3].m128_f32[0];
            const float py = animatedWorld.r[3].m128_f32[1];
            const float pz = animatedWorld.r[3].m128_f32[2];

            const float previousAngle = 0.4f * (time - dt) * (ri->ObjCBIndex % 2 == 0 ? 1.0f : -1.0f);
            const XMMATRIX previousAnimatedWorld = baseWorld * XMMatrixRotationY(previousAngle);
            const float prvPx = previousAnimatedWorld.r[3].m128_f32[0];
            const float prvPy = previousAnimatedWorld.r[3].m128_f32[1];
            const float prvPz = previousAnimatedWorld.r[3].m128_f32[2];

            if (pos.x > px - 8.0f && pos.x < px + 8.0f &&
                pos.z > pz - 8.0f && pos.z < pz + 8.0f) {
                if (mVerticalVelocity <= 0 && pos.y >= py + 4.5f && nextY <= py + eyeLevel) {
                    nextY = py + eyeLevel;
                    mVerticalVelocity = 0.0f;
                    mIsJumping = false;

                    pos.x += px - prvPx;
                    pos.y += py - prvPy;
                    pos.z += pz - prvPz;
                    break;
                }
            }
        }

        if (mPedestalRitem != nullptr) {
            if (std::abs(pos.x) < 34.0f && std::abs(pos.z) < 34.0f) {
                if (mVerticalVelocity <= 0 && pos.y >= mSummitPedestalTop + 4.5f && nextY <= mSummitPedestalTop + eyeLevel) {
                    nextY = mSummitPedestalTop + eyeLevel;
                    mVerticalVelocity = 0.0f;
                    mIsJumping = false;
                }
            }
        }

        if (nextY < eyeLevel) {
            nextY = eyeLevel;
            mVerticalVelocity = 0.0f;
            mIsJumping = false;
        }

        if (!mHasWon) {
            XMFLOAT3 orbPos = XMFLOAT3(0.0f, 1100.0f, 0.0f);
            float dx = pos.x - orbPos.x;
            float dy = pos.y - orbPos.y;
            float dz = pos.z - orbPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < 50.0f * 50.0f) {
                mHasWon = true;
                MessageBox(mhMainWnd, L"You reached the orb! YOU WIN!", L"VICTORY!", MB_OK);
            }
        }

        mPlayerPosition = XMFLOAT3(pos.x, nextY, pos.z);
        UpdateChaseCamera();
    }

    UpdateCharacterTransform();

    PassConstants passCB;
    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();
    XMMATRIX viewProj = view * proj;
    XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
    passCB.EyePosW = mCamera.GetPosition3f();
    passCB.AmbientLight = XMFLOAT4(0.09f, 0.10f, 0.13f, 1.0f);
    passCB.TotalTime = gt.TotalTime();

    float lightHeights[6] = { 50.0f, 250.0f, 450.0f, 650.0f, 850.0f, 1050.0f };
    int lightIndex = 0;

    for (int ring = 0; ring < 6; ++ring) {
        for (int i = 0; i < 8; ++i) {
            float angle = i * (MathHelper::Pi / 4.0f);
            float primaryFlicker = 0.72f + 0.28f * (0.5f + 0.5f * std::sin(gt.TotalTime() * 11.0f + ring * 1.7f + i * 0.9f));
            float secondaryFlicker = 0.85f + 0.15f * std::sin(gt.TotalTime() * 23.0f + ring * 0.6f + i * 2.1f);
            float lightScale = primaryFlicker * secondaryFlicker;

            passCB.Lights[lightIndex].Position = { 145.0f * std::cos(angle), lightHeights[ring], 145.0f * std::sin(angle) };
            passCB.Lights[lightIndex].Strength = { 0.78f * lightScale, 0.62f * lightScale, 0.18f * lightScale };
            passCB.Lights[lightIndex].FalloffStart = 16.0f;
            passCB.Lights[lightIndex].FalloffEnd = 105.0f + 18.0f * primaryFlicker;

            lightIndex++;
        }
    }

    mCurrFrameResource->PassCB->CopyData(0, passCB);
}

void TowerGameApp::Draw(const GameTimer& gt) {
    auto alloc = mCurrFrameResource->CmdListAlloc.Get();
    ThrowIfFailed(alloc->Reset());
    ThrowIfFailed(mCommandList->Reset(alloc, mOpaquePSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto b1 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &b1);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DepthStencilView();
    const float eerieBackdrop[4] = { 0.02f, 0.03f, 0.05f, 1.0f };
    mCommandList->ClearRenderTargetView(rtv, eerieBackdrop, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetGraphicsRootConstantBufferView(0, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    auto bindTexture = [&](const RenderItem* ri) {
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(ri->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);
        mCommandList->SetGraphicsRootDescriptorTable(2, handle);
    };

    auto bindObjectConstants = [&](const RenderItem* ri, const XMMATRIX& world) {
        ObjectConstants objCB;
        XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
        XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(XMLoadFloat4x4(&ri->TexTransform)));
        objCB.MaterialIndex = ri->MaterialIndex;
        mCurrFrameResource->ObjectCB->CopyData(ri->ObjCBIndex, objCB);

        auto addr = mCurrFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress() + (ri->ObjCBIndex * objCBByteSize);
        mCommandList->SetGraphicsRootConstantBufferView(1, addr);
        bindTexture(ri);
    };

    auto drawItem = [&](const RenderItem* ri, const XMMATRIX& world) {
        if (ri == nullptr || ri->Geo == nullptr || ri->Geo->VertexBufferGPU == nullptr || ri->Geo->IndexBufferGPU == nullptr) {
            return;
        }

        mCommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        mCommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);
        bindObjectConstants(ri, world);
        mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    };

    mCommandList->SetPipelineState(mOpaquePSO.Get());
    for (auto* ri : mOpaqueRitems) {
        XMMATRIX world = XMLoadFloat4x4(&ri->World);
        if (ri->IsAnimated) {
            float angle = 0.4f * gt.TotalTime() * (ri->ObjCBIndex % 2 == 0 ? 1.0f : -1.0f);
            world = world * XMMatrixRotationY(angle);
        }
        drawItem(ri, world);
    }

    mCommandList->SetPipelineState(mStencilMaskPSO.Get());
    for (auto* ri : mStencilMaskRitems) {
        drawItem(ri, XMLoadFloat4x4(&ri->World));
    }

    mCommandList->SetPipelineState(mStencilPortalPSO.Get());
    for (auto* ri : mPortalRitems) {
        drawItem(ri, XMLoadFloat4x4(&ri->World));
    }

    // --- PASS 2: Draw transparent orb last, with blending PSO ---
    mCommandList->SetPipelineState(mTransparentPSO.Get());
    {
        drawItem(mOrbRitem, XMLoadFloat4x4(&mOrbRitem->World));
#if 0

        ObjectConstants objCB;
        objCB.MaterialIndex = 2; // Orb material � handled in shader
        mCurrFrameResource->ObjectCB->CopyData(ri->ObjCBIndex, objCB);

        auto addr = mCurrFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress() + (ri->ObjCBIndex * objCBByteSize);
        mCommandList->SetGraphicsRootConstantBufferView(1, addr);
        mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
#endif
    }

    // --- PASS 3: Draw Geometry Shader Sparks ---
    mCommandList->SetPipelineState(mSparkPSO.Get());
    for (auto& ri : mSparkRitems) {
        drawItem(ri, XMLoadFloat4x4(&ri->World));
#if 0

        ObjectConstants objCB;
        XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(XMLoadFloat4x4(&ri->World)));
        objCB.MaterialIndex = 2; // Share the pulsing color of the Orb!

        mCurrFrameResource->ObjectCB->CopyData(ri->ObjCBIndex, objCB);

        auto addr = mCurrFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress() + (ri->ObjCBIndex * objCBByteSize);
        mCommandList->SetGraphicsRootConstantBufferView(1, addr);
        mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
#endif
    }

    // --- PASS 4: Draw Torch Billboards ---
    mCommandList->SetPipelineState(mTorchPSO.Get());
    for (auto& ri : mTorchRitems) {
        drawItem(ri, XMLoadFloat4x4(&ri->World));
#if 0

        ObjectConstants objCB;
        XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(XMLoadFloat4x4(&ri->World)));
        objCB.MaterialIndex = 4; // New torch material index
        mCurrFrameResource->ObjectCB->CopyData(ri->ObjCBIndex, objCB);

        auto addr = mCurrFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress()
            + (ri->ObjCBIndex * objCBByteSize);
        mCommandList->SetGraphicsRootConstantBufferView(1, addr);
        mCommandList->DrawIndexedInstanced(
            ri->IndexCount, 1,
            ri->StartIndexLocation,
            ri->BaseVertexLocation, 0);
#endif
    }

    auto b2 = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &b2);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TowerGameApp::BuildTowerGeometry() {
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(12.0f, 1.0f, 12.0f, 3);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(150.0f, 150.0f, 1500.0f, 60, 60);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(400.0f, 400.0f, 20, 20);
    // NEW: Sphere for the goal orb at the top of the tower
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(20.0f, 32, 32);
    GeometryGenerator::MeshData lightBox = geoGen.CreateBox(4.0f, 4.0f, 1.0f, 0);

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    auto addMesh = [&](GeometryGenerator::MeshData& data, SubmeshGeometry& sub) {
        sub.IndexCount = (UINT)data.Indices32.size();
        sub.StartIndexLocation = (UINT)indices.size();
        sub.BaseVertexLocation = (UINT)vertices.size();
        for (auto& v : data.Vertices) {
            Vertex nv; nv.Pos = v.Position; nv.Normal = v.Normal; nv.TexC = v.TexC;
            vertices.push_back(nv);
        }
        for (auto& i : data.Indices32) indices.push_back(i);
        };

    SubmeshGeometry boxSub, cylSub, gridSub, sphereSub, lightBoxSub;
    addMesh(box, boxSub);
    addMesh(cylinder, cylSub);
    addMesh(grid, gridSub);
    addMesh(sphere, sphereSub);
    addMesh(lightBox, lightBoxSub);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "towerGeo";
    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), (UINT)vertices.size() * sizeof(Vertex), geo->VertexBufferUploader);
    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), (UINT)indices.size() * sizeof(uint32_t), geo->IndexBufferUploader);
    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = (UINT)indices.size() * sizeof(uint32_t);
    geo->DrawArgs["box"] = boxSub;
    geo->DrawArgs["cylinder"] = cylSub;
    geo->DrawArgs["grid"] = gridSub;
    geo->DrawArgs["sphere"] = sphereSub;
    geo->DrawArgs["lightBox"] = lightBoxSub;
    mGeometries[geo->Name] = std::move(geo);

    // 6. Energy Sparks (ObjCBIndex 117)
    // We only need to define Positions. The GS will build the actual geometry.
    std::vector<Vertex> sparkVertices;
    std::vector<std::uint32_t> sparkIndices;

    for (int i = 0; i < 50; ++i) {
        Vertex v;
        // Random point within a 60 unit radius around the orb
        v.Pos.x = (MathHelper::RandF() * 120.0f) - 60.0f;
        v.Pos.y = 1100.0f + ((MathHelper::RandF() * 40.0f) - 20.0f);
        v.Pos.z = (MathHelper::RandF() * 120.0f) - 60.0f;

        v.Normal = { 0, 1, 0 }; // Ignored by GS
        v.TexC = { 0, 0 };      // Ignored by GS

        sparkVertices.push_back(v);
        sparkIndices.push_back(i);
    }

    // Build the MeshGeometry for the points
    auto sparkGeo = std::make_unique<MeshGeometry>();
    sparkGeo->Name = "sparkGeo";
    sparkGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), sparkVertices.data(), (UINT)sparkVertices.size() * sizeof(Vertex), sparkGeo->VertexBufferUploader);
    sparkGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), sparkIndices.data(), (UINT)sparkIndices.size() * sizeof(uint32_t), sparkGeo->IndexBufferUploader);
    sparkGeo->VertexByteStride = sizeof(Vertex);
    sparkGeo->VertexBufferByteSize = (UINT)sparkVertices.size() * sizeof(Vertex);
    sparkGeo->IndexFormat = DXGI_FORMAT_R32_UINT;
    sparkGeo->IndexBufferByteSize = (UINT)sparkIndices.size() * sizeof(uint32_t);

    SubmeshGeometry sparkSub;
    sparkSub.IndexCount = (UINT)sparkIndices.size();
    sparkSub.StartIndexLocation = 0;
    sparkSub.BaseVertexLocation = 0;
    sparkGeo->DrawArgs["sparks"] = sparkSub;

    mGeometries[sparkGeo->Name] = std::move(sparkGeo);

    // Place torches in rings around the tower interior wall
    std::vector<Vertex> torchVertices;
    std::vector<std::uint32_t> torchIndices;

    float lightHeights[6] = { 50.0f, 250.0f, 450.0f, 650.0f, 850.0f, 1050.0f }; // Match the box heights
    int torchesPerRing = 8;
    int torchCount = 0;

    for (float h : lightHeights) {
        for (int i = 0; i < torchesPerRing; ++i) {
            float angle = i * (2.0f * MathHelper::Pi / torchesPerRing);

            // Use 146.0f radius to sit flush against the box (which is at 148.0f)
            float r = 146.0f;

            Vertex v;
            // Add +2.0f to 'h' so the flame base sits on top of the 4-unit tall box
            v.Pos = { r * cosf(angle), h + 2.0f, r * sinf(angle) };
            v.Normal = { -cosf(angle), 0.0f, -sinf(angle) };
            v.TexC = { 0.0f, 0.0f };
            torchVertices.push_back(v);
            torchIndices.push_back(torchCount++);
        }
    }

    auto torchGeo = std::make_unique<MeshGeometry>();
    torchGeo->Name = "torchGeo";
    torchGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(),
        torchVertices.data(), (UINT)torchVertices.size() * sizeof(Vertex),
        torchGeo->VertexBufferUploader);
    torchGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), mCommandList.Get(),
        torchIndices.data(), (UINT)torchIndices.size() * sizeof(uint32_t),
        torchGeo->IndexBufferUploader);
    torchGeo->VertexByteStride = sizeof(Vertex);
    torchGeo->VertexBufferByteSize = (UINT)torchVertices.size() * sizeof(Vertex);
    torchGeo->IndexFormat = DXGI_FORMAT_R32_UINT;
    torchGeo->IndexBufferByteSize = (UINT)torchIndices.size() * sizeof(uint32_t);

    SubmeshGeometry torchSub;
    torchSub.IndexCount = (UINT)torchIndices.size();
    torchSub.StartIndexLocation = 0;
    torchSub.BaseVertexLocation = 0;
    torchGeo->DrawArgs["torches"] = torchSub;

    mGeometries[torchGeo->Name] = std::move(torchGeo);

}

void TowerGameApp::BuildCustomGeometry() {
    ManualMeshData pedestalMesh = BuildRegularPrismMesh(8, 30.0f, 8.0f);
    ManualMeshData portalMaskMesh = BuildPortalMaskMesh(40, 26.0f);
    ManualMeshData portalPlaneMesh = BuildPortalPlaneMesh(38.0f);

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    auto addMesh = [&](const ManualMeshData& mesh, SubmeshGeometry& sub) {
        sub.IndexCount = (UINT)mesh.Indices.size();
        sub.StartIndexLocation = (UINT)indices.size();
        sub.BaseVertexLocation = (UINT)vertices.size();

        vertices.insert(vertices.end(), mesh.Vertices.begin(), mesh.Vertices.end());
        for (auto index : mesh.Indices) {
            indices.push_back(index + sub.BaseVertexLocation);
        }
    };

    SubmeshGeometry pedestalSub, portalMaskSub, portalPlaneSub;
    addMesh(pedestalMesh, pedestalSub);
    addMesh(portalMaskMesh, portalMaskSub);
    addMesh(portalPlaneMesh, portalPlaneSub);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "customGeo";
    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), (UINT)vertices.size() * sizeof(Vertex), geo->VertexBufferUploader);
    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), (UINT)indices.size() * sizeof(std::uint32_t), geo->IndexBufferUploader);
    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = (UINT)indices.size() * sizeof(std::uint32_t);
    geo->DrawArgs["pedestal"] = pedestalSub;
    geo->DrawArgs["portalMask"] = portalMaskSub;
    geo->DrawArgs["portalPlane"] = portalPlaneSub;
    mGeometries[geo->Name] = std::move(geo);
}

void TowerGameApp::BuildCarGeometry() {
    // NEW - Car
    const std::filesystem::path carModelPath = ResolveAssetPath({ L"models\\car.txt" });
    if (carModelPath.empty())
    {
        throw DxException(E_FAIL, L"ResolveAssetPath(models/car.txt)", L"TowerGameApp.cpp", __LINE__);
    }

    std::ifstream fin(carModelPath);

    if (!fin)
    {
        throw DxException(E_FAIL, L"std::ifstream(models/car.txt)", L"TowerGameApp.cpp", __LINE__);
    }

    UINT vcount = 0;
    UINT tcount = 0;
    std::string ignore;

    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    std::vector<Vertex> carVertices(vcount);
    for (UINT i = 0; i < vcount; ++i)
    {
        fin >> carVertices[i].Pos.x >> carVertices[i].Pos.y >> carVertices[i].Pos.z;
        fin >> carVertices[i].Normal.x >> carVertices[i].Normal.y >> carVertices[i].Normal.z;
        carVertices[i].TexC = {
            (carVertices[i].Pos.x + 3.0f) / 6.0f,
            (carVertices[i].Pos.z + 4.0f) / 8.0f
        };
    }

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> carIndices(3 * tcount);
    for (UINT i = 0; i < tcount; ++i)
    {
        fin >> carIndices[i * 3 + 0] >> carIndices[i * 3 + 1] >> carIndices[i * 3 + 2];
    }

    fin.close();

    const UINT vbByteSize = (UINT)carVertices.size() * sizeof(Vertex);

    const UINT ibByteSize = (UINT)carIndices.size() * sizeof(std::int32_t);

    auto geoForCar = std::make_unique<MeshGeometry>();
    geoForCar->Name = "carGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoForCar->VertexBufferCPU));
    CopyMemory(geoForCar->VertexBufferCPU->GetBufferPointer(), carVertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoForCar->IndexBufferCPU));
    CopyMemory(geoForCar->IndexBufferCPU->GetBufferPointer(), carIndices.data(), ibByteSize);

    geoForCar->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), carVertices.data(), vbByteSize, geoForCar->VertexBufferUploader);

    geoForCar->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), carIndices.data(), ibByteSize, geoForCar->IndexBufferUploader);

    geoForCar->VertexByteStride = sizeof(Vertex);
    geoForCar->VertexBufferByteSize = vbByteSize;
    geoForCar->IndexFormat = DXGI_FORMAT_R32_UINT;
    geoForCar->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)carIndices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geoForCar->DrawArgs["car"] = submesh;

    mGeometries[geoForCar->Name] = std::move(geoForCar);
}

void TowerGameApp::BuildCharacterGeometry() {
    const std::filesystem::path characterModelPath = EnsureCharacterModelPath();
    if (characterModelPath.empty())
    {
        throw DxException(E_FAIL, L"EnsureCharacterModelPath()", L"TowerGameApp.cpp", __LINE__);
    }

    std::ifstream fin(characterModelPath);

    if (!fin)
    {
        throw DxException(E_FAIL, L"std::ifstream(models/guardian.txt)", L"TowerGameApp.cpp", __LINE__);
    }

    UINT vcount = 0;
    UINT tcount = 0;
    std::string ignore;

    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    std::vector<Vertex> characterVertices(vcount);
    for (UINT i = 0; i < vcount; ++i)
    {
        fin >> characterVertices[i].Pos.x >> characterVertices[i].Pos.y >> characterVertices[i].Pos.z;
        fin >> characterVertices[i].Normal.x >> characterVertices[i].Normal.y >> characterVertices[i].Normal.z;
        characterVertices[i].TexC = {
            (characterVertices[i].Pos.x + 4.5f) / 9.0f,
            1.0f - (characterVertices[i].Pos.y / 14.0f)
        };
    }

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> characterIndices(3 * tcount);
    for (UINT i = 0; i < tcount; ++i)
    {
        fin >> characterIndices[i * 3 + 0] >> characterIndices[i * 3 + 1] >> characterIndices[i * 3 + 2];
    }

    fin.close();

    const UINT vbByteSize = static_cast<UINT>(characterVertices.size() * sizeof(Vertex));
    const UINT ibByteSize = static_cast<UINT>(characterIndices.size() * sizeof(std::int32_t));

    auto geoForCharacter = std::make_unique<MeshGeometry>();
    geoForCharacter->Name = "characterGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geoForCharacter->VertexBufferCPU));
    CopyMemory(geoForCharacter->VertexBufferCPU->GetBufferPointer(), characterVertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geoForCharacter->IndexBufferCPU));
    CopyMemory(geoForCharacter->IndexBufferCPU->GetBufferPointer(), characterIndices.data(), ibByteSize);

    geoForCharacter->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), characterVertices.data(), vbByteSize, geoForCharacter->VertexBufferUploader);

    geoForCharacter->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), characterIndices.data(), ibByteSize, geoForCharacter->IndexBufferUploader);

    geoForCharacter->VertexByteStride = sizeof(Vertex);
    geoForCharacter->VertexBufferByteSize = vbByteSize;
    geoForCharacter->IndexFormat = DXGI_FORMAT_R32_UINT;
    geoForCharacter->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = static_cast<UINT>(characterIndices.size());
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geoForCharacter->DrawArgs["character"] = submesh;

    mGeometries[geoForCharacter->Name] = std::move(geoForCharacter);
}

void TowerGameApp::BuildTextures() {
    auto createTexture = [&](const std::string& name, const GeneratedTextureData& data) {
        auto texture = std::make_unique<TextureAsset>();
        texture->Name = name;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = data.Width;
        texDesc.Height = data.Height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(md3dDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(texture->Resource.GetAddressOf())));

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->Resource.Get(), 0, 1);
        ThrowIfFailed(md3dDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(texture->UploadHeap.GetAddressOf())));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = data.Pixels.data();
        subresourceData.RowPitch = (LONG_PTR)(data.Width * sizeof(std::uint32_t));
        subresourceData.SlicePitch = subresourceData.RowPitch * data.Height;

        UpdateSubresources(mCommandList.Get(), texture->Resource.Get(), texture->UploadHeap.Get(), 0, 0, 1, &subresourceData);
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            texture->Resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        mTextures[name] = std::move(texture);
    };

    createTexture("wallTex", CreateStoneTexture());
    createTexture("floorTex", CreateFloorTexture());
    createTexture("platformTex", CreatePlatformTexture());
    createTexture("carTex", CreateCarTexture());
    createTexture("pedestalTex", CreatePedestalTexture());
    createTexture("portalTex", CreatePortalTexture());
    createTexture("characterTex", CreateCharacterTexture());
}

void TowerGameApp::BuildDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = kTextureCount;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    const std::array<std::string, kTextureCount> textureOrder = {
        "wallTex",
        "floorTex",
        "platformTex",
        "carTex",
        "pedestalTex",
        "portalTex",
        "characterTex"
    };

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (const auto& textureName : textureOrder) {
        const auto textureIt = mTextures.find(textureName);
        if (textureIt == mTextures.end() || !textureIt->second || textureIt->second->Resource == nullptr) {
            throw DxException(E_FAIL, std::wstring(textureName.begin(), textureName.end()), L"TowerGameApp.cpp", __LINE__);
        }

        auto texture = textureIt->second.get();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture->Resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        md3dDevice->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, handle);
        handle.Offset(1, mCbvSrvUavDescriptorSize);
    }
}

void TowerGameApp::BuildRenderItems() {
    // 1. Tower Walls
    auto walls = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&walls->World, XMMatrixTranslation(0, 450, 0));
    XMStoreFloat4x4(&walls->TexTransform, XMMatrixScaling(4.0f, 6.0f, 1.0f));
    walls->ObjCBIndex = 0; walls->Geo = mGeometries["towerGeo"].get();
    walls->IndexCount = walls->Geo->DrawArgs["cylinder"].IndexCount;
    walls->StartIndexLocation = walls->Geo->DrawArgs["cylinder"].StartIndexLocation;
    walls->BaseVertexLocation = walls->Geo->DrawArgs["cylinder"].BaseVertexLocation;
    walls->MaterialIndex = kMaterialWall;
    walls->DiffuseSrvHeapIndex = kWallTextureIndex;
    mOpaqueRitems.push_back(walls.get());
    mAllRitems.push_back(std::move(walls));

    // 2. Floor
    auto ground = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&ground->World, XMMatrixTranslation(0, 0, 0));
    XMStoreFloat4x4(&ground->TexTransform, XMMatrixScaling(6.0f, 6.0f, 1.0f));
    ground->ObjCBIndex = 1; ground->Geo = mGeometries["towerGeo"].get();
    ground->IndexCount = ground->Geo->DrawArgs["grid"].IndexCount;
    ground->StartIndexLocation = ground->Geo->DrawArgs["grid"].StartIndexLocation;
    ground->BaseVertexLocation = ground->Geo->DrawArgs["grid"].BaseVertexLocation;
    ground->MaterialIndex = kMaterialFloor;
    ground->DiffuseSrvHeapIndex = kFloorTextureIndex;
    mOpaqueRitems.push_back(ground.get());
    mAllRitems.push_back(std::move(ground));

    // 3. Platforms
    for (int i = 0; i < 90; ++i) {
        auto ri = std::make_unique<RenderItem>();
        float r = 30.0f + MathHelper::RandF() * 100.0f;
        float theta = MathHelper::RandF() * 2.0f * MathHelper::Pi;
        float y = 12.0f + (i * 12.0f);
        XMStoreFloat4x4(&ri->World, XMMatrixTranslation(r * std::cos(theta), y, r * std::sin(theta)));
        XMStoreFloat4x4(&ri->TexTransform, XMMatrixScaling(1.4f, 1.4f, 1.0f));
        ri->ObjCBIndex = i + 2; ri->Geo = mGeometries["towerGeo"].get();
        ri->IndexCount = ri->Geo->DrawArgs["box"].IndexCount;
        ri->StartIndexLocation = ri->Geo->DrawArgs["box"].StartIndexLocation;
        ri->BaseVertexLocation = ri->Geo->DrawArgs["box"].BaseVertexLocation;
        ri->MaterialIndex = kMaterialPlatform;
        ri->DiffuseSrvHeapIndex = kPlatformTextureIndex;
        ri->IsAnimated = true;
        mOpaqueRitems.push_back(ri.get());
        mPlatformRitems.push_back(ri.get());
        mAllRitems.push_back(std::move(ri));
    }

    // 4. Goal Orb � transparent sphere at the top of the tower (ObjCBIndex = 92)
    auto orb = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&orb->World, XMMatrixTranslation(0.0f, 1100.0f, 0.0f));
    orb->ObjCBIndex = 92; orb->Geo = mGeometries["towerGeo"].get();
    orb->IndexCount = orb->Geo->DrawArgs["sphere"].IndexCount;
    orb->StartIndexLocation = orb->Geo->DrawArgs["sphere"].StartIndexLocation;
    orb->BaseVertexLocation = orb->Geo->DrawArgs["sphere"].BaseVertexLocation;
    orb->MaterialIndex = kMaterialOrb;
    orb->DiffuseSrvHeapIndex = kPortalTextureIndex;
    mOrbRitem = orb.get(); // Keep raw pointer for Draw()
    mAllRitems.push_back(std::move(orb));

    // 5. Wall Light Fixtures (ObjCBIndex 93 through 116)
    float lightHeights[6] = { 50.0f, 250.0f, 450.0f, 650.0f, 850.0f, 1050.0f }; // Floor, Midpoint, Ceiling
    int lightIndex = 0;

    for (int ring = 0; ring < 6; ++ring) {
        for (int i = 0; i < 8; ++i) {
            auto ri = std::make_unique<RenderItem>();
            float angle = i * (MathHelper::Pi / 4.0f);
            float r = 148.0f;
            float y = lightHeights[ring];

            XMMATRIX world = XMMatrixRotationY(-angle) * XMMatrixTranslation(r * std::cos(angle), y, r * std::sin(angle));
            XMStoreFloat4x4(&ri->World, world);

            ri->ObjCBIndex = 93 + lightIndex;
            ri->Geo = mGeometries["towerGeo"].get();
            ri->IndexCount = ri->Geo->DrawArgs["lightBox"].IndexCount;
            ri->StartIndexLocation = ri->Geo->DrawArgs["lightBox"].StartIndexLocation;
            ri->BaseVertexLocation = ri->Geo->DrawArgs["lightBox"].BaseVertexLocation;
            ri->MaterialIndex = kMaterialLight;
            ri->DiffuseSrvHeapIndex = kWallTextureIndex;
            mOpaqueRitems.push_back(ri.get());
            mAllRitems.push_back(std::move(ri));

            lightIndex++; // Increment so each box gets a unique ObjCBIndex
        }
    }

    // 6. Energy Sparks (ObjCBIndex 117)
    auto sparks = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&sparks->World, XMMatrixIdentity());
    sparks->ObjCBIndex = 117; // Give it the next available index
    sparks->Geo = mGeometries["sparkGeo"].get();

    // CRITICAL: Tell the API these are POINTS, not triangles
    sparks->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

    sparks->IndexCount = sparks->Geo->DrawArgs["sparks"].IndexCount;
    sparks->StartIndexLocation = sparks->Geo->DrawArgs["sparks"].StartIndexLocation;
    sparks->BaseVertexLocation = sparks->Geo->DrawArgs["sparks"].BaseVertexLocation;
    sparks->MaterialIndex = kMaterialOrb;
    sparks->DiffuseSrvHeapIndex = kPortalTextureIndex;

    mSparkRitems.push_back(sparks.get());
    mAllRitems.push_back(std::move(sparks));

    // 7. Car (OBjCBIndex 118)
    auto car = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&car->World, XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixTranslation(60.0f, carHeight, 70.0f));
    car->ObjCBIndex = 118;
    car->Geo = mGeometries["carGeo"].get();
    car->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    car->IndexCount = car->Geo->DrawArgs["car"].IndexCount;
    car->StartIndexLocation = car->Geo->DrawArgs["car"].StartIndexLocation;
    car->BaseVertexLocation = car->Geo->DrawArgs["car"].BaseVertexLocation;
    car->MaterialIndex = kMaterialCar;
    car->DiffuseSrvHeapIndex = kCarTextureIndex;
    mCarRitem = car.get();
    mOpaqueRitems.push_back(car.get());
    mAllRitems.push_back(std::move(car));

    auto character = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&character->World, XMMatrixScaling(0.90f, 0.90f, 0.90f) * XMMatrixTranslation(0.0f, 0.0f, -30.0f));
    character->ObjCBIndex = 123;
    const auto characterGeoIt = mGeometries.find("characterGeo");
    if (characterGeoIt == mGeometries.end() || !characterGeoIt->second) {
        throw DxException(E_FAIL, L"characterGeo", L"TowerGameApp.cpp", __LINE__);
    }
    character->Geo = characterGeoIt->second.get();
    character->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    const auto characterSubmeshIt = character->Geo->DrawArgs.find("character");
    if (characterSubmeshIt == character->Geo->DrawArgs.end()) {
        throw DxException(E_FAIL, L"character draw args", L"TowerGameApp.cpp", __LINE__);
    }
    character->IndexCount = characterSubmeshIt->second.IndexCount;
    character->StartIndexLocation = characterSubmeshIt->second.StartIndexLocation;
    character->BaseVertexLocation = characterSubmeshIt->second.BaseVertexLocation;
    character->MaterialIndex = kMaterialCharacter;
    character->DiffuseSrvHeapIndex = kCharacterTextureIndex;
    mCharacterRitem = character.get();
    mOpaqueRitems.push_back(character.get());
    mAllRitems.push_back(std::move(character));

    // 7b. Torch Billboards (ObjCBIndex 119)
    auto torches = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&torches->World, XMMatrixIdentity());
    torches->ObjCBIndex = 120;
    torches->Geo = mGeometries["torchGeo"].get();
    torches->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    torches->IndexCount = torches->Geo->DrawArgs["torches"].IndexCount;
    torches->StartIndexLocation = torches->Geo->DrawArgs["torches"].StartIndexLocation;
    torches->BaseVertexLocation = torches->Geo->DrawArgs["torches"].BaseVertexLocation;
    torches->MaterialIndex = kMaterialPortal;
    torches->DiffuseSrvHeapIndex = kPortalTextureIndex;
    mTorchRitems.push_back(torches.get());
    mAllRitems.push_back(std::move(torches));

    auto pedestal = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&pedestal->World, XMMatrixTranslation(0.0f, 1080.0f, 0.0f));
    pedestal->ObjCBIndex = 119;
    pedestal->Geo = mGeometries["customGeo"].get();
    pedestal->IndexCount = pedestal->Geo->DrawArgs["pedestal"].IndexCount;
    pedestal->StartIndexLocation = pedestal->Geo->DrawArgs["pedestal"].StartIndexLocation;
    pedestal->BaseVertexLocation = pedestal->Geo->DrawArgs["pedestal"].BaseVertexLocation;
    pedestal->MaterialIndex = kMaterialPedestal;
    pedestal->DiffuseSrvHeapIndex = kPedestalTextureIndex;
    mPedestalRitem = pedestal.get();
    mOpaqueRitems.push_back(pedestal.get());
    mAllRitems.push_back(std::move(pedestal));

    auto portalMask = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&portalMask->World, XMMatrixTranslation(0.0f, 0.12f, 0.0f));
    portalMask->ObjCBIndex = 121;
    portalMask->Geo = mGeometries["customGeo"].get();
    portalMask->IndexCount = portalMask->Geo->DrawArgs["portalMask"].IndexCount;
    portalMask->StartIndexLocation = portalMask->Geo->DrawArgs["portalMask"].StartIndexLocation;
    portalMask->BaseVertexLocation = portalMask->Geo->DrawArgs["portalMask"].BaseVertexLocation;
    portalMask->MaterialIndex = kMaterialPortal;
    portalMask->DiffuseSrvHeapIndex = kPortalTextureIndex;
    mStencilMaskRitems.push_back(portalMask.get());
    mAllRitems.push_back(std::move(portalMask));

    auto portalPlane = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&portalPlane->World, XMMatrixTranslation(0.0f, 0.14f, 0.0f));
    portalPlane->ObjCBIndex = 122;
    portalPlane->Geo = mGeometries["customGeo"].get();
    portalPlane->IndexCount = portalPlane->Geo->DrawArgs["portalPlane"].IndexCount;
    portalPlane->StartIndexLocation = portalPlane->Geo->DrawArgs["portalPlane"].StartIndexLocation;
    portalPlane->BaseVertexLocation = portalPlane->Geo->DrawArgs["portalPlane"].BaseVertexLocation;
    portalPlane->MaterialIndex = kMaterialPortal;
    portalPlane->DiffuseSrvHeapIndex = kPortalTextureIndex;
    mPortalRitems.push_back(portalPlane.get());
    mAllRitems.push_back(std::move(portalPlane));
}

void TowerGameApp::BuildRootSignature() {
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER slot[3];
    slot[0].InitAsConstantBufferView(0);
    slot[1].InitAsConstantBufferView(1);
    slot[2].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

    const auto samplers = GetStaticSamplers();
    CD3DX12_ROOT_SIGNATURE_DESC rsDesc(
        3,
        slot,
        (UINT)samplers.size(),
        samplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serialize, error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialize, &error));
    ThrowIfFailed(md3dDevice->CreateRootSignature(0, serialize->GetBufferPointer(), serialize->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void TowerGameApp::BuildShadersAndInputLayout() {
    const std::filesystem::path shaderPath = ResolveAssetPath({ L"shaders\\Default.hlsl" });
    if (shaderPath.empty())
    {
        throw DxException(E_FAIL, L"ResolveAssetPath(shaders/Default.hlsl)", L"TowerGameApp.cpp", __LINE__);
    }

    mvsByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "VS", "vs_5_0");
    mpsByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "PS", "ps_5_0");
    mgsByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "GS", "gs_5_0");
    mgsTorchByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "GS_Torch", "gs_5_0");
    mvsTorchByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "VS_Torch", "vs_5_0");
    mpsTorchByteCode = d3dUtil::CompileShader(shaderPath.wstring(), nullptr, "PS_Torch", "ps_5_0");

    mInputLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void TowerGameApp::BuildPSOs() {
    // --- Base PSO descriptor (shared settings) ---
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;

    // Opaque PSO (unchanged)
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mOpaquePSO)));

    // --- NEW: Transparent PSO � copy opaque desc, then enable alpha blending ---
    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentDesc = psoDesc;

    D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
    blendDesc.BlendEnable = TRUE;
    blendDesc.LogicOpEnable = FALSE;
    blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;       // Multiply source by its alpha
    blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;   // Multiply dest by (1 - alpha)
    blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    transparentDesc.BlendState.RenderTarget[0] = blendDesc;

    // Keep depth testing ON so orb is occluded by geometry in front of it,
    // but disable depth writes so it doesn't block objects behind it
    transparentDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilMaskDesc = psoDesc;
    stencilMaskDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;
    stencilMaskDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    stencilMaskDesc.DepthStencilState.StencilEnable = TRUE;
    stencilMaskDesc.DepthStencilState.StencilReadMask = 0xff;
    stencilMaskDesc.DepthStencilState.StencilWriteMask = 0xff;
    stencilMaskDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    stencilMaskDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    stencilMaskDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    stencilMaskDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    stencilMaskDesc.DepthStencilState.BackFace = stencilMaskDesc.DepthStencilState.FrontFace;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilPortalDesc = transparentDesc;
    stencilPortalDesc.DepthStencilState.StencilEnable = TRUE;
    stencilPortalDesc.DepthStencilState.StencilReadMask = 0xff;
    stencilPortalDesc.DepthStencilState.StencilWriteMask = 0x00;
    stencilPortalDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    stencilPortalDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    stencilPortalDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    stencilPortalDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    stencilPortalDesc.DepthStencilState.BackFace = stencilPortalDesc.DepthStencilState.FrontFace;

    // --- Spark Particle PSO ---
    D3D12_GRAPHICS_PIPELINE_STATE_DESC sparkDesc = transparentDesc; // Copy the alpha blending settings
    sparkDesc.GS = { reinterpret_cast<BYTE*>(mgsByteCode->GetBufferPointer()), mgsByteCode->GetBufferSize() };
    sparkDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentDesc, IID_PPV_ARGS(&mTransparentPSO)));
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stencilMaskDesc, IID_PPV_ARGS(&mStencilMaskPSO)));
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&stencilPortalDesc, IID_PPV_ARGS(&mStencilPortalPSO)));
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&sparkDesc, IID_PPV_ARGS(&mSparkPSO)));

    // --- Torch Billboard PSO ---
    D3D12_GRAPHICS_PIPELINE_STATE_DESC torchDesc = transparentDesc;
    torchDesc.VS = { reinterpret_cast<BYTE*>(mvsTorchByteCode->GetBufferPointer()), mvsTorchByteCode->GetBufferSize() };
    torchDesc.GS = { reinterpret_cast<BYTE*>(mgsTorchByteCode->GetBufferPointer()), mgsTorchByteCode->GetBufferSize() };
    torchDesc.PS = { reinterpret_cast<BYTE*>(mpsTorchByteCode->GetBufferPointer()), mpsTorchByteCode->GetBufferSize() };
    torchDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&torchDesc, IID_PPV_ARGS(&mTorchPSO)));
}

void TowerGameApp::BuildFrameResources() {
    for (int i = 0; i < 3; ++i) mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, 256));
}

void TowerGameApp::OnMouseDown(WPARAM btnState, int x, int y) { mLastMousePos.x = x; mLastMousePos.y = y; mHasMousePosition = true; }
void TowerGameApp::OnMouseUp(WPARAM btnState, int x, int y) { mLastMousePos.x = x; mLastMousePos.y = y; mHasMousePosition = true; }
void TowerGameApp::OnMouseMove(WPARAM btnState, int x, int y) {
    if (!mHasMousePosition) {
        mLastMousePos.x = x;
        mLastMousePos.y = y;
        mHasMousePosition = true;
        return;
    }

    if (GetForegroundWindow() == mhMainWnd && !mAppPaused) {
        const float dx = XMConvertToRadians(0.18f * static_cast<float>(x - mLastMousePos.x));
        const float dy = XMConvertToRadians(0.18f * static_cast<float>(y - mLastMousePos.y));

        if (mFreeFlyMode) {
            mFlyYaw += dx;
            mFlyPitch = std::clamp(mFlyPitch + dy, -1.15f, 1.15f);
            UpdateFlyCamera();
        }
        else {
            mYaw += dx;
            mPitch = std::clamp(mPitch + dy, 0.12f, 0.95f);
            UpdateChaseCamera();
        }
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
