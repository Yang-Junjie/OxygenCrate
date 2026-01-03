# OxygenCrate Lua 脚本指南

本文说明引擎向 Lua 暴露的接口、回调生命周期以及一个完整的渲染示例，帮助你在编辑器内快速编写脚本并将内容显示到 ImGui 面板。

## 生命周期与必备结构

Lua 脚本必须返回一个表，表中可实现如下函数（全部可选）：

| 函数名（同义） | 调用时机 | 典型用途 |
| --- | --- | --- |
| `render(dt)` / `on_render` / `onRender` | `ExampleLayer::OnUpdate()` 开头 | 执行离屏渲染、更新 GPU 缓冲、驱动动画。`dt` 为本帧秒数。 |
| `draw()` | `ExampleLayer::OnRenderUI()` | 使用 `imgui` API 绘制 UI、显示渲染结果。 |
| `update(dt)` / `on_update` / `onUpdate` | `ExampleLayer::OnUpdate()` 中（`render` 之后） | 附加的游戏逻辑或状态更新；不做渲染也可。 |

只写 `draw()` 也能运行；当脚本需要动画/渲染时再实现 `render(dt)`。

## 全局函数与模块

以下函数直接存在于 Lua 全局命名空间：

| 函数 | 参数 | 行为 |
| --- | --- | --- |
| `log(...)` | 任意数量参数 | 把消息拼接后写入左侧 Lua Console。常用于调试。 |
| `create_image(width, height)` | `width:int ≥1`, `height:int ≥1` | 创建 `Flux::Image` 并返回整数句柄。失败时返回 `-1`。需要与 `flux_image` 或 `imgui.image` 搭配使用。 |
| `set_image_data(image_id, as_table{uint8})` | 图像句柄、包含 RGBA 字节的数组 | 将原始像素上传到 `image_id` 对应纹理。数组长度必须是 `width*height*4`。 |
| `load_module_file(path)` | 相对路径（禁止 `..`） | 读取 `lua/` 目录下的任意文件并以字符串形式返回。可用于加载额外 GLSL、JSON。 |

### 模块路径
`package.path` 预设为 `<运行目录>/lua/?.lua` 与 `/lua/?/init.lua`，因此你可以直接 `require("Vec2")` 或 `require("modules.VertexArray")`。常用模块：

- `Vec2`: 简单二维向量。
- `modules.VertexArray`, `modules.VertexBuffer`, `modules.IndexBuffer`, `modules.BufferLayout`: C++ OpenGL 封装。
- `modules.Shader`: 从 `lua/shaders/...` 目录读取 GLSL 文件并编译。

### OpenGL/Flux

- `opengl` 或 `opengles`（根据平台自动选择）包含低阶函数，如 `create_vertex_buffer`, `bind_buffer`, `draw_elements`, `clear_color` 等，命名与 C API 一致。
- `flux_image` 提供：
  - `flux_image.bind_framebuffer(image_id)`：将 `Flux::Image` 的 FBO 设为当前 `GL_FRAMEBUFFER`，成功返回 `true`。
  - `flux_image.unbind_framebuffer()`：恢复默认 FBO（0）。

### ImGui Lua API

| 函数 | 描述 |
| --- | --- |
| `imgui.begin_window(title, opts)` / `imgui.end_window()` | 包裹 ImGui 面板。`opts` 可包含 `open` 和 `flags`。 |
| `imgui.text(text)` / `imgui.text_wrapped(text)` | 输出文本。 |
| `imgui.button(label)`、`imgui.checkbox(label, value)`、`imgui.slider_float(label, current, min, max)` 等 | 与 C++ 参数一致，返回更新后的值。 |
| `imgui.image(image_id, width, height)` | 直接显示 `Flux::Image` 的颜色附件纹理。 |
| 其它辅助：`imgui.same_line()`, `imgui.spacing()`, `imgui.separator()` 等。

## 简单渲染示例

