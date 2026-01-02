local gl = rawget(_G, "opengles") or rawget(_G, "opengl")
assert(gl, "OpenGL bindings are not available in Lua")

local Shader = {}
Shader.__index = Shader

local function load_file(path)
    assert(type(path) == "string" and path ~= "", "Shader path must be a non-empty string")
    if type(load_module_file) ~= "function" then
        error("load_module_file function is not available")
    end
    local contents = load_module_file(path)
    assert(contents and contents ~= "", string.format("Failed to load shader file: %s", path))
    return contents
end

function Shader.new(vertexSrc, fragmentSrc)
    assert(type(vertexSrc) == "string" and vertexSrc ~= "", "vertexSrc must be a string")
    assert(type(fragmentSrc) == "string" and fragmentSrc ~= "", "fragmentSrc must be a string")
    local handle = gl.create_shader_program(vertexSrc, fragmentSrc)
    return setmetatable({ handle = handle }, Shader)
end

function Shader.from_files(vertexPath, fragmentPath)
    local vertexSrc = load_file(vertexPath)
    local fragmentSrc = load_file(fragmentPath)
    return Shader.new(vertexSrc, fragmentSrc)
end

function Shader:use()
    if self.handle then
        gl.use_shader_program(self.handle)
    end
end

function Shader:stop()
    gl.use_shader_program(0)
end

function Shader:set_float(name, value)
    assert(type(name) == "string", "uniform name must be a string")
    assert(type(value) == "number", "uniform value must be a number")
    if self.handle then
        gl.set_uniform_float(self.handle, name, value)
    end
end

function Shader:delete()
    if self.handle then
        gl.delete_shader_program(self.handle)
        self.handle = nil
    end
end

return Shader
