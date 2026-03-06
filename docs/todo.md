# Hymission TODO

这份 TODO 只按 `../hycov/README.md` 对照整理，不额外参考 `hycov` 源码实现细节。

## 相对 hycov 的缺口

| 功能 | hymission 当前状态 | 处理结论 |
| --- | --- | --- |
| overview 中右键关闭窗口 | 未实现 | `TODO` |
| 触控板手势进入 overview / 在 overview 中手势切换目标 | 未实现 | `TODO` |
| Alt-release / Alt-Tab 风格的“按住主键进入，松开主键退出”模式 | 未实现 | `TODO` |
| 独立的 `movefocus` dispatcher（方向切换 / 循环切换） | 未实现 | `TODO` |
| 跨显示器的 `movefocus` | 未实现 | `TODO`，依赖独立 `movefocus` 语义先落地 |
| `forceall` / `onlycurrentworkspace` 这类 overview 范围切换 | 已实现；`forceallinone` 未实现 | `TODO`，仅剩 `forceallinone` |
| special workspace 窗口纳入 overview | 默认配置 `show_special` 和 `forceall` 已支持当前可见 special workspace | `Done` |
| 多显示器同时 overview | 已实现 | `Done` |
| 退出 overview 后自动 fullscreen / maximize | 未实现 | `Not Planned`，与 hymission “不重排真实窗口状态”的主设计不一致 |
| 退出 overview 后把 floating 窗口抬到最上层 | 未实现 | `Not Planned`，当前不想把 overview 变成真实窗口状态修复器 |

## 建议实现顺序

1. overview 右键关闭窗口
2. 触控板 overview 手势
3. Alt-release 退出模式
4. 独立 `movefocus` dispatcher
5. 视需要再讨论 `forceallinone`
