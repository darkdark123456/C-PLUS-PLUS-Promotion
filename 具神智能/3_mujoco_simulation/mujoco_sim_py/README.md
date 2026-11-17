# SO-ARM101 MuJoCo 仿真目录说明

本目录用于 **MuJoCo 机械臂仿真**教学与调试，含官方精确模型 + 教学简化版示例 + 运行脚本。

## 文件清单

| 文件 | 说明 |
| --- | --- |
| `scene.xml` | **官方场景**（包含 so101_new_calib.xml + 地面），推荐直接打开 |
| `so101_new_calib.xml` | 官方精确模型：**虚拟零点 = 各关节范围中点**（与我们校准的"0=中点"一致） |
| `so101_old_calib.xml` | 官方旧标定版（零点 = 手臂水平伸直位），一般不用 |
| `assets/` | 官方 STL 网格（13 个部件，外观精确） |
| `joints_properties.xml` | STS3215 舵机参数（阻尼/摩擦/刚度），被主 XML 引用 |
| `so101_simple.xml` | **教学 XML 示例**（基础几何体、无网格），仅用来讲 MJCF 结构，**代码不加载运行** |
| `run_sim.py` | **自动夹取演示**：夹红色滑块 → 放到旁边 → 夹回来（自动循环；滑块在代码里动态注入，**不改任何原 XML**） |
| `sim_teleop.py` | **操控仿真**：键盘 1~6/←→/0 或鼠标拖 Control 滑块控制虚拟臂；`--leader` 拖动主臂(COM3) → 仿真从臂跟随（详见下文） |
| `gimbal_lock_demo.py` | **教学案例：万向锁与欧拉角**（3 部分纯数学 + SO-ARM101 腕部万向锁实物演示；可导出 CSV 给学生画图） |

来源：TheRobotStudio/SO-ARM100 官方仓库（onshape-to-robot 从 CAD 生成）。

## 怎么运行

```bash
conda activate lerobot            :: 本机 lerobot 环境已装 mujoco 3.12.0
cd H:\czbk\embeded_ai\mujoco_sim

:: 【推荐】真实主臂录制 → 回放（只回放真实手臂动作，无红色滑块）
python sim_teleop.py --leader --record demo.npz   :: 拖动真实主臂走一遍“夹取→搬运→放下”，Ctrl+C 结束
python run_sim.py --replay demo.npz               :: 回放真实关节角；--start N 可跳过开头静止段

:: 无录制时
python run_sim.py                 :: 【脚本化演示】夹取红色滑块 → 放到 B → 夹回 A（循环；含红色滑块）
python sim_teleop.py              :: 【操控】默认静止展示，键盘/鼠标拖 Control 滑块控制
python sim_teleop.py --leader     :: 【遥操作】拖动真实主臂 → 仿真从臂跟随

:: 教学案例：万向锁与欧拉角
python gimbal_lock_demo.py                     :: 控制台讲义（4 部分全跑，无需 GUI）
python gimbal_lock_demo.py --view              :: 【动态】三环万向架动画（24s 一轮，推荐投屏）
python gimbal_lock_demo.py --view arm          :: 【动态】机械臂腕部万向锁动画
python gimbal_lock_demo.py --part 2            :: 只看第 N 部分（1~4）
python gimbal_lock_demo.py --part 3 --csv gimbal.csv  :: 导出数据给学生画图

（说明：运行脚本固定加载官方 `scene.xml`；`so101_simple.xml` 只作为教学示例讲解，不参与运行。）
```

打开后：**左键拖拽旋转视角**，滚轮缩放。ESC 退出。

### run_sim.py 说明（自动夹取演示，怎么做到的）

