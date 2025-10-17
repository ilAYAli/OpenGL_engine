#define STB_IMAGE_IMPLEMENTATION

#include "opengl_support.hpp"
#include "model.hpp"
//#include "shader.hpp"
//#include "mesh.hpp"
//#include "model.hpp"
//#include "wavefront.hpp"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "camera.hpp"

#include <vector>
#include <fstream>
#include <string>
#include <string_view>

#include "fmt/format.h"

// globals:
pwgl::gls gls{1920, 1080};

namespace fmt {

template<>
struct formatter<glm::uvec3> : formatter<std::string_view> {
    auto format(glm::uvec3 v, format_context & ctx) {
        return formatter<std::string_view>::format(fmt::format(
            "{}, {}, {}", v.x, v.y, v.z), ctx);
    }
};

template<>
struct formatter<glm::vec3> : formatter<std::string_view> {
    auto format(glm::vec3 v, format_context & ctx) const {
        return formatter<std::string_view>::format(fmt::format(
            "{:+.1f}, {:+.1f}, {:+.1f}", v.x, v.y, v.z), ctx);
    }
};

template<>
struct formatter<glm::vec4> : formatter<std::string_view> {
    auto format(glm::vec4 v, format_context & ctx) {
        return formatter<std::string_view>::format(fmt::format(
            "{:+.1f}, {:+.1f}, {:+.1f}, {:+.1f}", v.x, v.y, v.z, v.w), ctx);
    }
};


} // fmt

