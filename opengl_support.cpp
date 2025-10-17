#include "opengl_support.hpp"

namespace pwgl {

void print_glinfo()
{
    GLenum params[] = {
        GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
        GL_MAX_CUBE_MAP_TEXTURE_SIZE,
        GL_MAX_DRAW_BUFFERS,
        GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
        GL_MAX_TEXTURE_IMAGE_UNITS,
        GL_MAX_TEXTURE_SIZE,
        GL_MAX_VARYING_FLOATS,
        GL_MAX_VERTEX_ATTRIBS,
        GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
        GL_MAX_VERTEX_UNIFORM_COMPONENTS,
        GL_MAX_VIEWPORT_DIMS,
        GL_STEREO,
    };
    std::string_view names[] = {
        "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
        "GL_MAX_CUBE_MAP_TEXTURE_SIZE",
        "GL_MAX_DRAW_BUFFERS",
        "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
        "GL_MAX_TEXTURE_IMAGE_UNITS",
        "GL_MAX_TEXTURE_SIZE",
        "GL_MAX_VARYING_FLOATS",
        "GL_MAX_VERTEX_ATTRIBS",
        "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
        "GL_MAX_VERTEX_UNIFORM_COMPONENTS",
        "GL_MAX_VIEWPORT_DIMS",
        "GL_STEREO",
    };
    fmt::print("GL Context Params:\n");
    for (int i = 0; i < 10; i++) {
        int v = 0;
        glGetIntegerv(params[i], &v);
        fmt::print("  {} {}\n", names[i], v);
    }
    // others
    int v[2];
    v[0] = v[1] = 0;
    glGetIntegerv(params[10], v);
    fmt::print("  {} {} {}\n", names[10], v[0], v[1]);
    unsigned char s = 0;
    glGetBooleanv(params[11], &s);
    fmt::print("  {} {}\n", names[11], (unsigned int)s);

    GLubyte const * renderer = glGetString(GL_RENDERER);
    fmt::print("  Renderer: {}\n", reinterpret_cast<const char*>(renderer));

    GLubyte const * version = glGetString(GL_VERSION);
    fmt::print("  OpenGL version supported {}\n", reinterpret_cast<const char*>(version));
}

std::map<std::string, std::stringstream> parse_shaders(std::string const filename)
{
    fmt::print("[~] parsing: \"{}\"\n", filename);
    std::map<std::string, std::stringstream> data;
    std::ifstream stream(filename);
    if (!stream) {
        fmt::print("error, could not open file: {}\n", filename);
        return data;
    }
    std::string line;
    std::stringstream ss;
    std::string section = "";
    while (getline(stream, line)) {
        std::regex const base_regex("#\\s?shader\\s+(\\w+)");
        std::smatch base_match;
        if (std::regex_match(line, base_match, base_regex)) {
            if (base_match.size() == 2) {
                std::ssub_match base_sub_match = base_match[1];
                section = base_sub_match.str();
                continue;
            }
        }
        //fmt::print("[{:10s}] {}\n", section, line);
        data[section] << line << "\n";
    }
    return data;
}

unsigned compile_shader(unsigned type, std::string const & source)
{
    unsigned const id = glCreateShader(type);
    char const * src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);

    auto shader_str = [](unsigned type) {
        switch (type) {
            case GL_VERTEX_SHADER:    return "GL_VERTEX_SHADER";
            case GL_FRAGMENT_SHADER : return "GL_FRAGMENT_SHADER";
        }
        return "";
    };

    fmt::print("[~] compiling \"{}\"\n", shader_str(type));
    glCompileShader(id);

    int success = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(static_cast<std::size_t>(length + 1));
        glGetShaderInfoLog(id, length, &length, message.data());
        fmt::print("[-] failed to compile {} shader\n", shader_str(type));
        fmt::print("{}\n", message.data());
        glDeleteShader(id);
        return 0;
    }

    return id;
}

shader create_shader(std::string const & vertex_source, std::string const & fragment_source)
{
    unsigned vs = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (!vs) {
        fmt::print("<shader>\n{}</shader>\n", vertex_source);
        return shader{};
    }
    unsigned fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fs) {
        fmt::print("<shader>\n{}</shader>\n", fragment_source);
        return shader{};
    }

    unsigned const program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    fmt::print("[~] linking shader program\n");
    glLinkProgram(program);

    int success = 0;
    glValidateProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        std::vector<char> message(512);
        glGetProgramInfoLog(program, message.size(), nullptr, message.data());
        fmt::print("[-] failed to link program\n");
        fmt::print("{}\n", message.data());
    }

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!success)
        return {};
    return shader(program);
}

} // pwgl namespace
