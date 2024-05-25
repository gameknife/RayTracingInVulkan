#include "Model.hpp"
#include "CornellBox.hpp"
#include "Procedural.hpp"
#include "Sphere.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/Console.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>

#define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "Texture.hpp"

#define FLATTEN_VERTICE 1

using namespace glm;

namespace std
{
    template <>
    struct hash<Assets::Vertex> final
    {
        size_t operator()(Assets::Vertex const& vertex) const noexcept
        {
            return
                Combine(hash<vec3>()(vertex.Position),
                        Combine(hash<vec3>()(vertex.Normal),
                                Combine(hash<vec2>()(vertex.TexCoord),
                                        hash<int>()(vertex.MaterialIndex))));
        }

    private:
        static size_t Combine(size_t hash0, size_t hash1)
        {
            return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
        }
    };
}

namespace Assets
{
    Model Model::LoadModel(const std::string& filename, std::vector<Texture>& textures)
    {
        // if filename endwith .glb
        // load glb file
        // else
        // load obj file
        if (filename.find(".glb") != std::string::npos)
        {
            return LoadGltfModel(filename, textures);
        }
        else
        {
            return LoadObjModel(filename, textures);
        }
    }

    Model Model::LoadGltfModel(const std::string& filename, std::vector<Texture>& textures)
    {
        std::vector<LightObject> lights;
        std::vector<Material> materials;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        //std::unordered_map<Vertex, uint32_t> uniqueVertices(objAttrib.vertices.size());

        tinygltf::Model model;
        tinygltf::TinyGLTF gltfLoader;
        std::string err;
        std::string warn;

        bool ret = gltfLoader.LoadBinaryFromFile(&model, &err, &warn, filename);

        // export whole scene into a big buffer, with vertice indices materials
        for (tinygltf::Mesh& mesh : model.meshes)
        {
            for (tinygltf::Primitive& primtive : mesh.primitives)
            {
                // pos normal texcoord material
            }
        }

        return Model(std::move(vertices), std::move(indices), std::move(materials), std::move(lights), nullptr);
    }

    void Model::FlattenVertices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        std::vector<Vertex> vertices_flatten;
        std::vector<uint32_t> indices_flatten;

        uint32_t idx_counter = 0;
        for (uint32_t index : indices)
        {
            vertices_flatten.push_back(vertices[index]);
            indices_flatten.push_back(idx_counter++);
        }

