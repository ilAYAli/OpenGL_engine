#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // make_mat

//#include "fmt/format.h"

#include <string>
#include <vector>


namespace pwgl {

struct shader {
    shader() = default;
    shader(unsigned id)
        : id(id)
    { }
    ~shader() {
        //fmt::print("~shader()\n");
        for (unsigned elt : vaos) {
            //fmt::print("[~] deleting vao: {}\n", elt);
            glDeleteVertexArrays(1, &elt);
        }
        for (unsigned elt : vbos) {
            //fmt::print("[~] deleting vbo: {}\n", elt);
            glDeleteBuffers(1, &elt);
        }
        for (unsigned elt : ebos) {
            //fmt::print("[~] deleting ebo: {}\n", elt);
            glDeleteBuffers(1, &elt);
        }
        //fmt::print("[~] deleting shader: {}\n", id);
        glDeleteProgram(id);
    }

    unsigned vao_alloc() {
        unsigned vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        vaos.emplace_back(vao);
        return vao;
    }

    unsigned vbo_alloc(void const * data, std::size_t size, std::string name) {
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

        auto const attr_id = this->getAttribute(name.c_str());
        glEnableVertexAttribArray(attr_id);
        glVertexAttribPointer(attr_id, 3,  GL_FLOAT, 0, 0, 0); // GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        vbos.emplace_back(vbo);
        return vbo;
    }

    unsigned vbo_alloc(std::vector<glm::vec3> const & data, std::string name) {
        return vbo_alloc(data.data(), data.size() * sizeof(glm::vec3), name);
    }

    unsigned ebo_alloc(void const * data, std::size_t size)
    {
        unsigned int ebo;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        ebos.emplace_back(ebo);
        return ebo;
    }

    unsigned texture_alloc(std::string path = "resources/textures/container.jpg") {
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // repeat:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // filtering:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        unsigned char *data = stbi_load(
            path.c_str(),
            &width,
            &height,
            &nrChannels,
            0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            texture = 0;
        }
        stbi_image_free(data);
        return texture;
    }

    template <typename T>
    unsigned ebo_alloc(std::vector<T> const & data) {
        return ebo_alloc(data.data(), data.size() * sizeof(T));
    }

    int getAttribute(std::string const & name) const {
        return glGetAttribLocation(id, name.c_str());
    }
    int getLocation(std::string const & name) const {
        return glGetUniformLocation(id, name.c_str());
    }
    void set(glm::vec3 v, std::string name) const {
        glUniform3fv(this->getLocation(name), 1, &v[0]);
    }
    void set(glm::mat4 m, std::string name) const {
        glUniformMatrix4fv(this->getLocation(name), 1, GL_FALSE, &m[0][0]);
    }
    void set(int i, std::string name) const {
        glUniform1i(this->getLocation(name), i);
    }
    void set(unsigned long ul, std::string name) const {
        glUniform1i(this->getLocation(name), ul);
    }

    template <typename T>
    static inline constexpr bool dependent_false_v{ false };

    template <typename T>
    void set(T t, std::string name) const {
        if (std::is_integral<T>::value)
            return glUniform1i(this->getLocation(name.c_str()), static_cast<int>(t));
        else if (std::is_floating_point<T>::value)
            return glUniform1f(this->getLocation(name.c_str()), t);
        else
            static_assert(dependent_false_v<T>, "type unsupported");
    }
    void use() const {
        glUseProgram(id);
    }

    std::vector<unsigned> vaos;
    std::vector<unsigned> vbos;
    std::vector<unsigned> ebos;
    unsigned id { };
};


} // pwgl ns

#endif
