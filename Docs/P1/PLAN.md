# P1 计划：一个角色 · 一把枪 · 一个不会还手的 Bot

> **终极目标**：多武器、多皮肤、**多人联机**的 FPS。
> **P1 目标**：把「开火 → 命中 → 扣血 → 死亡」这条链路跑通，**并且骨架按终态立**，后续加武器/皮肤/联机不推倒重来。
>
> 落地手册：[`GAS_STEP_BY_STEP.md`](./GAS_STEP_BY_STEP.md)（含完整可编译代码，5 章 / 3 个验收门）
> 早期非 GAS 版本：[`STEP_BY_STEP.md`](./STEP_BY_STEP.md)（仅存档，血量/伤害与 Bot 的部分已被取代）

---

## 1. 目标分层（先分清"现在做"和"将来做"）

| 能力 | 终极形态 | P1 做到 | 差在哪 |
| --- | --- | --- | --- |
| 角色 | 1 个 C++ 基类 + N 个皮肤 | 1 个角色（`CT` 蓝图），网格可换 | 皮肤系统（P3） |
| 武器 | 多把枪，数据驱动，可切换 | 1 把枪，数值/网格/挂点均为 BP 可覆盖属性 | 武器数据资产 + 武器槽（P3） |
| 敌人 | 多种 AI：巡逻/掩体/还击/组队 | 1 个**不动、不还手**的靶子 | AI（P2） |
| 血量 | GAS AttributeSet | ✅ GAS AttributeSet（已是终态形状） | — |
| 伤害 | GE + ExecCalc（爆头/衰减/护甲） | ✅ GE + `SetByCaller`，单一收口点 | ExecCalc（P2） |
| 联机 | 服务器权威 + 属性复制 + ASC 在 PlayerState | ❌ 不跑联机，但访问全走接口 | 附录 B.1 的迁移（P4） |

**为什么 P1 的 Bot 不还手**：现在只有一条链路要验证。Bot 一旦会动会打，出问题时你要同时怀疑射线、GE、AI 三件事，排错成本翻倍。**先能打死靶子，再让靶子还手。**

---

## 2. 现状盘点（已逐项核实）

### 2.1 工程

| 项 | 状态 |
| --- | --- |
| 引擎 | UE 5.8.4 |
| 模块 | 单模块 `IronCurtain`，C++ 约 767 行 |
| 已启用插件 | EnhancedInput、GameplayAbilities（已启用但**代码里 0 处使用**）、CommonUI（未使用）、ModelContextProtocol + EditorToolset（编辑器 MCP 在线 `127.0.0.1:8000`） |
| Build.cs 依赖 | 已含 `GameplayAbilities` / `GameplayTags` / `GameplayTasks` / UMG / CommonUI |

### 2.2 C++ 类

| 类 | 现状 | 缺什么 |
| --- | --- | --- |
| `APVPCharacter` | 1P 网格（`OnlyOwnerSee`）+ 1P 相机（FOV70 / Scale0.6，`bUsePawnControlRotation`）+ Enhanced Input（Jump/Move/Look） | 无 |
| `APVPShooterCharacter` | 空壳：只有 `RotationRate` 和恒返回 false 的 `IsDead()` | ASC / 血量 / 武器 / 开火 / 死亡 |
| `APVPGameMode` / `APVPShooterGameMode` | 分层就绪 | 击杀计分、PlayerStateClass |
| `APVPPlayerController` | 注册 IMC、锁定 GameOnly | HUD、自伤调试命令 |
| `APVPCameraManager` | 俯仰 ±75° | 后坐力镜头 |

### 2.3 内容资产

- **输入**：`IA_Jump` / `IA_Move` / `IA_Look` + 1 个 IMC。**没有射击输入。**
- **地图**
  - `L_Dev_Test`：灰盒场地（21 个 StaticMeshActor）、`PlayerStart`、3 个 `TargetPoint`、`NavMeshBoundsVolume` + `RecastNavMesh`、光照齐全、WorldSettings 覆盖 GameMode = `ShooterGameMode`。
  - `L_Trainyard`：训练场。
- **蓝图**
  - `Character/Shooter/CT`（父类 `APVPShooterCharacter`）：1P/3P 网格均 `SKM_Manny_Simple`，AnimClass 均 `ABP_Unarmed`，且 `IsDataOnly = true`。
  - `ShooterGameMode`：DefaultPawn = `CT`，PC = `PVPShooterPlayerController`（PIE 跑的就是它）。
  - `Core/PVPGameMode`：`DefaultPawnClass = None`、`PlayerControllerClass` 是引擎默认 → **其他地图进去没有角色**。