        vertices = std::move(vertices_flatten);
        indices = std::move(indices_flatten);
    }

    Model Model::LoadObjModel(const std::string& filename, std::vector<Texture>& textures)
    {
        std::cout << "- loading '" << filename << "'... " << std::flush;

        const auto timer = std::chrono::high_resolution_clock::now();
        const std::string materialPath = std::filesystem::path(filename).parent_path().string();

        tinyobj::ObjReader objReader;

        if (!objReader.ParseFromFile(filename))
        {
            Throw(std::runtime_error("failed to load model '" + filename + "':\n" + objReader.Error()));
        }

        if (!objReader.Warning().empty())
        {
            Utilities::Console::Write(Utilities::Severity::Warning, [&objReader]()
            {
                std::cout << "\nWARNING: " << objReader.Warning() << std::flush;
            });
        }
        // Lights
        std::vector<LightObject> lights;
        
        // Materials
        std::vector<Material> materials;

        for (const auto& _material : objReader.GetMaterials())
        {
            tinyobj::material_t material = _material;
            Material m{};

            // texture stuff
            m.DiffuseTextureId = -1;
            if (material.diffuse_texname != "")
            {
                material.diffuse[0] = 1.0f;
                material.diffuse[1] = 1.0f;
                material.diffuse[2] = 1.0f;

                // find if textures contain texture with loadname equals diffuse_texname
                std::string loadname = "../assets/textures/" + material.diffuse_texname;
                for (size_t i = 0; i < textures.size(); i++)
                {
                    if (textures[i].Loadname() == loadname)
                    {
                        m.DiffuseTextureId = i;
                        break;
                    }
                }

                if (m.DiffuseTextureId == -1)
                {
                    textures.push_back(Texture::LoadTexture(loadname, Vulkan::SamplerConfig()));
                    m.DiffuseTextureId = textures.size() - 1;
                }
            }

            m.Diffuse = vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0);

            m.MaterialModel = Material::Enum::Mixture;
            m.Fuzziness = material.roughness;
            m.RefractionIndex = 1.46f; // plastic
            m.Metalness = material.metallic * material.metallic;

            if (material.name == "Window-Fake-Glass" || material.name == "Wine-Glasses" || material.name.find("Water")
                != std::string::npos || material.name.find("Glass") != std::string::npos || material.name.find("glass")
                != std::string::npos)
            {
                m.MaterialModel = Material::Enum::Dielectric;
            }

            if (material.emission[0] > 0)
            {
                m = Material::DiffuseLight(vec3(material.emission[0], material.emission[1], material.emission[2]));
                // add to lights
            }

            if (material.metallic > .99f)
            {
                m.MaterialModel = Material::Enum::Metallic;
            }

            materials.emplace_back(m);
        }

        if (materials.empty())
        {
            Material m{};

            m.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
            m.DiffuseTextureId = -1;

            materials.emplace_back(m);
        }

        // Geometry
        const auto& objAttrib = objReader.GetAttrib();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices(objAttrib.vertices.size());

        

        for (const auto& shape : objReader.GetShapes())
        {
            glm::vec3 aabb_min(999999,999999,999999);
            glm::vec3 aabb_max(-999999,-999999,-999999);
            glm::vec3 direction(0,0,0);

            const auto& mesh = shape.mesh;
            size_t faceId = 0;
            for (const auto& index : mesh.indices)
            {
                Vertex vertex = {};

                vertex.Position =
                {
                    objAttrib.vertices[3 * index.vertex_index + 0],
                    objAttrib.vertices[3 * index.vertex_index + 1],
                    objAttrib.vertices[3 * index.vertex_index + 2],
                };

                aabb_min = glm::min( aabb_min, vertex.Position);
                aabb_max = glm::max( aabb_max, vertex.Position);

                if (!objAttrib.normals.empty())
                {
                    vertex.Normal =
                    {
                        objAttrib.normals[3 * index.normal_index + 0],
                        objAttrib.normals[3 * index.normal_index + 1],
                        objAttrib.normals[3 * index.normal_index + 2]
                    };

                    direction = vertex.Normal;
                }

                if (!objAttrib.texcoords.empty())
                {
                    vertex.TexCoord =
                    {
                        objAttrib.texcoords[2 * max(0, index.texcoord_index) + 0],
                        1 - objAttrib.texcoords[2 * max(0, index.texcoord_index) + 1]
                    };
                }

                vertex.MaterialIndex = std::max(0, mesh.material_ids[faceId++ / 3]);

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }

            if(shape.name.find("lightquad") != std::string::npos)
            {
                // use the aabb to build a light, using the average normals and area
                LightObject light;
                light.WorldPosMin = glm::vec4(aabb_min,1.0);
                light.WorldPosMax = glm::vec4(aabb_max, 1.0);
                light.WorldDirection = glm::vec4(direction, 0.0);

                float radius_big = glm::distance(aabb_max, aabb_min);
                light.area = radius_big * radius_big * 0.5;

                lights.push_back(light);
            }
        }

        // If the model did not specify normals, then create smooth normals that conserve the same number of vertices.
        // Using flat normals would mean creating more vertices than we currently have, so for simplicity and better visuals we don't do it.
        // See https://stackoverflow.com/questions/12139840/obj-file-averaging-normals.
        if (objAttrib.normals.empty())
        {
            std::vector<vec3> normals(vertices.size());

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const auto normal = normalize(cross(
                    vec3(vertices[indices[i + 1]].Position) - vec3(vertices[indices[i]].Position),
                    vec3(vertices[indices[i + 2]].Position) - vec3(vertices[indices[i]].Position)));

                vertices[indices[i + 0]].Normal += normal;
                vertices[indices[i + 1]].Normal += normal;
                vertices[indices[i + 2]].Normal += normal;
            }

            for (auto& vertex : vertices)
            {
                vertex.Normal = normalize(vertex.Normal);
            }
        }

        const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(
            std::chrono::high_resolution_clock::now() - timer).count();

        std::cout << "(" << objAttrib.vertices.size() << " vertices, " << uniqueVertices.size() << " unique vertices, "
            << materials.size() << " materials) ";
        std::cout << elapsed << "s" << std::endl;

        

        // flatten the vertice and indices, individual vertice
#if FLATTEN_VERTICE
        FlattenVertices(vertices, indices);
#endif

        return Model(std::move(vertices), std::move(indices), std::move(materials), std::move(lights), nullptr);
    }

    Model Model::CreateCornellBox(const float scale)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Material> materials;
        std::vector<LightObject> lights;

        CornellBox::Create(scale, vertices, indices, materials, lights);

#if FLATTEN_VERTICE
        FlattenVertices(vertices, indices);
