-- Lua + OpenGL sample: render a rectangle to a Flux::Image with a time-based color pulse.
-- Demonstrates binding the image framebuffer, issuing GL commands, and updating uniforms every frame.

local Vec2 = require("Vec2")
local gl = opengles or opengl
local flux = flux_image
local GL_FLOAT = 0x1406
local GL_UNSIGNED_INT = 0x1405
local GL_TRIANGLES = 0x0004
local GL_COLOR_BUFFER_BIT = 0x00004000

local ui = {
    window = { open = true },
    size = { 256, 256 },
    background = { 0.05, 0.05, 0.08, 1.0 },
    image_id = nil,
    resources = nil,
    time_accumulator = 0.0,
    last_time = nil
}

local function ensure_resources()
    if not gl or not flux or ui.resources then
        return
    end

    ui.image_id = create_image(ui.size[1], ui.size[2])

    local baseVertices = {
        { Vec2.new(-0.9, -0.9), { 1.0, 0.2, 0.2 } },
        { Vec2.new( 0.9, -0.9), { 0.2, 1.0, 0.2 } },
        { Vec2.new( 0.9,  0.9), { 0.2, 0.2, 1.0 } },
        { Vec2.new(-0.9,  0.9), { 1.0, 1.0, 0.3 } },
    }
    local vertices = {}
    for _, entry in ipairs(baseVertices) do
        local pos = entry[1]
        local color = entry[2]
        table.insert(vertices, pos.x)
        table.insert(vertices, pos.y)
        table.insert(vertices, 0.0)
        table.insert(vertices, color[1])
        table.insert(vertices, color[2])
        table.insert(vertices, color[3])
    end

    local indices = { 0, 1, 2, 0, 2, 3 }

    local vao = gl.create_vertex_array()
    gl.bind_vertex_array(vao)

    local vbo = gl.create_vertex_buffer(vertices)
    gl.bind_buffer(vbo)

    local ebo = gl.create_index_buffer(indices)
    gl.bind_buffer(ebo)

    local stride = 6 * 4
    gl.enable_vertex_attrib_array(0)
    gl.vertex_attrib_pointer(0, 3, GL_FLOAT, false, stride, 0)
    gl.enable_vertex_attrib_array(1)
    gl.vertex_attrib_pointer(1, 3, GL_FLOAT, false, stride, 3 * 4)

    local isGLES = (opengles ~= nil)
    local vertexSrc = isGLES and [[#version 300 es
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
out vec3 v_Color;
void main() {
    v_Color = a_Color;
    gl_Position = vec4(a_Position, 1.0);
}]] or [[#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
out vec3 v_Color;
void main() {
    v_Color = a_Color;
    gl_Position = vec4(a_Position, 1.0);
}]]

    local fragmentSrc = isGLES and [[#version 300 es
precision mediump float;
in vec3 v_Color;
uniform float u_Time;
out vec4 FragColor;
void main() {
    float pulse = 0.5 + 0.5 * sin(u_Time);
    FragColor = vec4(v_Color * pulse, 1.0);
}]] or [[#version 330 core
in vec3 v_Color;
uniform float u_Time;
out vec4 FragColor;
void main() {
    float pulse = 0.5 + 0.5 * sin(u_Time);
    FragColor = vec4(v_Color * pulse, 1.0);
}]]

    local shader = gl.create_shader_program(vertexSrc, fragmentSrc)

    ui.resources = {
        vao = vao,
        vbo = vbo,
        ebo = ebo,
        shader = shader,
        index_count = #indices
    }
end

local function render_rectangle()
    if not ui.resources or not flux or not ui.image_id then
        return
    end

    if not flux.bind_framebuffer(ui.image_id) then
        log("bind_framebuffer failed")
        return
    end

    local now = os.clock()
    if ui.last_time then
        ui.time_accumulator = ui.time_accumulator + (now - ui.last_time)
    end
    ui.last_time = now

    gl.viewport(0, 0, ui.size[1], ui.size[2])
    gl.clear_color(ui.background[1], ui.background[2], ui.background[3], ui.background[4])
    gl.clear(GL_COLOR_BUFFER_BIT)

    local res = ui.resources
    gl.set_uniform_float(res.shader, "u_Time", ui.time_accumulator)
    gl.use_shader_program(res.shader)
    gl.bind_vertex_array(res.vao)
    gl.draw_elements(GL_TRIANGLES, res.index_count, GL_UNSIGNED_INT, 0)

    flux.unbind_framebuffer()
end

function ui.draw()
    ensure_resources()

    if not ui.window.open then
        return
    end

    if imgui.begin_window("Lua Image Panel", ui.window) then
        if not gl or not flux then
            imgui.text("OpenGL bindings are unavailable.")
        elseif not ui.image_id then
            imgui.text("Failed to allocate image.")
        else
            render_rectangle()
            imgui.text_wrapped("Lua renders a rectangle into a Flux::Image framebuffer and shows it below. You can require Vec2 or any custom module placed in the lua folder.")
            imgui.image(ui.image_id, ui.size[1], ui.size[2])
        end
    end
    imgui.end_window()
end

return ui