- **外部资产（195 个）**
  - 骨骼网格：`SKM_Manny_Simple`、`SKM_Quinn_Simple`（已绑 `PA_Mannequin` 物理资产）
  - 动画 128 个：Pistol/Rifle 全套（Fire/Reload/ADS/Idle/Jog/Walk/Jump）、Death ×6、HitReact ×8、Unarmed 全套
  - 武器：Pistol / Rifle / GrenadeLauncher 网格（**Pistol 网格自带 `Muzzle` socket**）
  - **角色骨骼自带 `weapon_r` / `weapon_r_muzzle` 两个 socket —— 不用自己建挂点**
  - 唯一现成 Montage：`MM_Pistol_Fire_Montage`；Death / HitReact 只有 Sequence
  - 特效/音效：`NS_JumpPad`、`M_SimpleGlow`、一发枪声

### 2.4 缺口清单（= P1 要做的事）

1. 没有射击输入（`IA_Fire` / `IA_Reload`）
2. 没有武器 Actor（枪没地方挂）
3. 没有血量 / 伤害系统 → 用 GAS 建
4. 没有 Bot（P1 只要一个"靶子"，不要 AI）
5. 没有 HUD（准星 / 血条 / 击杀数）
6. 缺音效、枪口火光、弹孔 —— 先用占位

---

## 3. 验收标准

### 3.1 P1（本阶段）

- [ ] 进 `L_Dev_Test` 手持手枪，HUD 显示 `HP 100` / `AMMO 30/30` / `KILLS 0`
- [ ] 左键连发：枪声 + 弹道曳光，弹着点跟准星一致，30 发打空自动停火，`R` 换弹
- [ ] 打中 Bot 掉血（日志 `受到 34.0 伤害 → 66.0 / 100.0`）
- [ ] 3 发打死 Bot：布娃娃倒地、留尸体、`KILLS +1`
- [ ] 死后按左键无法开枪（`ActivationBlockedTags` 生效）
- [ ] **Bot 全程不动、不开枪、玩家 HP 恒为 100（这是设计目标，不是 bug）**
- [ ] 全程无 `Accessed None`、无 `ensure` 崩溃

### 3.2 P2（下一阶段，不在本计划内）

巡逻 → 看见玩家 → 靠近 → 还击 → 玩家掉血 → 玩家死亡重开地图。

---

## 4. 架构

### 4.1 分层

```
输入层    IA_Fire ──► ASC::AbilityInputTagPressed(InputTag.Shoot)
                            │  （AI 将来也走这个入口，不写第二套弹道）
                            ▼
技能层    UPVPGA_Shoot  ── 射线(相机中心) ──► 命中 ──► 造 GE
                            │
武器层    APVPWeaponActor ── 网格/枪口 socket/弹药/射速/散布/伤害（纯数据+表现）
                            │
伤害层    GE_Damage ── SetByCaller(Damage) ──► 目标 ASC
                            │
属性层    UPVPHealthSet::PostGameplayEffectExecute   ← 唯一收口点
                            │  Damage → -Health，夹到 [0, MaxHealth]
                            ▼
状态层    UPVPHealthComponent（属性 → 委托 → 死亡三态）
                            │
表现层    HUD / 布娃娃 / 击杀计数
```

### 4.2 三个关键决策

| 决策 | 选择 | 理由 |
| --- | --- | --- |
| 射线起点 | **相机中心**，枪口只做视觉起点 | 用枪口发射，近距离弹着点会明显偏离准星，第一人称手感直接崩 |
| 伤害入口 | **GE + `SetByCaller` 修饰 `Damage` 元属性** | 所有伤害都经过同一个收口点，将来加护甲/爆头/衰减只改一处，不动技能 |
| Bot 与玩家共用一条链路 | Bot 也挂 ASC + `UPVPGA_Shoot`，通过**输入标签**开火 | 不写第二套弹道；P2 加 AI 时只是"谁来按这个虚拟按钮"的问题 |

> 完整的"取舍账本"（哪处简化、将来补回多大成本）在手册 **0.1 节**，共 9 条，开工前值得看一遍。

---

## 5. 里程碑（P1）

