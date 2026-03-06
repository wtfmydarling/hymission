# hymission

`hymission` 是一个面向 Hyprland 的 Mission Control 风格 overview 插件项目。目标不是做“临时切到另一个 layout”，而是在 compositor 侧把当前工作区窗口投影成一组可交互的预览卡片，尽量接近 macOS Mission Control 的视觉和操作方式。

当前仓库还处于原型阶段，但已经有两块可复用基础能力：

- 一个独立的窗口预览布局引擎
- 一个最小 Hyprland 插件入口，用来读取配置并调试当前布局结果

详细设计见：

- [`docs/spec.md`](docs/spec.md)
- [`docs/architecture.md`](docs/architecture.md)
- [`docs/research.md`](docs/research.md)
- [`docs/todo.md`](docs/todo.md)

## 当前状态

已经实现：

- `MissionControlLayout::compute(...)`，输入窗口自然几何，输出 overview 目标预览几何
- overview 状态机与最小 opening/closing 动画
- overview preview 渲染、backdrop、hover/selected 高亮
- preview 对屏幕外窗口、Wayland/Xwayland 缩放和 pinned 浮窗的基础支持
- scope-aware overview 收集：支持按当前 workspace、按配置默认范围或 `forceall` 跨 monitor / 跨 workspace 收集
- overview 打开后的同窗口集重建会尽量保持 preview slot 顺序稳定，避免 scrolling focus 波动把 preview 洗牌
- 鼠标命中测试、点击激活、方向键导航、`Esc` / `Return`
- dispatcher：`hymission:toggle`、`hymission:open`、`hymission:close`、`hymission:debug_current_layout`
- 一组 overview / 布局配置项
- 独立 demo 程序 `hymission-layout-demo`

尚未实现：

- 多 workspace overview 条带
- 拖拽、搜索、窗口分组

README 只描述仓库现状和最小使用方式；行为定义以 [`docs/spec.md`](docs/spec.md) 为准。

## 设计目标

- 预览保持窗口原始宽高比，不裁切内容
- 不改变客户端逻辑分辨率，不向客户端发送 resize 作为 overview 主路径
- 尽量保留窗口原有的上下、左右空间关系，而不是退化成死板网格
- 把布局算法和渲染路径拆开，便于单测和替换

## 当前公开接口

### Dispatcher

当前支持以下 dispatcher：

```conf
bind = SUPER, TAB, hymission:toggle
bind = SUPER SHIFT, TAB, hymission:open
bind = SUPER CTRL, TAB, hymission:close
bind = SUPER, C, hymission:toggle,onlycurrentworkspace
bind = SUPER, A, hymission:toggle,forceall
bind = SUPER, M, hymission:debug_current_layout
```

- `hymission:toggle`: 打开或关闭 overview；支持可选参数 `onlycurrentworkspace` 和 `forceall`
- `hymission:open`: 打开 overview；支持可选参数 `onlycurrentworkspace` 和 `forceall`
- `hymission:close`: 关闭 overview
- `hymission:debug_current_layout`: 按默认 scope 只计算当前 preview slots，并用通知显示摘要，不进入 overview

scope 参数语义：

- 无参数：走配置驱动的默认收集范围
- `onlycurrentworkspace`：只收集光标所在 monitor 的当前普通 workspace，不纳入 special workspace
- `forceall`：跨所有 monitor 收集所有普通 workspace，并额外纳入当前可见的 special workspace 窗口

`debug_current_layout` 会：

- 取光标所在显示器作为 anchor monitor
- 按默认配置收集 overview 参与窗口，以及参与 monitor 上可见的 pinned 浮窗
- 计算 Mission Control 风格 preview slots
- 用通知显示摘要结果

### 当前配置项

以下配置项已经在插件入口注册，但目前仅用于布局计算，不代表最终用户配置面已经冻结：

```conf
plugin {
    hymission {
        outer_padding_top = 48
        outer_padding_right = 48
        outer_padding_bottom = 48
        outer_padding_left = 48
        row_spacing = 32
        column_spacing = 32
        min_window_length = 120
        small_window_boost = 1.35
        max_preview_scale = 0.95
        min_slot_scale = 0.10
        layout_scale_weight = 1.0
        layout_space_weight = 0.10
        overview_focus_follows_mouse = 0
        only_active_workspace = 0
        only_active_monitor = 0
        show_special = 0
    }
}
```

语义摘要：

