#ifndef MODEL_H
#define MODEL_H

//#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>
#include "mesh.hpp"
#include "shader.hpp"
//#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>


namespace {

unsigned texture_from_file(std::string filename, std::string directory, std::size_t indent = 0)
{
    fmt::print("{} texture_from_file: filename: {}, directory: {}\n", std::string(indent, ' '), filename, directory);

    unsigned textureID;
    glGenTextures(1, &textureID);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    filename = directory + '/' + filename;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            assert(false && "incorrect number of components");

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(format), width, height, 0, static_cast<unsigned>(format), GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        fmt::print("Texture failed to load at file: {}\n", filename);
        stbi_image_free(data);
    }

    return textureID;
}

std::vector<pwgl::texture> load_material_textures(std::string directory, aiMaterial *mat, aiTextureType type, std::string typeName, std::size_t indent = 0) {
    fmt::print("{} {} directory: {}, textures: {}, typename: {}\n", std::string(indent, ' '), __func__, directory, mat->GetTextureCount(type), typeName);
    std::vector<pwgl::texture> textures;
    for (unsigned i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        pwgl::texture texture;
        texture.id = texture_from_file(str.C_Str(), directory, indent);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.emplace_back(texture);
    }
    return textures;
}


pwgl::mesh process_mesh(std::string directory, aiMesh *mesh, const aiScene *scene, std::size_t indent = 4)
{
    std::vector<pwgl::vertex> vertices;
    std::vector<unsigned> indices;
    std::vector<pwgl::texture> textures;

    // walk through each of the mesh's vertices
    for(unsigned i = 0; i < mesh->mNumVertices; i++) {
        pwgl::vertex vertex;

        // location 0: position
        vertex.Position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // location 1: normals
        if (mesh->HasNormals()) {
            vertex.Normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // location 2: texture coords.
        if(mesh->mTextureCoords[0]) {
            vertex.TexCoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
            // tangent
            vertex.Tangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };
            // bitangent
            vertex.Bitangent = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.emplace_back(vertex);
    }

    for(unsigned i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned j = 0; j < face.mNumIndices; j++)
            indices.emplace_back(face.mIndices[j]);
    }
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // 1. diffuse maps
    std::vector<pwgl::texture> diffuseMaps = load_material_textures(directory, material, aiTextureType_DIFFUSE, "texture_diffuse", indent + 4);
    textures.insert(std::end(textures), std::begin(diffuseMaps), std::end(diffuseMaps));
    // 2. specular maps
    std::vector<pwgl::texture> specularMaps = load_material_textures(directory, material, aiTextureType_SPECULAR, "texture_specular", indent + 4);
    textures.insert(std::end(textures), std::begin(specularMaps), std::end(specularMaps));
    // 3. normal maps
    std::vector<pwgl::texture> normalMaps = load_material_textures(directory, material, aiTextureType_HEIGHT, "texture_normal", indent + 4);
    textures.insert(textures.end(), std::begin(normalMaps), std::end(normalMaps));
    // 4. height maps
    std::vector<pwgl::texture> heightMaps = load_material_textures(directory, material, aiTextureType_AMBIENT, "texture_height", indent + 4);
    textures.insert(std::end(textures), std::begin(heightMaps), std::end(heightMaps));

    fmt::print("{} process_mesh: creating mesh, vertices: {}, indices: {}, textures: {}\n",
               std::string(indent, ' '),  vertices.size(), indices.size(), textures.size());

    return pwgl::mesh(vertices, indices, textures);
}

void processNode(std::string directory, std::vector<pwgl::mesh> & meshes, aiNode *node, const aiScene *scene, int child = 0, int max_children = 0, std::size_t indent = 4)
{
    fmt::print("{} [{}/{}]processNode, meshes: {}, children: {}\n", std::string(indent, ' '),
               child, max_children, node->mNumMeshes, node->mNumChildren);

    for (unsigned i = 0; i < node->mNumMeshes; i++) {
        aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(
            process_mesh(directory, mesh, scene, indent + 4)
        );
    }

    //fmt::print("{} [{}/{}]processNode, children: {}\n", std::string(indent, ' '), child, max_children, node->mNumChildren);

    for (unsigned i = 0; i < node->mNumChildren; i++) {
        processNode(directory, meshes, node->mChildren[i], scene,
                    int(i), int(node->mNumChildren), indent + 4);
    }
}

void loadModel(std::vector<pwgl::mesh> & meshes, std::string const & path) {
    fmt::print("loadModel: name: {}\n", path);
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( path,
        aiProcess_Triangulate
      | aiProcess_GenSmoothNormals
      | aiProcess_FlipUVs
      | aiProcess_CalcTangentSpace
    );
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        fmt::print("ERROR::ASSIMP:: {}\n", importer.GetErrorString());
        throw std::logic_error("could not initialize pwgl::model");
    }

    std::string directory = path.substr(0, path.find_last_of('/'));
    processNode(directory, meshes, scene->mRootNode, scene,
                0, 0, 4);
}

} // anon ns

namespace pwgl {

struct model {
    model(std::string path) {
        stbi_set_flip_vertically_on_load(true);
        loadModel(meshes, path);
    }
    ~model() {
        fmt::print("~model()\n");
    }
    void draw(pwgl::shader &shader)
    {
        for(unsigned i = 0; i < meshes.size(); i++)
            meshes[i].draw(shader);
    }

    std::vector<pwgl::mesh> meshes;
    std::vector<pwgl::texture> textures_loaded;
    std::string directory;
};

} // pwgl ns
#endif