namespace {

[[maybe_unused]] std::vector<glm::vec3> gen_colors(std::size_t faces)
{
    auto const stride = 3.0f / faces;
    std::vector<glm::vec3> ret;
    float val = 0;
    for (std::size_t idx = 0; idx < faces; ++idx) {
        float r = 0;
        float g = 0;
        float b = 0;
        val += stride;
        if (val > 2.0f)
            b = val - 2.0f;
        else if (val > 1.0f)
            g = val - 1.0f;
        else
            r = val;
        ret.emplace_back(glm::vec3(r, g, b));
    }
    return ret;
}

void scroll_callback(GLFWwindow * /* window */, double /* xoffset  */, double yoffset)
{
    gls.camera.ProcessMouseScroll(yoffset);
}

void mouse_callback(GLFWwindow * /* window */, double xpos, double ypos)
{
    static bool firstMouse = true;
    auto const x = static_cast<float>(xpos);
    auto const y = static_cast<float>(ypos);
    if (firstMouse) {
        gls.lastx = x;
        gls.lasty = y;
        firstMouse = false;
    }

    float const xoffset = x - gls.lastx;
    float const yoffset = gls.lasty - y;  // Reversed since y-coordinates go from bottom to left

    gls.lastx = x;
    gls.lasty = y;

    gls.camera.ProcessMouseMovement(xoffset, yoffset);
}

void update_fps_counter(GLFWwindow * window)
{
    static double previous_seconds = glfwGetTime();
    static int frame_count;
    double current_seconds = glfwGetTime();
    double elapsed_seconds = current_seconds - previous_seconds;
    if (elapsed_seconds > 0.25) {
        previous_seconds = current_seconds;
        double fps = (double)frame_count / elapsed_seconds;
        glfwSetWindowTitle(window, fmt::format("opengl @ fps: {:.2f}", fps).c_str());
        frame_count = 0;
    }
    frame_count++;
}

void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // movement:
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(FORWARD, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(LEFT, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(BACKWARD, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(RIGHT, gls.deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(UP, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(DOWN, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(LEFT, gls.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        gls.camera.ProcessKeyboard(RIGHT, gls.deltaTime);
}

} // anon ns.

namespace {

//[[maybe_unused]] inline glm::uvec3 uvec3_cast(const unsigned * v) { return glm::uvec3(&v); }
[[maybe_unused]] inline glm::vec3 vec3_cast(const aiVector3D &v) { return glm::vec3(v.x, v.y, v.z); }
[[maybe_unused]] inline glm::vec2 vec2_cast(const aiVector3D &v) { return glm::vec2(v.x, v.y); }
[[maybe_unused]] inline glm::quat quat_cast(const aiQuaternion &q) { return glm::quat(q.w, q.x, q.y, q.z); }
[[maybe_unused]] inline glm::mat4 mat4_cast(const aiMatrix4x4 &m) { return glm::transpose(glm::make_mat4(&m.a1)); }
[[maybe_unused]] inline glm::mat4 mat4_cast(const aiMatrix3x3 &m) { return glm::transpose(glm::make_mat3(&m.a1)); }

}


struct object {
    object() = default;
    object(aiScene const * s)
    : scene(s) { }
    ~object() {
        fmt::print("~object\n");
        //Assimp::Importer::FreeScene(scene);
    }
    aiScene const * scene { nullptr };
    std::vector<glm::vec3> vertices;
    std::vector<unsigned> indices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textures;
};


object load_object(std::string path, bool debug = false)
{
    fmt::print("[~] parsing model : \"{}\"\n", path);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
          aiProcess_CalcTangentSpace
        | aiProcess_Triangulate
        //| aiProcess_GenNormals
        | aiProcess_JoinIdenticalVertices
        | aiProcess_SortByPType);
    if (!scene) {
        fmt::print("error, could not parse: {}\n", path);
        exit(1);
    }

    //    | aiProcess_FixInfacingNormals

    //aiProcess_RemoveComponent( aiComponent_NORMALS);

    object obj(importer.GetOrphanedScene());

    // for each mesh...
    for (std::size_t mi = 0; mi < scene->mNumMeshes; ++mi) {
        auto const mesh = scene->mMeshes[mi];
        glm::vec3 vertex;

        if (debug) {
            fmt::print("vertices: {}\n", mesh->mNumVertices);
            fmt::print("hasNormals: {}\n", mesh->HasNormals());
        }

        // for each vertex...
        for (std::size_t vi = 0; vi < mesh->mNumVertices; ++vi) {
            // vertex:
            vertex = vec3_cast(mesh->mVertices[vi]);
            obj.vertices.emplace_back(vertex);

            // normal:
            if (mesh->HasNormals()) {
                glm::vec3 normal = vec3_cast(mesh->mNormals[vi]);
                obj.normals.emplace_back(normal);
                if (debug)
                    fmt::print("{:2d}: vertex: {}, normal: {}\n", vi + 1, vertex, normal);
            }
            if (mesh->HasTextureCoords(0)) {
                glm::vec2 texture { mesh->mTextureCoords[0][vi].x, mesh->mTextureCoords[0][vi].y };
                obj.textures.emplace_back(texture);
            }
        }

        // indices:
        for (std::size_t f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (std::size_t fi = 0; fi < face.mNumIndices; fi++)
                obj.indices.emplace_back(face.mIndices[fi]);
        }
    }
    if (debug) {
        fmt::print("num indices: {}\n", obj.indices.size());
        fmt::print("object model : {}\n", path);
    }

    return obj;
}


int main(int argc, char ** argv)
{
    fmt::print("resolution: {}x{}\n", gls.width, gls.height);
    glfwSetCursorPosCallback(gls.window, mouse_callback);
    glfwSetScrollCallback(gls.window, scroll_callback);

    auto create_shaders = [](std::string file) {
        auto source = pwgl::parse_shaders(file);
        assert(!source["vertex"].str().empty());
        assert(!source["fragment"].str().empty());
        auto opt = pwgl::create_shader(
            source["vertex"].str(),
            source["fragment"].str());
        return opt;
    };

    //---[ model ]--------------------------------------------------------------
    std::string const model_file = argc < 2 ? "./resources/models/cube.obj" : argv[1];
    //auto model_object = load_object(model_file, true);
    auto model_shader = create_shaders("resources/shaders/model_loading.glsl");
    if (!model_shader.id) {
        fmt::print("error, failed to create shader from: {}\n", "shaders/lightning.glsl");
        return 1;
    }

    pwgl::model backpack_model(argc < 2 ? "resources/models/nanosuit/nanosuit.obj" : argv[1]);


    //---[ lamp ]---------------------------------------------------------------
    Assimp::Importer foo;
    auto lamp_object = load_object("./resources/models/cube.obj");
    auto lamp_shader = create_shaders("./resources/shaders/lamp.glsl");
    {
        std::string fn = "shaders/lamp.glsl";
        if (!lamp_shader.id) {
            fmt::print("error, failed to create shader: {}\n", fn);
            return 1;
        }

        lamp_shader.vao_alloc();
        lamp_shader.vbo_alloc(lamp_object.vertices, "position");
        lamp_shader.ebo_alloc(lamp_object.indices);
    }

    double lastFrame = 0.0f;
    while(!glfwWindowShouldClose(gls.window)) {
        double currentFrame = glfwGetTime();
        gls.deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        process_input(gls.window);
        update_fps_counter(gls.window);

        // render:
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 //---[ model ]------------------------------------------
        {
            glm::vec3 modelpos{0.0f, -2.8f, -5.0f};

            // MVP:
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, modelpos);
            //model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, (float)glfwGetTime() / 1.0f, glm::vec3(0.0f, 0.1f, 0.0f));
            model = glm::scale(model, glm::vec3{0.5f});

            glm::mat4 view;
            view = gls.camera.get_view_matrix();

            glm::mat4 projection = glm::perspective(gls.camera.get_zoom(), gls.width / gls.height, 0.1f, 100.0f);

            // model uniforms:
            model_shader.use();
            model_shader.set(model, "model");
            model_shader.set(view, "view");
            model_shader.set(projection, "projection");
            model_shader.use();
            backpack_model.draw(model_shader);
        }
 //---[ lamp ]-------------------------------------------
        {
            glm::vec3 lightpos{1.0f, 3.0f, -1.0f};

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, lightpos);
            model = glm::scale(model, glm::vec3{0.1f});

            glm::mat4 view;
            view = gls.camera.get_view_matrix();

            glm::mat4 projection = glm::perspective(gls.camera.get_zoom(), gls.width / gls.height, 0.1f, 100.0f);
            // use model_shader program:
            lamp_shader.use();
            lamp_shader.set(model, "model");
            lamp_shader.set(view, "view");
            lamp_shader.set(projection, "projection");

            // draw light box:
            for (auto vao : lamp_shader.vaos)
                glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, static_cast<int>(lamp_object.indices.size() * 3), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(gls.window);
        glfwPollEvents();
    }

    fmt::print("exit\n");
    glfwTerminate();
    return 0;
}