- **推荐：录制-回放（只展示真实手臂动作，无红色滑块）**。`sim_teleop.py --leader --record demo.npz` 把真实主臂动作录成归一化关节角（含夹爪张/合）；`run_sim.py --replay demo.npz` 按录制时的真实关节角回放手臂运动，`--start N` 可跳过开头静止段。**角度和流程来自真实操作，夹爪自然不会插进地面**；红色滑块只在脚本化兜底演示里出现；
- （脚本化兜底，无录制时）演示动作：下探到滑块上方 → 合爪 → 抬起 → 旋转 ~52° 到 B → 张开放下 → 夹回 A，循环；
- **真实姿态 + 触地即放**：脚本化姿态按"弯腰下探"调好（肘弯、腕压）；回放时另有"滑块底部若将穿地立即放好"的兜底，保证滑块不会被按进地里；
- 红色滑块是**代码里动态注入**的：读原 `scene.xml` / `so101_new_calib.xml` → 内存里拼一段 `<body red_slider><freejoint/>…` → `from_xml_string` 重新编译，**磁盘上的 XML 一字未动**；
- "抓/放"是**运动学吸附**，锚定在**夹爪基座刚体**上（合爪瞬间记录滑块相对夹爪基座的 4×4 变换，之后每帧跟随，滑块与夹爪保持刚性）；
- 说明：脚本化兜底因为要"抓地板上的方块"，受本模型"底座贴地、爪指离地高"的几何限制，真·接触夹取会先撞手腕/插地——所以不用脚本化兜底、改用**录制真实动作回放**最稳妥（想真接触抓取见 `sim_teleop.py` 的物理推进写法）。

### sim_teleop.py 说明（怎么"操控 + 看运动"）

- 原理：归一化目标（身体 -100~100 / 夹爪 0~100）→ 按仿真各关节限位换算成弧度 → 写位置执行器 `ctrl` → 物理推进（与真机同一套单位）；
- **键盘**：数字键 1~6 选关节，←/→ 调大小（每次 ±10），0 全部回中点；
- **鼠标**：直接拖查看器左侧 **“Control”** 面板滑块也能控制对应关节 —— 脚本只在键盘目标变化时才写 ctrl，平时不覆盖，滑块值会被采纳并驱动机械臂（拖 “Joint” 滑块无效，那是直接掰角度，会被位置控制拉回）；
- 注意：受位置执行器 `forcerange` 限力和重力影响，某些大角度静态姿态可能“顶不住”（尤其手臂伸直接近桌面时），属正常物理现象；
- 与 demo05 的关系：**demo05 = 主臂驱动真从臂；sim_teleop = 主臂驱动仿真从臂**（不需要真从臂就能演示/备课）；
- 方向不对？把脚本顶部 `FLIP` 表里对应关节改为 `-1`；
- 仿真固定加载官方 `scene.xml`（`so101_simple.xml` 仅作教学示例，不运行）。

### gimbal_lock_demo.py 说明（万向锁与欧拉角教学案例）

四层递进，纯 numpy + MuJoCo，**不需要 matplotlib/scipy**：

| 部分 | 内容 | 关键结论（实测数字） |
| --- | --- | --- |
| 第 1 部分 | 欧拉角 ↔ 旋转矩阵（ZYX 约定 R = Rz·Ry·Rx）、多组解 | 相同姿态可由相差 360° 的角表示 |
| 第 2 部分 | 万向锁的数学本质：角速度雅可比 J 降秩 | pitch 0°→90°：轴1·轴3 由 `0.000`→`1.000`，`|det(J)|` 由 1→0，rank 3→2；pitch=+90° 时保持 `yaw − roll` 不变则姿态完全相同（差 0.00e+00） |
| 第 3 部分 | 锁附近的病态：微小转动 → 欧拉角跳变 | pitch=0° 时转 0.2° → 欧拉角也变 0.2°；pitch=89.9° 时同样转 0.2° → **Δyaw/Δroll = −180°**；pitch=90° 时 Δyaw=Δroll=90° |
| 第 4 部分 | SO-ARM101 实物：`shoulder_pan + wrist_flex + wrist_roll` 腕部万向锁 | `wrist_flex=90°` 时 pan 轴 · roll 轴 = **−1.000**、`|det(三轴)|=0`；此时 pan=+t 与 roll=+t 同步转动，关节共转 140° 而**夹爪朝向变化 0.0000°**；对照 `wrist_flex=0°` 时同样动作夹爪变化 82.82° |

- `--view`（**推荐动态演示**）三环万向架：蓝环 yaw / 绿环 pitch / 红环 roll 三层同心套转，各带转轴与半透明平面。24 秒一轮的 5 个阶段：① 三轴独立 → ② pitch 升到 90°（红环平面与蓝环平面重合）→ ③ 锁定后分别转 yaw 和 roll（机头方向都不变，两者效果相同）→ ④ 退出锁定 → ⑤ 对照（非锁定时再转 yaw 机头又跟着转）。屏幕实时显示 **共线度（1.000=锁）**、机头方向与**机头转向速度**（锁定阶段为 0.0°/s）；
- `--view arm` 机械臂版：`wrist_flex=90°` 时 pan 与 roll 同步转，关节共转 140° 而夹爪朝向偏差恒为 0.0000°；
- `--csv` 导出第 3 部分数据（`base_pitch_deg, perturbation, true_angle_deg, dyaw, dpitch, droll`），学生可自己画柱状图；
- 教学落点：**姿态插值/下发指令用四元数或旋转矩阵，欧拉角只用于给人看**；锁定位形附近逆解会出现关节速度暴涨。

