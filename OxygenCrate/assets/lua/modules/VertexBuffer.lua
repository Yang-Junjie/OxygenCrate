local gl = rawget(_G, "opengles") or rawget(_G, "opengl")
assert(gl, "OpenGL bindings are not available in Lua")

local VertexBuffer = {}
VertexBuffer.__index = VertexBuffer

local function ensureTable(data)
    assert(type(data) == "table", "VertexBuffer expects a table of numbers")
end

function VertexBuffer.new(vertices, usage)
    ensureTable(vertices)
    local handle = gl.create_vertex_buffer(vertices, usage)
    return setmetatable({ handle = handle }, VertexBuffer)
end

function VertexBuffer:set_data(vertices, usage)
    ensureTable(vertices)
    if self.handle then
        gl.update_vertex_buffer(self.handle, vertices, usage)
    end
end

function VertexBuffer:bind(target)
    if self.handle then
        gl.bind_buffer(self.handle, target)
    end
end

function VertexBuffer.unbind(target)
    gl.bind_buffer(0, target)
end

function VertexBuffer:delete()
    if self.handle then
        gl.delete_buffer(self.handle)
        self.handle = nil
    end
end

return VertexBuffer
