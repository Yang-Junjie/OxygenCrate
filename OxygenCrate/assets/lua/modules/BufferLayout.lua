local gl = rawget(_G, "opengles") or rawget(_G, "opengl")
assert(gl, "OpenGL bindings are not available in Lua")

local typeSizes = {
    [0x1406] = 4, -- GL_FLOAT
    [0x1404] = 4, -- GL_INT
    [0x1405] = 4, -- GL_UNSIGNED_INT
    [0x1401] = 1, -- GL_UNSIGNED_BYTE
}

local BufferLayout = {}
BufferLayout.__index = BufferLayout

local function validateElements(elements)
    assert(type(elements) == "table" and #elements > 0, "BufferLayout requires at least one element")
    for _, element in ipairs(elements) do
        assert(type(element.index) == "number", "element.index must be a number")
        assert(type(element.size) == "number", "element.size must be a number")
        assert(type(element.type) == "number", "element.type must be a number")
    end
end

local function calculateStride(elements)
    local stride = 0
    for _, element in ipairs(elements) do
        local bytes = typeSizes[element.type] or 4
        stride = stride + element.size * bytes
    end
    return stride
end

function BufferLayout.new(elements, stride)
    validateElements(elements)
    local layout = {
        elements = elements,
        stride = stride or calculateStride(elements),
    }
    return setmetatable(layout, BufferLayout)
end

function BufferLayout:apply(offset)
    local currentOffset = offset or 0
    for _, element in ipairs(self.elements) do
        gl.enable_vertex_attrib_array(element.index)
        gl.vertex_attrib_pointer(
            element.index,
            element.size,
            element.type,
            element.normalized or false,
            self.stride,
            currentOffset
        )
        local bytes = typeSizes[element.type] or 4
        currentOffset = currentOffset + element.size * bytes
    end
end

return BufferLayout
