#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

struct Vertex3D {
    float position[3];
    float normal[3];
    float uv[2];
};

struct MeshHeader {
    char magic[4]; // "MESH"
    uint32_t version;
    uint32_t vertexCount;
    uint32_t indexCount;
    float boundsMin[3];
    float boundsMax[3];
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: AssetCooker <input.gltf/glb> <output.meshbin>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = false;
    if (inputPath.substr(inputPath.find_last_of(".") + 1) == "glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, inputPath);
    } else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, inputPath);
    }

    if (!warn.empty()) std::cout << "Warn: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Err: " << err << std::endl;
    if (!ret) {
        std::cerr << "Failed to parse glTF" << std::endl;
        return 1;
    }

    std::vector<Vertex3D> allVertices;
    std::vector<uint32_t> allIndices;

    // Simplified: Just grab the first primitive of the first mesh
    if (model.meshes.empty()) {
        std::cerr << "No meshes found in glTF" << std::endl;
        return 1;
    }

    const auto& mesh = model.meshes[0];
    for (const auto& primitive : mesh.primitives) {
        // Position
        const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
        const auto& posBuffer = model.buffers[posBufferView.buffer];
        const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

        // Normal
        const float* normals = nullptr;
        if (primitive.attributes.count("NORMAL")) {
            const auto& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            const auto& normBufferView = model.bufferViews[normAccessor.bufferView];
            const auto& normBuffer = model.buffers[normBufferView.buffer];
            normals = reinterpret_cast<const float*>(&normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset]);
        }

        // UV
        const float* uvs = nullptr;
        if (primitive.attributes.count("TEXCOORD_0")) {
            const auto& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const auto& uvBufferView = model.bufferViews[uvAccessor.bufferView];
            const auto& uvBuffer = model.buffers[uvBufferView.buffer];
            uvs = reinterpret_cast<const float*>(&uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);
        }

        uint32_t vertexStart = (uint32_t)allVertices.size();
        for (size_t i = 0; i < posAccessor.count; ++i) {
            Vertex3D v;
            v.position[0] = positions[i * 3 + 0];
            v.position[1] = positions[i * 3 + 1];
            v.position[2] = positions[i * 3 + 2];

            if (normals) {
                v.normal[0] = normals[i * 3 + 0];
                v.normal[1] = normals[i * 3 + 1];
                v.normal[2] = normals[i * 3 + 2];
            } else {
                v.normal[0] = 0; v.normal[1] = 1; v.normal[2] = 0;
            }

            if (uvs) {
                v.uv[0] = uvs[i * 2 + 0];
                v.uv[1] = uvs[i * 2 + 1];
            } else {
                v.uv[0] = 0; v.uv[1] = 0;
            }
            allVertices.push_back(v);
        }

        // Indices
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = model.buffers[indexBufferView.buffer];
        
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
            for (size_t i = 0; i < indexAccessor.count; ++i) allIndices.push_back(vertexStart + indices[i]);
        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
            for (size_t i = 0; i < indexAccessor.count; ++i) allIndices.push_back(vertexStart + indices[i]);
        }
    }

    // Write binary file
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file" << std::endl;
        return 1;
    }

    MeshHeader header;
    header.magic[0] = 'M'; header.magic[1] = 'E'; header.magic[2] = 'S'; header.magic[3] = 'H';
    header.version = 1;
    header.vertexCount = (uint32_t)allVertices.size();
    header.indexCount = (uint32_t)allIndices.size();
    // Simplified bounds
    header.boundsMin[0] = -1; header.boundsMin[1] = -1; header.boundsMin[2] = -1;
    header.boundsMax[0] = 1; header.boundsMax[1] = 1; header.boundsMax[2] = 1;

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(reinterpret_cast<const char*>(allVertices.data()), allVertices.size() * sizeof(Vertex3D));
    out.write(reinterpret_cast<const char*>(allIndices.data()), allIndices.size() * sizeof(uint32_t));
    out.close();

    std::cout << "Successfully cooked " << inputPath << " to " << outputPath << " (" << allVertices.size() << " verts, " << allIndices.size() << " indices)" << std::endl;

    return 0;
}
