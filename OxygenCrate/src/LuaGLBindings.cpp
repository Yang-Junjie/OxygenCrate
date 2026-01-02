#include "LuaGLBindings.hpp"
#include "GLWrappers.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

namespace {

struct BufferResource {
    GLuint id = 0;
    GLenum target = GL_ARRAY_BUFFER;
};

struct VertexArrayResource {
    GLuint id = 0;
};

struct ShaderResource {
    GLuint program = 0;
};

std::unordered_map<int, BufferResource> s_Buffers;
std::unordered_map<int, VertexArrayResource> s_VertexArrays;
std::unordered_map<int, ShaderResource> s_Shaders;
int s_NextHandle = 1;

int StoreBuffer(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    GLuint id = Flux::GL::CreateBuffer(target, static_cast<std::size_t>(size), data, usage);
    Flux::GL::BindBuffer(target, 0);
    const int handle = s_NextHandle++;
    s_Buffers[handle] = BufferResource{ id, target };
    return handle;
}

int StoreVertexArray() {
    GLuint id = Flux::GL::CreateVertexArray();
    const int handle = s_NextHandle++;
    s_VertexArrays[handle] = VertexArrayResource{ id };
    return handle;
}

int StoreShaderProgram(const std::string& vertexSrc, const std::string& fragmentSrc) {
    GLuint program = Flux::GL::CreateShaderProgram(vertexSrc, fragmentSrc);
    const int handle = s_NextHandle++;
    s_Shaders[handle] = ShaderResource{ program };
    return handle;
}

sol::table GetOrCreateTable(sol::state& lua, const char* name) {
    sol::object existing = lua[name];
    if (existing.is<sol::table>())
        return existing.as<sol::table>();
    return lua.create_named_table(name);
}

} // namespace

namespace LuaGLBindings {

void Register(sol::state& lua) {
    auto registerCommon = [&](const char* tableName) {
        sol::table glTable = GetOrCreateTable(lua, tableName);

        glTable.set_function("clear_color", [](float r, float g, float b, float a) {
            Flux::GL::ClearColor(r, g, b, a);
        });
        glTable.set_function("clear", [](unsigned int mask) {
            Flux::GL::Clear(mask);
        });
        glTable.set_function("viewport", [](int x, int y, int width, int height) {
            Flux::GL::Viewport(x, y, width, height);
        });

        glTable.set_function("create_vertex_buffer", [](sol::as_table_t<std::vector<float>> vertices, sol::optional<unsigned int> usage) {
            const auto data = vertices.value();
            return StoreBuffer(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), usage.value_or(GL_STATIC_DRAW));
        });
        glTable.set_function("create_index_buffer", [](sol::as_table_t<std::vector<unsigned int>> indices, sol::optional<unsigned int> usage) {
            const auto data = indices.value();
            return StoreBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(unsigned int)), data.data(), usage.value_or(GL_STATIC_DRAW));
        });
        glTable.set_function("delete_buffer", [](int handle) {
            auto it = s_Buffers.find(handle);
            if (it != s_Buffers.end()) {
                Flux::GL::DeleteBuffer(it->second.id);
                s_Buffers.erase(it);
            }
        });
        glTable.set_function("bind_buffer", [](int handle, sol::optional<unsigned int> targetOverride) {
            if (handle == 0) {
                if (targetOverride)
                    Flux::GL::BindBuffer(targetOverride.value(), 0);
                return;
            }
            auto it = s_Buffers.find(handle);
            if (it != s_Buffers.end())
                Flux::GL::BindBuffer(targetOverride.value_or(it->second.target), it->second.id);
        });

        glTable.set_function("create_vertex_array", []() {
            return StoreVertexArray();
        });
        glTable.set_function("bind_vertex_array", [](int handle) {
            auto it = s_VertexArrays.find(handle);
            if (it != s_VertexArrays.end())
                Flux::GL::BindVertexArray(it->second.id);
            else
                Flux::GL::BindVertexArray(0);
        });
        glTable.set_function("delete_vertex_array", [](int handle) {
            auto it = s_VertexArrays.find(handle);
            if (it != s_VertexArrays.end()) {
                Flux::GL::DeleteVertexArray(it->second.id);
                s_VertexArrays.erase(it);
            }
        });
        glTable.set_function("enable_vertex_attrib_array", [](unsigned int index) {
            Flux::GL::EnableVertexAttribArray(index);
        });
        glTable.set_function("vertex_attrib_pointer", [](unsigned int index, int size, unsigned int type, bool normalized, int stride, intptr_t offset) {
            Flux::GL::VertexAttribPointer(index, size, type, normalized, stride, offset);
        });
        glTable.set_function("draw_elements", [](unsigned int mode, int count, unsigned int type, intptr_t offset) {
            Flux::GL::DrawElements(mode, count, type, offset);
        });

        glTable.set_function("create_shader_program", [](const std::string& vertexSrc, const std::string& fragmentSrc) {
            return StoreShaderProgram(vertexSrc, fragmentSrc);
        });
        glTable.set_function("use_shader_program", [](int handle) {
            auto it = s_Shaders.find(handle);
            if (it != s_Shaders.end())
                Flux::GL::UseProgram(it->second.program);
            else
                Flux::GL::UseProgram(0);
        });
        glTable.set_function("delete_shader_program", [](int handle) {
            auto it = s_Shaders.find(handle);
            if (it != s_Shaders.end()) {
                Flux::GL::DeleteShaderProgram(it->second.program);
                s_Shaders.erase(it);
            }
        });
        glTable.set_function("set_uniform_float", [](int handle, const std::string& name, float value) {
            auto it = s_Shaders.find(handle);
            if (it == s_Shaders.end())
                return false;
            return Flux::GL::SetUniformFloat(it->second.program, name.c_str(), value);
        });
    };

    registerCommon("opengl");
    registerCommon("opengles");
}

} // namespace LuaGLBindings