- `outer_padding_top`: overview 内容到显示器上边缘的内边距
- `outer_padding_right`: overview 内容到显示器右边缘的内边距
- `outer_padding_bottom`: overview 内容到显示器下边缘的内边距
- `outer_padding_left`: overview 内容到显示器左边缘的内边距
- `row_spacing`: preview 行间距
- `column_spacing`: preview 列间距
- `min_window_length`: 小窗口参与布局前的最小边长钳制
- `small_window_boost`: 对小窗口的放大权重
- `max_preview_scale`: 预览最大缩放上限
- `min_slot_scale`: 布局最小缩放下限
- `layout_scale_weight`: 行数候选评分中对缩放大小的权重
- `layout_space_weight`: 行数候选评分中对空间利用率的权重
- `overview_focus_follows_mouse`: 是否让 overview 内部当前选中项跟随鼠标 hover；开启后退出 overview 会提交到当前选中的 preview，但 overview 打开期间不会持续改真实窗口 focus。对 scrolling 工作区，退出动画会先等待真实布局收敛到目标 focus，再朝该最终位置收尾
- `only_active_workspace`: 默认 scope 下是否只纳入参与 monitor 的当前活动普通 workspace
- `only_active_monitor`: 默认 scope 下是否只纳入光标所在 monitor
- `show_special`: 默认 scope 下是否额外纳入参与 monitor 上当前可见的 special workspace 窗口

兼容性说明：

- 旧配置 `outer_padding` 仍然保留，作为四个方向 padding 的统一回退值
- 如果设置了 `outer_padding_top/right/bottom/left`，则对应方向会覆盖 `outer_padding`
- `onlycurrentworkspace` 和 `forceall` dispatcher 参数会覆盖默认 scope 配置

当前 runtime 还会临时接管两项现有 Hyprland 行为：

- overview 激活时会暂时关闭全局 `input:follow_mouse`，避免光标移动时 Hyprland 的真实窗口 focus 被动变化；overview 关闭后恢复原值
- scrolling 工作区下会暂时关闭 `scrolling:follow_focus`，避免 layout 自己跟着真实 focus 跳动；overview 关闭后恢复原值
- 如果退出 overview 时目标窗口已经在当前显示器上有可见区域，插件会把光标挪到该可见区域中心；如果目标窗口在 scrolling 下仍然不在屏内，则会临时保持该窗口为真实 focus，直到下一次真实鼠标事件

示例：开启 overview 内部选中项跟随鼠标，并在退出 overview 时提交到当前选中窗口

```conf
plugin {
    hymission {
        overview_focus_follows_mouse = 1
    }
}
```

## 布局引擎

布局引擎定义在：

- [`src/mission_layout.hpp`](src/mission_layout.hpp)
- [`src/mission_layout.cpp`](src/mission_layout.cpp)

输入：

- 一组窗口自然矩形 `WindowInput::natural`
- 一个 overview 可用区域 `Rect area`
- 一组布局参数 `LayoutConfig`

输出：

- 每个窗口对应一个 `WindowSlot`
- `WindowSlot::target` 是 overview 中的目标预览矩形
- `WindowSlot::scale` 是该窗口最终等比缩放值

当前算法不是固定栅格，而是 row-based layout：

- 先按窗口中心点的 `y` 值大致分行
- 再在每行按 `x` 值排序
- 评估不同 row count
- 选择总分最高的候选布局

具体来源和取舍见 [`docs/research.md`](docs/research.md)。

## 构建

### CMake

```sh
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j"$(nproc)"
```

产物：

- 插件：`build/libhymission.so`
- demo：`build/hymission-layout-demo`

### Meson

```sh
meson setup build-meson
meson compile -C build-meson
```

## 最小调试流程

1. 编译插件
2. 在 `hyprland.conf` 中加载 `libhymission.so`
3. 绑定 `hymission:toggle` 和 `hymission:debug_current_layout`
4. 先触发 `hymission:debug_current_layout`，确认通知里能看到 preview 数量和前几个目标矩形
5. 再触发 `hymission:toggle`，确认 overview 能正常打开、关闭和选窗

额外回归建议：

- 当前 monitor 上有 pinned 浮窗时，切换 workspace 后进入 overview，确认 pinned 浮窗仍在 overview 中
- 打开 overview 后用触控板 workspace swipe 切换工作区，确认手势能正常触发，且 workspace 变化后 overview 会退出

如果只想验证布局算法而不启动 Hyprland 插件，可以直接运行：

```sh
./build/hymission-layout-demo
```

## 近期路线

近期实现顺序已经在文档中定死：

1. 保持当前布局引擎接口稳定
2. 用 render hook 接 overview 渲染
3. 加入打开/关闭状态机
4. 加入命中测试和点击激活
5. 加入键盘导航
6. 最后再做动画、多 workspace 条带和高级交互

原因见 [`docs/architecture.md`](docs/architecture.md)。
