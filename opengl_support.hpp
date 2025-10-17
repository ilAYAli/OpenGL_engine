//#include "glad/glad.h"
//#include <GL/gl.h>
#include "shader.hpp"
#include "camera.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // make_mat

#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <optional>
#include <regex>


#include "fmt/format.h"

namespace pwgl {

struct gls {
    gls(int w = 2560, int h = 1440) {
        if (!glfwInit())
            throw std::logic_error("could not initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        static constexpr bool fullscreen = false;
        static constexpr bool maximize = true;
        std::string window_title = "pwGL v.1.0";

        GLFWmonitor * mon = glfwGetPrimaryMonitor();

        width = w;
        height = h;

        if (!w || !h) {
            GLFWvidmode const * vmode = glfwGetVideoMode(mon);
            width = vmode->width;
            height = vmode->height;
        }

        if (maximize)
            glfwWindowHint(GLFW_MAXIMIZED , GL_TRUE);

        window = glfwCreateWindow(
            w,
            h,
            window_title.c_str(),
            fullscreen ? mon : nullptr,
            nullptr);
        if (!window) {
            glfwTerminate();
            throw std::logic_error("could not open window with GLFW3");
        }

        lastx = width / 2.0f;
        lasty = height / 2.0f;

        glfwMakeContextCurrent(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture mouse

        // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // start GLEW extension handler
        glewExperimental = GL_TRUE;
        if (glewInit())
            throw std::logic_error("could not initialize GLEW");

        //pwgl::print_glinfo();


        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }


    ~gls() {
        glfwTerminate();
    }

    float width { };
    float height { };
    float lastx { };
    float lasty { };
    double deltaTime { };

    GLFWwindow * window { };
    GLuint shader_id { };
    GLuint vao { };
    GLuint vbo_idx { };
    Camera camera { glm::vec3 { 0.0f, 1.0f, 3.0f } };
};


// prototypes:
unsigned compile_shader(unsigned type, std::string const & source);
shader create_shader(std::string const & vertex_source, std::string const & fragment_source);
std::map<std::string, std::stringstream> parse_shaders(std::string const filename);

} // pwgl ns.
