-- Lua + OpenGL sample: render a rectangle to a Flux::Image with a time-based color pulse.
-- Demonstrates binding the image framebuffer, issuing GL commands, and updating uniforms every frame.

local Vec2 = require("Vec2")
local VertexArray = require("modules.VertexArray")
local VertexBuffer = require("modules.VertexBuffer")
local IndexBuffer = require("modules.IndexBuffer")
local BufferLayout = require("modules.BufferLayout")
local Shader = require("modules.Shader")
local gl = opengles or opengl
local flux = flux_image
local GL_FLOAT = 0x1406
local GL_UNSIGNED_INT = 0x1405
local GL_TRIANGLES = 0x0004
local GL_COLOR_BUFFER_BIT = 0x00004000
local GL_DYNAMIC_DRAW = 0x88E8

local sin, cos = math.sin, math.cos
local function wave(time, phase)
    return 0.5 + 0.5 * sin(time + phase)
end

local ui = {
    window = { open = true },
    size = { 256, 256 },
    background = { 0.05, 0.05, 0.08, 1.0 },
    image_id = nil,
    resources = nil,
    time_accumulator = 0.0,
    last_time = nil
}

local function build_vertex_stream(info, time_value)
    local vertices = {}
    for idx, entry in ipairs(info) do
        local pos = entry.position
        local spin = time_value * 0.45 + idx * 0.35
        local wobble = 0.15 * sin(time_value * 0.7 + idx)
        local scale = 0.85 + 0.15 * sin(time_value * 0.3 + idx * 1.2)
        local rot = wobble + entry.angle
        local c, s = cos(rot), sin(rot)
        local px = (pos.x * c - pos.y * s) * scale
        local py = (pos.x * s + pos.y * c) * scale
        local pz = 0.03 * sin(spin)

        local r = wave(time_value + idx * 0.7, 0.0)
        local g = wave(time_value + idx * 0.7, 2.094) -- 120° phase shift
        local b = wave(time_value + idx * 0.7, 4.188) -- 240° phase shift

        table.insert(vertices, px)
        table.insert(vertices, py)
        table.insert(vertices, pz)
        table.insert(vertices, r)
        table.insert(vertices, g)
        table.insert(vertices, b)
    end
    return vertices
end

local function upload_vertex_stream(resources, info, time_value)
    local dynamic_vertices = build_vertex_stream(info, time_value)
    if resources.vbo then
        resources.vbo:set_data(dynamic_vertices, GL_DYNAMIC_DRAW)
    else
        resources.vbo = VertexBuffer.new(dynamic_vertices, GL_DYNAMIC_DRAW)
        resources.vao:bind()
        resources.vbo:bind()
        resources.layout:apply()
    end
end

local function ensure_resources()
    if not gl or not flux or ui.resources then
        return
    end

    ui.image_id = create_image(ui.size[1], ui.size[2])

    local vertex_info = {
        { position = Vec2.new(-0.9, -0.9), angle = 0.0 },
        { position = Vec2.new( 0.9, -0.9), angle = 0.5 },
        { position = Vec2.new( 0.9,  0.9), angle = 1.0 },
        { position = Vec2.new(-0.9,  0.9), angle = 1.5 },
    }
    local indices = { 0, 1, 2, 0, 2, 3 }

    local vao = VertexArray.new()
    vao:bind()

    local layout = BufferLayout.new({
        { index = 0, size = 3, type = GL_FLOAT, normalized = false },
        { index = 1, size = 3, type = GL_FLOAT, normalized = false },
    })

    local ebo = IndexBuffer.new(indices)
    ebo:bind()

    local isGLES = (opengles ~= nil)
    local shaderFolder = isGLES and "shaders/opengles" or "shaders/opengl"
    local shader = Shader.from_files(shaderFolder .. "/simple.vert", shaderFolder .. "/simple.frag")

    ui.resources = {
        vao = vao,
        vbo = nil,
        ebo = ebo,
        shader = shader,
        layout = layout,
        vertex_info = vertex_info,
        index_count = #indices
    }

    upload_vertex_stream(ui.resources, vertex_info, 0.0)
end

local function render_rectangle()
    if not ui.resources or not flux or not ui.image_id then
        return
    end

    upload_vertex_stream(ui.resources, ui.resources.vertex_info, ui.time_accumulator)

    if not flux.bind_framebuffer(ui.image_id) then
        log("bind_framebuffer failed")
        return
    end

    local t = ui.time_accumulator
    gl.viewport(0, 0, ui.size[1], ui.size[2])
    ui.background[1] = 0.15 + 0.10 * sin(t * 0.5)
    ui.background[2] = 0.12 + 0.08 * sin(t * 0.7 + 2.0)
    ui.background[3] = 0.18 + 0.12 * sin(t * 0.9 + 4.0)
    gl.clear_color(ui.background[1], ui.background[2], ui.background[3], ui.background[4])
    gl.clear(GL_COLOR_BUFFER_BIT)

    local res = ui.resources
    res.shader:use()
    res.shader:set_float("u_Time", t)
    res.vao:bind()
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

function ui.update(delta_time)
    ensure_resources()
    if not ui.window.open then
        return
    end
    ui.time_accumulator = ui.time_accumulator + (delta_time or 0.0)
end

return ui