#endif

        return Model(
            std::move(vertices),
            std::move(indices),
            std::move(materials),
            std::move(lights),
            nullptr
        );
    }

    Model Model::CreateBox(const vec3& p0, const vec3& p1, const Material& material)
    {
        std::vector<Vertex> vertices =
        {
            Vertex{vec3(p0.x, p0.y, p0.z), vec3(-1, 0, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p0.y, p1.z), vec3(-1, 0, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p1.z), vec3(-1, 0, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p0.z), vec3(-1, 0, 0), vec2(0), 0},

            Vertex{vec3(p1.x, p0.y, p1.z), vec3(1, 0, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p0.y, p0.z), vec3(1, 0, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p1.y, p0.z), vec3(1, 0, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p1.y, p1.z), vec3(1, 0, 0), vec2(0), 0},

            Vertex{vec3(p1.x, p0.y, p0.z), vec3(0, 0, -1), vec2(0), 0},
            Vertex{vec3(p0.x, p0.y, p0.z), vec3(0, 0, -1), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p0.z), vec3(0, 0, -1), vec2(0), 0},
            Vertex{vec3(p1.x, p1.y, p0.z), vec3(0, 0, -1), vec2(0), 0},

            Vertex{vec3(p0.x, p0.y, p1.z), vec3(0, 0, 1), vec2(0), 0},
            Vertex{vec3(p1.x, p0.y, p1.z), vec3(0, 0, 1), vec2(0), 0},
            Vertex{vec3(p1.x, p1.y, p1.z), vec3(0, 0, 1), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p1.z), vec3(0, 0, 1), vec2(0), 0},

            Vertex{vec3(p0.x, p0.y, p0.z), vec3(0, -1, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p0.y, p0.z), vec3(0, -1, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p0.y, p1.z), vec3(0, -1, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p0.y, p1.z), vec3(0, -1, 0), vec2(0), 0},

            Vertex{vec3(p1.x, p1.y, p0.z), vec3(0, 1, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p0.z), vec3(0, 1, 0), vec2(0), 0},
            Vertex{vec3(p0.x, p1.y, p1.z), vec3(0, 1, 0), vec2(0), 0},
            Vertex{vec3(p1.x, p1.y, p1.z), vec3(0, 1, 0), vec2(0), 0},
        };

        std::vector<uint32_t> indices =
        {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23
        };

#if FLATTEN_VERTICE
        FlattenVertices(vertices, indices);
#endif

        return Model(
            std::move(vertices),
            std::move(indices),
            std::vector<Material>{material},
            std::vector<LightObject>{},
            nullptr);
    }

    Model Model::CreateSphere(const vec3& center, float radius, const Material& material, const bool isProcedural)
    {
        const int slices = 32;
        const int stacks = 16;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        const float pi = 3.14159265358979f;

        for (int j = 0; j <= stacks; ++j)
        {
            const float j0 = pi * j / stacks;

            // Vertex
            const float v = radius * -std::sin(j0);
            const float z = radius * std::cos(j0);

            // Normals		
            const float n0 = -std::sin(j0);
            const float n1 = std::cos(j0);

            for (int i = 0; i <= slices; ++i)
            {
                const float i0 = 2 * pi * i / slices;

                const vec3 position(
                    center.x + v * std::sin(i0),
                    center.y + z,
                    center.z + v * std::cos(i0));

                const vec3 normal(
                    n0 * std::sin(i0),
                    n1,
                    n0 * std::cos(i0));

                const vec2 texCoord(
                    static_cast<float>(i) / slices,
                    static_cast<float>(j) / stacks);

                vertices.push_back(Vertex{position, normal, texCoord, 0});
            }
        }

        for (int j = 0; j < stacks; ++j)
        {
            for (int i = 0; i < slices; ++i)
            {
                const auto j0 = (j + 0) * (slices + 1);
                const auto j1 = (j + 1) * (slices + 1);
                const auto i0 = i + 0;
                const auto i1 = i + 1;

                indices.push_back(j0 + i0);
                indices.push_back(j1 + i0);
                indices.push_back(j1 + i1);

                indices.push_back(j0 + i0);
                indices.push_back(j1 + i1);
                indices.push_back(j0 + i1);
            }
        }


#if FLATTEN_VERTICE
        FlattenVertices(vertices, indices);
#endif

        return Model(
            std::move(vertices),
            std::move(indices),
            std::vector<Material>{material},
            std::vector<LightObject>{},
            isProcedural ? new Sphere(center, radius) : nullptr);
    }

    void Model::SetMaterial(const Material& material)
    {
        if (materials_.size() != 1)
        {
            Throw(std::runtime_error("cannot change material on a multi-material model"));
        }

        materials_[0] = material;
    }

    void Model::Transform(const mat4& transform)
    {
        const auto transformIT = inverseTranspose(transform);

        for (auto& vertex : vertices_)
        {
            vertex.Position = transform * vec4(vertex.Position, 1);
            vertex.Normal = transformIT * vec4(vertex.Normal, 0);
        }
    }

    Model::Model(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<Material>&& materials,
                 std::vector<LightObject>&& lights,
                 const class Procedural* procedural) :
        vertices_(std::move(vertices)),
        indices_(std::move(indices)),
        materials_(std::move(materials)),
        lights_(std::move(lights)),
        procedural_(procedural)
    {
    }
}
