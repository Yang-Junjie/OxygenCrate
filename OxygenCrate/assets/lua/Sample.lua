-- Lua + OpenGL sample: render a user-controlled triangle into a Flux::Image.
-- Demonstrates building vertex buffers from ImGui input and drawing with vertex colors.

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

local sin = math.sin
local pi = math.pi

local function make_default_vertices()
    return {
        { name = "Vertex A", position = { -0.65, -0.55 }, color = { 1.0, 0.2, 0.2 } },
        { name = "Vertex B", position = { 0.65, -0.45 }, color = { 0.25, 1.0, 0.35 } },
        { name = "Vertex C", position = { 0.0, 0.75 }, color = { 0.25, 0.45, 1.0 } },
    }
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
    last_delta_time = 0.0,
    vertex_controls = make_default_vertices(),
}

local function build_vertex_stream(target, controls)
    local vertices = target or {}
    for idx, entry in ipairs(controls) do
        local base = (idx - 1) * 6
        local position = entry.position
        local color = entry.color
        vertices[base + 1] = position[1]
        vertices[base + 2] = position[2]
        vertices[base + 3] = 0.0
        vertices[base + 4] = color[1]
        vertices[base + 5] = color[2]
        vertices[base + 6] = color[3]
    end
    return vertices
end

local function upload_vertex_stream(resources, controls)
    resources.dynamic_vertices = build_vertex_stream(resources.dynamic_vertices, controls)
    local data = resources.dynamic_vertices
    if resources.vbo then
        resources.vbo:set_data(data, GL_DYNAMIC_DRAW)
    else
        resources.vbo = VertexBuffer.new(data, GL_DYNAMIC_DRAW)
        resources.vao:bind()
        resources.vbo:bind()
        resources.layout:apply()
    end
end

local function reset_vertex_controls()
    ui.vertex_controls = make_default_vertices()
    if ui.resources then
        upload_vertex_stream(ui.resources, ui.vertex_controls)
    end
end

local function ensure_resources()
    if not gl or not flux or ui.resources then
        return
    end

    ui.image_buffers = {
        create_image(ui.size[1], ui.size[2]),
        create_image(ui.size[1], ui.size[2]),
    }
    if not ui.image_buffers[1] or not ui.image_buffers[2] then
        ui.image_buffers = nil
        return
    end
    ui.front_buffer = 1
    ui.back_buffer = 2

    local indices = { 0, 1, 2 }

    local vao = VertexArray.new()
    vao:bind()

    local layout = BufferLayout.new({
        { index = 0, size = 3, type = GL_FLOAT, normalized = false },
        { index = 1, size = 3, type = GL_FLOAT, normalized = false },
    })

    local ebo = IndexBuffer.new(indices)
    ebo:bind()

    local isGLES = (opengles ~= nil)
    local shader_folder = isGLES and "shaders/opengles" or "shaders/opengl"
    local shader = Shader.from_files(shader_folder .. "/simple.vert", shader_folder .. "/simple.frag")

    ui.resources = {
        vao = vao,
        vbo = nil,
        ebo = ebo,
        shader = shader,
        layout = layout,
        index_count = #indices,
        dynamic_vertices = nil,
    }

    upload_vertex_stream(ui.resources, ui.vertex_controls)
end

local function render_triangle()
    if not ui.resources or not flux or not ui.image_buffers then
        return false
    end

    upload_vertex_stream(ui.resources, ui.vertex_controls)

    local target_image = ui.image_buffers[ui.back_buffer]
    if not target_image or not flux.bind_framebuffer(target_image) then
        log("bind_framebuffer failed")
        return false
    end

    local t = ui.time_accumulator
    gl.viewport(0, 0, ui.size[1], ui.size[2])
    ui.background[1] = 0.10 + 0.10 * sin(t * 0.6)
    ui.background[2] = 0.12 + 0.08 * sin(t * 0.8 + 2.1)
    ui.background[3] = 0.18 + 0.10 * sin(t * 1.1 + 4.2)
    gl.clear_color(ui.background[1], ui.background[2], ui.background[3], ui.background[4])
    gl.clear(GL_COLOR_BUFFER_BIT)

    local res = ui.resources
    res.shader:use()
    res.shader:set_float("u_Time", pi * 0.5)
    res.vao:bind()
    gl.draw_elements(GL_TRIANGLES, res.index_count, GL_UNSIGNED_INT, 0)

    flux.unbind_framebuffer()
    return true
end

local function slider_component(label, value, min_value, max_value)
    local new_value, changed = imgui.slider_float(label, value, min_value, max_value)
    if changed then
        return new_value, true
    end
    return value, false
end

local function current_fps()
    if imgui.get_framerate then
        return imgui.get_framerate()
    end
    if ui.last_delta_time and ui.last_delta_time > 0.0 then
        return 1.0 / ui.last_delta_time
    end
    return 0.0
end

local function draw_vertex_controls()
    imgui.text_wrapped("Use the sliders below to tweak each vertex. Values are clamped to safe ranges for the demo.")
    imgui.spacing()

    local changed = false
    for _, vertex in ipairs(ui.vertex_controls) do
        imgui.separator()
        imgui.text(vertex.name)

        local x_label = vertex.name .. " X"
        local new_x, x_changed = slider_component(x_label, vertex.position[1], -1.5, 1.5)
        if x_changed then
            vertex.position[1] = new_x
            changed = true
        end

        local y_label = vertex.name .. " Y"
        local new_y, y_changed = slider_component(y_label, vertex.position[2], -1.5, 1.5)
        if y_changed then
            vertex.position[2] = new_y
            changed = true
        end

        local r_label = vertex.name .. " R"
        local new_r, r_changed = slider_component(r_label, vertex.color[1], 0.0, 1.0)
        if r_changed then
            vertex.color[1] = new_r
            changed = true
        end

        local g_label = vertex.name .. " G"
        local new_g, g_changed = slider_component(g_label, vertex.color[2], 0.0, 1.0)
        if g_changed then
            vertex.color[2] = new_g
            changed = true
        end

        local b_label = vertex.name .. " B"
        local new_b, b_changed = slider_component(b_label, vertex.color[3], 0.0, 1.0)
        if b_changed then
            vertex.color[3] = new_b
            changed = true
        end

        imgui.spacing()
    end

    imgui.separator()
    if imgui.button("Reset Triangle") then
        reset_vertex_controls()
        changed = false
    end

    if changed and ui.resources then
        upload_vertex_stream(ui.resources, ui.vertex_controls)
    end
end

function ui.render(delta_time)
    ensure_resources()
    delta_time = delta_time or 0.0
    ui.last_delta_time = delta_time
    if not ui.window.open then
        return
    end
    ui.time_accumulator = ui.time_accumulator + delta_time
    local rendered = render_triangle()
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
            draw_vertex_controls()

            local display_image = ui.image_buffers[ui.front_buffer]
            if display_image then
                imgui.separator()
                imgui.text("Rendered image:")
                imgui.image(display_image, ui.size[1], ui.size[2])
            else
                imgui.text("Front buffer unavailable.")
            end

            imgui.text(string.format("FPS: %.2f", current_fps()))
        end
    end
    imgui.end_window()
end

function ui.onUpdate(delta_time)
    -- Reserved for additional per-frame logic supplied by Lua scripts.
end

ui.update = ui.onUpdate

return ui
