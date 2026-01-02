local gl = rawget(_G, "opengles") or rawget(_G, "opengl")
assert(gl, "OpenGL bindings are not available in Lua")

local IndexBuffer = {}
IndexBuffer.__index = IndexBuffer

local function ensureTable(data)
    assert(type(data) == "table", "IndexBuffer expects a table of numbers")
end

function IndexBuffer.new(indices, usage)
    ensureTable(indices)
    local handle = gl.create_index_buffer(indices, usage)
    return setmetatable({ handle = handle }, IndexBuffer)
end

function IndexBuffer:bind()
    if self.handle then
        gl.bind_buffer(self.handle)
    end
end

function IndexBuffer.unbind()
    gl.bind_buffer(0)
end

function IndexBuffer:delete()
    if self.handle then
        gl.delete_buffer(self.handle)
        self.handle = nil
    end
end

return IndexBuffer
