#ifndef MESH_HPP
#define MESH_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include "stb_image.h"

#include <string>
#include <vector>

namespace pwgl {

struct vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class mesh {
public:
    mesh(std::vector<vertex> vertices, std::vector<unsigned> indices, std::vector<texture> textures)
        : vertices(vertices)
        , indices(indices)
        , textures(textures)
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        unsigned VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), &vertices[0], GL_STATIC_DRAW);

        unsigned EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Bitangent));

        glBindVertexArray(0);
    }

    ~mesh() {
        //fmt::print("~mesh()\n");
#if 0
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, VBO);
        glDeleteBuffers(1, EBO);
#endif
    }

    void draw(pwgl::shader & shader) {
        unsigned diffuseNr = 1;
        unsigned specularNr = 1;
        unsigned normalNr = 1;
        unsigned heightNr = 1;
        for (std::size_t i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
             else if (name == "texture_height")
                number = std::to_string(heightNr++);

            shader.set(i, std::string(name + number));
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }

    // render data

    std::vector<vertex> vertices;
    std::vector<unsigned> indices;
    std::vector<texture> textures;
    unsigned VAO;
};

} // pwgl ns
#endif

