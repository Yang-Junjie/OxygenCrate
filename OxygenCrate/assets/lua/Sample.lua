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

local TARGET_FRAME = 1.0 / 60.0
local MAX_FRAME = 1.0 / 20.0
local MIN_FRAME = 1.0 / 240.0

local function frame_time_color(delta_time)
    local dt = delta_time or TARGET_FRAME
    if dt < MIN_FRAME then
        dt = MIN_FRAME
    elseif dt > MAX_FRAME then
        dt = MAX_FRAME
    end
    local ratio = (dt - MIN_FRAME) / (MAX_FRAME - MIN_FRAME)
    local r = 0.25 + 0.75 * ratio
    local g = 0.9 - 0.6 * ratio
    local b = 0.45 - 0.3 * ratio
    return r, g, b
end

local ui = {
    window = { open = true },
    size = { 256, 256 },
    background = { 0.05, 0.05, 0.08, 1.0 },
    image_buffers = nil,
    front_buffer = 1,
    back_buffer = 2,
    resources = nil,
    time_accumulator = 0.0,
    last_time = nil,
    last_delta_time = 0.0
}

local function build_vertex_stream(target, info, time_value, frame_dt)
    local vertices = target or {}
    local r, g, b = frame_time_color(frame_dt)
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

        local base = (idx - 1) * 6
        vertices[base + 1] = px
        vertices[base + 2] = py
        vertices[base + 3] = pz
        vertices[base + 4] = r
        vertices[base + 5] = g
        vertices[base + 6] = b
    end
    return vertices
end

local function upload_vertex_stream(resources, info, time_value, frame_dt)
    resources.dynamic_vertices = build_vertex_stream(resources.dynamic_vertices, info, time_value, frame_dt)
    local dynamic_vertices = resources.dynamic_vertices
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

    ui.image_buffers = {
        create_image(ui.size[1], ui.size[2]),
        create_image(ui.size[1], ui.size[2])
    }
    if not ui.image_buffers[1] or not ui.image_buffers[2] then
        ui.image_buffers = nil
        return
    end
    ui.front_buffer = 1
    ui.back_buffer = 2

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

    upload_vertex_stream(ui.resources, vertex_info, 0.0, ui.last_delta_time)
end

local function render_rectangle()
    if not ui.resources or not flux or not ui.image_buffers then
        return false
    end

    upload_vertex_stream(ui.resources, ui.resources.vertex_info, ui.time_accumulator, ui.last_delta_time)

    local target_image = ui.image_buffers[ui.back_buffer]
    if not target_image or not flux.bind_framebuffer(target_image) then
        log("bind_framebuffer failed")
        return false
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
    return true
end

function ui.render(delta_time)
    ensure_resources()
    delta_time = delta_time or 0.0
    ui.last_delta_time = delta_time
    if not ui.window.open then
        return
    end
    ui.time_accumulator = ui.time_accumulator + delta_time
    local rendered = render_rectangle()
    if rendered then
        ui.front_buffer, ui.back_buffer = ui.back_buffer, ui.front_buffer
    end
end

function ui.draw()
    ensure_resources()

    if not ui.window.open then
        return
    end

    if imgui.begin_window("Lua Image Panel", ui.window) then
        if not gl or not flux then
            imgui.text("OpenGL bindings are unavailable.")
        elseif not ui.image_buffers then
            imgui.text("Failed to allocate image.")
        else
            imgui.text_wrapped("Lua renders a rectangle into a Flux::Image framebuffer and shows it below. You can require Vec2 or any custom module placed in the lua folder.")
            local display_image = ui.image_buffers[ui.front_buffer]
            if display_image then
                imgui.image(display_image, ui.size[1], ui.size[2])
            else
                imgui.text("Front buffer unavailable.")
            end
            local fps = (ui.last_delta_time and ui.last_delta_time > 0.0) and (1.0 / ui.last_delta_time) or 0.0
            imgui.text(string.format("FPS: %.2f", fps))
        end
    end
    imgui.end_window()
end

function ui.onUpdate(delta_time)
    -- Reserved for additional per-frame logic supplied by Lua scripts.
end

ui.update = ui.onUpdate

return ui
