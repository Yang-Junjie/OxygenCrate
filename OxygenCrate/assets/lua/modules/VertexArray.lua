local gl = rawget(_G, "opengles") or rawget(_G, "opengl")
assert(gl, "OpenGL bindings are not available in Lua")

local VertexArray = {}
VertexArray.__index = VertexArray

function VertexArray.new()
    local handle = gl.create_vertex_array()
    return setmetatable({ handle = handle }, VertexArray)
end

function VertexArray:bind()
    if self.handle then
        gl.bind_vertex_array(self.handle)
    end
end

function VertexArray.unbind()
    gl.bind_vertex_array(0)
end

function VertexArray:delete()
    if self.handle then
        gl.delete_vertex_array(self.handle)
        self.handle = nil
    end
end

return VertexArray