| 里程碑 | 内容 | 验收门 |
| --- | --- | --- |
| **M0 基线清障** | 修 `Core/PVPGameMode` 的两个类引用；建 `IA_Fire` / `IA_Reload` + IMC 绑定 | PIE 有角色、按键有反应 |
| **M1 GAS 骨架** | Tag / AttributeSet / HealthSet / ASC / HealthComponent + 玩家接入 | **Gate A**：`showdebug abilitysystem` 看到 `Health = 100` |
| **M2 伤害链路** | `GE_Damage` + `PVPDamageSelf` 控制台命令 | **Gate B**：自伤 30 → 70；打到 0 → 死亡事件 + `Status.Death.Dying` |
| **M3 武器 + 技能 + 靶子** | `APVPWeaponActor` / `UPVPGA_Shoot` / Bot（无 AI）/ HUD | **Gate C**：三枪打死靶子，KILLS+1 ← **P1 主目标** |

> M1 / M2 是刻意"不碰武器和 Bot"的两步。跳过它们直接上 M3，出问题时你无法区分是 GAS 配错还是武器写错。

---

## 6. 后续路线

| 阶段 | 内容 | 关键改动 |
| --- | --- | --- |
| **P2** | Bot AI（巡逻/追击/还击）、GameplayCue（枪口火光/命中火花）、`ExecCalc_Damage` | 加 `AIModule`/`NavigationSystem`；ASC 输入标签开火；Cue 路径要在项目设置里配 |
| **P3** | **多武器**（`UPVPWeaponDataAsset` + 武器槽 + 切枪）、**多皮肤**（`FPVPSkinDefinition`，只复制 SkinId） | 武器从"代码默认值"变成"数据资产"；皮肤不参与数值 |
| **P4** | **多人联机**：ASC 挪 `PlayerState`、服务器权威伤害、属性复制、弹药进 AttributeSet | 手册附录 B.1 有完整迁移清单 |

---

## 7. 风险清单（都是这个工程里真实存在的）

1. **射线起点用相机**（不是枪口）—— 第一人称最容易翻车的地方。
2. **1P/3P 两套网格各自挂 `ABP_Unarmed`** —— 同帧算两遍动画，姿态可能不同步；1P 建议改 LeaderPose。
3. **没有持枪姿态**：`ABP_Unarmed` 是空手动画，持枪会穿模。正经做法是复制出 `ABP_Pistol`（`MF_Pistol_*` 全套 locomotion 都有）。
4. **Bot 没有 Controller 时 ASC 不会初始化** —— 如果 `BeginPlay` 里写成 `if (GetController())`，靶子永远打不死。手册 Step 3.4 已改成无条件初始化。
5. **死亡只有 Sequence**，`MM_Death_*` 要能播必须先做成 Montage 并给 AnimBP 加 Slot —— P1 用布娃娃绕开，不阻塞。
6. **`CT` 蓝图是 `IsDataOnly = true`** —— 别再往里塞逻辑，要加组件只能改 C++ 或转成普通 BP。
7. **新增 C++ 文件后必须重新 Generate Visual Studio project files**，否则 `generated.h` 报错。
8. **`GE_Damage` 的 Attribute 下拉框要有 `Damage` 选项** —— 必须先编译 `UPVPHealthSet` 并在编辑器加载过一次。
9. **死亡事件会重复广播** —— `HealthSet` 里要有 `bOutOfHealth` 去重。
10. **`GameplayCueNotifyPaths` 默认是空的**（5.5+ 移到项目设置），不配 Cue 永远不播 —— P2 才需要。
11. **资源缺口**：枪声只有一发、没有枪口 NS / 弹孔 / 准星贴图 —— 先用 `DrawDebugLine` + `M_SimpleGlow` 占位。

---

## 8. 排期（P1）

| 里程碑 | 工作量 | 结束状态 |
| --- | --- | --- |
| M0 基线清障 | 0.5 天 | 能跑 |
| M1 GAS 骨架 | 1 天 | 能看到血量 |
| M2 伤害链路 | 0.5 天 | 能自伤扣血 |
| M3 武器 + 技能 + 靶子 | 1.5 天 | **P1 主目标达成** |
| 合计 | **约 3.5 个工作日** | |

---

## 9. 与手册的关系

- **这份文档**：为什么这么做（目标分层、架构、取舍、路线、风险）。
- **[`GAS_STEP_BY_STEP.md`](./GAS_STEP_BY_STEP.md)**：具体怎么做（完整文件、编辑器操作、Gate 验收、排错表）。
- 两份文档的口径以 **GAS 版为准**；`STEP_BY_STEP.md` 里的自定义 `UHealthComponent` 与 Bot 还击部分已作废，仅作历史对照。
