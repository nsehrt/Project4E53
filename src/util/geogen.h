#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeoGenerator
{

public:

    struct Vertex
    {
        Vertex(){}

        Vertex(
            const DirectX::XMFLOAT3& p,
            const DirectX::XMFLOAT3& n,
            const DirectX::XMFLOAT3& t,
            const DirectX::XMFLOAT2& uv) :
            Position(p),
            Normal(n),
            TangentU(t),
            TexC(uv)
        {
        }
        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz,
            float u, float v) :
            Position(px, py, pz),
            Normal(nx, ny, nz),
            TangentU(tx, ty, tz),
            TexC(u, v)
        {
        }

        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT2 TexC;
    };

    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<unsigned int> Indices32;

        std::vector<unsigned short>& GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); ++i)
                    mIndices16[i] = static_cast<unsigned short>(Indices32[i]);
            }

            return mIndices16;
        }

    private:
        std::vector<unsigned short> mIndices16;
    };


    MeshData createBoxMesh(float width, float height, float depth, unsigned int numSubdivisions);
    MeshData createSphereMesh(float radius, unsigned int sliceCount, unsigned int stackCount);
    MeshData createFullscreenQuad(float x, float y, float w, float h, float depth);
    MeshData createGeosphere(float radius, unsigned int numSubdivisions);
    MeshData createGrid(float width, float depth, unsigned int m, unsigned int n);

private:

    void Subdivide(MeshData& _meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);

};