## MJCF 结构速讲（怎么写 XML）

一份机械臂 MJCF 就四件事：

```xml
<mujoco model="so101_simple">
  <compiler angle="degree" coordinate="local"/>   <!-- 单位：角度用度；坐标局部系 -->
  <option gravity="0 0 -9.81"/>                   <!-- 重力 -->

  <default>                                        <!-- 全局默认参数，可省 -->
    <joint type="hinge" damping="0.05" frictionloss="0.01"/>
    <geom type="capsule" density="1500" rgba="0.75 0.6 0.25 1"/>
  </default>

  <worldbody>                                      <!-- 世界：地面 + 机械臂 -->
    <geom name="floor" type="plane" .../>
    <body name="base" pos="0 0 0.1">               <!-- 刚体（无关节 = 焊死） -->
      <body name="pan_link" pos="0 0 0.1">         <!-- 子刚体 -->
        <joint name="shoulder_pan" type="hinge" axis="0 0 1" range="-170 170"/>
        <geom .../>                                <!-- 外观/碰撞体 -->
        <body name="upper_arm">                    <!-- 继续下一级关节 -->
          <joint name="shoulder_lift" axis="0 1 0" range="-110 110"/>
          ...
  <actuator>                                       <!-- 执行器：把控制量接上关节 -->
    <position name="shoulder_pan_act" joint="shoulder_pan" ctrlrange="-170 170"/>
  </actuator>
</mujoco>
```

**关键元素**：

| 元素 | 作用 |
| --- | --- |
| `<body>` | 一个刚体（连杆），嵌套形成运动链 |
| `<joint type="hinge">` | 旋转关节：`axis` 转轴、`range` 限位（度） |
| `<joint type="slide">` | 滑动关节（官方夹爪用线性关节，对应 0~100 开合） |
| `<geom>` | 外观/碰撞体（type：mesh 网格、capsule 胶囊、cylinder 圆柱、box 方盒…） |
| `<actuator><position>` | 位置控制执行器：`ctrl` 给目标角度，`kp` 刚度跟踪 |
| `<default>` | 默认参数（材质、摩擦、阻尼）统一管理 |

**关节与舵机 ID 对应**（从上到下）：gripper=6 → wrist_roll=5 → wrist_flex=4 → elbow_flex=3 → shoulder_lift=2 → shoulder_pan=1。

## 校准 ↔ 数值映射（重点）

- 官方 `so101_new_calib.xml`：**虚拟零点 = 范围中点**，和 LeRobot 校准"0=中点"一致；
- XML 里 `range` 直接是**弧度/度**（角度单位取决于 `compiler angle`，内部一律弧度）；
- 真实 LeRobot 用**归一化 -100~100**：`物理值 = 下限 + (归一化+100)/200 × (上限-下限)`；
- 想桥接：写个换算函数（归一化 ↔ 弧度），仿真/真机共用一套姿态定义。

## 改哪些参数（教学常改）

| 要改什么 | 改哪里 |
| --- | --- |
| 关节活动范围 | `<joint range="...">` |
| 连杆长度 | `<body pos>` / `geom fromto`（教学版）或直接改官方 XML 的 body pos |
| 运动快慢/顺滑 | `default joint damping`、`actuator kp` |
| 颜色/材质 | `geom rgba` |
| 夹爪行程 | 官方：夹爪关节 range（`so101_simple.xml` 示例里的两指 slide range 仅供讲解） |

## 教学建议

1. **先跑官方 scene.xml** 看真实外观 → **再对照 `so101_simple.xml` 讲 MJCF 结构**（`<body>/<joint>/<geom>/<actuator>` 各字段怎么对应，它是静态讲解示例，不参与运行）→ 想让学生改尺寸/限位，可直接改官方 `scene.xml` 或另存副本观察；
2. 结合真实机械臂：仿真里的限位、关节顺序、正方向符号都**需要和真机逐一核对**（本仓库官方模型方向已按 CAD 生成，但舵机装向可能与实物有差异）；
3. sim2real 前，先在真机用 `demo02` 验证各关节正方向，再把换算关系写进仿真控制器。