```lua
local Shader = require("modules.Shader")
local VertexArray = require("modules.VertexArray")
local VertexBuffer = require("modules.VertexBuffer")
local BufferLayout = require("modules.BufferLayout")
local gl = opengles or opengl
local flux = flux_image

local ui = {
    window = { open = true },
    size = { 256, 256 },
    buffers = nil,
    shader = nil,
    vao = nil,
    vbo = nil,
    time = 0,
}

local function ensure_resources()
    if ui.buffers then return end
    ui.buffers = {
        create_image(ui.size[1], ui.size[2]),
        create_image(ui.size[1], ui.size[2]),
    }
    local vertices = {
        -0.5, -0.5, 0.0, 1.0, 0.0, 0.0,
         0.5, -0.5, 0.0, 0.0, 1.0, 0.0,
         0.0,  0.5, 0.0, 0.0, 0.0, 1.0,
    }
    ui.vao = VertexArray.new()
    ui.vao:bind()
    ui.vbo = VertexBuffer.new(vertices)
    ui.layout = BufferLayout.new({
        { index = 0, size = 3, type = gl.FLOAT, normalized = false },
        { index = 1, size = 3, type = gl.FLOAT, normalized = false },
    })
    ui.layout:apply()
    ui.shader = Shader.from_files("shaders/opengl/simple.vert", "shaders/opengl/simple.frag")
    ui.front, ui.back = 1, 2
end

function ui.render(dt)
    ensure_resources()
    dt = dt or 0.0
    ui.time = ui.time + dt

    local target = ui.buffers[ui.back]
    if not flux.bind_framebuffer(target) then
        log("bind framebuffer failed")
        return
    end

    gl.viewport(0, 0, ui.size[1], ui.size[2])
    gl.clear_color(0.1, 0.1, 0.12, 1.0)
    gl.clear(gl.COLOR_BUFFER_BIT)

    ui.shader:use()
    ui.shader:set_float("u_Time", ui.time)
    ui.vao:bind()
    gl.draw_arrays(gl.TRIANGLES, 0, 3)

    flux.unbind_framebuffer()
    ui.front, ui.back = ui.back, ui.front
end

function ui.draw()
    ensure_resources()
    if imgui.begin_window("Lua Render") then
        imgui.text("Time: " .. string.format("%.2f", ui.time))
        local image = ui.buffers and ui.buffers[ui.front]
        if image then
            imgui.image(image, ui.size[1], ui.size[2])
        end
    end
    imgui.end_window()
end

return ui
```

### 关键点
- **双缓冲**：`render()` 交替写入两个 image，避免在渲染与 UI 显示时竞争同一纹理。
- **资源初始化**：把昂贵的 `create_image`、VAO/VBO 创建包在 `ensure_resources()` 中，只在第一次运行时执行。
- **参数传递**：`render(dt)` 的 `dt` 就是 `ExampleLayer::OnUpdate(float dt)` 传进来的值，适合驱动动画。
- **UI 展示**：`imgui.image()` 直接接受 `create_image()` 的句柄即可显示离屏结果。
- **日志与调试**：`log("time", ui.time)` 可随时输出到控制台，脚本出错时错误也会显示在那里。

## 调试与部署

1. **资源同步**：`LuaScriptHost::EnsureDefaultModulesInstalled()` 会把 `assets/lua/` 复制到运行目录。若你手动修改 `lua/` 下的文件，记得同步到实际运行位置（如 `DesktopApp/bin/lua/`）。
2. **控制台**：打开 Example Layer 的 “Lua Console” 可以看到 `log()` 输出、脚本报错堆栈。
3. **热重载**：在编辑器里点击 “Run Lua Script” 会重新编译当前脚本：清空所有 Lua 创建的 `Flux::Image`，并重新载入模块。
4. **常见问题**：
   - **帧缓冲取用失败**：确保 `create_image()` 的返回值被保存，不要在 `render()` 中反复创建。
   - **颜色闪烁**：每帧渲染前调用 `flux_image.bind_framebuffer(image_id)`，结束后调用 `flux_image.unbind_framebuffer()`，并在 `draw()` 中只显示前一帧的纹理。
   - **性能抖动**：尽量复用 Lua table（参考 `Sample.lua` 的 `build_vertex_stream`），避免频繁 `table.insert`/GC。

通过上述接口与示例，你可以快速编写自定义 UI、渲染效果，并与 C++ 层交互。建议先拷贝 `Sample.lua` 作为模板，再逐步替换为自己的逻辑。

将此文档放在 `assets/docs` 中，可在编辑器/README 引用，让 Lua 使用者快速了解可用接口与基本模式。
