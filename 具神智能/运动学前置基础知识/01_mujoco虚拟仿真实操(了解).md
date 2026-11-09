## 学习目标
1 了解 MuJoCo 仿真的意义和作用

2 了解 MuJoCo 仿真文件参数含义

3  了解 MuJoCo 仿真抓取滑块代码含义

4  了解 MuJoCo 仿真真实主臂控制虚拟从臂代码含义



## MuJoCo 仿真的意义


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788773492250-4a26f159-a5f3-48b4-a007-9dd25a5c4329.jpeg)



MuJoCo 是高性能多体物理仿真引擎，核心价值：**在虚拟世界复现机器人物理行为，在不碰真实硬件的前提下完成算法开发、验证、调参，降低实物调试成本与风险**。

### 安全，避免硬件损坏
SO‑ARM100 用的是 STS3215 舵机，实物调试时：

+ 算法逻辑 bug、运动规划出错，会让机械臂发生剧烈抖动、撞机，直接烧舵机、打坏机械结构。
+ 在 MuJoCo 仿真中反复跑代码、测试极端运动、碰撞抓取，**不会损耗任何硬件**，把危险测试全部放在虚拟环境完成，验证稳定后再部署真机。

### 低成本快速迭代，缩短开发周期
真实机械臂调试存在大量现实开销：

+ 真机上电、接线、复位、校准、零件磨损、现场环境干扰；每次改控制代码都要反复上机测试。
+ MuJoCo 秒级重启仿真环境，可以批量跑大量实验：运动规划、闭环控制、抓取策略、强化学习训练。

尤其强化学习：真机训练需要成千上万次尝试，实物根本跑不起，只能依赖仿真。

### 可复现、可控的实验环境
现实世界有不可控干扰：摩擦力波动、舵机间隙、灯光、摆放位置变化，实验结果很难复现。

+ MuJoCo 可以精准设定：质量、惯性、摩擦系数、舵机伺服参数、物体重量；环境完全可控。
+ 同样代码每次运行条件一致，方便对比不同算法优劣，便于做实验记录与论文报告。

### 完整物理效应模拟，贴近真实机器人
对比仅做运动学计算的 URDF/MoveIt：URDF 只算关节位置，**不模拟力、碰撞、接触、打滑**。 MuJoCo MJCF 原生支持：

+ 关节阻尼、舵机伺服控制、接触碰撞、摩擦、物体抓取掉落；
+ 可以复现舵机滞后、夹爪打滑、碰撞冲击力这些真实机器人会遇到的现象。

这样仿真调出来的控制器，迁移到 SO‑ARM100 真机上，迁移差距（sim‑to‑real）更小。

###  算法原型验证：控制、规划、强化学习
1. **运动控制**：PID、位置闭环、舵机参数调优，先仿真调参，再把参数搬到真机；
2. **抓取操作**：测试不同夹爪开合策略、物体抓取逻辑；
3. **强化学习 / LeRobot**：现在开源机器人项目大量基于 MuJoCo 做机器人策略训练，在虚拟环境训练智能策略，再尝试迁移到实体机械臂。

### 可视化与调试
可以直观看到机械臂运动、碰撞接触点、关节力矩、传感器数据；方便定位代码 bug，不用反复拆解硬件排查问题。





## Mujoco 仿真项目实战
### 项目背景
<font style="color:rgb(0, 0, 0);">SO-ARM101 真机链路（遥操作 → 采集 → 云训练 → 推理）已经跑通，但实际开发和调试中有三个</font>**<font style="color:rgb(0, 0, 0);">真机无法解决</font>**<font style="color:rgb(0, 0, 0);">的问题：</font>

| **<font style="color:rgb(0, 0, 0);">真机痛点</font>** | **<font style="color:rgb(0, 0, 0);">仿真的价值</font>** |
| :--- | :--- |
| **<font style="color:rgb(0, 0, 0);">硬件是稀缺资源</font>**<font style="color:rgb(0, 0, 0);">：主从双臂只有一套，学生轮流摸</font> | <font style="color:rgb(0, 0, 0);">仿真可以</font>**<font style="color:rgb(0, 0, 0);">人手一份</font>**<font style="color:rgb(0, 0, 0);">，随时重来，不占真机</font> |
| **<font style="color:rgb(0, 0, 0);">危险与成本</font>**<font style="color:rgb(0, 0, 0);">：夹爪堵转、碰撞、掉线都可能损伤舵机</font> | <font style="color:rgb(0, 0, 0);">仿真里随便撞，</font>**<font style="color:rgb(0, 0, 0);">零硬件损耗</font>**<font style="color:rgb(0, 0, 0);">；先仿真验证再上真机</font> |
| **<font style="color:rgb(0, 0, 0);">不可复现</font>**<font style="color:rgb(0, 0, 0);">：真机每次示教都有差异，无法精确回放</font> | <font style="color:rgb(0, 0, 0);">仿真可</font>**<font style="color:rgb(0, 0, 0);">逐帧回放</font>**<font style="color:rgb(0, 0, 0);">录制轨迹、固定随机性，便于教学演示与回归测试</font> |
| **<font style="color:rgb(0, 0, 0);">看不见的量</font>**<font style="color:rgb(0, 0, 0);">：关节力矩、离地高度、接触状态无法直接观测</font> | <font style="color:rgb(0, 0, 0);">仿真里可</font>**<font style="color:rgb(0, 0, 0);">直接量测</font>**<font style="color:rgb(0, 0, 0);">（本项目的"爪离地余量""滑块最低角"都靠它）</font> |


### <font style="color:rgb(0, 0, 0);">本仿真子系统</font>
1. **<font style="color:black;">演示"夹取—搬运—放置"而不依赖真机</font>**<font style="color:black;">：课堂上要反复展示同一套动作，真机既慢又费舵机；仿真可以高频循环播放。</font>
2. **<font style="color:black;">演示主臂操作，虚拟从臂模仿</font>**
3. **<font style="color:black;">让真实示教动作可复用</font>**<font style="color:black;">：用真实主臂录一段动作，就能在仿真里</font>**<font style="color:black;">回放同一套关节角</font>**<font style="color:black;">做教学与对照——既真实又不碰硬件。</font>

### 项目架构和组成
**架构图**


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789558340915-bd1b50c9-73e0-418d-9c29-e06be6579460.png)



**框架组成**


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789557604438-f28539ef-7a1d-4a71-97f7-5d8e0fe214dd.png?x-oss-process=image%2Fformat%2Cwebp)

### 仿真效果演示
打开资源包，切换到 lerobot 环境， 执行 run_sim 这个 py 文件

```bash
(lerobot) H:\czbk\embeded_ai\mujoco_sim>conda activate lerobot
(lerobot) H:\czbk\embeded_ai\mujoco_sim>python ./run_sim.py
```

可以看到效果

[此处为语雀卡片，点击链接查看](about:blank#wLv6U)



那么这样一个仿真效果如何实现的呢？


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788839338010-dac99b06-1b9a-49d2-acb2-38275c12421c.jpeg)

整体流程


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788839358270-809cb216-43c3-46f5-ac25-1a4567eac8aa.jpeg)

所以要掌握仿真的整体流程，需要先从 XML 仿真文件入手



### mujoco 操作仿真臂
可以在 mujoco 中通过键盘和滑块控制仿真臂

[此处为语雀卡片，点击链接查看](about:blank#cWgMr)

### 主臂操作仿真模拟
可以通过操作主臂让仿真中的从臂模拟

[此处为语雀卡片，点击链接查看](about:blank#ZViNG)

### 录制仿真文件
我们可以通过操作主臂，使用 sim_teleop .py 录制仿真信息，将来可以通过 run_sim 回放

效果如下

[此处为语雀卡片，点击链接查看](about:blank#e5m0V)

## 仿真文件解读
要进行 mujoco， 需要拿到 mujoco 能读懂的仿真文件。

因为我们用的是 so101 开源机械臂，所以需要去官方源码下载仿真文件

源码地址:  

[https://github.com/TheRobotStudio/SO-ARM100/tree/main](https://github.com/TheRobotStudio/SO-ARM100/tree/main)



<!-- 这是一张图片，ocr 内容为： -->
![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788774799806-85996396-9a52-4d4a-8c04-df87123ddbcc.png)



### **以下是两种模型的区别**


| 项目 | 3D 打印 STL（仓库根目录 STL 文件夹） | 仿真专用网格 assets/*.stl |
| --- | --- | --- |
| 用途 | 给切片软件，输出实物零件，必须完整细节 | 给 MuJoCo/URDF，用于虚拟渲染、碰撞计算，**不用于打印** |
| 细节 | 保留螺纹、小孔、卡扣、倒角、安装槽，全部工程细节齐全 | **删掉螺纹、细小孔、微小卡扣等细碎特征**，做面片减面（简化三角面数量） |
| 面片数量 | 三角面片多，文件大；细节越多面数越高 | 面数大幅降低，文件更小，仿真跑的更快 |
| 几何要求 | 必须水密闭合，满足打印成型要求 | 水密即可，不在乎打印工艺；**不需要打印可行性** |
| 能不能互换 | ❌打印 STL 直接丢进 MuJoCo：面数爆炸，仿真卡顿、速度暴跌 | ❌仿真网格直接打印：缺螺纹、缺小孔，零件装不上，打印失败 |


### 仿真网格要干两件事（URDF/MJCF 里）
1. **视觉网格 visual**：仿真界面看到机械臂长什么样子；
2. **碰撞网格 collision**：物理引擎计算碰撞、接触、抓取。

MuJoCo 会读取这份 stl 做碰撞检测；如果面数太高，每一步物理求解都会很慢，强化学习训练直接卡死MuJoCo。

### 为什么 SO‑ARM100 要维护两套 STL
1. 打印模型细节拉满，适合做实物；但直接扔进 MuJoCo，大量螺纹小孔带来几十万三角面片，仿真性能崩盘。
2. 所以单独导出一套 assets 仿真网格：**保留零件大体外形，剔除工程制造细节，降低三角面数量**，兼顾外观和仿真速度。

⚠️注意：两套网格外形大体一致，尺寸完全对齐，只是内部细小制造特征被删除。

### 常见踩坑
1. 拿打印 STL 直接当仿真网格：MuJoCo 加载极慢，强化学习训练耗时暴涨，甚至报错面数超限。
2. 拿 assets 里仿真网格拿去 3D 打印：打印出来零件没有螺丝孔、螺纹，装配失败。



## URDF vs MuJoCo MJCF 核心区别
背景：SO‑ARM100 项目里两套都提供：`.urdf` 是机器人描述格式；`.xml`(MJCF) 是 MuJoCo 原生格式，都可以描述机械臂，但设计目标、能力、生态完全不一样。



可以在项目中看到有两种仿真配置文件


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788775529013-7ba9d711-a1fe-4340-b61e-1ea015bf9d1c.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788775681196-f38494a7-cdd8-4c19-9a21-2d8c129010de.png)



### 简单一句话概括
+ **URDF**：ROS 生态的机器人描述格式，侧重**运动学**，描述连杆、关节，对动力学支持弱。
+ **MJCF(MuJoCo XML)**：MuJoCo 仿真器**原生格式**，运动学 + 动力学 + 场景 + 材质 + 传感器全部一体，专为物理仿真设计。




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788927407260-fcc1e976-3ec1-4f5f-8342-8b626ffcf2d0.png)

---

### 详细对比
| 对比项 | URDF | MJCF(MuJoCo XML) |
| --- | --- | --- |
| 出身 | ROS(Robot Operating System) | DeepMind MuJoCo 物理引擎 |
| 文件后缀 | `.urdf` / `.xacro` | `.xml` |
| 擅长 | 机器人运动学描述：连杆、关节、坐标系、视觉 / 碰撞网格 | 完整物理仿真：动力学、接触、摩擦、执行器、传感器、地面、光照、场景物体 |
| 动力学能力 | **很弱**。只能填质量、惯性矩阵；没有接触参数、摩擦、阻尼、求解器配置。做动力学仿真很多参数要外部额外补。 | **原生强动力学**：支持摩擦、恢复系数、接触约束、弹簧阻尼、执行器模型、求解器调参。 |
| 关节类型 | 旋转、平移、固定；缺少很多仿真专用关节 | 除基础关节，还有滑块、球铰、自由关节等仿真专用关节 |
| 场景能力 | 只能描述机器人本体，**不能写环境、地面、桌子、光照**。环境要另外写。 | 机器人 + 地面 + 障碍物 + 灯光 + 相机全部写在同一个 MJCF 文件里 |
| 执行器 | 只有简单传动；舵机 / 电机模型简陋 | 丰富执行器：位置伺服、速度伺服、扭矩驱动，适合舵机机械臂仿真 |
| 材质 / 纹理 | 基础，功能有限 | 完整材质、贴图，直接引用 assets 下 stl + 纹理 |
| 生态 | ROS、MoveIt、Pinocchio、RViz；机器人运动规划首选 | MuJoCo 仿真、LeRobot、强化学习机器人仿真首选 |
| 项目中 SO‑ARM100 使用 | 用于 MoveIt 运动规划、RViz 可视化、运动学计算 | 用于 MuJoCo 物理仿真，做控制算法、强化学习仿真环境 |


xacro：URDF 的宏语言，用来简化重复 URDF 编写，本质输出还是 URDF。



### 文件之间的关系（针对 SO‑ARM100）
1. **URDF**`so101_new_calib.urdf` 引用 `assets/*.stl`
+ 做运动学、MoveIt 规划、RViz 看机器人
+ 如果你直接丢进 MuJoCo，MuJoCo 可以导入 URDF，但**会丢失大量动力学参数**，摩擦、接触行为不准。



2. **MJCF scene.xml**`scene.xml` 同样引用 `assets/*.stl`
+ 完整物理仿真，舵机伺服、碰撞、抓取都能模拟
+ 自带地面、光照，直接 `mjpython scene.xml` 跑起来
+ ROS/MoveIt 不能直接读取 MJCF，需要转成 URDF（会丢失动力学信息）



重点坑： 很多人图省事，MuJoCo 直接 load URDF，仿真出来动力学行为很怪，就是因为 URDF 没有定义接触、摩擦、求解器参数。官方 SO‑ARM100 提供 MJCF 就是为了解决这个问题。

---

###  什么时候选哪个
**选 URDF / XACRO**

+ 你要上 ROS / MoveIt 做运动规划
+ 只做**运动学计算**，不关心精细物理碰撞动力学
+ 在 RViz 可视化机器人本体
+ 

**选 MJCF**

+ 使用 MuJoCo 做物理仿真，强化学习、控制器仿真验证（比如 LeRobot 训练机械臂）
+ 需要模拟真实的舵机扭矩、抓取接触、物体掉落
+ 需要同时定义机器人 + 仿真环境

### 互相转换
1. URDF → MJCF：MuJoCo 内置工具可以导入，**动力学参数会丢失，需要手动补摩擦、接触等**，不推荐作为正式方案。
2. MJCF → URDF：可以导出，但**大量仿真专属信息直接丢掉**，只剩下连杆关节。

SO‑ARM100 项目做法：两份独立维护，共享同一套`assets` STL 模型文件，保证外观一致，但动力学参数各自写一套。



## MJCF 文件解析
我们准备了一份简易版本的 MJCF 文件，用来学习各个节点的含义

``` xml
<!--  ============================================================
so101_simple.xml —— SO-ARM101 教学简化版（无网格，基础几何体）
用途：理解 MuJoCo (MJCF) 的关节链写法；官方精确外观见
so101_new_calib.xml（带 STL 网格）。
结构（从上到下 6 自由度，与真实舵机 ID 对应）：
基座 → shoulder_pan(1) → shoulder_lift(2) → elbow_flex(3)
→ wrist_flex(4) → wrist_roll(5) → 夹爪(6)
单位：长度=米，角度=度（compiler angle="degree"）
说明：尺寸为示意值，请按实际 CAD 测量修改（官方模型里有真实值）。
============================================================  -->
<mujoco model="so101_simple">
  <!-- 编译器配置：
  angle="degree"：关节range、ctrl输入使用角度(°)，而非弧度
  coordinate="local"：body的pos、关节axis使用父物体局部坐标系，机械臂模型标准写法
  -->
  <compiler angle="degree" coordinate="local"/>

  <!-- 全局仿真选项：重力矢量 X Y Z，单位 m/s²
  Z轴向上；gravity="0 0 -9.81" 代表重力竖直向下，模拟地球重力
  连杆会产生自重，执行器需要输出力矩对抗重力抬起手臂
  -->
  <option gravity="0 0 -9.81"/>

  <!-- default：全局默认参数模板
  子标签如果没有显式写参数，则自动继承这里的值；显式写参数会覆盖默认值
  -->
  <default>
    <!-- hinge旋转关节默认参数
    damping：关节阻尼，抑制抖动
    frictionloss：关节静摩擦损耗
    armature：电机转子惯量，防止高kp下仿真数值震荡爆炸，非常关键
    -->
    <joint type="hinge" damping="0.1" frictionloss="0.02" armature="0.002"/>
    <!-- 几何体默认参数
    density：密度 kg/m³，MuJoCo根据几何体体积自动计算刚体质量
    rgba：R G B A，A=1表示不透明
    friction：接触摩擦系数
    -->
    <geom type="capsule" density="1500" rgba="0.75 0.6 0.25 1" friction="0.9"/>
    <!-- position位置执行器默认
    kp：位置刚度，越大跟踪目标越快
    forcerange：关节输出力矩上下限(N·m)，限制最大出力，避免NaN数值爆炸
    -->
    <position kp="50" forcerange="-2 2"/>
  </default>

  <!-- worldbody：世界物体容器，所有机器人、地面、灯光全部写在这里 -->
  <worldbody>
    <!-- 定向平行光源，pos光源位置，dir照射方向 -->
    <light pos="0.5 1.5 2.5" dir="0 0 -1" directional="true"/>

    <!-- 地面平面几何体 -->
    <geom name="floor" type="plane" size="2 2 0.1" rgba="0.45 0.45 0.5 1"/>

    <!-- ========== 基座（固定刚体，无joint标签 = 焊死在世界，不会运动）========== -->
    <body name="base" pos="0 0 0.10">
      <geom name="base_pedestal" type="cylinder" size="0.035 0.10" pos="0 0 0" rgba="0.3 0.3 0.35 1"/>

      <!-- 关节1 shoulder_pan：绕局部Z轴旋转，肩部水平旋转（舵机1） -->
      <body name="pan_link" pos="0 0 0.10">
        <joint name="shoulder_pan" axis="0 0 1" range="-170 170"/>
        <geom name="pan_servo" type="cylinder" size="0.024 0.02" pos="0 0 0" rgba="0.4 0.4 0.45 1"/>

        <!-- 关节2 shoulder_lift：绕局部Y轴俯仰，上臂抬起放下（舵机2） -->
        <body name="upper_arm" pos="0 0 0.0">
          <joint name="shoulder_lift" axis="0 1 0" range="-110 110"/>
          <geom name="upper_arm_link" type="capsule" fromto="0 0 0 0 0 0.16" size="0.022"/>

          <!-- 关节3 elbow_flex：肘部俯仰（舵机3），角度限位匹配真实硬件 -->
          <body name="forearm" pos="0 0 0.16">
            <joint name="elbow_flex" axis="0 1 0" range="-150 10"/>
            <geom name="forearm_link" type="capsule" fromto="0 0 0 0 0 0.15" size="0.019"/>

                        <!-- 关节4 wrist_flex：腕部俯仰（舵机4） -->
                        <body name="wrist1" pos="0 0 0.15">
                            <joint name="wrist_flex" axis="0 1 0" range="-120 120"/>
                            <geom name="wrist1_link" type="cylinder" size="0.016 0.012" pos="0 0 0" rgba="0.5 0.5 0.55 1"/>

                            <!-- 关节5 wrist_roll：绕局部Z轴旋转，腕部自转（舵机5） -->
                            <body name="wrist2" pos="0 0 0.0">
                                <joint name="wrist_roll" axis="0 0 1" range="-180 180"/>
                                <geom name="wrist2_link" type="cylinder" size="0.014 0.012" pos="0 0 0" rgba="0.5 0.5 0.55 1"/>

                                <!-- ========== 夹爪（示意：两指滑动关节，不是旋转关节）========== -->
                                <body name="gripper_base" pos="0 0 0.03">
                                    <geom name="gripper_base_link" type="box" size="0.018 0.010 0.012" rgba="0.55 0.55 0.6 1"/>

                                    <!-- 右指：slide滑动关节，沿局部X轴平移
                                        range 0~0.02m：0=闭合，0.02=张开
                                    -->
                                    <body name="right_finger" pos="0.018 0 0.004">
                                        <joint name="right_finger" type="slide" axis="1 0 0" range="0 0.02"/>
                                        <geom name="right_finger_g" type="box" size="0.006 0.010 0.012"/>
                                    </body>

                                    <!-- 左指：slide滑动关节，沿局部X轴平移
                                        range -0.02~0m：0=闭合，‑0.02=张开
                                        两指ctrl都为0时夹爪闭合
                                    -->
                                    <body name="left_finger" pos="-0.018 0 0.004">
                                        <joint name="left_finger" type="slide" axis="1 0 0" range="-0.02 0"/>
                                        <geom name="left_finger_g" type="box" size="0.006 0.010 0.012"/>
                                    </body>
                                </body>
                            </body>
                        </body>
                    </body>
                </body>
            </body>
        </body>
    </worldbody>

    <!-- actuator执行器模块：把控制信号绑定到关节
        position：位置伺服控制器
        joint：绑定对应关节名
        ctrlrange：ctrl数组输入信号允许范围
            旋转关节单位：度；滑动夹爪关节单位：米
        kp：夹爪单独设置刚度，覆盖default默认kp=50
        程序中 data.ctrl 数组就是给这里每个actuator赋值，实现控制
    -->
    <actuator>
        <position name="shoulder_pan_act" joint="shoulder_pan" ctrlrange="-170 170"/>
        <position name="shoulder_lift_act" joint="shoulder_lift" ctrlrange="-110 110"/>
        <position name="elbow_flex_act" joint="elbow_flex" ctrlrange="-150 10"/>
        <position name="wrist_flex_act" joint="wrist_flex" ctrlrange="-120 120"/>
        <position name="wrist_roll_act" joint="wrist_roll" ctrlrange="-180 180"/>
        <position name="right_finger_act" joint="right_finger" ctrlrange="0 0.02" kp="100"/>
        <position name="left_finger_act" joint="left_finger" ctrlrange="-0.02 0" kp="100"/>
    </actuator>

</mujoco>

```



### 如何定义重力系统
> `**<option gravity="0 0 -9.81"/>**`** **
>


这是 MuJoCo 的**重力矢量**，格式：`gravity="X Y Z"`，单位：**m/s²**。



坐标系约定（MuJoCo 默认右手坐标系）：

+ X：左右
+ Y：前后
+ Z：**竖直向上**

`gravity="0 0 -9.81"`

+ X=0：没有水平方向重力
+ Y=0：没有前后方向重力
+ **Z = -9.81：重力指向 Z 轴负方向 = 向下**

****

通俗理解


+ **Z 轴向上，所以向下的重力，Z 分量就是负数。**
+ **地球表面重力加速度大小是 **`**9.81 m/s²**`**，所以写 **`**-9.81**`**。**
+ **如果写 **`**0 0 9.81**`**：重力向上，物体飘起来；**
+ **如果写 **`**0 0 0**`**：零重力，太空漂浮效果。**



****

### 局部坐标系
> **<compiler angle="degree" coordinate="local"/>**

**详解**

+ `angle="degree"`：关节的`range`、ctrl 输入用**角度 (°)**，不是弧度；
+ `coordinate="local"`：body 的 pos、axis 使用**局部坐标系**，这是写机械臂 MJCF 的标准写法。



### 力矩是什么
 力矩就是**扭动 / 旋转的力量**


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788852584515-375c77e5-bcad-4268-92cd-5f230177bab6.png)

****

**力矩 = 力 × 力臂**  


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788852659789-76d8f1e2-07f3-4b72-bb25-fc89af45d331.png)

+ **F：施加的力**
+ **L：力距离转轴的长度（力臂）**

****

举生活例子：

1. **拧扳手拧螺丝：你手用的是推力，****扳手对螺丝产生的就是力矩****。手使劲越大、扳手越长，力矩越大，螺丝越容易拧动。**

****


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788852844692-7f1a083b-7f35-4ad0-8b7e-7bd57eec2090.jpeg)



****

2. **舵机 / 机械臂关节：电机内部不是输出 “推力”，而是输出力矩，扭动转轴让胳膊转起来。**


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788833618972-801ed0bb-2301-4531-8ecd-eea6e0f26994.png)

****

### 力矩运算公式


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788853242692-32fdbf2a-ef8f-4a2c-9124-38a3b81b8a0d.png)

```markdown
注释：
N.m = N.m/rad rad  -   N.m. s/rad  rad/s
最终得到的是力矩，也就是从real到target这段弧度需要多大的力
在运动学中，轨迹运动要尽可能平滑接近于曲线，所以有两个比例项在始终
约束它们。 其中Kp约束系数的单位为N.m.s,它想让当前刚体尽可能快的接近目标位置，kd用来防止力矩过大用来产生动荡的现象。

```

```xml
 <joint damping="1" frictionloss="0.1" armature="0.005"/>

这里的armature代表转动惯量，通俗的理解，当armature大时轮子不容易被转动，当armature小时轮子容易被转动。

它主要与阻尼系数 增益系数有以下关系
kd=2*sqrt(kp*armature)





```

![](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261107195926222.png)



![image-20261107204827495](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261107204827495.png)

![](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261107204818543.png)



![image-20261107204917210](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261107204917210.png)





![image-20261107204908370](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261107204908370.png)





![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788858434782-7f131566-fb1d-440c-b618-a0811239fbd8.png)





 **每个符号含义 ** 


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788853611525-889c2b90-b383-4139-a199-f5adfb81fbf2.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788853725787-bc4b4ac4-8a60-4a9d-abc7-37171eeac275.png)


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788858622200-e2e5e79f-489b-46c2-8ff4-b3cbc66c9d60.jpeg)



### **kp 伺服刚度**
<position kp="50" forcerange="-2 2"/>  


 **kp：也叫做位置伺服刚度（position 控制器） ** 

`kp = position gain`，**位置比例增益**。  





**kp 的作用**


1. **kp 越大**：同样位置误差下输出力矩越大，机械臂拼命往目标位置拽，跟踪越快；
2. kp 太大：系统容易震荡、抖动，甚至仿真数值爆炸出现`NaN`；
3. kp 太小：手臂软，跟不上目标，被重力压得垂下来，达不到设定角度。





### 关节和连杆

![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1788862866019-c48fcc9b-025b-44a5-85ca-b2ff49870bc5.jpeg)



<font style="color:#2F8EF4;">关节定义</font>

> **<joint type="hinge" damping="0.1" frictionloss="0.02" armature="0.002"/> **
>



+ `<joint>`：描述**关节转轴本身**（舵机转轴、轴承），管转动的摩擦、阻尼、电机转子惯量。**没有 density、rgba、friction（接触摩擦）这些参数**。



| 参数 | 含义 | 物理类比 |
| --- | --- | --- |
| ` type="hinge"` | 类型为旋转关节 | 人的关节，舵机转动轴 |
| `damping="0.1"` | **关节速度阻尼** | 轴承自带阻力：转得越快，阻力越大，抑制抖动。是**硬件轴承的物理阻尼**，不是控制器 kd。 |
| `frictionloss="0.02"` | 关节静摩擦损耗 | 转轴 “启动阻力”，想要转动，必须先输出大于这个值的力矩才能转起来，模拟舵机轴承卡顿。 |
| `armature="0.002"` | 电机转子惯量 | 电机内部转子的转动惯量。MuJoCo 用它自动计算控制器<br/>`kd=2*sqrt(kp*armature)`；<br/>armature=0 高 kp 会直接震荡 NaN。 |




 一句话：joint 只管**转轴转动时的力学行为**，不管连杆长啥样、质量多大。 连杆的质量不由 joint 决定！**质量来自 geom 的 density 密度**。  



<font style="color:#2F8EF4;">连杆定义</font>

> <font style="color:#000000;"> </font><font style="color:#000000;"><geom type="capsule" density="1500" rgba="0.75 0.6 0.25 1" friction="0.9"/>  </font>

 

geom = 几何刚体，代表机械臂连杆实体。  


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788863115995-93456ef8-5d87-4f36-8707-677dbb13e204.png)



> 举个生活例子
>




想象一个舵机 + 塑料连杆：

1. `<joint>` = 舵机转轴 + 内部轴承
    - damping：轴承转动的粘滞阻力
    - frictionloss：轴承卡住，需要一点力气才能转起来
    - armature：舵机内部电机转子的转动惯量
2. `<geom>` = 外面那根塑料手臂
    - density：塑料密度，决定这根胳膊多重，重力往下拽
    - rgba：手臂外壳颜色
    - friction：手臂外壳表面有多滑



### 非常容易踩坑的两对混淆
1. `joint damping`（轴承转动阻尼） vs 控制器`kd`（算法虚拟刹车）
+ `joint damping`：真实轴承自带阻力，物理本体。
+ `kd`：位置伺服控制器算法虚拟阻尼，软件层面。
2. `joint frictionloss`（转轴内部静摩擦） vs `geom friction`（物体表面碰撞摩擦）
+ `frictionloss`：转轴想转，要克服内部阻力。
+ `geom friction`：两个 geom 碰到一起，接触面的摩擦力，比如夹爪抓方块会不会打滑。




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788865272464-0acb0683-11bb-466c-90a9-a46dd7dc814b.png)



### 光源和地面


```xml
<worldbody>
  <!-- 定向平行光源，pos光源位置，dir照射方向 -->
  <light pos="0.5 1.5 2.5" dir="0 0 -1" directional="true"/>
  <!-- 地面平面几何体 -->
  <geom name="floor" type="plane" size="2 2 0.1" rgba="0.45 0.45 0.5 1"/>
```



1.  `<worldbody>`

**世界物体容器标签** 所有场景里的物体：地面、机器人、方块障碍物、灯光，全部写在`worldbody`内部。

+ 在`worldbody`下直接写的`<geom>`，是**固定不动的物体**，绑定在世界坐标系，不会飘、不会动。
+ `<body>`写在 worldbody 里面，用来搭建可运动的连杆、机械臂。



2.  `<light pos="0.5 1.5 2.5" dir="0 0 -1" directional="true"/>  `

MuJoCo 场景光源，只影响渲染视觉，**不参与物理仿真计算**。



| 参数 | 含义 |
| --- | --- |
| `pos="0.5 1.5 2.5"` | 光源坐标：X=0.5，Y=1.5，Z=2.5，光源放在空间这个位置 |
| `dir="0 0 -1"` | 照射方向向量：`0 0 -1`<br/> = 沿着 Z 轴负方向，**从上往下照**（Z 轴向上，负号朝下） |
| `directional="true"` | 定向平行光，类似太阳光。光线互相平行，不会随着距离衰减。 |


3.  `<geom name="floor" type="plane" size="2 2 0.1" rgba="0.45 0.45 0.5 1"/>  `



`<geom>`：几何体；写在`worldbody`直接子节点，就是**固定静止地面，不会运动**。  



| 参数 | 含义 |
| --- | --- |
| `name="floor"` | 给几何体起名字，方便调试、代码查找 |
| `type="plane"` | 平面无限薄地面；plane 本身数学上是无限大，`size`是**渲染可视范围，不是物理边界**。物理上物体无论 XY 走多远，都会碰到地面。 |
| `size="2 2 0.1"` | 平面可视化尺寸：X 方向 ±2 米，Y 方向 ±2 米；第三个 0.1 是用于碰撞填充厚度，防止物体穿模。渲染出来能看见的地面方块是 4m×4m。**但是物理碰撞是无限大平面！超出看到的方块，依然会碰撞。** |
| `rgba="0.45 0.45 0.5 1"` | 颜色：R=0.45 G=0.45 B=0.5；A=1 完全不透明；偏灰蓝色地面。 |




4. 非常重要的坑：`type="plane"`



`plane`：**物理碰撞无限大**。 哪怕机器人跑到可视方块外面，依然会被地面托住，不会掉下去。 如果你想要一块有限大小的地板，不要用`plane`，改用`type="box"`盒子几何体。

示例有限大小地板（盒子）：

```xml
<geom name="floor_box" type="box" size="2 2 0.05" rgba="0.45 0.45 0.5 1"/>
```



box 的 size：X/Y/Z 半长，这块地板实际 4m×4m，厚度 0.1m，跑出这个范围就会掉下去。



### 高度如何计算
```xml
<body name="base" pos="0 0 0.10">
  <geom name="base_pedestal" type="cylinder" size="0.035 0.10" pos="0 0 0" rgba="0.3 0.3 0.35 1"/>
</body>
```

这一段是**机械臂底座基座**：`body`是刚体容器，里面放几何形体`geom`；底座是整个机械臂最根部的刚体。

---

1. `<body name="base" pos="0 0 0.10">`

`<body>`：刚体容器，MuJoCo 里每一个连杆都是一个 body。

+ `name="base"`：刚体名字，叫 base 底座，代码可以按名字访问这个刚体。
+ `pos="0 0 0.10"`：**这个 body 自身原点在世界坐标系的位置：X=0，Y=0，Z=0.10 米**。




Z=0.10：把整个底座刚体向上抬 0.1 米，离开地面。 body 内部所有子元素（geom、joint）的坐标，都是**相对于这个 body 的局部坐标系**，不是世界坐标。

关键点： body 的 pos = 局部坐标系原点在世界空间的位置。 body 里面 geom 写`pos="0 0 0"`，代表这个圆柱体放在 base 刚体自己的原点上。





2. `<geom name="base_pedestal" type="cylinder" size="0.035 0.10" pos="0 0 0" rgba="0.3 0.3 0.35 1"/>`

`<geom>`：圆柱体几何体，底座看得见、参与碰撞、计算质量。




cylinder 圆柱体 `size="A B"`

圆柱体在 MuJoCo：`size="半径 半高"`

+ `0.035` → **半径 0.035 m**（直径 0.07 米，7 厘米）
+ `0.10` → **半高 0.10 m**，圆柱体总高度 = 2 × 0.10 = **0.2 米**



****


cylinder size 参数顺序：`size="radius, half‑height"`，不是直径、不是总高！

+ `pos="0 0 0"`：geom 相对于父 body (base) 的局部坐标，放在 base 的局部原点。 因为 base 整体已经 Z=0.1，圆柱体中心就处于世界 Z=0.1 的位置。 圆柱体总高 0.2m：
    - 圆柱体底部世界 Z：`0.1 − 0.1 = 0.0 m`（刚好贴地面）
    - 圆柱体顶部世界 Z：`0.1 + 0.1 = 0.2 m`
+ `rgba="0.3 0.3 0.35 1"`：深灰偏蓝，A=1 完全不透明。







**整体空间位置梳理**



1. `<body name="base" pos="0 0 0.10">`：刚体原点在世界`(0,0,0.1)`
2. 内部 cylinder `pos="0 0 0"`：圆柱中心和 body 原点重合
3. 圆柱半高 0.1：向下延伸 0.1m 到 Z=0 地面，向上延伸 0.1m 到 Z=0.2。 结果：底座柱子从地面 (Z=0) 一直立到 Z=0.2m，半径 3.5cm。

注意：`<body name="base">`这里**没有写 joint**！ 没有 joint，代表这个 body 是**固定基座，不能转动**，牢牢钉在世界空间，机械臂的根部。 后面子层级才会出现`<joint>`，实现各个关节旋转。



### 底座旋转原理
```xml
<body name="pan_link" pos="0 0 0.10">
  <joint name="shoulder_pan" axis="0 0 1" range="-170 170"/>
  <geom name="pan_servo" type="cylinder" size="0.024 0.02" pos="0 0 0" rgba="0.4 0.4 0.45 1"/>
</body>
```

这是**机械臂第一个可转动连杆：水平旋转底座（云台 / 偏航关节）**，挂载在上一级 `base` 底座里面。



#### `<body name="pan_link" pos="0 0 0.10">`
+ `name="pan_link"`：连杆名称，pan = 水平旋转（左右转）。
+ `pos="0 0 0.10"`：**相对于父 body（base）的局部坐标**，不是世界坐标。 父 body (base) 原点在世界 Z=0.1m； `pan_link` 在父局部下 Z=0.10，所以 `pan_link` 的原点世界 Z = 0.1 + 0.1 = **0.2 m**。



也就是：这个连杆的旋转中心，在底座圆柱的顶端（Z=0.2）。


重点： 父节点是 `<body base>`，所以这里 pos 是**相对 base 的局部坐标系**。 body 里面的 joint、geom 全部以本 body 的原点为基准。





#### `<joint name="shoulder_pan" axis="0 0 1" range="-170 170"/>`
这是**hinge 旋转关节（舵机转轴）**，因为父 body 是 base，这个 joint 连接`base(固定基座)` , `pan_link(可转动连杆)`。

```xml
<body name="base" pos="0 0 0.10">
  <body name="pan_link" pos="0 0 0.10">
    <joint name="shoulder_pan" axis="0 0 1" range="-170 170"/>
    <geom name="pan_servo" type="cylinder" size="0.024 0.02" pos="0 0 0" rgba="0.4 0.4 0.45 1"/>
  </body>
</body>
```

各参数含义



| 参数 | 含义 |
| --- | --- |
| `name="shoulder_pan"` | 关节名字，后面 actuator 执行器通过名字绑定这个关节 |
| `axis="0 0 1"` | 旋转轴：沿**Z 轴正方向**竖直向上。实现水平左右旋转（像云台转头） |
| `range="-170 170"` | 关节机械角度限位，单位**角度 °**。物理上只能在‑170° ~ +170° 之间转动，防止机械结构撞坏。 |


 axis="0 0 1"：转轴竖直朝上，连杆就在水平面转圈。  

####  `<geom name="pan_servo" type="cylinder" size="0.024 0.02" pos="0 0 0" rgba="0.4 0.4 0.45 1"/> ` 


圆柱体，代表这个关节舵机外壳。

+ `type="cylinder"`圆柱体，`size="半径 半高"`
+ `size="0.024 0.02"`：半径 0.024 m；半高 0.02 m；**总高度 = 0.04m**。
+ `pos="0 0 0"`：圆柱中心对齐 `pan_link` 自身原点。 世界坐标：圆柱中心 Z=0.2m；向下到 Z=0.18，向上到 Z=0.22。
+ `rgba="0.4 0.4 0.45 1"`：浅灰蓝色，不透明。

#### 整体空间关系梳理
1. base 底座圆柱体：从世界 Z=0 到 Z=0.2。
2. pan_link 连杆原点：世界 Z=0.2，正好在底座顶端。
3. joint 转轴竖直向上 (Z 轴)，`pan_link` 可以绕 Z 轴左右水平旋转 ±170°。
4. 圆柱舵机外壳，套在旋转轴位置，高度薄薄一层（总高 4cm）。

层级关系

```plain
body base（固定不动，Z0.1）
└── body pan_link（可绕Z轴旋转）
├── joint shoulder_pan：base ↔ pan_link 的旋转转轴
└── geom pan_servo：舵机外壳圆柱
```

关键知识点 MuJoCo 的 body‑joint 规则： 


joint 写在**子 body 内部**；joint 连接「父 body」和「当前子 body」。 

这里 joint 在 pan_link 里面，所以：父 = base，子 = pan_link；pan_link 可以相对 base 转动。

其他关机旋转类似，比如axis="0 1 0"，就是 y 轴不动，绕着 y 轴旋转



### 执行器模块解析
 actuator：**执行器，相当于舵机驱动器**。 作用：把仿真控制信号 `data.ctrl` 和模型里的 `<joint>`关节绑定在一起，告诉 MuJoCo：“我要控制哪个关节、允许给多大的目标值、用什么伺服参数”。  

```xml
<!-- actuator执行器模块：把控制信号绑定到关节
position：位置伺服控制器
joint：绑定对应关节名
ctrlrange：ctrl数组输入信号允许范围
旋转关节单位：度；滑动夹爪关节单位：米
kp：夹爪单独设置刚度，覆盖default默认kp=50
程序中 data.ctrl 数组就是给这里每个actuator赋值，实现控制
-->
<actuator>
  <position name="shoulder_pan_act" joint="shoulder_pan" ctrlrange="-170 170"/>
  <position name="shoulder_lift_act" joint="shoulder_lift" ctrlrange="-110 110"/>
  <position name="elbow_flex_act" joint="elbow_flex" ctrlrange="-150 10"/>
  <position name="wrist_flex_act" joint="wrist_flex" ctrlrange="-120 120"/>
  <position name="wrist_roll_act" joint="wrist_roll" ctrlrange="-180 180"/>
  <position name="right_finger_act" joint="right_finger" ctrlrange="0 0.02" kp="100"/>
  <position name="left_finger_act" joint="left_finger" ctrlrange="-0.02 0" kp="100"/>
</actuator>

```

#### 基础标签说明
`<actuator>`：所有执行器放在这个标签内部。 `<position>`：**位置型伺服执行器**，

就是前面 PD 公式


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788916735660-74096c86-55df-4d25-869a-a6b18fc07eba.png)

的控制器。

每一行一个`<position>`，对应一个舵机 / 一个关节。

每一个 position 参数

| 参数 | 含义 |
| --- | --- |
| `name="xxx_act"` | 执行器自己的名字，调试看日志用，非必须 |
| `joint="shoulder_pan"` | **绑定哪一个关节**，和前面`<joint name="shoulder_pan">`<br/>一一对应。建立控制器和物理关节的桥梁。 |
| `ctrlrange="min max"` | **控制输入的允许范围**，代码中`data.ctrl`<br/>的目标值不能超出这个区间。旋转 hinge 关节：单位是**度 °**；<br/>滑动（slide）夹爪关节：单位是**米 m**。 |
| `kp="100"` | 可选参数；不显式写，就继承`<default>`里面的`kp=50`；写了就覆盖默认值。夹爪需要更大刚度，所以手动设 100。 |



**⚠️区分三组极易混淆的 range，这是 MuJoCo 高频坑：**



1. `<joint range="-170 170"/>`：**关节物理机械限位**。就算你控制器给更大角度，物理结构也转不过去，硬件层面卡死。
2. `<position ctrlrange="-170 170"/>`：**软件输入限制**。代码给`data.ctrl`的值会被钳位在这个区间，不让你输入非法目标。
3. `<position forcerange="-2 2"/>`：**力矩输出上限**，舵机最多输出多大扭力。

三者配合： `joint range` 是硬件不能超； `ctrlrange` 是软件不让你传超范围的目标； `forcerange` 是力气不能太大。





## 自动运动代码
```python
# -*- coding: utf-8 -*-
""" run_sim.py —— SO-ARM101 MuJoCo 自动演示：夹取红色滑块 → 放到旁边 → 再夹回来
====================================================================
用法：
  python run_sim.py --replay demo.npz    # 【推荐】回放真实主臂录制的夹取动作(角度真实、不插地)
  python run_sim.py                 # 内置脚本化演示（兜底，仅供无录制时用）

【推荐做法：录制再回放】（回放模式【不显示红色滑块】，只展示真实手臂动作）
 1) 用真实主臂把"夹取→搬运→放下"动作录下来：
        python sim_teleop.py --leader --record demo.npz
      （拖动真实主臂走一遍夹取流程：下探→合爪→抬起→移到 B→张开→...，结束后 Ctrl+C）
 2) 回放（只回放手臂真实关节角，无红色滑块；--start 可跳过开头静止段）：
        python run_sim.py --replay demo.npz --start 18
      run_sim 会按录制时的真实关节角回放手臂运动。
      因为角度和流程来自真实操作，夹爪不会插进地面。
   （红色滑块只在"脚本化兜底演示 python run_sim.py"里出现）

 演示内容（自动循环）：
   1. 机械臂从起始位出发，弯腰下探到放在地板上的【红色滑块】上方
   2. 夹爪合拢“夹住”滑块（滑块被吸附到夹爪，模拟抓稳）
   3. 抬起 → 旋转 ~52° 到旁边另一个位置 → 放下滑块（夹爪张开）
   4. 再飞到那边把滑块夹回来、放回原位 → 循环往复
   观察：左键拖拽旋转视角，滚轮缩放，ESC 退出。
 """
import argparse
import os
import time

import numpy as np
import mujoco
import mujoco.viewer

# 获取当前脚本所在目录，用于读取xml、stl资源文件
SIM_DIR = os.path.dirname(os.path.abspath(__file__))

# ---------------- 演示参数（都可以改） ----------------
# 红色滑块box几何体半长宽高，单位米；完整实体尺寸：3.4cm ×3.4cm ×6.0cm
SLIDER_HALF = (0.017, 0.017, 0.030)
GRIP_OPEN = 1.7453                    # 夹爪“张开”角度（弧度，模型上限）
GRIP_CLOSE = 0.95                     # 夹爪“合拢”角度（弧度）
PAN_SWING = 0.9                       # A点到B点基座旋转角（弧度 ≈52°）
LIFT_RAISE = 0.55                     # “悬停/搬运”姿态，相比抓取姿态抬高的关节角度（弧度）
# SO-ARM101机械臂全部关节名称，顺序：基座偏航→肩抬升→肘部弯曲→腕弯曲→腕旋转→夹爪
JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex",
               "wrist_flex", "wrist_roll", "gripper"]

# 抓取姿态（弧度）：肘部弯曲(~80°)、腕部下压，像人“弯腰下探”去拿物体。
# 顶点校验保证：夹爪张开/合拢全程最低点离地 ≥ 3.0cm，固定爪不会戳地
Q_GRASP = {
    "shoulder_pan": 0.0, "shoulder_lift": 1.36, "elbow_flex": -1.40,
    "wrist_flex": 1.16, "wrist_roll": 0.0, "gripper": GRIP_OPEN,
}
Q_HOME = {  # 起始待机姿态（机械臂收起，安全初始位置）
    "shoulder_pan": 0.0, "shoulder_lift": -0.30, "elbow_flex": -0.55,
    "wrist_flex": 0.35, "wrist_roll": 0.0, "gripper": GRIP_OPEN,
}
# 两片夹爪对应的mesh名称，用于计算夹爪几何中心点（抓取点）
JAW_MESHES = ("moving_jaw_so101_v1", "wrist_roll_follower_so101_v1")

# ================= 1) 在内存里“动态添加”红色滑块 =================
def _read_assets_and_scene():
    """
    读取原始场景XML、机械臂模型xml、全部stl网格资源
    不在磁盘修改任何文件，全部加载进内存，返回scene文本 + 资源字典
    assets字典key为资源路径，value为二进制内容，供MjModel.from_xml_string读取
    """
    with open(os.path.join(SIM_DIR, "scene.xml"), encoding="utf-8") as f:
        scene_xml = f.read()
    with open(os.path.join(SIM_DIR, "so101_new_calib.xml"), encoding="utf-8") as f:
        assets = {"so101_new_calib.xml": f.read().encode("utf-8")}
    adir = os.path.join(SIM_DIR, "assets")
    # 遍历assets文件夹，加载所有stl模型二进制数据
    for fn in os.listdir(adir):
        if fn.lower().endswith(".stl"):
            with open(os.path.join(adir, fn), "rb") as f:
                assets["assets/" + fn] = f.read()
    return scene_xml, assets


def load_model_plain():
    """
    只加载官方机械臂模型（不含红色滑块）
    用于回放录制动作模式：只展示手臂运动，不加载滑块物体
    """
    scene_xml, assets = _read_assets_and_scene()
    return mujoco.MjModel.from_xml_string(scene_xml, assets=assets)


def load_model_with_slider():
    """
    加载场景，并且在内存动态注入红色滑块body+geom
    脚本自动夹取演示模式使用；修改xml字符串，不改动磁盘原始文件
    """
    scene_xml, assets = _read_assets_and_scene()
    # 定义红色滑块：freejoint表示6自由度自由刚体，可以落地、被夹爪带动
    # geom为box方块，rgba红色，设置摩擦系数、质量0.05kg
    slider_body = f"""
    <body name="red_slider" pos="0.33 0 0">
      <freejoint/>
      <geom name="slider_geom" type="box"
            size="{SLIDER_HALF[0]} {SLIDER_HALF[1]} {SLIDER_HALF[2]}"
            pos="0 0 {SLIDER_HALF[2]}" rgba="1 0.08 0.08 1"
            friction="1.2 0.005 0.0001" mass="0.05"/>
    </body>
    """
    # 在worldbody闭合标签前插入滑块body定义
    xml = scene_xml.replace("</worldbody>", slider_body + "\n  </worldbody>", 1)
    return mujoco.MjModel.from_xml_string(xml, assets=assets)

# ================= 2) 小工具函数（坐标变换、点云、位姿相关） =================
def mesh_block(model, mesh_id):
    """
    读取指定mesh_id网格的全部【STL局部顶点】
    输入：model模型对象，mesh_id网格编号
    返回：N×3 numpy数组，N个顶点，每行[x,y,z]，是CAD导出的原始局部坐标（固定不变）
    原理：通过mesh_vertadr找到该mesh顶点在mesh_vert大数组的起始偏移，mesh_vertnum读取顶点数量
    """
    a = int(np.asarray(model.mesh_vertadr[mesh_id]).reshape(-1)[0])
    n = int(np.asarray(model.mesh_vertnum[mesh_id]).reshape(-1)[0])
    return model.mesh_vert[a:a + n].reshape(-1, 3)


def mesh_world(model, data, mesh_name):
    """
    获取指定名称mesh网格所有顶点的【世界坐标】
    原理：mesh_block拿到静态局部顶点v；读取geom实时世界平移b、旋转矩阵R
    坐标变换公式 world_vertex = b + v @ R.T
    输入：model静态模型，data仿真动态数据，mesh_name网格名称字符串
    返回N×3世界坐标点云；找不到对应mesh返回None
    """
    # 遍历全部geom几何体
    for g in range(model.ngeom):
        # 判断该geom类型是否为stl网格mesh
        if model.geom(g).type == mujoco.mjtGeom.mjGEOM_MESH:
            # 判断该geom引用的mesh名字匹配目标mesh_name
            if model.mesh(model.geom(g).dataid).name == mesh_name:
                mid = model.geom(g).dataid  # 获取mesh编号mesh_id
                v = mesh_block(model, mid)  # 取出stl局部顶点 N×3
                b = data.geom_xpos[g]       # geom当前世界平移向量 shape(3,)
                R = data.geom_xmat[g].reshape(3, 3) # geom世界旋转矩阵3×3
                return b + v @ R.T          # 全部顶点转换到世界坐标系
    return None


def finger_centroid(model, data):
    """
    计算两片爪指的几何中心点（世界坐标），用作抓取参考点
    逻辑：读取两个爪mesh全部顶点世界坐标，垂直堆叠点云，对x/y/z分别求平均
    注意：是顶点几何平均，不是物理质心，工程上快速估算抓取位置
    返回：一维数组 [center_x, center_y, center_z]
    """
    # 遍历两个爪mesh，获取各自全部顶点世界坐标，存入列表parts
    parts = [mesh_world(model, data, nm) for nm in JAW_MESHES]
    # np.vstack垂直拼接两组点云，合并为 (N1+N2, 3) 大数组
    C = np.vstack(parts)
    # axis=0：沿着所有行求均值，每一列(x,y,z)分别计算平均值
    return C.mean(0)


def set_arm(data, model, q):
    """
    批量设置多个关节目标角度，执行正向运动学更新全部刚体状态
    参数 q:字典 {关节名:目标弧度值}
    1. 根据关节名字查找关节id
    2. 根据jnt_qposadr找到该关节在qpos一维数组中的偏移位置，赋值
    3. mj_forward：正向计算，刷新data内所有body、geom的xpos/xmat世界位姿
   		不调用mj_forward，后面读取的坐标、位姿全部是旧数据！
    """
    for name, v in q.items():
        # 根据名字查找关节对象id，指定查找类型为JOINT关节
        jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
        # jnt_qposadr：该关节在广义坐标qpos数组内的起始下标
        data.qpos[model.jnt_qposadr[jid]] = v
    # 正向运动学求解，更新所有body、geom的世界位置姿态
    mujoco.mj_forward(model, data)


def body_pose(model, data, name="gripper"):
    """
    获取指定body刚体的4×4齐次变换矩阵（世界坐标系下位姿）
    T = [[R, b],
         [0, 1]]
    R:3×3旋转矩阵；b:3维平移向量[x,y,z]
    输入body名称，返回4×4齐次矩阵，用于坐标变换
    """
    # 根据body名字查询body的id编号
    bid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_BODY, name)
    T = np.eye(4)  # 初始化4阶单位矩阵
    T[:3, :3] = data.xmat[bid].reshape(3, 3) # 左上角填入body旋转矩阵
    T[:3, 3] = data.xpos[bid]                # 第四列前3行填入body世界平移
    return T


def rot_to_quat(R):
    """
    3×3旋转矩阵 → 四元数(w,x,y,z)转换
    MuJoCo中freejoint的姿态存储使用四元数，用于滑块姿态更新

    算法说明：
        采用 Shepperd 算法（矩阵转四元数稳定版），避免数值奇异问题。
        直接根据旋转矩阵迹tr的大小，选择最大分量来计算，防止开根号出现精度损失。
        返回四元数格式：(w, x, y, z)，w为实部，(x,y,z)为虚部矢量。
        注意：输出为单位四元数，对应旋转。

    参数：
        R : np.ndarray, shape=(3,3)
            输入正交3x3旋转矩阵，行列式=1，无缩放。

    返回：
        w, x, y, z : float
            单位四元数，顺序：实部w，虚部x,y,z
    """
    # 计算旋转矩阵的迹 tr(R) = R[0,0]+R[1,1]+R[2,2]
    tr = R[0, 0] + R[1, 1] + R[2, 2]

    # 情况1：迹大于0，此时w分量最大，优先计算w，数值稳定性好
    if tr > 0:
        s = np.sqrt(tr + 1.0) * 2
        w = 0.25 * s
        x = (R[2, 1] - R[1, 2]) / s
        y = (R[0, 2] - R[2, 0]) / s
        z = (R[1, 0] - R[0, 1]) / s
    else:
        # 情况2：迹<=0，w比较小，找到矩阵对角元最大的下标i，优先计算对应的虚部分量
        i = int(np.argmax(np.diag(R)))
        j, k = (i + 1) % 3, (i + 2) % 3
        q = np.zeros(4)

        # 计算最大的那个虚部分量 q[i]
        q[i] = 0.5 * np.sqrt(max(0.0, 1 + R[i, i] - R[j, j] - R[k, k]))
        # 由已经算出的q[i]，反推剩余三个四元数分量
        q[0] = (R[k, j] - R[j, k]) / (4 * q[i])
        q[j] = (R[j, i] + R[i, j]) / (4 * q[i])
        q[k] = (R[k, i] + R[i, k]) / (4 * q[i])

        # 解包得到 w,x,y,z
        w, x, y, z = q[0], q[1], q[2], q[3]

    return w, x, y, z



def slider_qadr(model):
    """
    获取red_slider自由关节在qpos数组内的起始偏移下标
    freejoint占用qpos连续7个位置：[x,y,z, qw,qx,qy,qz] 平移+四元数姿态
    """
    return model.jnt_qposadr[mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, "red_slider")]


def place_slider(data, qadr, pos):
    """
    设置红色滑块在地面的位置，姿态重置为无旋转
    参数 pos：(x,y,z) 世界坐标，z=0放在地面
    qadr：滑块freejoint在qpos的起始偏移
    """
    data.qpos[qadr:qadr + 3] = pos
    # 四元数 [1,0,0,0] 代表无任何旋转
    data.qpos[qadr + 3:qadr + 7] = [1.0, 0.0, 0.0, 0.0]


def ease(t):
    """缓动插值函数，0<=t<=1；平滑启停，避免机械臂运动生硬、突变冲击"""
    return t * t * (3 - 2 * t)


def lerp_q(q0, q1, t):
    """
    关节字典线性插值
    q0：起始关节字典，q1目标关节字典，t∈[0,1]
    返回插值后的关节状态字典，实现平滑过渡
    """
    return {k: q0[k] + (q1[k] - q0[k]) * t for k in q0}


def slider_bottom_min(data, qadr, half):
    """
    计算红色滑块8个角点中，最低的Z坐标（世界坐标系）
    用来做触地检测：判断滑块底部有没有扎进地面
    原理：取出滑块位置+四元数转旋转矩阵，遍历box8个顶点，求最小z
    half：滑块半尺寸
    """
    qp = data.qpos[qadr:qadr + 7]
    w, x, y, z = qp[3:]
    # 四元数转3×3旋转矩阵
    R = np.array([[1 - 2*(y*y + z*z), 2*(x*y - z*w), 2*(x*z + y*w)],
                  [2*(x*y + z*w), 1 - 2*(x*x + z*z), 2*(y*z - x*w)],
                  [2*(x*z - y*w), 2*(y*z + x*w), 1 - 2*(x*x + y*y)]])
    hx, hy, hz = half
    m = 1e9
    # 遍历box方块8个顶点，计算每个角点世界z坐标，找最小值
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (0.0, 2*hz):
                m = min(m, float((qp[:3] + R @ np.array([sx*hx, sy*hy, sz]))[2]))
    return m

# ================= 3) 演示编排（状态机） =================
class PickPlace:
    """
    自动拾取放置状态机：A点夹取→搬运到B放下，B夹取搬运回A，循环往复
    原理：分多阶段phase，缓动插值控制机械臂关节；抓取时滑块绑定在夹爪坐标系跟随运动
    触地即放逻辑：下降阶段滑块底部碰到地面，自动释放，防止物体穿透地面
    """

    def __init__(self, model, data, q_hover, q_grasp_a, q_grasp_b, q_home, spots):
        self.model, self.data = model, data
        self.q_home = q_home
        self.q = dict(q_home)  # 当前关节角度状态
        self.q_hover = q_hover # 悬停姿态（抓取前抬起来的安全高度）
        self.q_grasp = [q_grasp_a, q_grasp_b] # A、B两点抓取姿态
        self.spots = spots # A,B两个地面放置点坐标
        self.qadr = slider_qadr(model) # 滑块freejoint在qpos中的偏移下标
        self.slider_half = SLIDER_HALF # 滑块半尺寸保存

        U0 = q_hover
        U1 = {**q_hover, "shoulder_pan": PAN_SWING} # 旋转后的悬停姿态
        A, B = q_grasp_a, q_grasp_b
        # 动作阶段列表：(名称,目标关节q,目标夹爪角度,阶段持续时间,事件标记)
        leg = [
            ("desc_A", A, GRIP_OPEN, 2.6, "approach"),    # 靠近A点，下探准备抓取
            ("close_A", A, GRIP_CLOSE, 0.9, "grab"),       # 合爪抓取
            ("lift_A", U0, GRIP_CLOSE, 2.2, None),         # 抬起，离开地面
            ("pan_AB", U1, GRIP_CLOSE, 2.8, None),         # 基座旋转，移动到B上方
            ("down_B", B, GRIP_CLOSE, 2.2, "place"),       # 下降到B点，place标记：触地释放
            ("open_B", B, GRIP_OPEN, 0.9, None),           # 张开夹爪放下物体
            ("up_B", U1, GRIP_OPEN, 2.0, None),            # 抬升离开
            ("desc_B", B, GRIP_OPEN, 2.2, "approach"),     # 靠近B点准备抓取
            ("close_B", B, GRIP_CLOSE, 0.9, "grab"),       # B点合爪抓取
            ("lift_B", U1, GRIP_CLOSE, 2.0, None),         # B点抬起
            ("pan_BA", U0, GRIP_CLOSE, 2.8, None),         # 旋转回到A上方
            ("down_A", A, GRIP_CLOSE, 2.2, "place"),       # 下降放到A点
            ("open_A", A, GRIP_OPEN, 0.9, None),           # 张开夹爪释放
            ("up_A", U0, GRIP_OPEN, 2.0, None),            # 抬升离开
        ]
        self.phases = leg
        self.i = 0  # 当前阶段索引
        self.t = 0.0 # 当前阶段已经流逝的时间
        self.holding = False # 是否正在抓取、吸附滑块
        self.hold_T = None   # 抓取保存的齐次变换矩阵：滑块相对于gripper的相对位姿
        self.cur_spot = 0    # 当前滑块所在点位索引0:A，1:B

    def _release(self):
        """释放滑块：取消吸附，把滑块固定放置到地面点位，切换目标点位"""
        self.holding = False
        self.hold_T = None
        self.cur_spot = 1 - self.cur_spot
        place_slider(self.data, self.qadr, self.spots[self.cur_spot])
        mujoco.mj_forward(self.model, self.data)

    def update(self, dt):
        """
        每帧更新状态机，dt是单帧时间步长
        1. 时间累加，缓动插值生成当前目标关节角度
        2. set_arm驱动机械臂
        3. 如果处于holding抓取状态：跟随夹爪同步更新滑块位置姿态
        4. 检测触地，自动释放物体；阶段时间结束切换下一动作阶段
        """
        model, data = self.model, self.data
        q_target, g_target, dur, ev = self.phases[self.i][1:]
        q0, g0 = self.q, self.q["gripper"]

        self.t = min(self.t + dt, dur)
        t = ease(self.t / dur) # 归一化+缓动函数
        qm = lerp_q(q0, q_target, t) # 插值得到当前目标关节
        qm["gripper"] = g0 + (g_target - g0) * t # 夹爪角度单独插值
        set_arm(data, model, qm)
        self.q = qm

        if self.holding:
            # 抓取状态：滑块跟随gripper刚体运动
            T_box = body_pose(model, data, "gripper") @ self.hold_T
            # 更新滑块freejoint的平移和姿态
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
            mujoco.mj_forward(model, data)
            # place阶段触地检测：滑块底部低于阈值，立刻释放，防止扎进地面
            if ev == "place" and slider_bottom_min(data, self.qadr, self.slider_half) <= 0.0012:
                self._release()
                self.t = 0.0
                self.i = (self.i + 1) % len(self.phases)
                return
        else:
            # 未抓取，滑块固定放在地面点位
            place_slider(data, self.qadr, self.spots[self.cur_spot])
            mujoco.mj_forward(model, data)

        # 当前阶段时间跑完，切换下一阶段
        if self.t >= dur:
            if ev == "grab":
                # grab事件：抓取，记录滑块相对于夹爪的相对变换矩阵hold_T
                T_body = body_pose(model, data, "gripper")
                T_box = np.eye(4)
                T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
                self.hold_T = np.linalg.inv(T_body) @ T_box
                self.holding = True
            elif ev == "place":
                self._release()
            self.i = (self.i + 1) % len(self.phases)
            self.t = 0.0


def joint_range(model, name):
    """查询指定关节的运动范围 [最小值，最大值]（弧度）"""
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
    if jid < 0:
        return None
    return model.jnt_range[jid].copy()


def norm_body_to_rad(v, lo, hi):
    """归一化值[-100,100] → 关节弧度，用于录制回放主臂关节数据"""
    return lo + (v + 100.0) / 200.0 * (hi - lo)


def norm_grip_to_rad(v, lo, hi):
    """夹爪归一化值[0,100] → 夹爪关节弧度"""
    return lo + v / 100.0 * (hi - lo)


class ReplayDemo:
    """
    回放演示类：回放sim_teleop录制的npz轨迹文件
    原理：读取录制的归一化关节信号，转换为弧度，按时间插值驱动机械臂
    自动识别合爪/张爪：合爪吸附滑块跟随夹爪；张开时将滑块放置在当前夹爪位置
    录制轨迹来自真实主臂拖动，所以动作自然，不容易插地
    """

    JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex",
                   "wrist_flex", "wrist_roll", "gripper"]

    def __init__(self, model, data, path, speed=1.0, start_sec=0.0, len_sec=None,
                 use_slider=True):
        z = np.load(path)
        traj = z["traj"]                       # traj数组(N,7): 时间t + 6个关节归一化值
        self.model, self.data = model, data
        self.use_slider = use_slider # 是否启用红色滑块模拟抓取
        self.speed = speed           # 回放速度倍率
        self.holding = False
        self.hold_T = None
        self.last_spot = None                  # 滑块上一次落地保存的地面坐标
        self.qadr = slider_qadr(model) if use_slider else None

        t_abs = traj[:, 0] - traj[0, 0]
        rad_full = np.zeros((traj.shape[0], 6))
        # 把录制的归一化数值，逐个关节转换为弧度
        for c, name in enumerate(self.JOINT_NAMES):
            rg = joint_range(model, name)
            if rg is None:
                continue
            lo, hi = rg
            if name == "gripper":
                rad_full[:, c] = [norm_grip_to_rad(v, lo, hi) for v in traj[:, 6]]
            else:
                rad_full[:, c] = [norm_body_to_rad(v, lo, hi) for v in traj[:, c + 1]]
        grip_full = traj[:, 6]

        # 自动检测动作起始时间：找到关节有明显运动的时刻，用于提示用户--start参数
        d = np.abs(np.diff(rad_full, axis=0)).max(axis=1)
        act = np.where(d > 0.004)[0]
        self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0

        # 根据start、len参数裁剪回放时间窗口
        i0 = int(np.searchsorted(t_abs, start_sec, side="left"))
        i0 = max(0, min(i0, len(t_abs) - 1))
        if len_sec is None:
            i1 = len(t_abs) - 1
        else:
            i1 = int(np.searchsorted(t_abs, start_sec + len_sec, side="left"))
            i1 = max(i0 + 1, min(i1, len(t_abs) - 1))
        self.full_start = float(t_abs[i0])
        self.t = t_abs[i0:i1 + 1]
        self.t = self.t - self.t[0]
        self.duration = float(self.t[-1])
        self.grip_norm = grip_full[i0:i1 + 1]
        self.rad = rad_full[i0:i1 + 1]

        # 根据夹爪归一化数值，自动识别抓取片段（合爪段标记为hold_mask=True）
        self.holds = []
        self.hold_mask = np.zeros(len(self.t), dtype=bool)
        if self.use_slider:
            gmin, gmax = float(self.grip_norm.min()), float(self.grip_norm.max())
            thr = gmin + 0.30 * (gmax - gmin) if (gmax - gmin) > 8 else gmax
            held = self.grip_norm <= thr
            i = 0
            while i < len(held):
                if held[i]:
                    j = i
                    while j < len(held) and held[j]:
                        j += 1
                    if j - i >= 5:
                        self.holds.append((i, j - 1))
                    i = j
                else:
                    i += 1
            if not self.holds:
                # 如果没有识别到开合动作，全程视为抓取状态
                self.holds = [(0, len(self.t) - 1)]
            for s, e in self.holds:
                self.hold_mask[s:e + 1] = True
            self._reset()

    def _reset(self):
        """回放循环重置，初始化滑块位置"""
        if not self.use_slider:
            return
        s = self.holds[0][0]
        self.last_spot = self._finger_spot(self.rad[s])
        self.holding = False
        self.hold_T = None
        place_slider(self.data, self.qadr, self.last_spot)
        mujoco.mj_forward(self.model, self.data)

    def _row(self, qt):
        """根据当前回放时间qt，插值得到6个关节弧度值，返回一维数组"""
        return np.array([np.interp(qt, self.t, self.rad[:, c]) for c in range(6)])

    def _finger_spot(self, row):
        """输入一组关节弧度，驱动机械臂，返回夹爪中心点的地面放置位置(z=0)"""
        set_arm(self.data, self.model, dict(zip(self.JOINT_NAMES, row)))
        c = finger_centroid(self.model, self.data)
        return (float(c[0]), float(c[1]), 0.0)

    def update(self, dt, qt):
        """回放每帧更新逻辑，qt当前回放时间，dt时间步"""
        model, data = self.model, self.data
        row = self._row(qt)
        set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))
        if not self.use_slider:
            # 纯回放手臂，不加载滑块
            mujoco.mj_forward(model, data)
            return

        idx = int(np.searchsorted(self.t, qt, side="right") - 1)
        idx = max(0, min(idx, len(self.t) - 1))
        want_hold = bool(self.hold_mask[idx])

        # 状态切换：从松开→抓取，记录滑块相对夹爪的位姿
        if want_hold and not self.holding:
            T_body = body_pose(model, data, "gripper")
            T_box = np.eye(4)
            T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
            self.hold_T = np.linalg.inv(T_body) @ T_box
            self.holding = True
        # 状态切换：抓取→松开，把滑块放到当前夹爪中心点
        elif (not want_hold) and self.holding:
            self.last_spot = self._finger_spot(row)
            self.holding = False
            place_slider(data, self.qadr, self.last_spot)

        if self.holding:
            # 抓取状态，滑块跟随夹爪同步运动
            T_box = body_pose(model, data, "gripper") @ self.hold_T
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
            mujoco.mj_forward(model, data)
            # 兜底检测：滑块穿透地面，强制释放
            if slider_bottom_min(data, self.qadr, SLIDER_HALF) < -0.0006 and self.last_spot is not None:
                self.holding = False
                self.hold_T = None
                self.last_spot = self._finger_spot(row)
                place_slider(data, self.qadr, self.last_spot)
                mujoco.mj_forward(model, data)
        else:
            # 未抓取，固定在地面点位
            place_slider(data, self.qadr, self.last_spot)
            mujoco.mj_forward(model, data)


def main():
    parser = argparse.ArgumentParser(description="SO-ARM101 MuJoCo 自动演示")
    parser.add_argument("--replay", default=None,
                        help="回放 --leader --record 录下的动作文件(.npz)，用真实关节角夹取")
    parser.add_argument("--speed", type=float, default=1.0, help="回放速度倍率")
    parser.add_argument("--start", type=float, default=0.0,
                        help="从录制文件的第几秒开始回放(可跳过开头静止段)")
    parser.add_argument("--len", type=float, default=None,
                        help="回放时长(秒)，默认到结尾")
    args = parser.parse_args()

    model = load_model_with_slider()
    data = mujoco.MjData(model)
    mujoco.mj_resetData(model, data)
    mujoco.mj_forward(model, data)

    # ---------- 回放真实录制（只展示真实手臂动作，不含红色滑块）----------
    if args.replay:
        path = args.replay
        if not os.path.exists(path):
            alt = os.path.join(SIM_DIR, os.path.basename(path))
            if os.path.exists(alt):
                path = alt
            else:
                print(f"[run_sim] 找不到录制文件 {args.replay}。")
                print("  请确认在 mujoco_sim 目录下运行，或先录制：")
                print("    python sim_teleop.py --leader --record demo.npz")
                return
        model = load_model_plain()            # 回放模式：不注入红色滑块
        data = mujoco.MjData(model)
        mujoco.mj_resetData(model, data)
        mujoco.mj_forward(model, data)
        anim = ReplayDemo(model, data, path, speed=args.speed,
                          start_sec=args.start, len_sec=args.len, use_slider=False)
        print(f"[run_sim] 回放录制 {path}：{anim.duration:.1f}s"
              f"（录制开头约有 {anim.start_hint:.1f}s 静止段；想跳过可加 --start {anim.start_hint:.0f}）")
        set_arm(data, model, dict(zip(ReplayDemo.JOINT_NAMES, anim.rad[0])))
        # 启动MuJoCo可视化窗口，循环回放
        with mujoco.viewer.launch_passive(model, data) as viewer:
            prev = time.time()
            qt = 0.0
            while viewer.is_running():
                now = time.time()
                dt = min(0.05, now - prev)
                prev = now
                qt += dt * args.speed
                if qt >= anim.duration:
                    qt = qt % anim.duration     # 循环回放
                    anim._reset()
                anim.update(dt, qt)
                viewer.sync()
                time.sleep(0.002)
        return

    # ---------- 内置脚本化演示（无录制时的默认） ----------
    # 悬停姿态：抓取姿态基础上抬高LIFT_RAISE
    Q_HOVER = {**Q_GRASP, "shoulder_lift": Q_GRASP["shoulder_lift"] - LIFT_RAISE, "gripper": GRIP_OPEN}
    Q_GRASP_A = {**Q_GRASP}
    Q_GRASP_B = {**Q_GRASP, "shoulder_pan": PAN_SWING}

    def spot_of(q):
        """输入抓取姿态q，驱动机械臂，计算抓取点在地面的投影坐标"""
        set_arm(data, model, {**q, "gripper": GRIP_CLOSE})
        c = finger_centroid(model, data)
        return float(c[0]), float(c[1]), 0.0

    # 计算A、B两个抓取点位
    spotA = spot_of(Q_GRASP_A)
    spotB = spot_of(Q_GRASP_B)
    print(f"[run_sim] 抓取点 A={tuple(round(v, 3) for v in spotA)}"
          f"  B={tuple(round(v, 3) for v in spotB)}  (爪指质心投影)")

    qadr = slider_qadr(model)
    place_slider(data, qadr, spotA)
    mujoco.mj_forward(model, data)

    # 初始化拾取放置状态机
    anim = PickPlace(model, data, Q_HOVER, Q_GRASP_A, Q_GRASP_B, Q_HOME,
                     spots=[spotA, spotB])
    print("[run_sim] 演示开始：夹取 A → 放到 B → 夹回 A → ...（ESC 退出）")
    set_arm(data, model, Q_HOME)

    # 启动MuJoCo可视化窗口，运行状态机
    with mujoco.viewer.launch_passive(model, data) as viewer:
        prev = time.time()
        while viewer.is_running():
            now = time.time()
            dt = min(0.05, now - prev)
            prev = now
            anim.update(dt)
            viewer.sync()
            time.sleep(0.002)


if __name__ == "__main__":
    main()

```

### 锚点
MuJoCo 的物体锚点都是以物体中心为锚点，所以设置滑块到地面上，纵坐标应该为滑块高度的一半

`pos="0 0 {SLIDER_HALF[2]}`


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1788923584232-028343bb-62e6-4d62-b4c3-397459f90f89.png)



摩擦力以及重力  `friction="1.2 0.005 0.0001" mass="0.05"`

| 序号 | 数值 | 名称 | 通俗解释 |
| --- | --- | --- | --- |
| 第 1 个 `1.2` | **切向滑动摩擦 (tangential)** | 物体接触面**平移滑动**的摩擦力。数值越大越不容易打滑。 | 数值设置偏高，目的：**夹爪捏住方块不容易横向滑出去**。橡胶类高摩擦；冰面 0.1 左右。 |
| 第 2 个 `0.005` | **扭转摩擦 (torsional)** | 物体贴着接触面原地打转、拧转的阻力，阻止方块在夹爪之间原地旋转。 | 数值很小，允许方块轻微扭转，不会死死锁死。 |
| 第 3 个 `0.0001` | **滚动摩擦 (rolling)** | 物体在面上滚来滚去的阻力，比如方块边角打滚、翻滚。 | 几乎为 0，方块被碰撞后可以翻滚；如果改大，方块一碰就不容易翻滚。 |




## STL 网格（mesh）通俗讲解
**STL 是一种 3D 表面文件，把复杂物体表面拆成许许多多小三角形，只存表面顶点、三角面片；没有关节、没有质量、没有颜色**。 在 MuJoCo 里面叫 **mesh（网格资源）**，代码里`mesh_vertadr / mesh_vertnum / mesh_vert`数组保存的东西。

###  打比方
+ CAD 软件（SolidWorks、Onshape）画一个爪指零件，是光滑曲面。
+ 导出成 **STL**：软件把光滑曲面 “碎成无数小三角形瓷砖” 铺满表面。

就像圆球，用几千个小三角形拼出来，看起来接近圆形。

+ STL 文件里面只记录：**每个三角形的 3 个顶点 XYZ 坐标 + 面法向量**，仅此而已。

⚠️重要限制：

1. **没有颜色！** 颜色是 geom/material 给的，STL 本身不带颜色信息MuJoCo。
2. **没有关节、没有质量、没有运动信息**，纯粹几何外表。
3. 自带一套**局部坐标系**：STL 内部所有顶点坐标，是导出 CAD 时确定的原点，**固定写在文件里，不会随仿真运动改变**。👉就是`mesh_block()`拿出来的那套局部顶点`v`。

###  在 MuJoCo MJCF 里面的完整链路
```xml
<asset>
  <!-- 加载磁盘上的stl文件，注册成名叫 moving_jaw_so101_v1 的mesh资源 -->
  <mesh file="moving_jaw_so101_v1.stl" name="moving_jaw_so101_v1"/>
</asset>

<body name="gripper"> <!-- body刚体（物理零件，有质量，会转动） -->
  <!-- geom：几何体外壳，引用上面这个stl网格资源 -->
  <geom type="mesh" mesh="moving_jaw_so101_v1" class="visual"/>
</body>
```

### 加载后内存发生什么
1. MuJoCo 读取 stl，把**所有三角面片全部顶点**，全部摊平存进大数组：`model.mesh_vert`（一维大数组）
2. `model.mesh_vertadr[mesh_id]`：记录这个 mesh 的顶点在大数组的**起始偏移**
3. `model.mesh_vertnum[mesh_id]`：记录这个 mesh**有多少个顶点**

 这三个数组，就是`mesh_block()`函数读取的，取出**STL 原始局部顶点（不随手臂动）**。

4. 仿真运行每一帧，机械臂转动： 
    - `data.geom_xpos[g]`：这个 geom 现在**世界空间平移**
    - `data.geom_xmat[g]`：这个 geom 现在**世界旋转矩阵**
    - `mesh_world()`：`v @ R.T + b`，把 STL 固定的局部顶点，旋转平移，算出**仿真世界坐标**（跟着爪指一起动）。

### 理清 4 个容易搞混概念  


![](https://cdn.nlark.com/yuque/0/2026/jpeg/27211935/1789010002476-f6eada4d-eee0-461a-9a8f-37b76dae2e77.jpeg)

| 名词 | 是什么 | 是否随仿真运动 |
| --- | --- | --- |
| `.stl`磁盘文件 | 磁盘上的文件，一堆三角形顶点数据 | 静态文件 |
| **mesh（网格资源，model 内部）** | `model.mesh_xxx`<br/>数组保存，STL 读进内存后的静态数据；<br/>`mesh_id`是它编号 | **固定不变！CAD 导出的局部坐标** |
| **geom 几何体** | `<geom type="mesh">`<br/>，挂载在 body 刚体上，**引用一份 mesh 资源**；负责渲染 / 碰撞；一个 mesh 资源可以被多个 geom 复用 | geom 的位姿会随父 body 运动 |
| **body 刚体** | **刚体：形状、尺寸永远不会变形、不会弯曲的物体。** 不管怎么受力、旋转、移动，物体内部任意两个点之间的距离永远不变，刚体是物理零件，有质量惯性，joint 驱动它运动， | 会运动 |


关键点： **mesh 资源本身永远不动！动的是引用它的 geom/body。** mesh = 一张静态 “三角形图纸”；geom 把这张图纸贴在会动的刚体上面。



###  结合代码串起来
```python
# 1. mesh_block：读取【静态mesh资源】，拿到stl文件自带局部顶点（不动）
v = mesh_block(model, mid) 

# 2. mesh_world：拿每一帧【geom动态位姿】，把静态顶点变换成世界坐标（会跟着手臂动）
b = data.geom_xpos[g]
R = data.geom_xmat[g].reshape(3,3)
world_points = b + v @ R.T
```

### 举生活例子
+ `stl/mesh` = 一张打印好的剪纸（图案固定不变）
+ `body` = 你的手（会转动、移动）
+ `geom` = 胶水，把剪纸贴在你的手上。

手 (body) 挥舞转动，剪纸 (mesh) 图案本身不会变形，但是因为贴在手上，剪纸的**空间位置跟着手到处跑**。



## MuJoCo 网格内存结构示意图
三个核心数组：`mesh_vertadr`、`mesh_vertnum`、`mesh_vert`，演示如何变换成 N×3 顶点矩阵。

假设场景：一共有 2 个 mesh 网格

+ mesh_id=0：爪 A，**4 个顶点**
+ mesh_id=1：爪 B，**2 个顶点**

###  mesh_vertadr 数组（起始偏移，一维 int 数组）
下标 = mesh_id；值 = 在`mesh_vert`大数组里的**浮点数起始下标**

假设场景：一共有 2 个 mesh 网格

+ **mesh_id=0：爪 A，4 个顶点**
+ **mesh_id=1：爪 B，2 个顶点**



| index（=mesh_id） | 0 | 1 |
| --- | --- | --- |
| value（起始浮点数下标） | 0 | 12 |




###  mesh_vertnum  结构
| index（=mesh_id） | 0 | 1 |
| --- | --- | --- |
| value（顶点数量） | 4 | 2 |


###  **mesh_vert 一维数组排布 ** 
| 数组下标 | 数值 | 归属 |
| --- | --- | --- |
| 0 | x0 | mesh_id=0 |
| 1 | y0 | mesh_id=0 |
| 2 | z0 | mesh_id=0 |
| 3 | x1 | mesh_id=0 |
| 4 | y1 | mesh_id=0 |
| 5 | z1 | mesh_id=0 |
| 6 | x2 | mesh_id=0 |
| 7 | y2 | mesh_id=0 |
| 8 | z2 | mesh_id=0 |
| 9 | x3 | mesh_id=0 |
| 10 | y3 | mesh_id=0 |
| 11 | z3 | mesh_id=0 |
| 12 | x4 | mesh_id=1 |
| 13 | y4 | mesh_id=1 |
| 14 | z4 | mesh_id=1 |
| 15 | x5 | mesh_id=1 |
| 16 | y5 | mesh_id=1 |
| 17 | z5 | mesh_id=1 |


**整体执行流程**

```plain
输入 mesh_id = 0
     ↓
mesh_vertadr[0] → a = 0
mesh_vertnum[0] → n = 4
     ↓
切片 mesh_vert[ 0 : 0 + 4*3 ] → [x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3]
     ↓
reshape(-1, 3)
     ↓
┌────┬────┬────┐
│ x0 │ y0 │ z0 │
│ x1 │ y1 │ z1 │
│ x2 │ y2 │ z2 │
│ x3 │ y3 │ z3 │
└────┴────┴────┘
4行 × 3列 顶点矩阵（STL局部坐标）
```



### 局部坐标和世界坐标的转换


```python
def mesh_world(model, data, mesh_name):
    """某网格所有顶点的世界坐标（用 geom 世界位姿，含 geom 自身 pos/quat 偏移）。"""
    # 遍历仿真里面每一个geom（碰撞/外观几何体）
    for g in range(model.ngeom):
        # 判断这个geom是不是mesh网格类型(stl模型)
        if model.geom(g).type == mujoco.mjtGeom.mjGEOM_MESH:
            # 判断：这个geom引用的mesh名字是否等于我们要找的mesh_name
            if model.mesh(model.geom(g).dataid).name == mesh_name:
                # mid = mesh的编号mesh_id
                mid = model.geom(g).dataid
                # v：调用mesh_block，拿到【STL局部坐标系顶点 N×3】
                v = mesh_block(model, mid)
                # b：geom当前的世界坐标原点（平移向量 shape(3,)）
                b = data.geom_xpos[g]
                # R：geom当前的世界旋转矩阵，reshape成3×3旋转矩阵
                R = data.geom_xmat[g].reshape(3, 3)
                # 局部点 先旋转，再平移 → 全部转为世界坐标，返回N×3数组
                return b + v @ R.T
    # 循环跑完没找到匹配mesh_name，返回None
    return None
```



#### 核心概念区分（最重要）
1. `v = mesh_block(model, mid)`：N×3 数组，**STL 文件导出时自带的局部坐标系顶点**。 这个坐标是**固定写在 STL 文件里面**，不知道现在在仿真场景摆在哪里，没有跟随机械臂运动。
2. `data.geom_xpos[g]` → `geom_xpos` 在 `data` 对象！！是**每一帧动态变化**。 代表这个 geom（爪指网格）此刻在**仿真世界的原点位置（平移）**。

`model`：静态不变（xml、stl 原始数据） 

`data`：每帧更新动态状态（位置、角度、速度）

3. `data.geom_xmat[g]` → `R`：geom 的世界旋转矩阵，平铺 9 个数字，所以要`reshape(3,3)`变成 3×3 旋转矩阵。 代表：这个 geom 现在旋转到什么姿态。

#### 坐标变换公式

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789005921872-09a754ae-2888-4679-ba9b-5cd79b750390.png)

整体流程

```plain
输入 mesh_name（网格名字字符串）
        ↓
遍历全部geom(g=0,1,2...)
        ↓
判断 geom是不是MESH类型 && mesh名字匹配
        ↓
mid = geom.dataid (mesh_id)
        ↓
v = mesh_block(model, mid) → STL局部顶点(N×3)【固定不变】
        ↓
b = data.geom_xpos[g] → geom世界平移【每帧变化】
R = data.geom_xmat[g].reshape(3,3) → geom世界旋转【每帧变化】
        ↓
world_vertex =  v @ R.T + b
        ↓
return world_vertex  全部顶点的世界坐标 N×3
```

**举例**：我们要找爪指网格`moving_jaw_so101_v1`，就传这个字符串进来，循环匹配找到对应的 geom 和 mid。

```python
mid = model.geom(g).dataid
v = mesh_block(model, mid)
```

`mid`就是前面讲的`mesh_id`；调用`mesh_block`取出**STL 局部 N×3 顶点**。

```python
b = data.geom_xpos[g]
R = data.geom_xmat[g].reshape(3, 3)
```

+ `data.geom_xpos[g]`：**每一帧动态**，这个 geom 现在世界空间的原点坐标；机械臂动，这个值跟着变。
+ `data.geom_xmat[g]`：原始是长度为 9 的一维数组，`reshape(3,3)`得到标准 3×3 旋转矩阵。

```python
return b + v @ R.T
```

`v @ R.T`：N×3 乘以 3×3，所有局部点做旋转变换。 再加上平移向量`b`，输出**全部顶点的世界坐标（N 行 3 列）**。

为什么是`R.T`转置？MuJoCo 存储的旋转矩阵是**物体到世界矩阵**，numpy 矩阵乘法需要取转置，才能把局部点变换到世界坐标系。



## 如何获取夹爪中心点


```python
def finger_centroid(model, data):
    """
    计算两片爪指的几何中心点（世界坐标），用作抓取参考点
    逻辑：读取两个爪mesh全部顶点世界坐标，垂直堆叠点云，对x/y/z分别求平均
    注意：是顶点几何平均，不是物理质心，工程上快速估算抓取位置
    返回：一维数组 [center_x, center_y, center_z]
    """
    # 遍历两个爪mesh，获取各自全部顶点世界坐标，存入列表parts
    parts = [mesh_world(model, data, nm) for nm in JAW_MESHES]
    # np.vstack垂直拼接两组点云，合并为 (N1+N2, 3) 大数组
    C = np.vstack(parts)
    # axis=0：沿着所有行求均值，每一列(x,y,z)分别计算平均值
    return C.mean(0)

```

### 原理
`JAW_MESHES = ["moving_jaw_so101_v1", "wrist_roll_follower_so101_v1"]` 就是左右两片夹爪的 mesh 名字列表。

逐行：

```python
# 列表推导式：循环每个爪mesh名称，调用mesh_world拿到【该爪全部顶点世界坐标 N×3】
parts = [mesh_world(model, data, nm) for nm in JAW_MESHES]
```

`parts`得到一个**长度 = 2 的列表**

+ parts [0]：活动爪所有顶点 N₁×3
+ parts [1]：固定爪所有顶点 N₂×3

```plain
C = np.vstack(parts)
```

`np.vstack`：垂直堆叠，把两组点云合并成**一整块大数组 **`(N₁+N₂, 3)`

所有两片爪的全部顶点放在一起。

```python
return C.mean(0)
```

`.mean(0)`：**沿着第 0 维（所有点）求平均** 输出 `[X,Y,Z]` 一维数组，就是**所有爪顶点的几何平均中心点**

****


C = [

[x1,y1,z1],

[x2,y2,z2],

[x3,y3,z3]

]

c.mean() = [(x1+x2+x3)/3,  (y1+y2+y3)/3, (z1+z2+z3)/3]

重点注释： 这是**几何中心点（点云平均），不是物理质心！** 物理质心要根据质量分布计算；这里只是取顶点平均，用来当作抓取位置，工程上简单好用。

一句话：把左右夹爪所有三角形顶点拿出来求平均，得到夹爪中间的抓取点（世界坐标 xyz）





## 设置关节角度
**功能：批量设置多个关节角度，然后执行正向计算更新 data 所有状态**

```python
def set_arm(data, model, q):
    """
    批量设置多个关节目标角度，执行正向运动学更新全部刚体状态
    参数 q:字典 {关节名:目标弧度值}
    1. 根据关节名字查找关节id
    2. 根据jnt_qposadr找到该关节在qpos一维数组中的偏移位置，赋值
    3. mj_forward：正向计算，刷新data内所有body、geom的xpos/xmat世界位姿
    ❗不调用mj_forward，后面读取的坐标、位姿全部是旧数据！
    """
    for name, v in q.items():
        # 根据名字查找关节对象id，指定查找类型为JOINT关节
        jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
        # jnt_qposadr：该关节在广义坐标qpos数组内的起始下标
        data.qpos[model.jnt_qposadr[jid]] = v
    # 正向运动学求解，更新所有body、geom的世界位置姿态
    mujoco.mj_forward(model, data)
```

****

### **参数 q 的含义**
入参`q`是字典，示例：

```python
q = {
    "shoulder_pan": 30,
    "shoulder_lift": -45,
    "elbow_flex": -90
}
```

key = 关节名字，value = 目标角度（和你 xml 里面 compiler 角度单位匹配，deg）

```python
jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
```

`mj_name2id`：**根据物体名字，查内部索引 id**

+ 类型：`mjOBJ_JOINT` 代表去【关节池】查找
+ name：关节字符串名字，例如`shoulder_pan`
+ 返回`jid`：这个关节在 model 里的编号



### **修改角度**
根据 jid 获取位置索引，再根据位置索引获取实际的位置值，通过 v 修改位置值

```python
data.qpos[model.jnt_qposadr[jid]] = v
```

+ `model.jnt_qposadr[jid]`：该关节在`qpos`一维数组里的**偏移下标**
+ `data.qpos`：MuJoCo 广义坐标数组，保存**所有关节当前位置（角度 / 位移）**

****

**model.jnt_qposadr 结构可以这么理解**

```python
model.jnt_qposadr = [0, 1, 2, 3, 4, 5]  # 固定写在model里
jid = 0
addr = model.jnt_qposadr[jid] # addr=0
data.qpos[addr] = 30
```



model 里很多关节全部挤在扁平一维数组 qpos，每个关节占用一段位置，jnt_qposadr 记录每个关节的起始位置。 这和前面`mesh_vertadr`思路一模一样！目录式索引。



### **运动到新的位置**
```python
mujoco.mj_forward(model, data)
```


**极其关键！** 修改完`data.qpos`只是手动改了关节位置数值，**MuJoCo 不会自动更新连杆、geom 的位置、旋转**。 调用`mj_forward()`执行**正向运动学**： 输入关节角度 qpos → 一次性计算所有 body、geom 的 xpos/xmat（世界位姿），更新到`data`。

不调用 mj_forward，后面调用`mesh_world`、`body_pose`拿到的位置还是旧的！

一句话：给一批关节赋值，然后触发正向计算，刷新整个机械臂所有刚体的空间位置。





##  齐次变换矩阵


> 一句话功能：输入 body 名字，返回这个刚体在世界坐标系下的 4×4 齐次变换矩阵 T。
>
> 常用于机械臂：拿到夹爪、末端连杆的世界位姿，做坐标变换、逆运动学、目标点计算。  
>

```python
def body_pose(model, data, name="gripper"):
    """
    获取指定body刚体的4×4齐次变换矩阵（世界坐标系下位姿）
    T = [[R, b],
         [0, 1]]
    R:3×3旋转矩阵；b:3维平移向量[x,y,z]
    输入body名称，返回4×4齐次矩阵，用于坐标变换
    """
    # 根据body名字查询body的id编号
    bid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_BODY, name)
    T = np.eye(4)  # 初始化4阶单位矩阵
    T[:3, :3] = data.xmat[bid].reshape(3, 3) # 左上角填入body旋转矩阵
    T[:3, 3] = data.xpos[bid]                # 第四列前3行填入body世界平移
    return T

```

### `根据名字获取 bodyid`
```python
bid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_BODY, name)
```

+ `name`：你在 XML 写的 body 名字，比如`gripper`、`pan_link`、`end_effector`
+ `mjOBJ_BODY`：限定**只在 body 列表查找**，避免 geom/joint 重名冲突
+ 返回`bid`：body 在 model 内部的整数编号（body id）

和前面关节的`jid`逻辑一模一样： 字符串名字 内部整数 ID 映射。



###  MuJoCo 两个核心状态：`data.xpos` 和 `data.xmat`
 注意：**这两个数据必须在 mj_forward () 之后才会更新成最新值**！ 

如果调用 `set_arm` 之后没跑`mj_forward`，`xpos/xmat`还是旧姿态！

### `data.xpos[bid]`
+ 形状：`(3,)` 的数组 `[x, y, z]`
+ 含义：**body 原点在世界坐标系的三维坐标（单位 m）**
+ 对应齐次矩阵里的平移向量 `b`

### `data.xmat[bid]`
+ MuJoCo 存储格式：**一维扁平数组，长度 9**`[R00,R01,R02, R10,R11,R12, R20,R21,R22]`
+ 代表 3×3 旋转矩阵 R，描述 body 在世界坐标系的朝向
+ `.reshape(3,3)`：把长度为 9 的一维数组重塑成标准 3 行 3 列旋转矩阵

### 齐次矩阵构造

### ![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789031121401-5764edb9-9370-42f1-b943-ee793bf5bd67.png)

```python
T = np.eye(4)        # 4x4单位矩阵，默认对角线全1，其余0
T[:3, :3] = data.xmat[bid].reshape(3, 3) # 左上角3×3：旋转R
T[:3, 3] = data.xpos[bid]                # 第4列前3行：平移b
```

 用来**坐标变换**：把 body 局部坐标系上的点，转换到世界坐标系。

 设 `p_local = [px, py, pz, 1]`（局部齐次坐标）  

```python
p_world = T @ p_local
```




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789033372133-a180eb9e-b9b4-43c4-96d9-da6ceb159686.png)

矩阵相乘效果


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789033518144-ff6a1559-5ee6-4bd2-abd3-0a7461bc002d.png)

一次矩阵相乘，相当于先旋转再相加
![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789033861529-fc059b9f-6db4-49c6-893e-4ea16b81c95e.png)



## 万向锁


万向锁不是机械卡死，**是欧拉角表示旋转时的数学奇异性**：当其中一个旋转轴转到和另一个旋转轴重合，三个独立旋转自由度退化，只剩下 2 个，丢失一个旋转方向。

先分清两个概念：

1. **万向节（Gimbal）**：物理装置，3 个互相嵌套的环，每个环绕一根轴旋转；
2. **欧拉角**：用三次绕固定 / 随体轴的旋转描述姿态，就是模仿万向节的旋转顺序。



可参考姿态解算网站模拟万向锁  [https://shice666.github.io/attitude-visualizer/](https://shice666.github.io/attitude-visualizer/)

### 物理模型直观理解
三个环：外环、中环、内环，对应三个旋转轴（比如：偏航 Yaw、俯仰 Pitch、滚转 Roll）。

+ 正常状态：三根轴互相正交，3 个独立自由度，可以任意朝向。
+ 当**中环旋转到 90°（或 - 90°）**，内环的旋转轴会和外环的旋转轴**重合到同一条直线**。 此时：
+ 转动内环，和转动外环，产生的是**同一个方向的旋转效果**；
+ 原本 3 个独立轴，现在只剩 2 个独立旋转；
+ 你失去了绕某一方向旋转的能力，这就是万向锁。

 关键点：**不是机械卡住不能转，是两个轴的旋转作用耦合在一起，无法独立控制某一个方向**。


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789091626492-c9851779-36c3-445d-8de9-c230741cc46a.png)

俯仰 90 °出现了万向锁现象


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789091568029-cb585223-9320-4e85-bec2-ac054e41c19e.png)



通俗理解  Z(偏航 Yaw)-Y(俯仰 Pitch)-X(滚转 Roll)

可以理解后面的旋转不会影响到前面的轴


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789097699512-23748c4c-0c3f-4d62-ba55-43cf5816acc6.png)

绕 y 轴旋转 90°后，导致 x 轴和 z 轴重合

绕 x 轴和 z 轴旋转，效果是一样的。

## 旋转如何实现
 通过旋转公式实现旋转，将 p 变为 p'


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094093562-1172be33-8170-4f4b-97a5-b295d47b32c3.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094074319-79065718-3a16-409e-b98d-093f3673084b.png)

### 列向量定义

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094120196-a6118a61-1508-40a9-8ac7-90e9b333deab.png)

### 单位矩阵

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094413276-c351d626-cf99-4028-8870-d61d0d7a0c0e.png)

###  绕 X 轴旋转 90°

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094505654-cd36cbdf-a662-4aaf-84ad-97933bf6a89f.png)

###  重点：矩阵乘法顺序 


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094638095-a12d5442-dbd9-41c5-bd7d-28117fdd6ba8.png)

###  和行向量写法对比 

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094760313-d14c1fa0-3ecd-48e3-9988-6c2d2af67438.png)

###  旋转矩阵的几何理解  


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094823741-f8adec87-d371-42e2-b9b7-d11239b6222b.png)

![image-20261108222628947](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261108222628947.png)



![image-20261108222635627](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261108222635627.png)



![image-20261108222648198](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261108222648198.png)



![image-20261108222700282](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261108222700282.png)



## 万象角的数学解释

### 数学基础

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789094966753-5fff904c-5248-4ab7-ae4e-7e3df49faa80.png)

###  三个基础旋转矩阵  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789095023754-6fe550b1-2e69-43dd-b724-ff3d097643cd.png)

###  初始状态  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789095051052-29219855-9381-43e5-9203-b8887cf54d71.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789095065085-5249adc1-697c-4deb-80ab-0c03afb5c575.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789095865437-6f7128db-5211-4906-82cd-be7b159a02b6.png)

  

单位矩阵。物体局部坐标系与世界坐标系完全重合，X/Y/Z三轴两两正交，3个独立旋转自由度，**无万向锁**。

###  进入奇点  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789095997889-d25c29ac-7209-48d6-91ca-eb737f13a813.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096024846-68768727-02f5-425f-83f5-8e063620bd51.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096280873-a485ad6c-1742-48ff-9cca-e6e85f7bbb45.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096387696-da9c4ea9-f744-46ae-adaf-7a7d1857f30d.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096451873-e41b65a3-dd5f-4a1e-99db-578c23463f5f.png)



###  万向锁核心结论  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096497272-e507f71e-15ad-43b3-9fa2-146acec557d6.png)



![image-20261109144909591](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261109144909591.png)



![image-20261109145000168](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261109145000168.png)

![image-20261109145038973](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261109145038973.png)



![image-20261109145043734](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261109145043734.png)

```markdown
注解：当Rz(α)Ry(β)Rx(γ)中的β=90°时，最后整个旋转姿态只依赖W，不是单独的依赖α和γ，所以你在绕y轴旋转的时候，无论怎么改变xz的坐标，它们只能朝一个维度运动。
如果β！=90°，你最终是能看到，绕y轴旋转时候，是能看到物体在xz轴两个方向上运动的。
```


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096563486-b1829939-eaa0-473e-9a48-3f011c0c7cc2.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096583786-f948696f-6ca1-4042-b802-7c976abceba4.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096625838-1c36818b-65dd-4c43-8830-ec882b4cf41e.png)

两个完全不同的欧拉角组合（偏航、滚转各自不一样），得到**完全相同的空间姿态**。

 数学意义：  


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789096656467-2903b76c-4230-4edc-929d-d4559944f3b3.png)

无法单独调整偏航而不改变滚转效果，两个动作耦合在了一起。

---

### 一句话总结

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789097384202-5106569e-ee66-4baf-9f46-d3b8fd37a21d.png)



## 四元数 (Quaternion)
###  定义
四元数是哈密顿提出的**超复数**，形式：


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789098707186-533ec667-4687-42be-9120-547d7fe0f726.png)

###  基本运算  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789098858777-c4d0f346-c993-4c00-a7f9-9e8f34afc855.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789099364013-4df20d9a-df4c-473f-8ed1-541677e2cc02.png)

### 对于四元数每一个分量的了解及其背后所在的含义

![image-20261109174848889](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261109174848889.png)





###   对比欧拉角 / 旋转矩阵  

| 特性 | 欧拉角 | 旋转矩阵 | 四元数 |
| --- | --- | --- | --- |
| 存储 | 3 个数 | 9 个数 | 4 个数 |
| 万向锁 | 存在 | 无 |  无 |
| 插值 | 困难 | 麻烦 | 球面线性插值 Slerp，平滑 |
| 归一化 | — | 重正交化 | 简单归一化 |
| 计算代价 | 低 | 高 | 中等 |


>  万向锁：两个旋转轴重合，丢失一个自由度，动画、相机、IMU 里很头疼。  
>


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789108676883-f2efa349-483d-4351-abe9-ffc6b64881f2.png)



###   四元数旋转矩阵  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789108707769-dfbf5816-fb40-42be-884a-4e6013bf4d0f.png)



###  工程常用场景
+ 游戏引擎 (Unity/Unreal)、机器人、无人机 IMU 姿态解算
+ 3D 动画骨骼旋转、相机姿态
+ 惯性导航，姿态估计



**代码很简单**

```python
import numpy as np

# 四元数乘法
def quat_mul(q1, q2):
    w1, x1, y1, z1 = q1
    w2, x2, y2, z2 = q2
    w = w1*w2 - x1*x2 - y1*y2 - z1*z2
    x = w1*x2 + x1*w2 + y1*z2 - z1*y2
    y = w1*y2 - x1*z2 + y1*w2 + z1*x2
    z = w1*z2 + x1*y2 - y1*x2 + z1*w2
    return np.array([w, x, y, z])

# 四元数共轭
def quat_conj(q):
    w, x, y, z = q
    return np.array([w, -x, -y, -z])

# 使用单位四元数旋转三维向量
def rotate_vec(q, v):
    p = np.array([0, v[0], v[1], v[2]]) # 转为纯四元数
    q_inv = quat_conj(q)
    p_new = quat_mul(quat_mul(q, p), q_inv)
    return p_new[1:]

# 绕z轴旋转90度
theta = np.pi / 2
q = np.array([np.cos(theta/2), 0, 0, np.sin(theta/2)])
v = np.array([1,0,0])
v_rot = rotate_vec(q, v)
print(v_rot) # [0, 1, 0]

```



## 旋转矩阵转换为 四元数
```python
def rot_to_quat(R):
    """
    3×3旋转矩阵 → 四元数(w,x,y,z)转换
    MuJoCo中freejoint的姿态存储使用四元数，用于滑块姿态更新

    算法说明：
        采用 Shepperd 算法（矩阵转四元数稳定版），避免数值奇异问题。
        直接根据旋转矩阵迹tr的大小，选择最大分量来计算，防止开根号出现精度损失。
        返回四元数格式：(w, x, y, z)，w为实部，(x,y,z)为虚部矢量。
        注意：输出为单位四元数，对应旋转。

    参数：
        R : np.ndarray, shape=(3,3)
            输入正交3x3旋转矩阵，行列式=1，无缩放。

    返回：
        w, x, y, z : float
            单位四元数，顺序：实部w，虚部x,y,z
    """
    # 计算旋转矩阵的迹 tr(R) = R[0,0]+R[1,1]+R[2,2]
    tr = R[0, 0] + R[1, 1] + R[2, 2]

    # 情况1：迹大于0，此时w分量最大，优先计算w，数值稳定性好
    if tr > 0:
        s = np.sqrt(tr + 1.0) * 2
        w = 0.25 * s
        x = (R[2, 1] - R[1, 2]) / s
        y = (R[0, 2] - R[2, 0]) / s
        z = (R[1, 0] - R[0, 1]) / s
    else:
        # 情况2：迹<=0，w比较小，找到矩阵对角元最大的下标i，优先计算对应的虚部分量
        i = int(np.argmax(np.diag(R)))
        j, k = (i + 1) % 3, (i + 2) % 3
        q = np.zeros(4)

        # 计算最大的那个虚部分量 q[i]
        q[i] = 0.5 * np.sqrt(max(0.0, 1 + R[i, i] - R[j, j] - R[k, k]))
        # 由已经算出的q[i]，反推剩余三个四元数分量
        q[0] = (R[k, j] - R[j, k]) / (4 * q[i])
        q[j] = (R[j, i] + R[i, j]) / (4 * q[i])
        q[k] = (R[k, i] + R[i, k]) / (4 * q[i])

        # 解包得到 w,x,y,z
        w, x, y, z = q[0], q[1], q[2], q[3]

    return w, x, y, z

```



### 算法思想

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789109370506-016a2754-3d21-429e-b447-b41c7a6b7fe8.png)


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789109496672-f331c448-5ec2-4839-aad0-b31c7a425000.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789109556217-7dd8d39a-8fe2-4054-89aa-34c8e661777f.png)

**感兴趣可以自己推导下**


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789109808081-16e361a5-8de8-4e2c-a45b-276268b01fba.png)



## 根据关节名获取姿态信息
```python
def slider_qadr(model):
    """
    获取red_slider自由关节在qpos数组内的起始偏移下标
    freejoint占用qpos连续7个位置：[x,y,z, qw,qx,qy,qz] 平移+四元数姿态
    """
    return model.jnt_qposadr[mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, "red_slider")]

```

### 前置背景
MuJoCo 内部把**所有关节的位置状态**，全部打包进**一维数组 **`**qpos**`：

+ `qpos`：广义坐标，保存所有关节的位置 / 姿态
+ 不同关节占用 `qpos` 的长度不一样： 
    - 普通铰链关节 (revolute)：占 **1 个浮点数**（旋转角度）
    - 滑动关节 (prismatic)：占 **1 个浮点数**（平移距离）
    - **freejoint（自由关节，漂浮刚体）：连续占 7 个数**`[x, y, z, qw, qx, qy, qz]`
        * `x,y,z`：刚体质心的世界坐标平移
        * `qw,qx,qy,qz`：单位四元数，表示刚体姿态（和我们前面 `rot_to_quat` 的顺序完全一致！）

所以：一个 freejoint 在 qpos 中是**连续 7 个元素**，我们需要拿到它**第一个元素的索引**，也就是**起始偏移地址 **`**adr**`**（address）**

****

### **函数内部两步拆解**
```python
mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, "red_slider")
```

1. `**mj_name2id**`**：名字 → ID**
    - **输入：模型、对象类型、字符串名字**`**"red_slider"**`
    - **作用：在 **`**model**`** 里面，查找名字叫 **`**red_slider**`** 的****关节****，返回它的全局整数 ID（joint id）**
    - `**mjtObj.mjOBJ_JOINT**`**：告诉函数，我们要查找的对象类型是****关节 Joint****，不是 body/geom/mesh 等别的东西**

```python
model.jnt_qposadr[ joint_id ]
```

2. `**model.jnt_qposadr**`** 数组：关节 ID → qpos 起始下标**
    - `**jnt_qposadr**`** 全称：****joint qpos address**
    - **它是 MuJoCo 模型预计算好的数组：**`**jnt_qposadr[jid]**`** = 编号为**`**jid**`**的关节，在一维 qpos 数组里的****起始索引**
    - **返回值就是我们要的：**`**red_slider**`**这个 freejoint 在 qpos 里的起始下标。**

### **使用示例**
**假设函数返回 **`**adr = 12**`**，代表 red_slider 的 qpos 从下标 12 开始：**

```python
qpos[12] → x
qpos[13] → y
qpos[14] → z
qpos[15] → qw
qpos[16] → qx
qpos[17] → qy
qpos[18] → qz
```

 读取姿态四元数：  

```python
adr = slider_qadr(model)
qw,qx,qy,qz = data.qpos[adr+3], data.qpos[adr+4], data.qpos[adr+5], data.qpos[adr+6]
```

:::tips
 刚好接上`rot_to_quat`，旋转矩阵转出来的`w,x,y,z`，就可以直接赋值到 `data.qpos[adr+3 ~ adr+6]`。

:::

**写入新姿态（更新滑块姿态）：**

```python
# R是3×3旋转矩阵
w,x,y,z = rot_to_quat(R)
adr = slider_qadr(model)
data.qpos[adr+3] = w
data.qpos[adr+4] = x
data.qpos[adr+5] = y
data.qpos[adr+6] = z
```

### 关键点总结
1. `jnt_qposadr`：MuJoCo 预生成的「关节 ID → qpos 起始位置」查找表
2. `mj_name2id`：通过 xml 里写的关节名字，查到关节数字 ID
3. 函数最终返回**freejoint 在 qpos 一维数组的起始偏移**
4. freejoint 固定占用 7 个连续位置：`[x,y,z,qw,qx,qy,qz]`



## 滑块放置逻辑
```python
def place_slider(data, qadr, pos):
    """
    设置红色滑块在地面的位置，姿态重置为无旋转
    参数 pos：(x,y,z) 世界坐标，z=0放在地面
    qadr：滑块freejoint在qpos的起始偏移
    """
    data.qpos[qadr:qadr + 3] = pos
    # 四元数 [1,0,0,0] 代表无任何旋转
    data.qpos[qadr + 3:qadr + 7] = [1.0, 0.0, 0.0, 0.0]
```

### **前置回顾：**
:::info
freejoint 在 qpos 里一共 7 个连续浮点数： `[x, y, z, qw, qx, qy, qz]`

+ `qadr`：起始索引（就是前面 `slider_qadr(model)` 返回的值）
+ `qadr+0 ~ qadr+2`：平移 x,y,z（3 个数）
+ `qadr+3 ~ qadr+6`：四元数 qw,qx,qy,qz（4 个数） Python 切片规则：`a[start:end]` 是**左闭右开**，`end`下标不包含。

:::

### `data.qpos[qadr:qadr + 3] = pos`
切片 `qadr : qadr+3`

+ 包含下标：`qadr`，`qadr+1`，`qadr+2`
+ 正好对应 freejoint 的**平移 x,y,z**
+ `pos` 是传入的 `(x,y,z)` 三元组，直接覆盖这 3 个位置。

举例：qadr=12 `data.qpos[12:15]` → 索引 12、13、14 → x,y,z

### `data.qpos[qadr + 3:qadr + 7] = [1.0, 0.0, 0.0, 0.0]`
切片 `qadr+3 : qadr+7`

+ 包含下标：`qadr+3`、`qadr+4`、`qadr+5`、`qadr+6`
+ 正好对应四元数：`qw,qx,qy,qz`

### 为什么 `[1,0,0,0]` 是无旋转？
单位四元数：


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789112637030-75f2003d-9b98-4da2-97fe-53008e63ec7c.png)





![四元数旋转物理意义信息图](C:\Users\MZC228~1\AppData\Local\Temp\vmware-mzc228699\VMwareDnD\ac8ba4fe\四元数旋转物理意义信息图.png)











### 完整例子（代入数字直观感受）

假设 `qadr = 12`

```python
# x,y,z 写入 12,13,14
data.qpos[12:15] = [2, 0, 0]
# qw,qx,qy,qz 写入15,16,17,18
data.qpos[15:19] = [1.0, 0.0, 0.0, 0.0]
```

结果：

+ `qpos[12]=2, qpos[13]=0, qpos[14]=0` 滑块放在世界坐标 (2,0,0)
+ `qpos[15]=1, qpos[16]=0, qpos[17]=0, qpos[18]=0` 姿态完全不旋转

### 函数作用总结
`place_slider` 就是**重置自由刚体位姿的工具函数**：

1. 放到世界坐标 `pos`；
2. 姿态归零，不做任何旋转； 适合仿真重置、reset 环境时初始化滑块位置。



## 平滑曲线运动
`ease(t)`：**平滑缓动曲线（Smoothstep，三阶）**，把匀速的输入 `t` 变成「慢起→中间快→慢停」的运动，消除硬启停冲击。  

```python
def ease(t):
    """缓动插值函数，0<=t<=1；平滑启停，避免机械臂运动生硬、突变冲击"""
    return t * t * (3 - 2 * t)

```

### t 的含义
`t` 是**归一化的进度变量**，范围固定：
![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789113543097-fcbd15d1-7e40-42c5-9990-a890e10d30dc.png)，代表**运动走到了百分之多少**。




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789113303485-2642e0ef-ebd0-4291-9e7a-77338d8236bb.png)




![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789113378076-72f01be1-6d12-48d2-b74a-550cc8a53e4c.png)



```python
ease_t = ease(t)
pos_now = pos0 + (pos1-pos0)*ease_t
```


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789113423843-0efd04c3-a76a-479d-a2b0-a82e9d59b7f6.png)



##  lerp_q 关节字典线性插值  
>  核心：**Lerp = Linear Interpolation 线性插值**，机器人关节里最常用的平滑过渡算法。  
>

```python
def lerp_q(q0, q1, t):
    """
    关节字典线性插值
    q0：起始关节字典，q1目标关节字典，t∈[0,1]
    返回插值后的关节状态字典，实现平滑过渡
    """
    return {k: q0[k] + (q1[k] - q0[k]) * t for k in q0}

```

###  公式拆解  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789208831317-4db4aae1-67d5-41e9-a247-a73df9bb556b.png)

###  字典推导式  
`q0` 是**关节名称：关节值**的字典，比如机械臂例子：  



```python
q0 = {"joint1":0.0, "joint2":1.57, "joint3":0}
q1 = {"joint1":1.0, "joint2":0, "joint3":0.785}
```

`for k in q0`：遍历起始字典`q0`里所有关节名字`k`

 这里有个隐含前提：**q0 和 q1 必须拥有完全相同的键（关节名称一模一样）** 如果 q0 有某个关节，q1 没有 → 执行 `q1[k]` 会直接报 KeyError！

###  举个简单数值例子  
```python
q0 = {"j1": 0}
q1 = {"j1": 10}
print(lerp_q(q0, q1, 0))   # t=0 → {"j1":0}
print(lerp_q(q0, q1, 0.3)) # t=0.3 → 0 + (10-0)*0.3 = 3
print(lerp_q(q0, q1, 0.8)) # t=0.8 → 8
print(lerp_q(q0, q1, 1))   # t=1 → 10

```

 t 随时间不断增加（比如每帧 `t += 0.01`），就能生成连续的中间关节姿态，给机械臂做轨迹平滑。  

### 总结

这个函数对**每一个关节独立做线性插值**，输入起始姿态字典、目标姿态字典和进度 t，算出中间的整套关节姿态，用来实现机械臂从当前姿态平滑匀速运动到目标姿态。





## 计算滑块底部最低点坐标
>  一句话总述： 拿到滑块刚体的**位置 + 四元数**，算出盒子 8 个角点在**世界坐标系下的 Z 高度**，返回**8 个点里最低的那个 Z 值**；用这个最小值判断滑块底部有没有插到地面（地面一般 Z=0，`min_z<0` 就代表穿透地面）。  
>

```python
def slider_bottom_min(data, qadr, half):
    """
    计算红色滑块8个角点中，最低的Z坐标（世界坐标系）
    用来做触地检测：判断滑块底部有没有扎进地面
    原理：取出滑块位置+四元数转旋转矩阵，遍历box8个顶点，求最小z
    half：滑块半尺寸
    """
    qp = data.qpos[qadr:qadr + 7]
    w, x, y, z = qp[3:]
    # 四元数转3×3旋转矩阵
    R = np.array([[1 - 2*(y*y + z*z), 2*(x*y - z*w), 2*(x*z + y*w)],
                  [2*(x*y + z*w), 1 - 2*(x*x + z*z), 2*(y*z - x*w)],
                  [2*(x*z - y*w), 2*(y*z + x*w), 1 - 2*(x*x + y*y)]])
    hx, hy, hz = half
    m = 1e9
    # 遍历box方块8个顶点，计算每个角点世界z坐标，找最小值
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (0.0, 2*hz):
                m = min(m, float((qp[:3] + R @ np.array([sx*hx, sy*hy, sz]))[2]))
    return m
```

### MuJoCo qpos 基础背景
MuJoCo 中**一个自由刚体（free joint）** 在 `qpos` 里占 **7 个数字**： `[px, py, pz, qw, qx, qy, qz]`

+ `qp[:3]` → `px,py,pz`：刚体**本体原点**在世界坐标系的位置
+ `qp[3:]` → `w,x,y,z`：四元数，描述刚体自身姿态旋转

`qadr` 就是这个滑块刚体在 qpos 数组里的起始索引。



```python
qp = data.qpos[qadr:qadr + 7]
w, x, y, z = qp[3:]

```

 截取滑块的位姿（位置 + 四元数）。  



####   在这里需要知道，使用欧拉角的旋转矩阵进行计算时，即Rz(α)Ry(β)Rx(γ)时，如果β=90°，会出现万向锁，这里我在课上的随堂笔记的记录文档里面已经进行推导和注释过了。如何解决这种运动时带来的问题？只需要使用四元数解决即可，四元数的数学原理太难了，暂时不用理解，只需要知道怎么计算就行，就目前而看。

###  四元数 → 旋转矩阵 R  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789216455454-0186a600-4a56-4858-8938-25f3aca6e54a.png)

###  half = (hx, hy, hz) 半尺寸  

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789216574318-9f73bf5b-a8a3-4086-a476-0177650b1656.png)

```plain
for sx in (-1, 1):
    for sy in (-1, 1):
        for sz in (0.0, 2*hz):
```


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789216634984-92a087f3-327f-4888-a86a-ec3cabf60d17.png)

###  8 个角点求世界 Z，保留最小 Z  
```python
m = 1e9  # 初始设成一个很大的值
# 坐标变换：世界点 = 刚体原点 + 旋转矩阵 × 局部点
point_world = qp[:3] + R @ np.array([sx*hx, sy*hy, sz])
z_world = point_world[2]
m = min(m, z_world)

```


![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789216673776-a0e856a1-6e5e-423b-b5bf-180284891743.png)

### 触地检测逻辑
假设地面在世界 Z=0

+ `m > 0`：盒子所有角点都在地面上方，没有接触
+ `m ≈ 0`：最低点贴地，刚好接触
+ `m < 0`：**最低角点已经穿透地面，盒子扎进地里了**

****

## 缓存网格 mesh 信息
>  整体目的：**预缓存夹爪 mesh 的 geom 编号 + STL 原始顶点**，给后续逐帧计算夹爪最低点、做防穿地检测用。 对比前面的`slider_bottom_min`：滑块是简单 box，可以手算 8 个角点；夹爪是 STL 网格 mesh，顶点成千上万，**不能手写 8 顶点，需要读取 mesh 真实顶点**。  
>



```python
def jaw_cache(model):
    """
    预缓存两片夹爪geom的编号和STL局部顶点数组，供每帧快速计算最低点
    返回：[(geom_id, 顶点数组N×3), ...]（visual与collision共用同一mesh，取其一即可）
    """
    cache = []
    seen = set()
    for g in range(model.ngeom):
        if model.geom(g).type == mujoco.mjtGeom.mjGEOM_MESH:
            nm = model.mesh(model.geom(g).dataid).name
            if nm in JAW_MESHES and nm not in seen:
                seen.add(nm)
                cache.append((g, mesh_block(model, model.geom(g).dataid).copy()))
    return cache

```

###  背景知识
 SO‑101 夹爪是 STL 网格 (`mjGEOM_MESH`)，不是简单 box：

1. 同一个 STL mesh，会被定义两份 geom：一份`class="visual"`用于渲染显示；一份`class="collision"`用于碰撞检测。**它们指向同一个 mesh 数据**，顶点完全一样。
2. 如果不缓存：每帧都去读取 mesh 顶点，会重复 IO / 内存读取，速度慢。
3. `jaw_cache`只在**仿真初始化运行一次**；之后每帧直接用缓存，不用反复读取 mesh。





## 计算夹爪底部坐标


>  这是上一步 `jaw_cache` 的配套运行时函数。 输入：模型`model`、仿真状态`data`、初始化得到的`cache`（`[(geom_id, mesh原始顶点N×3), ...]`） 输出：夹爪所有网格顶点在**世界坐标系**下最小的 Z 值；地面 z=0，返回值小于 0 代表夹爪扎进地面，用于防穿地保护。  
>

```python
def jaw_lowest_z(model, data, cache):
    """
    计算夹爪网格在世界坐标系下的最低点Z值（地面在z=0）
    顶点世界坐标 = geom平移b + 局部顶点v @ geom旋转R转置（与mesh_world一致）
    已验证与MuJoCo碰撞引擎(geomDistance对floor)结果一致
    """
    m = 1e9
    for g, v in cache:
        b = data.geom_xpos[g]
        R = data.geom_xmat[g].reshape(3, 3)
        m = min(m, float((b + v @ R.T)[:, 2].min()))
    return m
```

###   核心关键：`data.geom_xpos` / `data.geom_xmat`


MuJoCo `data` 里面直接给你算好了 **geom 在世界坐标系下的位姿**（不需要自己一层层乘 body、geom 的 pos/quat！）

+ `data.geom_xpos[g]`：shape (3,) → geom 原点的**世界坐标**（b）
+ `data.geom_xmat[g]`：shape (9,)，reshape 成`(3,3)`矩阵`R` → **geom 从 mesh 局部 → 世界坐标系的旋转矩阵**

重点： 之前我们手写变换链条：`mesh局部 → geom偏移旋转 → body局部 → body世界旋转平移 → 世界`。 现在 `geom_xpos`、`geom_xmat` 是 MuJoCo 仿真器**已经预计算好的最终世界变换结果**，直接拿来用，省去手动做多层坐标变换，代码简短，也不容易写错层级。

### 矩阵乘法顺序说明

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789347083881-59b714f7-2c30-4a47-a614-17e17c9744ad.png)



### 逐行拆解
```python
m = 1e9
```

初始化最小值为一个很大的数，后续不断更新为更小的 Z。

```python
for g, v in cache:
```

遍历缓存：`g=geom_id`，`v=N×3` mesh 原始顶点（STL 局部）。

```python
b = data.geom_xpos[g]
```

`b`：这个 geom 原点的世界位置，`(3,)`。

```plain
R = data.geom_xmat[g].reshape(3, 3)
```

`data.geom_xmat[g]` 是展平长度为 9 的数组，恢复为 3×3 旋转矩阵`R`。

```plain
world_verts = b + v @ R.T
```

`world_verts`：N×3，所有顶点变换到世界坐标系。

```plain
world_verts[:, 2].min()
```

取全部顶点的 Z 列，求最小值 —— 这一片夹爪网格的最低点世界 Z。

```plain
m = min(m, float(...))
```

对比更新全局最小 Z（多片 mesh 的时候，取所有夹爪里面最低那一个点）。

```plain
return m
```

返回所有夹爪顶点中最低的世界 Z。

## 运转机械臂并返回夹爪最低点离地高度
将机械臂运转到某个姿态，然后判断夹爪离地最低高度

```python
def clearance_of(model, data, cache, q):
    """临时把机械臂设到姿态q，返回夹爪最低点离地高度（开爪/合爪取更差值由调用方决定）"""
    set_arm(data, model, q)
    return jaw_lowest_z(model, data, cache)
```

通过仿真，不断插值，预判设定运行轨迹，保证夹爪上下部分不要碰到地面

```python
def raise_until_clear(model, data, cache, q_ref, margin,
                      step=0.005, max_delta=0.60):
    """
    自动校准：把q_ref姿态逐步“抬高”，直到夹爪张开/合爪两种状态最低点
    都离地 ≥ margin，返回(校准后姿态, 抬高量描述str, 校准后余量float)
    原理：纯运动学搜索，每个关节正反方向都试（不同构型下“抬臂”方向不同），
    不依赖任何硬编码高度假设；换标定xml也能自适应
    """
    # 先量一下原姿态的离地余量（开爪、合爪都查，取更差值）
    c0 = min(clearance_of(model, data, cache, {**q_ref, "gripper": GRIP_OPEN}),
             clearance_of(model, data, cache, {**q_ref, "gripper": GRIP_CLOSE}))
    if c0 >= margin:
        return dict(q_ref), "无需校准", c0
    # 搜索计划：每个关节先试“更有希望”的方向，不行再试反方向
    plans = [("shoulder_lift", (-1, +1)), ("elbow_flex", (+1, -1)), ("wrist_flex", (+1, -1))]
    for joint, signs in plans:
        for sign in signs:
            for d in np.arange(step, max_delta, step):
                q = {**q_ref, joint: q_ref[joint] + sign * d}
                c = min(clearance_of(model, data, cache, {**q, "gripper": GRIP_OPEN}),
                        clearance_of(model, data, cache, {**q, "gripper": GRIP_CLOSE}))
                if c >= margin:
                    s = "-" if sign < 0 else "+"
                    return q, f"{joint} {s}{d:.3f} rad", c
    # 找不到就返回原姿态（调用方会打印警告，逐帧守卫仍会兜底）
    return dict(q_ref), "搜索失败(由逐帧守卫兜底)", c0

```

真实情况，用来做地面守护，也就是运行过程中实时监控

```python
def floor_guard(model, data, cache, q, margin, step=0.004, max_iter=200):
    """
    逐帧防穿地守卫：设置q后检查夹爪最低点，若低于margin则小步“抬臂”直到安全。
    方向自适应：先探测shoulder_lift ±两方向哪边离地更高往哪边抬（不同构型
    抬臂方向不同）；肩不行再试肘、腕。返回修正后的关节字典并已set_arm。
    这是最后一道防线：无论插值路径怎么走，画面上夹爪都不会插进地面
    """
    set_arm(data, model, q)
    if jaw_lowest_z(model, data, cache) >= margin:
        return q
    fixed = dict(q)
    for joint in ("shoulder_lift", "elbow_flex", "wrist_flex"):
        jr = joint_range(model, joint)
        lo, hi = (float(jr[0]), float(jr[1])) if jr is not None else (-np.inf, np.inf)
        # 方向探测：±3步各测一次离地高度，选更高的一侧
        probe = []
        for sign in (-1.0, +1.0):
            qq = {**fixed, joint: float(np.clip(fixed[joint] + sign * 3 * step, lo, hi))}
            set_arm(data, model, qq)
            probe.append((jaw_lowest_z(model, data, cache), sign))
        probe.sort(reverse=True)
        for _, sign in probe:
            tmp = dict(fixed)
            for _ in range(max_iter):
                nv = tmp[joint] + sign * step
                if nv < lo or nv > hi:
                    break  # 到关节限位，本方向失败
                tmp[joint] = nv
                set_arm(data, model, tmp)
                if jaw_lowest_z(model, data, cache) >= margin:
                    return tmp
            set_arm(data, model, fixed)  # 该方向失败，恢复原姿态再试另一方向/下一关节
    return fixed
```

两种对比

| 项目 | raise_until_clear（自动校准预抬高） | floor_guard（逐帧地板守卫） |
| --- | --- | --- |
| **执行时机** | **规划阶段，仿真步进之前**，生成轨迹点的时候调用；**不跑 mj_step** | **仿真循环内，每一次 mj_step 之后都执行，运行时实时** |
| 输入 | 目标关节姿态字典`q_ref` | 当前仿真真实状态`model, data` |
| 计算方式 | **纯运动学正解**，只算几何，不做物理仿真；批量评估开爪 / 合爪的夹爪最低点 | 读取`data.geom_xpos/xmat`<br/>得到**当前真实物理状态下夹爪最低点 Z** |
| 能干什么 | 修改**目标参考姿态**：把要去的目标点抬高，让规划出来的轨迹本身就尽量离地 | 检测**当前真实状态是否穿地**；一旦穿地，做保护（截断动作、限幅、报警、紧急回退） |
| 修改对象 | 修改**期望 / 目标 q（参考值）**，不干涉仿真当前真实 data.qpos | 监控**真实仿真状态 data**，对当前执行做保护干预 |
| 失败行为 | 找不到可行解：返回原始 q_ref，打日志提示「搜索失败，交给守卫兜底」 | 只要运行就一直生效；不管规划预校准成功失败，它永远在线 |
| 计算开销 | 低，每条轨迹点调用一次；不是每仿真步调用 | 每仿真步都跑；开销取决于 mesh 顶点数量 |
| 逻辑定位 | **第一道防线：尽量从源头生成安全的目标轨迹** | **第二道兜底防线：就算规划出危险目标，运行时也不让真的扎进地里** |




## 封装的回放类
```python
# ================= 3) 演示编排（状态机） =================
class PickPlace:
    """
    自动拾取放置状态机：A点夹取→搬运到B放下，B夹取搬运回A，循环往复
    原理：分多阶段phase，缓动插值控制机械臂关节；抓取时滑块绑定在夹爪坐标系跟随运动
    触地即放逻辑：下降阶段滑块底部碰到地面，自动释放，防止物体穿透地面
    """

    def __init__(self, model, data, q_hover, q_grasp_a, q_grasp_b, q_home, spots):
        self.model, self.data = model, data
        self.q_home = q_home
        self.q = dict(q_home)  # 当前关节角度状态
        self.q_hover = q_hover # 悬停姿态（抓取前抬起来的安全高度）
        self.q_grasp = [q_grasp_a, q_grasp_b] # A、B两点抓取姿态
        self.spots = spots # A,B两个地面放置点坐标
        self.qadr = slider_qadr(model) # 滑块freejoint在qpos中的偏移下标
        self.slider_half = SLIDER_HALF # 滑块半尺寸保存
        self.jaw_cache = jaw_cache(model) # 夹爪网格缓存，供防穿地守卫快速计算
        self.margin = FLOOR_MARGIN        # 夹爪离地安全余量

        U0 = q_hover
        U1 = {**q_hover, "shoulder_pan": PAN_SWING} # 旋转后的悬停姿态
        A, B = q_grasp_a, q_grasp_b
        # 动作阶段列表：(名称,目标关节q,目标夹爪角度,阶段持续时间,事件标记)
        leg = [
            ("desc_A", A, GRIP_OPEN, 2.6, "approach"),    # 靠近A点，下探准备抓取
            ("close_A", A, GRIP_CLOSE, 0.9, "grab"),       # 合爪抓取
            ("lift_A", U0, GRIP_CLOSE, 2.2, None),         # 抬起，离开地面
            ("pan_AB", U1, GRIP_CLOSE, 2.8, None),         # 基座旋转，移动到B上方
            ("down_B", B, GRIP_CLOSE, 2.2, "place"),       # 下降到B点，place标记：触地释放
            ("open_B", B, GRIP_OPEN, 0.9, None),           # 张开夹爪放下物体
            ("up_B", U1, GRIP_OPEN, 2.0, None),            # 抬升离开
            ("desc_B", B, GRIP_OPEN, 2.2, "approach"),     # 靠近B点准备抓取
            ("close_B", B, GRIP_CLOSE, 0.9, "grab"),       # B点合爪抓取
            ("lift_B", U1, GRIP_CLOSE, 2.0, None),         # B点抬起
            ("pan_BA", U0, GRIP_CLOSE, 2.8, None),         # 旋转回到A上方
            ("down_A", A, GRIP_CLOSE, 2.2, "place"),       # 下降放到A点
            ("open_A", A, GRIP_OPEN, 0.9, None),           # 张开夹爪释放
            ("up_A", U0, GRIP_OPEN, 2.0, None),            # 抬升离开
        ]
        self.phases = leg
        self.i = 0  # 当前阶段索引
        self.t = 0.0 # 当前阶段已经流逝的时间
        self.holding = False # 是否正在抓取、吸附滑块
        self.hold_T = None   # 抓取保存的齐次变换矩阵：滑块相对于gripper的相对位姿
        self.cur_spot = 0    # 当前滑块所在点位索引0:A，1:B

    def _release(self):
        """释放滑块：取消吸附，把滑块固定放置到地面点位，切换目标点位"""
        self.holding = False
        self.hold_T = None
        self.cur_spot = 1 - self.cur_spot
        place_slider(self.data, self.qadr, self.spots[self.cur_spot])
        mujoco.mj_forward(self.model, self.data)

    def update(self, dt):
        """
        每帧更新状态机，dt是单帧时间步长
        1. 时间累加，缓动插值生成当前目标关节角度
        2. set_arm驱动机械臂
        3. 如果处于holding抓取状态：跟随夹爪同步更新滑块位置姿态
        4. 检测触地，自动释放物体；阶段时间结束切换下一动作阶段
        """
        model, data = self.model, self.data
        q_target, g_target, dur, ev = self.phases[self.i][1:]
        q0, g0 = self.q, self.q["gripper"]

        self.t = min(self.t + dt, dur)
        t = ease(self.t / dur) # 归一化+缓动函数
        qm = lerp_q(q0, q_target, t) # 插值得到当前目标关节
        qm["gripper"] = g0 + (g_target - g0) * t # 夹爪角度单独插值
        # 逐帧防穿地守卫：若本帧姿态使夹爪低于安全余量，自动抬高肩关节修正
        qm = floor_guard(model, data, self.jaw_cache, qm, self.margin)
        self.q = qm

        if self.holding:
            # 抓取状态：滑块跟随gripper刚体运动
            T_box = body_pose(model, data, "gripper") @ self.hold_T
            # 更新滑块freejoint的平移和姿态
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
            mujoco.mj_forward(model, data)
            # place阶段触地检测：滑块底部低于阈值，立刻释放，防止扎进地面
            if ev == "place" and slider_bottom_min(data, self.qadr, self.slider_half) <= 0.0012:
                self._release()
                self.t = 0.0
                self.i = (self.i + 1) % len(self.phases)
                return
        else:
            # 未抓取，滑块固定放在地面点位
            place_slider(data, self.qadr, self.spots[self.cur_spot])
            mujoco.mj_forward(model, data)

        # 当前阶段时间跑完，切换下一阶段
        if self.t >= dur:
            if ev == "grab":
                # grab事件：抓取，记录滑块相对于夹爪的相对变换矩阵hold_T
                T_body = body_pose(model, data, "gripper")
                T_box = np.eye(4)
                T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
                self.hold_T = np.linalg.inv(T_body) @ T_box
                self.holding = True
            elif ev == "place":
                self._release()
            self.i = (self.i + 1) % len(self.phases)
            self.t = 0.0
```

### 整体架构：
1. **状态机 phase 列表**：把完整动作拆解成一串有序子动作（靠近、合爪、抬起、旋转、下降、张开……）
2. 每帧`update(dt)`：做**关节缓动插值**，驱动机械臂运动
3. 虚拟抓取：**不是物理碰撞抓取，是运动学绑定**；抓到物体后，方块直接跟随夹爪坐标系运动。
4. 两套保护：`floor_guard`夹爪防穿地；`slider_bottom_min`方块触地自动释放。
5. 事件标记：`grab`抓取、`place`放置，触发绑定 / 解绑逻辑。

注意：这个是**演示编排，不是  强化学习；是硬编码时序状态机**，每一段动作写死持续时间。



### phase 阶段说明
每个元组含义：`(name, q_target, gripper_target, duration, event)`

+ `q_target`：本阶段结束要到达的机械臂关节姿态；
+ `gripper_target`：本阶段结束夹爪角度；
+ `duration`：这个动作持续多少秒；
+ `event`事件标记： 
    - `"grab"`：本阶段结束，执行抓取逻辑，记录物体相对夹爪的变换矩阵，开启跟随；
    - `"place"`：下降放置阶段，**实时检测方块触地，触地就立刻释放，不用等时间跑完**；
    - `None`普通过渡阶段。

循环流程：A 抓 →抬→转 B→下降放 B → B 抓→抬→转回 A→下降放 A，周而复始。

### 关键成员变量
1. `self.q`：**插值出来的目标关节（期望 q），不是 data.qpos 真实仿真状态**；控制器会跟踪这个 q。
2. `holding / hold_T`：虚拟抓取核心。不是靠摩擦碰撞，抓取之后保存：**方块在夹爪局部坐标系下的位置姿态**；每一帧根据夹爪当前世界位姿重新反推方块世界坐标，直接写进`data.qpos`，实现方块跟着爪子跑。
3. `cur_spot`：记录方块应该落在 A 还是 B 点位。



### `_release()` 释放方块
+ 释放虚拟绑定；
+ 切换目标点位索引；
+ `place_slider`直接写 free‑joint qpos，强制把方块摆到地面 spot 坐标；
+ `mj_forward`必须调用，刷新`data.geom_xpos / geom_xmat`。



###  update 分步拆解
更新逻辑

```python
def update(self, dt):
    """
    每帧更新状态机，dt是单帧时间步长
    1. 时间累加，缓动插值生成当前目标关节角度
    2. set_arm驱动机械臂
    3. 如果处于holding抓取状态：跟随夹爪同步更新滑块位置姿态
    4. 检测触地，自动释放物体；阶段时间结束切换下一动作阶段
    """
    model, data = self.model, self.data
    # 取出当前阶段配置
    q_target, g_target, dur, ev = self.phases[self.i][1:]
    q0, g0 = self.q, self.q["gripper"]

    self.t = min(self.t + dt, dur)   # 阶段流逝时间，不超过总时长
    t = ease(self.t / dur)           # 归一化时间 [0,1] + 缓动函数，避免启停突变
    qm = lerp_q(q0, q_target, t)    # 关节插值，得到本帧期望机械臂姿态 qm
    qm["gripper"] = g0 + (g_target - g0) * t # 夹爪单独插值

    # -------- 运行时防穿地守卫 floor_guard --------
    # qm是期望目标姿态，如果会穿地，floor_guard会现场修正抬高肩关节
    qm = floor_guard(model, data, self.jaw_cache, qm, self.margin)
    self.q = qm   # 更新本帧期望目标q，交给控制器执行

    if self.holding:
        # ==========【虚拟抓取模式】方块跟随夹爪运动 ==========
        # body_pose(gripper) 获取夹爪body的4×4世界齐次变换矩阵
        T_box = body_pose(model, data, "gripper") @ self.hold_T
        # 把计算得到方块世界位姿直接写入free‑joint qpos
        data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
        data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
        mujoco.mj_forward(model, data)

        # place阶段：触地即释放！不需要等本阶段时间跑完
        # 调用slider_bottom_min计算滑块box在世界坐标系最低点Z
        if ev == "place" and slider_bottom_min(data, self.qadr, self.slider_half) <= 0.0012:
            self._release()          # 释放方块
            self.t = 0.0
            self.i = (self.i + 1) % len(self.phases) # 切下一阶段
            return  # 直接return，本帧后续逻辑不再执行

    else:
        # ========== 没有抓取：方块固定摆放在地面点位 ==========
        place_slider(data, self.qadr, self.spots[self.cur_spot])
        mujoco.mj_forward(model, data)

    # -------- 当前阶段时间跑完，正常切换下一阶段 --------
    if self.t >= dur:
        if ev == "grab":
            # grab事件：本阶段结束，执行【虚拟抓取】
            T_body = body_pose(model, data, "gripper") # 夹爪当前世界变换
            T_box = np.eye(4)
            T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy() # 方块当前世界位置
            # hold_T = gripper的逆变换 @ box世界矩阵：得到方块相对于夹爪的局部坐标系
            self.hold_T = np.linalg.inv(T_body) @ T_box
            self.holding = True
        elif ev == "place":
            # 如果place走完时间还没有触地，直接释放兜底
            self._release()
        # 切换阶段索引，循环
        self.i = (self.i + 1) % len(self.phases)
        self.t = 0.0

```



![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789350125118-24b210ae-be48-4b80-9dcc-13d16c1b237f.png)
## 根据运动比例转化当前弧度
```python
def joint_range(model, name):
    """查询指定关节的运动范围 [最小值，最大值]（弧度）"""
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
    if jid < 0:
        return None
    return model.jnt_range[jid].copy()


def norm_body_to_rad(v, lo, hi):
    """归一化值[-100,100] → 关节弧度，用于录制回放主臂关节数据"""
    return lo + (v + 100.0) / 200.0 * (hi - lo)


def norm_grip_to_rad(v, lo, hi):
    """夹爪归一化值[0,100] → 夹爪关节弧度"""
    return lo + v / 100.0 * (hi - lo)
```

## 回放类详解


> 一句话定位：**轨迹回放类，用来回放你通过遥操作拖动真实机械臂录制保存到 npz 的演示轨迹** 和前面硬编码时序状态机 `PickPlace` 形成对比：
>
> + PickPlace：代码写死固定动作时序（固定几秒做抓取、几秒下降）
> + ReplayDemo：**回放人手动拖动机械臂录制出来的真实轨迹**，时间、姿态完全复刻人手操作；附带红色滑块虚拟抓取逻辑，和 PickPlace 的虚拟抓取是同一套思路。
>

### 关节定义
```python
class ReplayDemo:
    """
    回放演示类：回放sim_teleop录制的npz轨迹文件
    原理：读取录制的归一化关节信号，转换为弧度，按时间插值驱动机械臂
    自动识别合爪/张爪：合爪吸附滑块跟随夹爪；张开时将滑块放置在当前夹爪位置
    录制轨迹来自真实主臂拖动，所以动作自然，不容易插地
    """
    JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex",
                   "wrist_flex", "wrist_roll", "gripper"]

```

### **init** 构造函数：加载轨迹、预处理、自动识别抓取片段  
```python
def __init__(self, model, data, path, speed=1.0, start_sec=0.0, len_sec=None,
             use_slider=True):
    z = np.load(path)
    traj = z["traj"]                         # traj数组(N,7): 时间t + 6个关节归一化值
    self.model, self.data = model, data
    self.use_slider = use_slider # 是否启用红色滑块模拟抓取
    self.speed = speed           # 回放速度倍率，0.5慢放、2倍快放
    self.holding = False
    self.hold_T = None
    self.last_spot = None                    # 滑块上一次落地保存的地面坐标
    self.qadr = slider_qadr(model) if use_slider else None
    t_abs = traj[:, 0] - traj[0, 0] # 把原始时间戳转为相对时间，起点归零

    rad_full = np.zeros((traj.shape[0], 6))
    # ========== 核心：归一值 → 弧度（调用你前面那3个工具函数！） ==========
    for c, name in enumerate(self.JOINT_NAMES):
        rg = joint_range(model, name) # 读取每个关节的弧度限位 [lo,hi]
        if rg is None:
            continue
        lo, hi = rg
        if name == "gripper":
            # 夹爪：0~100归一值转弧度
            rad_full[:, c] = [norm_grip_to_rad(v, lo, hi) for v in traj[:, 6]]
        else:
            # 手臂关节：-100~100归一值转弧度
            rad_full[:, c] = [norm_body_to_rad(v, lo, hi) for v in traj[:, c + 1]]
    grip_full = traj[:, 6] # 原始录制的夹爪归一值

    # 自动检测动作开始时间：找关节开始明显运动的时刻（给用户提示start参数）
    d = np.abs(np.diff(rad_full, axis=0)).max(axis=1)
    act = np.where(d > 0.004)[0]
    self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0

    # 根据 start_sec / len_sec 裁剪轨迹：只回放片段，不用跑完整长轨迹
    i0 = int(np.searchsorted(t_abs, start_sec, side="left"))
    i0 = max(0, min(i0, len(t_abs) - 1))
    if len_sec is None:
        i1 = len(t_abs) - 1
    else:
        i1 = int(np.searchsorted(t_abs, start_sec + len_sec, side="left"))
        i1 = max(i0 + 1, min(i1, len(t_abs) - 1))

    # 裁剪之后单独存一份轨迹，相对时间重新归零
    self.full_start = float(t_abs[i0])
    self.t = t_abs[i0:i1 + 1]
    self.t = self.t - self.t[0]
    self.duration = float(self.t[-1]) # 裁剪后总时长
    self.grip_norm = grip_full[i0:i1 + 1]
    self.rad = rad_full[i0:i1 + 1] # 已经换算成弧度的关节序列

    # ========= 自动识别抓取区间：根据夹爪归一值判断什么时候是“合爪抓取” =========
    self.holds = []
    self.hold_mask = np.zeros(len(self.t), dtype=bool)
    if self.use_slider:
        gmin, gmax = float(self.grip_norm.min()), float(self.grip_norm.max())
        # 自动计算开合阈值：夹爪数值低于thr就判定为【闭合抓取】
        thr = gmin + 0.30 * (gmax - gmin) if (gmax - gmin) > 8 else gmax
        held = self.grip_norm <= thr

        # 连续抓取片段提取，过滤太短的片段（至少>=5帧才认定为有效抓取，过滤抖动噪声）
        i = 0
        while i < len(held):
            if held[i]:
                j = i
                while j < len(held) and held[j]:
                    j += 1
                if j - i >= 5:
                    self.holds.append((i, j - 1))
                i = j
            else:
                i += 1
        if not self.holds:
            # 如果全程没有开合动作，默认整段轨迹都是抓取
            self.holds = [(0, len(self.t) - 1)]
        # hold_mask：bool数组，每一帧标记 True=抓取状态 False=松开
        for s, e in self.holds:
            self.hold_mask[s:e + 1] = True
        self._reset()

```



>  重点：**自动识别抓取区间，不用手动标记哪一段抓物体**。原理：录制时夹爪数值变小 = 合爪，代码自动分割连续抓取片段。 过滤掉小于 5 帧的抓取，目的是过滤人手遥操作带来短暂抖动误触发抓取。  
>



### 前置理解
`traj` 是 `(N,7)` 的数组： `traj[行] = [时间t, shoulder_pan, shoulder_lift, elbow_flex, wrist_flex, wrist_roll, gripper]`

+ 第 0 列：时间
+ 第 1~5 列：5 个手臂关节，归一值范围 `[-100,100]`
+ 第 6 列：gripper 夹爪，归一值范围 `[0,100]`

`JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex", "wrist_flex", "wrist_roll", "gripper"]` 一共 6 个关节，对应 `rad_full` 形状 `(N,6)`，用来存**转换成弧度之后的关节值**。



###  第一块：for 循环，归一值 → 弧度  
```python
for c, name in enumerate(self.JOINT_NAMES):
    rg = joint_range(model, name) # 读取每个关节弧度限位 [lo,hi]
    if rg is None:
        continue
    lo, hi = rg
    if name == "gripper":
        # 夹爪：0~100归一值转弧度
        rad_full[:, c] = [norm_grip_to_rad(v, lo, hi) for v in traj[:, 6]]
    else:
        # 手臂关节：-100~100归一值转弧度
        rad_full[:, c] = [norm_body_to_rad(v, lo, hi) for v in traj[:, c + 1]]

```

`enumerate(self.JOINT_NAMES)`： `c` 是索引 0~5，对应 `rad_full` 的第 c 列；`name` 关节名字。

1. `rg = joint_range(model, name)` 调用之前写的工具函数，从 MuJoCo 模型读取这个关节的最小、最大弧度 `[lo, hi]`。找不到关节返回 None 就跳过。
2. 分支判断
+ 如果是 `gripper`（c=5） 取 `traj[:,6]`（整列夹爪归一值），遍历每个 v，用`norm_grip_to_rad`把 0~100 映射到`[lo,hi]`弧度，写入`rad_full[:,5]`
+ 如果是手臂其余 5 个关节 `JOINT_NAMES[c]` 对应 traj 的第 `c+1` 列： 
    - c=0 shoulder_pan → traj[:,1]
    - c=1 shoulder_lift → traj[:,2]
    - c=2 elbow_flex → traj[:,3]
    - c=3 wrist_flex → traj[:,4]
    - c=4 wrist_roll → traj [:,5] 调用`norm_body_to_rad`：`[-100,100]`归一值 → 关节弧度，存入`rad_full[:,c]`

> 一句话：**一次性把录制文件里全部帧的归一化数值，批量换算成 MuJoCo 仿真能用的弧度，提前离线算好，回放时不用反复转换，提升效率**
>

```python
grip_full = traj[:, 6] # 原始录制的夹爪归一值
```

单独把夹爪那一列归一值拿出来，后面用来**自动识别抓取 / 松开**。



### 第二块：自动检测动作开始时间
```python
# 自动检测动作开始时间：找关节开始明显运动的时刻（给用户提示start参数）
d = np.abs(np.diff(rad_full, axis=0)).max(axis=1)
act = np.where(d > 0.004)[0]
self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0
```

****

1. `np.diff(rad_full, axis=0)`

****


`np.diff`：按行做差分，**相邻两帧做减法**，算出每一帧各个关节的弧度变化量。

rad_full [N,6] → diff 之后得到 (N-1,6)，每一行 = 当前帧弧度 − 上一帧弧度。



`np.diff(rad_full, axis=0)`：**沿着行方向做差分（行间）**，也就是「相邻帧之间相减」





2. `np.abs(...)`

取绝对值，只关心**变化大小，不管是正向转还是反向转**。



3. `.max(axis=1)`**在每一行内部，跨列取最大值**（一行里 6 个关节，挑变化最大的那个）

     

   `axis=1`：对每一行（每一帧的 6 个关节），取**最大变化量**。 输出一维数组`d`，长度 N-1： `d[i]` = 第 i 帧和 i-1 帧对比，6 个关节里面动得最多的那个关节的弧度变化。

d [i] 含义：这一时刻，机械臂最大转动幅度（rad）

先分清数组维度：`rad_full.shape = [N_frames, 6]`

+ 第 0 维 `axis=0`：**帧（行）**，N 行，一行 = 一帧 6 个关节弧度
+ 第 1 维 `axis=1`：**关节（列）**，6 列，一列是同一个关节全部时间序列





4. `act = np.where(d > 0.004)[0]`



`d` 是一维数组，`d[i]` = 第 i 个时间间隔，6 个关节里最大弧度变化量(按照列对比，找到变化最大的值作为这个间隔的值)。 例如：

```plain
d = [0.001, 0.002, 0.0005, 0.005, 0.006, 0.002]
```

`d > 0.004` 会生成**布尔数组（True/False）**：

```plain
[False, False, False, True, True, False]
```

True：这一帧关节运动幅度超过 0.004rad，判定为开始动作；False：静止。



`np.where(cond)` 返回一个**元组**：`(满足条件的下标数组, )`

```plain
np.where([False, False, False, True, True, False])
# 返回 (array([3, 4]), )
```

+ 元组第 0 项才是索引数组，所以代码加 `[0]`：

```plain
act = np.where(d > 0.004)[0]
# act = array([3,4])
```

 act 里面存放**所有满足条件的下标 i**：也就是所有发生明显运动的时间间隔编号。

**后面这一行**

```python
self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0
```

+ `act[0]`：**第一个满足条件的索引**，也就是**第一次关节出现明显运动的时间间隔下标**
+ `t_abs[act[0]]`：取出这个下标对应的时间（秒），作为动作起始提示时间
+ `if len(act) else 0.0` 保护：如果 act 是空数组（全程没有任何运动），start_hint=0




阈值 `0.004 rad`（很小，大概 0.23 度）。 找出所有**关节变化超过 0.004 弧度**的帧索引，存入 act 数组。

+ 录制轨迹开头经常有一段：人准备好、手还没拖动机械臂，手臂静止不动，d≈0
+ 当开始拖动，关节一动，d 就会超过 0.004，进入 act





5. `self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0`



+ `act[0]`：**第一次出现明显运动的那一帧下标**
+ `t_abs[act[0]]`：这一帧对应的相对时间（单位秒）
+ 赋值给 `start_hint`，作为**给用户的提示**：

这条轨迹真正开始动作的时间点，你启动回放的时候可以用`--start`直接填这个数值，跳过前面静止空片段。

+ 如果整条轨迹从头到尾一动不动（act 为空），start_hint=0



**举个直观例子**

> 假设录制轨迹前 2 秒人没动手，机械臂静止，所有关节弧度不变，`d≈0`； t=2.1s 时，你拖动手臂，肩关节开始转动，`d>0.004`。 那么`act[0]`就是这一帧，`start_hint = 2.1`。 控制台就可以提示用户：`建议--start=2.1`，直接跳过前面空白静止段。
>

**为什么要做这个？**

> 遥操作录制的时候，经常：
>
> 1. 先打开录制
> 2. 调整姿势、手放上去（几秒静止）
> 3. 再开始拖动示教动作
>
> 回放的时候不想播放前面那段一动不动的空白，代码自动帮你找到**动作正式启动的时间**，方便裁剪轨迹。
>

### 回放片段
```python
# 根据 start_sec / len_sec 裁剪轨迹：只回放片段，不用跑完整长轨迹
i0 = int(np.searchsorted(t_abs, start_sec, side="left"))
i0 = max(0, min(i0, len(t_abs) - 1))
if len_sec is None:
    i1 = len(t_abs) - 1
else:
    i1 = int(np.searchsorted(t_abs, start_sec + len_sec, side="left"))
    i1 = max(i0 + 1, min(i1, len(t_abs) - 1))

    # 裁剪之后单独存一份轨迹，相对时间重新归零
    self.full_start = float(t_abs[i0])
self.t = t_abs[i0:i1 + 1]
self.t = self.t - self.t[0]
self.duration = float(self.t[-1]) # 裁剪后总时长
self.grip_norm = grip_full[i0:i1 + 1]
self.rad = rad_full[i0:i1 + 1] # 已经换算成弧度的关节序列
```

这段代码：

> **根据用户输入的起始时间**`**start_sec**`**、回放时长**`**len_sec**`**，在原始时间数组**`**t_abs**`**里，找到对应的帧下标 i0、i1，把轨迹切片裁剪；裁剪完成后，把新轨迹的时间轴重新归零（新轨迹第 0 帧时间 = 0 秒），同时把弧度、夹爪归一值数组同步切片。**
>

前置前提： 

> `t_abs`：**原始整条轨迹的相对时间数组，单调递增**，`t_abs[0]=0`，`t_abs[1]`第 1 帧时间…… 
>
> `rad_full`：全部帧的关节弧度数组 (N,6) `grip_full`：全部帧夹爪归一值数组 (N,)
>

### 找到裁剪窗口【起点帧下标】
```python
i0 = int(np.searchsorted(t_abs, start_sec, side="left"))
i0 = max(0, min(i0, len(t_abs) - 1))
```


`np.searchsorted`：在**有序数组 t_abs**里，查找`start_sec`应该插入在哪个位置，保证数组仍然有序，返回这个位置下标。 

`side="left"`：目标值存在时，返回左侧位置。

举例子： `t_abs = [0, 1.2, 2.4, 3.1, 4.5, 6.0]` 用户输入`start_sec=2.0`

+ 2.0 插在 1.2 和 2.4 之间，searchsorted 返回`2` → `i0=2`，对应`t_abs[2]=2.4`

含义：**取大于等于 start_sec 的第一个帧，作为裁剪起点 i0**



第二行是边界保护：

```python
i0 = max(0, min(i0, len(t_abs) - 1))
```

+ 如果算出来 i0<0 → 强制变成 0（不能小于第一帧）
+ 如果 i0 超过最后一帧下标，强制限制为最后一帧，防止数组越界。

### 找到裁剪窗口【终点帧下标】
```python
if len_sec is None:
    i1 = len(t_abs) - 1
else:
    i1 = int(np.searchsorted(t_abs, start_sec + len_sec, side="left"))
    i1 = max(i0 + 1, min(i1, len(t_abs) - 1))
```

`len_sec`：用户指定**回放片段持续多少秒**。

+ 如果`len_sec=None`：不截断结尾，直接取整条轨迹最后一帧 `i1 = len(t_abs)-1`
+ 如果给了 len_sec：结束时刻 = `start_sec + len_sec`，同样用`searchsorted`找到结束时刻对应的帧下标 i1

边界保护：`max(i0+1, ...)`

保证终点下标 i1 一定**大于 i0**，防止用户输入的时长太短，i1<=i0，切片为空报错。

###  切片：截取 i0 ~ i1 所有帧
```python
self.full_start = float(t_abs[i0])
self.t = t_abs[i0:i1 + 1]
self.grip_norm = grip_full[i0:i1 + 1]
self.rad = rad_full[i0:i1 + 1]
```

Python 切片 `a[m:n]`**左闭右开**，取不到 n。 所以想要**包含 i1 这一帧**，需要写 `i0 : i1+1`。

+ `self.full_start`：原始大轨迹中，裁剪片段第一帧对应的原始时间（留着做日志 / 提示用）
+ `self.t`：裁剪后的时间数组
+ `self.grip_norm`：裁剪后的夹爪归一值
+ `self.rad`：裁剪后的关节弧度数组

### 时间轴重新归零
```python
self.t = self.t - self.t[0]
self.duration = float(self.t[-1])
```

原来裁剪后的`self.t`还是沿用原始大轨迹的时间，比如裁剪出来是`[2.4,3.1,4.5]`。

 减去首帧时间`self.t[0]=2.4`，得到新时间：`[0, 0.7, 2.1]` 

 **新回放片段的时间从 0 秒开始**，后面回放循环里的 qt（回放时间）就从 0 开始累加，逻辑更简单。

`self.duration = self.t[-1]`：裁剪片段总时长，回放代码判断什么时候播放结束就靠这个。



## 抓取片段识别
> **读取裁剪后的夹爪归一化序列 **`**grip_norm**`**，自动判断每一帧是「夹爪闭合抓取物体」还是「夹爪松开」；过滤掉短暂抖动误触发，把连续抓取区间保存下来，生成布尔掩码**`**hold_mask**`**（True 代表当前帧处于抓取状态），用来后面回放时控制红色滑块跟随 / 放置。**
>

```python
# 根据夹爪归一化数值，自动识别抓取片段（合爪段标记为hold_mask=True）
        self.holds = []
        self.hold_mask = np.zeros(len(self.t), dtype=bool)
        if self.use_slider:
            gmin, gmax = float(self.grip_norm.min()), float(self.grip_norm.max())
            thr = gmin + 0.30 * (gmax - gmin) if (gmax - gmin) > 8 else gmax
            held = self.grip_norm <= thr
            i = 0
            while i < len(held):
                if held[i]:
                    j = i
                    while j < len(held) and held[j]:
                        j += 1
                    if j - i >= 5:
                        self.holds.append((i, j - 1))
                    i = j
                else:
                    i += 1
            if not self.holds:
                # 如果没有识别到开合动作，全程视为抓取状态
                self.holds = [(0, len(self.t) - 1)]
            for s, e in self.holds:
                self.hold_mask[s:e + 1] = True
            self._reset()
```

****

前置： `self.grip_norm`：裁剪轨迹后，每一帧的夹爪归一值，**数值越小 = 夹爪闭得越紧；数值越大 = 夹爪张开**`self.t`：裁剪后轨迹的时间数组，`len(self.t)`就是总帧数

```python
self.holds = []
self.hold_mask = np.zeros(len(self.t), dtype=bool)
```

+ `self.holds = []`：用来存放**有效抓取片段**，每个元素是元组 `(start_frame_idx, end_frame_idx)`，代表从第几帧到第几帧是抓取
+ `hold_mask`：bool 数组，初始全部 False。`hold_mask[frame] = True` → 该帧抓取；False → 松开

```python
if self.use_slider:
```

只有开启红色滑块虚拟抓取功能，才执行这一段；不使用滑块直接跳过。

### 自动计算开合阈值 thr
```plain
gmin, gmax = float(self.grip_norm.min()), float(self.grip_norm.max())
thr = gmin + 0.30 * (gmax - gmin) if (gmax - gmin) > 8 else gmax
held = self.grip_norm <= thr
```

+ `gmin`：整条片段夹爪**最小归一值**（夹爪最紧）
+ `gmax`：整条片段夹爪**最大归一值**（夹爪最开）

逻辑：

1. 如果夹爪最大最小值差 >8：说明录制轨迹里面**既有张开、又有闭合动作**
    - 阈值 `thr = gmin + 0.3*(gmax-gmin)`

区间从最紧 gmin 往张开方向取 30% 位置作为分界线。 只要夹爪归一值 ≤ thr → 判定为**闭合抓取 held=True** 大于 thr → 判定为松开 held=False 也就是：靠近闭合端 30% 范围算作抓取；剩下 70% 算作张开

2. 如果 `gmax-gmin ≤8`：夹爪全程几乎没有开合变化，直接`thr = gmax`，全部判定 held=True（全程抓取）

`held = self.grip_norm <= thr` 生成一个 bool 一维数组，每一帧标记初步判定是否抓取（还没过滤抖动）

### while 循环：提取连续抓取片段，过滤短抖动
```python
i = 0
while i < len(held):
    if held[i]:
        j = i
        while j < len(held) and held[j]:
            j += 1
        # 从i到j-1 连续抓取
        if j - i >= 5:
            self.holds.append((i, j - 1))
        i = j
    else:
        i += 1
```

这段是**连续区间提取**（经典滑动找连续 True 块）

1. i 从头遍历每一帧
2. 遇到`held[i]=True`，说明抓到抓取片段起点`i`
3. j 从 i 开始往后走，直到`held[j]=False`，停止 
    - 区间：`[i , j-1]` 连续判定为抓取
4. `if j-i >=5`：**这个连续片段帧数≥5 帧才认定是有效抓取**

过滤人手遥操作时短暂抖动：比如偶然 1~2 帧夹爪数值瞬间变小，并不是真的抓取，直接丢弃，不加入`self.holds`

5. i 跳到 j，继续向后搜索下一段；如果 held [i]=False，i +=1 继续走

举个例子： held = [F,F,T,T,T,F,T,F,F,F]

+ 第一段 T 只有 3 帧，3<5 → 丢弃
+ 第二段 T 只有 1 帧，丢弃 → `self.holds`为空

目的：**消除噪声误触发，防止滑块疯狂乱抓乱放**

### 兜底逻辑：如果没找到任何有效抓取片段
```python
if not self.holds:
    # 如果没有识别到开合动作，全程视为抓取状态
    self.holds = [(0, len(self.t) - 1)]
```

如果遍历完，`self.holds`是空（没有满足≥5 帧的抓取区间），默认**整条轨迹全部是抓取**，元组`(0,最后一帧下标)`

### 填充 hold_mask 掩码
```python
for s, e in self.holds:
    self.hold_mask[s:e + 1] = True
```

遍历所有抓取区间 `(s,e)`，把 hold_mask 从 s 帧到 e 帧全部设置为 True

切片 `s:e+1`，因为 python 切片左闭右开，e 帧要包含进去，所以 + 1

## self._reset()
 调用重置函数，初始化红色滑块的初始放置位置、清空 holding 状态，为后面仿真回放循环做准备。  

```python

    def _reset(self):
        """回放循环重置，初始化滑块位置"""
        if not self.use_slider:
            return
        s = self.holds[0][0]
        self.last_spot = self._finger_spot(self.rad[s])
        self.holding = False
        self.hold_T = None
        place_slider(self.data, self.qadr, self.last_spot)
        mujoco.mj_forward(self.model, self.data)
```



## 通过插值获取六个关节角度
```python
def _row(self, qt):
    """根据当前回放时间qt，插值得到6个关节弧度值，返回一维数组"""
    return np.array([np.interp(qt, self.t, self.rad[:, c]) for c in range(6)])

```



> 前置变量回顾：
>
> + `self.t`：裁剪后轨迹的**时间数组（已经归零，单调递增）**，`shape = (N,)`
> + `self.rad`：裁剪后的关节弧度数组，`shape = (N,6)`，N 帧、6 个关节
> + `qt`：当前回放的**仿真时间（当前播放到第几秒）**，是一个浮点数，不一定刚好落在采样点上
>



### 核心作用
给定回放时刻 `qt`，**对 6 个关节，分别做一维线性插值**，得到这个瞬间平滑的 6 个关节角度，返回`[q0,q1,q2,q3,q4,q5]`一维数组。

你录制的轨迹是离散采样点（一帧一帧），仿真运行时间是连续的；qt 很可能落在两帧采样时间中间，需要插值拿到中间状态，让机械臂运动顺滑，不会卡顿跳变。

### 拆开看列表推导式

![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789358715034-b42f2d84-6d9e-4ca8-9783-44367808f6fc.png)

## 举个直观例子
```plain
self.t = [0, 0.7, 2.1]
# rad 3帧 × 2关节，简化只看2关节演示
self.rad = np.array([
    [0,    1.57],   # t=0
    [0.3,  1.40],   # t=0.7
    [0.8,  1.00]    # t=2.1
])
qt = 0.35  # 在0和0.7中间
c=0关节插值：0 + (0.3-0)*(0.35-0)/(0.7-0) = 0.15
c=1关节插值：1.57 + (1.40-1.57)*(0.35/0.7) = 1.485
输出：array([0.15, 1.485])
```

### 重要特性 np.interp
1. 如果 qt 超出时间范围（qt <self.t [0] 或者 qt>self.t [-1]）：直接**钳位**，取端点值，不会报错 
    - qt < 0 → 返回第 0 帧角度
    - qt > 2.1 → 返回最后一帧角度
2. 只能**线性插值**，和你最开始写的`lerp_q`数学完全一致！

### 和前面代码串联起来
+ 前面：`searchsorted`用来**找帧下标，划分区间**
+ 这里：`np.interp`用来**在两个采样帧之间做平滑插值**

****

## **根据夹爪中心点放置地面坐标**
```python
def _finger_spot(self, row):
    """输入一组关节弧度，驱动机械臂，返回夹爪中心点的地面放置位置(z=0)"""
    set_arm(self.data, self.model, dict(zip(self.JOINT_NAMES, row)))
    c = finger_centroid(self.model, self.data)
    return (float(c[0]), float(c[1]), 0.0)
```

### **整体一句话功能**
> **输入一套6 个关节弧度 **`**row**`**，把 Mujoco 仿真里机械臂设置到这个关节姿态；算出夹爪中心点在世界坐标系的 XY 坐标，强制把 Z 写死为 0，返回**`**(X,Y,0)**`**，也就是把夹爪当前中心点垂直投影到地面 z=0 的落地位置。**
>
> **这个函数是给回放逻辑用的：当夹爪要释放滑块时，滑块要放在夹爪中心点正下方地面上。**
>

****

## **回放功能的更新函数**
```python
    def update(self, dt, qt):
        """回放每帧更新逻辑，qt当前回放时间，dt时间步"""
        model, data = self.model, self.data
        row = self._row(qt)
        set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))
        if not self.use_slider:
            # 纯回放手臂，不加载滑块
            mujoco.mj_forward(model, data)
            return

        idx = int(np.searchsorted(self.t, qt, side="right") - 1)
        idx = max(0, min(idx, len(self.t) - 1))
        want_hold = bool(self.hold_mask[idx])

        # 状态切换：从松开→抓取，记录滑块相对夹爪的位姿
        if want_hold and not self.holding:
            T_body = body_pose(model, data, "gripper")
            T_box = np.eye(4)
            T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
            self.hold_T = np.linalg.inv(T_body) @ T_box
            self.holding = True
        # 状态切换：抓取→松开，把滑块放到当前夹爪中心点
        elif (not want_hold) and self.holding:
            self.last_spot = self._finger_spot(row)
            self.holding = False
            place_slider(data, self.qadr, self.last_spot)

        if self.holding:
            # 抓取状态，滑块跟随夹爪同步运动
            T_box = body_pose(model, data, "gripper") @ self.hold_T
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
            mujoco.mj_forward(model, data)
            # 兜底检测：滑块穿透地面，强制释放
            if slider_bottom_min(data, self.qadr, SLIDER_HALF) < -0.0006 and self.last_spot is not None:
                self.holding = False
                self.hold_T = None
                self.last_spot = self._finger_spot(row)
                place_slider(data, self.qadr, self.last_spot)
                mujoco.mj_forward(model, data)
        else:
            # 未抓取，固定在地面点位
            place_slider(data, self.qadr, self.last_spot)
            mujoco.mj_forward(model, data)
```

### **总览**
`update`是回放循环每帧执行的主更新函数。 核心思想：

1. **根据回放时间 **`qt`插值得到机械臂关节角度，驱动手臂；
2. **如果不开滑块，只跑手臂正运动学，直接结束；**
3. **开滑块：用预生成的 **`hold_mask` 判断这一时刻轨迹指令要不要抓取物体（want_hold）；
4. 模拟抓取逻辑： 
    - **松开 → 抓取：**一次性记录滑块相对于夹爪的相对位姿 `hold_T`
    - **抓取期间：每帧用夹爪当前位姿 × **`hold_T`，算出滑块位置，让滑块跟着夹爪一起运动（运动学绑定，不是物理抓取）
    - **抓取 → 松开：把滑块投影放到夹爪中心正下方地面 z=0**
5. **安全兜底：抓取时检测滑块最低点，如果扎进地下（穿地），强制释放，优先级高于轨迹指令**
6. **松开状态：滑块固定在上一次落地的地面点位不动**

变量区分（非常重要）

+ `want_hold`**：期望状态，来自预计算好的轨迹标记 hold_mask（轨迹规定此刻应该抓 / 放）**
+ `self.holding`**：**当前仿真真实状态，标记现在是不是正抓着滑块
+ `hold_T`**：4×4 齐次变换矩阵，滑块在夹爪局部坐标系下的相对位姿，抓取瞬间固定不再改变**

### **逐段拆解**
```python
model, data = self.model, self.data
row = self._row(qt)
set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))
```

1. `row = self._row(qt)`**：调用前面写的**`_row`**函数，用**`qt`**插值，算出 6 个关节弧度；**
2. `zip`**把关节名称和弧度配对，转为字典；**
3. `set_arm`**：将关节角度写入**`data.qpos`**，设置机械臂姿态。**

```python
if not self.use_slider:
    # 纯回放手臂，不加载滑块
    mujoco.mj_forward(model, data)
    return
```

**如果不启用滑块物体，直接做一次正运动学更新 geom 位置，后面抓取逻辑全部跳过，函数直接返回。**

```plain
idx = int(np.searchsorted(self.t, qt, side="right") - 1)
idx = max(0, min(idx, len(self.t) - 1))
want_hold = bool(self.hold_mask[idx])
```

1. `np.searchsorted(..., side="right")-1`**：找到回放时间**`qt`**落在轨迹采样时间**`self.t`**的哪一段，拿到对应的采样帧下标**`idx`**；**
2. `max/min`**边界保护，防止索引越界；**
3. `want_hold = hold_mask[idx]`**：查询预计算掩码，得到当前轨迹期望是否抓取。**

---

###  状态上升沿：松开 → 抓取（仅切换瞬间执行一次）
```python
if want_hold and not self.holding:
    T_body = body_pose(model, data, "gripper")
    T_box = np.eye(4)
    T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
    self.hold_T = np.linalg.inv(T_body) @ T_box
    self.holding = True
```

**触发条件：轨迹要求抓取 **`want_hold=True`**，并且当前还没有抓住物体。**

1. `T_body`**：夹爪 gripper 的世界坐标系 4×4 齐次变换矩阵（位置 + 旋转）；**
2. `T_box`**：构造滑块 box 当前的世界齐次矩阵，位置取自滑块当前**`qpos`**；**
3. 
![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789369736224-9e351881-e695-4050-b3a2-19b70ca30e95.png)

>  几何含义：**把滑块的世界位姿，转换到夹爪的局部坐标系下**。 一旦抓取，这个相对关系就固定保存，不再改变。后续夹爪移动，就用这个相对关系反推滑块世界坐标，模拟抓住物体一起动。  
>

4. `self.holding = True`，更新状态标记。

原理：抓住物体那一刻，记住【滑块相对于夹爪的位置朝向】，之后夹爪怎么动，滑块就跟着保持这个相对关系。

### 状态下降沿：抓取 → 松开（仅切换瞬间执行一次）
```python
elif (not want_hold) and self.holding:
self.last_spot = self._finger_spot(row)
self.holding = False
place_slider(data, self.qadr, self.last_spot)
```

**触发条件：轨迹指令要求松开，并且当前仿真正抓着滑块。**

1. `_finger_spot(row)`**：计算夹爪中心点 XY，Z 强制 = 0，得到地面放置点；**
2. **保存放置点**`last_spot`**；**
3. `holding=False`**，切换为松开状态；**
4. `place_slider`**：直接把滑块放到地面这个坐标。**

---

### **当前处于抓取状态 **`self.holding = True`**（每帧都执行）**
```python
if self.holding:
    # 抓取状态，滑块跟随夹爪同步运动
    T_box = body_pose(model, data, "gripper") @ self.hold_T
    data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
    data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
    mujoco.mj_forward(model, data)
```

1. 
![](https://cdn.nlark.com/yuque/0/2026/png/27211935/1789369862336-9a29259f-53e4-4e19-af3f-c5b1037a641d.png)
2. **用夹爪当前世界位姿，乘上抓取时记录的相对位姿**`hold_T`**，算出滑块此刻的世界位置 + 旋转；**
3. **把算出的平移、旋转（旋转矩阵转四元数）写入滑块 body 的 qpos；**
4. `mj_forward`**：MuJoCo 正运动学，更新所有 geom 的世界坐标。**

### **防穿地兜底（安全守卫，优先级高于轨迹指令）**
```python
if slider_bottom_min(data, self.qadr, SLIDER_HALF) < -0.0006 and self.last_spot is not None:
    self.holding = False
    self.hold_T = None
    self.last_spot = self._finger_spot(row)
    place_slider(data, self.qadr, self.last_spot)
    mujoco.mj_forward(model, data)
```

+ `slider_bottom_min`**：前面写的函数，计算滑块 8 个角点最低的世界 Z 坐标；**
+ **若最低点 Z<-0.0006，代表滑块已经扎进地面内部，发生穿地；**
+ **一旦检测穿地：强制释放滑块，不管轨迹**`want_hold`**指令，直接放到地面，防止仿真崩坏。**

**这是硬安全保护：哪怕轨迹指令要求继续抓取，一旦物体扎进地面，立刻松手。**



### **未抓取状态 **`self.holding = False`**（每帧执行）**
```python
else:
    # 未抓取，固定在地面点位
    place_slider(data, self.qadr, self.last_spot)
    mujoco.mj_forward(model, data)
```

**滑块静止固定在上一次释放的地面点位**`last_spot`**，不会自己乱跑。**

### **状态完整流转**
1. **初始：**`holding=False`**，滑块静止在地面；**
2. **qt 进入抓取区间 → **`want_hold=True`**，上升沿触发：计算 hold_T，**`holding=True`**； **
    - **持续抓取阶段：每帧根据夹爪位姿 + hold_T 更新滑块位置；同时检测穿地，穿地强制释放；**
3. **qt 离开抓取区间 → **`want_hold=False`**，下降沿触发：计算地面放置点，放下滑块，**`holding=False`**；**
4. **松开阶段：滑块固定在地面，直到下一次抓取触发。**

### **关键要点**
1. **这是运动学模拟抓取，不是物理接触抓取：不是靠碰撞 / 摩擦力抓住物体，而是几何绑定，适合示教轨迹回放；**
2. `hold_T`**只在抓取上升沿计算一次，抓取全程固定不变；**
3. **状态切换（抓 / 放）只在边沿执行一次，中间持续抓取 / 持续松开只做跟随或者固定放置；**
4. **安全规则优先级：防穿地 > 轨迹 want_hold 指令。**

### **整条全链路回顾**
**录制轨迹 npz → 预处理：找动作起点、裁剪时间窗口、识别抓取区间生成 hold_mask → 回放循环，每帧调用 update (qt)**

**update：插值关节 → 设置手臂姿态 → 判断 want_hold → 状态切换 / 滑块跟随 / 放置 + 防穿地校验**

****

## **run_sim 整体调度流程**
```python
def main():
    parser = argparse.ArgumentParser(description="SO-ARM101 MuJoCo 自动演示")
    parser.add_argument("--replay", default=None,
                        help="回放 --leader --record 录下的动作文件(.npz)，用真实关节角夹取")
    parser.add_argument("--speed", type=float, default=1.0, help="回放速度倍率")
    parser.add_argument("--start", type=float, default=0.0,
                        help="从录制文件的第几秒开始回放(可跳过开头静止段)")
    parser.add_argument("--len", type=float, default=None,
                        help="回放时长(秒)，默认到结尾")
    args = parser.parse_args()

    model = load_model_with_slider()
    data = mujoco.MjData(model)
    mujoco.mj_resetData(model, data)
    mujoco.mj_forward(model, data)

    # ---------- 回放真实录制（只展示真实手臂动作，不含红色滑块）----------
    if args.replay:
        path = args.replay
        if not os.path.exists(path):
            alt = os.path.join(SIM_DIR, os.path.basename(path))
            if os.path.exists(alt):
                path = alt
            else:
                print(f"[run_sim] 找不到录制文件 {args.replay}。")
                print("  请确认在 mujoco_sim 目录下运行，或先录制：")
                print("    python sim_teleop.py --leader --record demo.npz")
                return
        model = load_model_plain()            # 回放模式：不注入红色滑块
        data = mujoco.MjData(model)
        mujoco.mj_resetData(model, data)
        mujoco.mj_forward(model, data)
        anim = ReplayDemo(model, data, path, speed=args.speed,
                          start_sec=args.start, len_sec=args.len, use_slider=False)
        print(f"[run_sim] 回放录制 {path}：{anim.duration:.1f}s"
              f"（录制开头约有 {anim.start_hint:.1f}s 静止段；想跳过可加 --start {anim.start_hint:.0f}）")
        set_arm(data, model, dict(zip(ReplayDemo.JOINT_NAMES, anim.rad[0])))
        # 启动MuJoCo可视化窗口，循环回放
        with mujoco.viewer.launch_passive(model, data) as viewer:
            prev = time.time()
            qt = 0.0
            while viewer.is_running():
                now = time.time()
                dt = min(0.05, now - prev)
                prev = now
                qt += dt * args.speed
                if qt >= anim.duration:
                    qt = qt % anim.duration     # 循环回放
                    anim._reset()
                anim.update(dt, qt)
                viewer.sync()
                time.sleep(0.002)
        return

    # ---------- 内置脚本化演示（无录制时的默认） ----------
    # 启动时自动校准抓取姿态：按真实网格顶点把Q_GRASP抬到离地安全余量之上，
    # 修复“硬编码姿态戳穿地面”的问题（换标定xml也能自适应，无需手调数值）
    _cache = jaw_cache(model)
    Q_GRASP, _how, _clr = raise_until_clear(model, data, _cache, Q_GRASP, FLOOR_MARGIN)
    if "无需" in _how:
        print(f"[run_sim] 姿态校验：夹爪最低点离地 {(_clr * 100):.1f}cm"
              f"（≥余量 {FLOOR_MARGIN * 100:.1f}cm，无需校准）")
    else:
        print(f"[run_sim] 防穿地校准：{_how}，夹爪最低点离地 {(_clr * 100):.1f}cm")

    # 悬停姿态：抓取姿态基础上抬高LIFT_RAISE
    Q_HOVER = {**Q_GRASP, "shoulder_lift": Q_GRASP["shoulder_lift"] - LIFT_RAISE, "gripper": GRIP_OPEN}
    Q_GRASP_A = {**Q_GRASP}
    Q_GRASP_B = {**Q_GRASP, "shoulder_pan": PAN_SWING}

    def spot_of(q):
        """输入抓取姿态q，驱动机械臂，计算抓取点在地面的投影坐标"""
        set_arm(data, model, {**q, "gripper": GRIP_CLOSE})
        c = finger_centroid(model, data)
        return float(c[0]), float(c[1]), 0.0

    # 计算A、B两个抓取点位
    spotA = spot_of(Q_GRASP_A)
    spotB = spot_of(Q_GRASP_B)
    print(f"[run_sim] 抓取点 A={tuple(round(v, 3) for v in spotA)}"
          f"  B={tuple(round(v, 3) for v in spotB)}  (爪指质心投影)")

    qadr = slider_qadr(model)
    place_slider(data, qadr, spotA)
    mujoco.mj_forward(model, data)

    # 初始化拾取放置状态机
    anim = PickPlace(model, data, Q_HOVER, Q_GRASP_A, Q_GRASP_B, Q_HOME,
                     spots=[spotA, spotB])
    print("[run_sim] 演示开始：夹取 A → 放到 B → 夹回 A → ...（ESC 退出）")
    set_arm(data, model, Q_HOME)

    # 启动MuJoCo可视化窗口，运行状态机
    with mujoco.viewer.launch_passive(model, data) as viewer:
        prev = time.time()
        while viewer.is_running():
            now = time.time()
            dt = min(0.05, now - prev)
            prev = now
            anim.update(dt)
            viewer.sync()
            time.sleep(0.002)


if __name__ == "__main__":
    main()
```

### `main()` 整体解读
**main 是整个仿真程序的入口函数**，核心分成两大分支：

1. 带参数 `--replay xxx.npz`：**回放你遥操作录制好的轨迹文件**（就是前面我们拆解一整套 `ReplayDemo` 类）
2. 不带 `--replay`：**默认内置拾取放置状态机演示**，机械臂自动循环 A 点抓 → B 点放，来回搬运滑块。

一句话概括：命令行参数决定运行模式，二选一。

### 第一部分：argparse 命令行参数解析
```python
parser = argparse.ArgumentParser(description="SO-ARM101 MuJoCo 自动演示")
parser.add_argument("--replay", default=None,
                    help="回放 --leader --record 录下的动作文件(.npz)，用真实关节角夹取")
parser.add_argument("--speed", type=float, default=1.0, help="回放速度倍率")
parser.add_argument("--start", type=float, default=0.0,
                    help="从录制文件的第几秒开始回放(可跳过开头静止段)")
parser.add_argument("--len", type=float, default=None,
                    help="回放时长(秒)，默认到结尾")
args = parser.parse_args()
```

定义 4 个命令行入参：

+ `--replay`：npz 录制轨迹文件路径；不为空就进入回放模式
+ `--speed`：回放速度倍率，0.5 慢放、2 倍加速，默认 1 倍速
+ `--start`：回放起始秒（对应前面`start_sec`，跳过前面静止段）
+ `--len`：回放持续时长（对应`len_sec`）

### 初始化基础模型
```python
model = load_model_with_slider()
data = mujoco.MjData(model)
mujoco.mj_resetData(model, data)
mujoco.mj_forward(model, data)
```

先加载**带红色滑块**的 xml 模型，重置仿真状态，做一次正解。

注意：如果走回放分支，后面会**重新加载纯净模型（不带滑块）**。

### 分支一：`if args.replay:` 轨迹回放模式
```python
path = args.replay
if not os.path.exists(path):
    alt = os.path.join(SIM_DIR, os.path.basename(path))
    if os.path.exists(alt):
        path = alt
    else:
        print(f"[run_sim] 找不到录制文件 {args.replay}。")
        print("  请确认在 mujoco_sim 目录下运行，或先录制：")
        print("    python sim_teleop.py --leader --record demo.npz")
        return
model = load_model_plain()        # 回放模式：不注入红色滑块
data = mujoco.MjData(model)
mujoco.mj_resetData(model, data)
mujoco.mj_forward(model, data)
anim = ReplayDemo(model, data, path, speed=args.speed,
                  start_sec=args.start, len_sec=args.len, use_slider=False)
```

1. 文件查找逻辑：先看用户给的路径；找不到，自动去 SIM_DIR 目录找同名文件；找不到就打印提示退出。
2. **加载 plain 纯净模型 **`load_model_plain()`：**回放模式默认关闭滑块**，只回放手臂运动，所以`use_slider=False`
3. 实例化 `ReplayDemo`，也就是前面我们逐行拆解的那个回放类，传入：模型、数据、npz 文件、回放速度、起始时间、回放时长。

```python
print(f"[run_sim] 回放录制 {path}：{anim.duration:.1f}s"
      f"（录制开头约有 {anim.start_hint:.1f}s 静止段；想跳过可加 --start {anim.start_hint:.0f}）")
set_arm(data, model, dict(zip(ReplayDemo.JOINT_NAMES, anim.rad[0])))
```

+ 打印提示：总回放时长 + 自动识别的静止段提示（就是`start_hint`），并且提示你可以直接用`--start`跳过静止段。
+ 初始化手臂到轨迹**第 0 帧**姿态。

### MuJoCo 可视化 + 回放主循环
```python
with mujoco.viewer.launch_passive(model, data) as viewer:
    prev = time.time()
    qt = 0.0
    while viewer.is_running():
        now = time.time()
        dt = min(0.05, now - prev)
        prev = now
        qt += dt * args.speed
        if qt >= anim.duration:
            qt = qt % anim.duration     # 循环回放
            anim._reset()
        anim.update(dt, qt)
        viewer.sync()
        time.sleep(0.002)
return
```

这是**实时可视化循环**：

1. `prev / now` 获取真实墙上时钟时间，计算帧间隔`dt`；`min(0.05, ...)`限制最大步长，防止卡顿导致 dt 爆炸。
2. `qt += dt * args.speed`：**回放时间 qt = 真实时间 × 速度倍率**，qt 就是传给`anim.update(dt,qt)`的回放时间。
3. 如果 qt 超过片段总时长：取模循环播放，并且调用`anim._reset()`重置回放状态。
4. 调用`anim.update(dt, qt)`：就是上一节我们详细分析的 update 函数，驱动手臂。
5. `viewer.sync()`：刷新 MuJoCo 可视化窗口。
6. `time.sleep(0.002)`：轻微休眠，降低 CPU 占用。

走这个分支，代码执行完 return，后面内置拾取演示代码不会运行。

### 分支二：不带 --replay，默认【内置拾取放置自动演示】
```python
# ---------- 内置脚本化演示（无录制时的默认） ----------
# 启动时自动校准抓取姿态：按真实网格顶点把Q_GRASP抬到离地安全余量之上，
# 修复“硬编码姿态戳穿地面”的问题（换标定xml也能自适应，无需手调数值）
_cache = jaw_cache(model)
Q_GRASP, _how, _clr = raise_until_clear(model, data, _cache, Q_GRASP, FLOOR_MARGIN)
```

1. `jaw_cache(model)`：前面讲解的，预缓存夹爪 mesh 顶点，用来计算最低点；
2. `raise_until_clear()`：**自动防穿地校准函数**
    - 输入预设抓取姿态`Q_GRASP`，不断抬高机械臂，直到夹爪最低点距离地面大于安全余量`FLOOR_MARGIN`
    - 解决硬编码关节角度容易扎地的问题，自适应模型，不用手动调参
    - 返回校准后的抓取姿态、说明文本、离地高度。

```python
# 悬停姿态：抓取姿态基础上抬高LIFT_RAISE
Q_HOVER = {**Q_GRASP, "shoulder_lift": Q_GRASP["shoulder_lift"] - LIFT_RAISE, "gripper": GRIP_OPEN}
Q_GRASP_A = {**Q_GRASP}
Q_GRASP_B = {**Q_GRASP, "shoulder_pan": PAN_SWING}
```

用字典拷贝生成多套预设关节姿态（字典展开`{**xxx}`）

+ `Q_HOVER`：悬停姿态，夹爪张开，在抓取点上方抬起；
+ `Q_GRASP_A`：A 点抓取姿态；
+ `Q_GRASP_B`：B 点抓取姿态，肩关节水平旋转 PAN_SWING，移动到第二个点位。

```python
def spot_of(q):
    """输入抓取姿态q，驱动机械臂，计算抓取点在地面的投影坐标"""
    set_arm(data, model, {**q, "gripper": GRIP_CLOSE})
    c = finger_centroid(model, data)
    return float(c[0]), float(c[1]), 0.0
# 计算A、B两个抓取点位
spotA = spot_of(Q_GRASP_A)
spotB = spot_of(Q_GRASP_B)
print(f"[run_sim] 抓取点 A={tuple(round(v, 3) for v in spotA)}"
      f"  B={tuple(round(v, 3) for v in spotB)}  (爪指质心投影)")
```

`spot_of`：就是简化版本的`_finger_spot`，输入关节姿态，驱动手臂，计算夹爪中心投影到地面 z=0 的坐标，算出 A、B 两个放置点。

```python
qadr = slider_qadr(model)
place_slider(data, qadr, spotA)
mujoco.mj_forward(model, data)
# 初始化拾取放置状态机
anim = PickPlace(model, data, Q_HOVER, Q_GRASP_A, Q_GRASP_B, Q_HOME,
                 spots=[spotA, spotB])
print("[run_sim] 演示开始：夹取 A → 放到 B → 夹回 A → ...（ESC 退出）")
set_arm(data, model, Q_HOME)
```

1. 找到滑块 body 的 qpos 起始索引`qadr`；
2. `place_slider`：把红色滑块预先放到 A 点地面；
3. 实例化`PickPlace`状态机类，实现自动搬运逻辑（A 抓→B 放循环）；
4. 机械臂先回到 HOME 原点姿态。

### 内置演示的可视化循环
```python
with mujoco.viewer.launch_passive(model, data) as viewer:
    prev = time.time()
    while viewer.is_running():
        now = time.time()
        dt = min(0.05, now - prev)
        prev = now
        anim.update(dt)
        viewer.sync()
        time.sleep(0.002)
```

同样 MuJoCo 可视化窗口，循环调用`PickPlace`的 update。

和回放循环区别：**没有 qt 回放时间**，完全由状态机 + 实时 dt 驱动，自动执行拾取放置。

### 两条运行路径对比


| 模式 | 启动命令 | 核心类 | 特点 |
| --- | --- | --- | --- |
| 回放录制轨迹 | `python sim_xxx.py --replay demo.npz --start 2.0` | `ReplayDemo` | 播放人遥操作录制动作，默认**无红色滑块**；qt 由真实时钟 × 速度倍率推进 |
| 内置自动拾取 | `python sim_xxx.py` | `PickPlace` | 自动循环 A 到 B 搬运，**带红色滑块**；自动姿态防穿地校准，状态机驱动 |


### 重点关键点
1. 二选一：`--replay`存在就走回放，不走自动拾取；
2. 回放模式会**重新加载不带滑块的模型**；默认自动演示才加载带滑块模型；
3. `raise_until_clear`是启动前一次性校准，防止写死的抓取姿态直接扎地；
4. 两个分支的可视化循环结构几乎一样，只是一个调用`ReplayDemo.update(dt,qt)`，一个调用`PickPlace.update(dt)`；
5. `qt`是回放专用的虚拟时间；内置演示没有 qt，完全依靠状态机管理动作阶段。

### 完整全链路总串
录制脚本`sim_teleop.py`录制 npz 轨迹 → main 启动，

带`--replay`参数 → 加载 npz → `ReplayDemo`预处理（start_hint、裁剪窗口、hold_mask）→ 可视化循环，每一帧`anim.update(dt,qt)`插值关节驱动手臂。 

不带参数：main 启动 → 自动校准抓取姿态 → 计算 A/B 点位 → PickPlace 状态机循环搬运滑块。



## sim_teleop 录制代码解析


```python
# -*- coding: utf-8 -*-
"""
sim_teleop.py —— 在 MuJoCo 仿真里"操控 + 看运动"
============================================================
三种控制方式（任选其一）：
  python sim_teleop.py                 # 键盘控制（推荐，保证能动）
  python sim_teleop.py --demo          # 自动正弦摆动
  python sim_teleop.py --leader        # 拖动真实主臂(COM3) → 仿真从臂跟随

注意：固定加载官方 scene.xml；so101_simple.xml 仅作为教学讲解示例（MJCF 结构说明），不在代码中运行。

【键盘控制】操作说明：
  数字键 1~6  选择关节（1=shoulder_pan 2=shoulder_lift 3=elbow_flex
                           4=wrist_flex 5=wrist_roll 6=gripper）
  左/右方向键  减小/增大该关节目标（身体 ±10，夹爪 ±10，范围自动限制）
  0 键         全部关节回到中点

【鼠标控制】操作说明（默认键盘模式下有效）：
  直接拖动查看器左侧 “Control” 面板里的滑块，也可以控制对应关节！
  原理：脚本只在“键盘目标变化”时才写 ctrl，其余时间不再每帧覆盖，
        所以鼠标拖动的滑块值能留下来 → 物理推进 → 机械臂跟着动。
  （“Joint” 滑块拖的是角度本身，会被位置控制拉回去，请用 Control 滑块）

  统一原理：归一化 -100~100 目标 ↔ 弧度 ctrl → 位置执行器 → 物理推进
       （和真实机械臂同一套单位，方向不对改顶部 FLIP）
"""

import argparse
import math
import threading
import time

import numpy as np
import mujoco
import mujoco.viewer
from pynput import keyboard

# ============ 参数 ============
MODEL_DEFAULT = "scene.xml"          # 官方精确模型（唯一可运行模型；so101_simple.xml 仅作教学示例，不运行）
LEADER_PORT = "COM3"
LEADER_ID = "my_awesome_leader_arm"

# 方向翻转表：真机方向与仿真相反时，把该项设 -1
FLIP = {
    "shoulder_pan": 1,
    "shoulder_lift": 1,
    "elbow_flex": 1,
    "wrist_flex": 1,
    "wrist_roll": 1,
    "gripper": 1,
}

BODY_JOINTS = ["shoulder_pan", "shoulder_lift", "elbow_flex", "wrist_flex", "wrist_roll"]
JOINT_KEYS = BODY_JOINTS + ["gripper"]   # 键盘 1~6 对应

STEP_NORM = 10.0   # 每按一次方向键增减多少（归一化单位）


class KeyControl:
    """键盘线程：选中关节 + 方向键调目标（归一化单位）。"""

    def __init__(self):
        self.target = {j: 0.0 for j in BODY_JOINTS}
        self.target["gripper"] = 50.0
        self.sel = 0                      # 当前选中关节（JOINT_KEYS 下标）
        self.lock = threading.Lock()
        self._listener = keyboard.Listener(on_press=self._on_press)
        self._listener.start()

    def _on_press(self, key):
        with self.lock:
            name = None
            try:
                if key == keyboard.Key.left:
                    name = "minus"
                elif key == keyboard.Key.right:
                    name = "plus"
                elif key == keyboard.KeyCode.from_char("0"):
                    name = "reset"
                elif hasattr(key, "char") and key.char in "123456":
                    name = key.char
            except AttributeError:
                pass
            if name in "123456":
                self.sel = int(name) - 1
                print(f">> 选中 {JOINT_KEYS[self.sel]}（当前 {self.target[JOINT_KEYS[self.sel]]:.0f}）")
            elif name == "minus":
                self._adjust(-STEP_NORM)
            elif name == "plus":
                self._adjust(+STEP_NORM)
            elif name == "reset":
                for j in JOINT_KEYS:
                    self.target[j] = 50.0 if j == "gripper" else 0.0
                print(">> 全部回中点")

    def _adjust(self, delta):
        j = JOINT_KEYS[self.sel]
        lo, hi = (0.0, 100.0) if j == "gripper" else (-100.0, 100.0)
        self.target[j] = min(hi, max(lo, self.target[j] + delta))
        print(f">> {j} = {self.target[j]:.0f}")


def get_joint_range(model, name):
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
    if jid < 0:
        return None
    return jid, model.jnt_range[jid].copy()


def norm_to_rad(v, lo, hi, flip=1):
    """归一化 [-100,100] → 弧度 [lo,hi]。0 = 中点。"""
    return flip * (lo + (v + 100.0) / 200.0 * (hi - lo))


def gripper_to_rad(v, lo, hi, flip=1):
    """夹爪 [0,100] → 弧度 [lo,hi]。0=闭合,100=张开。"""
    return flip * (lo + v / 100.0 * (hi - lo))


def action_to_rad(model, action):
    """把 action 字典（"关节.pos": 归一化值）转成 {关节: 弧度}。"""
    out = {}
    for jname in BODY_JOINTS:
        key = f"{jname}.pos"
        if key not in action:
            continue
        got = get_joint_range(model, jname)
        if not got:
            continue
        _, (lo, hi) = got
        out[jname] = norm_to_rad(action[key], lo, hi, FLIP.get(jname, 1))

    gval = action.get("gripper.pos")
    if gval is not None:
        got = get_joint_range(model, "gripper")
        if got:
            _, (lo, hi) = got
            out["gripper"] = gripper_to_rad(gval, lo, hi, FLIP.get("gripper", 1))
    return out


def joint_target_to_rad(model, name, norm):
    """单个关节：归一化值 → 弧度（官方模型夹爪用单一 gripper 关节）。"""
    if name in BODY_JOINTS:
        got = get_joint_range(model, name)
        if not got:
            return {}
        _, (lo, hi) = got
        return {name: norm_to_rad(norm, lo, hi, FLIP.get(name, 1))}
    if name == "gripper":
        got = get_joint_range(model, "gripper")
        if got:
            _, (lo, hi) = got
            return {"gripper": gripper_to_rad(norm, lo, hi, FLIP.get("gripper", 1))}
    return {}


def apply_to_sim(model, data, joint_values_rad):
    """把 {关节: 弧度} 设成对应执行器的 ctrl 目标（位置控制）。"""
    for name, rad in joint_values_rad.items():
        jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name)
        if jid < 0:
            continue
        aid = None
        for a in range(model.nu):
            if model.actuator_trnid[a, 0] == jid:
                aid = a
                break
        if aid is not None:
            lo, hi = model.jnt_range[jid]
            data.ctrl[aid] = min(max(rad, lo), hi)


def keyboard_step(model, data, sim_d, cur_target, last_target):
    """键盘模式"一帧"的处理（两个要点，专门解决滑块被覆盖的问题）：
    1) 先"采纳"查看器里鼠标拖动的 Control 滑块值：
       查看器(GUI)侧的 ctrl 若和我们 data 里的不一样，说明是鼠标刚拖的，
       把它同步进我们的 data，物理推进才会让机械臂真的动过去；
    2) 键盘目标只在【变化】的关节上写 ctrl，其余关节绝不再覆盖，
       这样上一帧拖的滑块值不会被下一帧冲掉。
    返回这一帧的 last_target。
    """
    if sim_d is not None:
        for a in range(model.nu):
            if sim_d.ctrl[a] != data.ctrl[a]:
                data.ctrl[a] = sim_d.ctrl[a]

    changed = [j for j in cur_target if cur_target[j] != last_target.get(j)]
    for j in changed:
        rads = joint_target_to_rad(model, j, cur_target[j])
        if rads:
            apply_to_sim(model, data, rads)
    return cur_target


def step_sim(model, data):
    """按模型时间步长推进物理（让 ctrl 目标真的驱动机械臂）。"""
    dt = model.opt.timestep
    n = max(1, int(round(0.016 / dt)))
    for _ in range(n):
        mujoco.mj_step(model, data)


def main():
    parser = argparse.ArgumentParser(description="在 MuJoCo 仿真里操控 SO-ARM101（官方模型）")
    parser.add_argument("--leader", action="store_true", help="用真实主臂遥操作")
    parser.add_argument("--leader-port", default=LEADER_PORT, help="主臂端口")
    parser.add_argument("--demo", action="store_true", help="自动正弦摆动演示")
    parser.add_argument("--record", default=None, help="把主臂动作录成 npz(归一化值)",
                        metavar="FILE.npz")
    args = parser.parse_args()

    model = mujoco.MjModel.from_xml_path(MODEL_DEFAULT)   # 固定官方模型，不运行 so101_simple.xml
    data = mujoco.MjData(model)
    mujoco.mj_resetData(model, data)
    mujoco.mj_forward(model, data)

    kc = KeyControl()
    print("【控制说明】数字键1~6选关节，←→调大小，0回中点；也可鼠标拖 Control 滑块；Ctrl+C 退出")

    leader = None
    if args.leader:
        try:
            from lerobot.teleoperators.so101_leader import SO101Leader, SO101LeaderConfig
            leader = SO101Leader(SO101LeaderConfig(port=args.leader_port, id=LEADER_ID))
            leader.connect()
            print(f"主臂已连接: {args.leader_port} —— 拖动主臂，仿真从臂跟随")
        except Exception as e:
            print(f"主臂连接失败（改用键盘）: {type(e).__name__}: {e}")

    with mujoco.viewer.launch_passive(model, data) as viewer:
        t0 = time.perf_counter()
        last_target = {}
        rec = []           # 录制: [t, pan,lift,elbow,wflex,wroll,gripper] 归一化
        try:
            while viewer.is_running():
                if leader is not None:
                    action = leader.get_action()
                    if args.record:
                        rec.append([
                            time.perf_counter() - t0,
                            action["shoulder_pan.pos"], action["shoulder_lift.pos"],
                            action["elbow_flex.pos"], action["wrist_flex.pos"],
                            action["wrist_roll.pos"], action["gripper.pos"],
                        ])
                    joint_rad = action_to_rad(model, action)
                    apply_to_sim(model, data, joint_rad)
                elif args.demo:
                    t = time.perf_counter() - t0
                    action = {
                        "shoulder_pan.pos": 40 * math.sin(2 * math.pi * 0.2 * t),
                        "shoulder_lift.pos": 50 * math.sin(2 * math.pi * 0.3 * t),
                        "elbow_flex.pos": 30 * math.sin(2 * math.pi * 0.4 * t),
                        "wrist_flex.pos": 0.0, "wrist_roll.pos": 0.0,
                        "gripper.pos": 50 + 40 * math.sin(2 * math.pi * 0.3 * t),
                    }
                    joint_rad = action_to_rad(model, action)
                    apply_to_sim(model, data, joint_rad)
                else:
                    # 键盘模式：先采纳鼠标拖的 Control 滑块，
                    # 再只在键盘目标变化时写 ctrl —— 两者不打架
                    with kc.lock:
                        cur = dict(kc.target)
                    last_target = keyboard_step(model, data, viewer.d, cur, last_target)

                step_sim(model, data)
                viewer.sync()
                time.sleep(0.005)
        except KeyboardInterrupt:
            print("\n已停止")
        finally:
            kc._listener.stop()
            if leader is not None:
                leader.disconnect()
                print("主臂已断开")
            if args.record and rec:
                rec = np.array(rec, dtype=np.float64)
                np.savez(args.record, traj=rec,
                         cols=["t", "shoulder_pan", "shoulder_lift", "elbow_flex",
                               "wrist_flex", "wrist_roll", "gripper"])
                print(f"已录制 {len(rec)} 帧 -> {args.record}")


if __name__ == "__main__":
    main()

```



### 整体一句话概括
`sim_teleop.py`：**SO-ARM101 遥操作录制脚本**，MuJoCo 仿真里支持三种操控模式（键盘 / 自动正弦 demo / 真实主臂拖动从臂跟随），并且可以把动作录制保存成 `.npz` 轨迹文件，**这个 npz 文件，就是前面**`**main()**`**用**`**--replay**`**回放加载的文件**。

链路闭环：`sim_teleop.py --leader --record demo.npz` 录制 → `main.py --replay demo.npz` 回放。

### 顶部注释先看懂
```python
三种控制方式（任选其一）：
python sim_teleop.py                    # 键盘控制（推荐，保证能动）
python sim_teleop.py --demo             # 自动正弦摆动
python sim_teleop.py --leader           # 拖动真实主臂(COM3) → 仿真从臂跟随
```

+ `scene.xml`：**真正运行的官方 MJCF 模型**
+ `so101_simple.xml`：只是教学讲解用，**代码不会加载运行**

键盘操作： 1~6 数字键选中 6 个关节，左右方向键增减目标；0 键全部回中点。

统一单位：**归一化 [-100~100]**，代码内部再转换成关节弧度，送给 MuJoCo 执行器`ctrl`。 FLIP：用来修正真机和仿真关节转动方向相反，乘以 ±1 翻转。



### 全局常量
```python
MODEL_DEFAULT = "scene.xml"
LEADER_PORT = "COM3"
FLIP = {各个关节方向翻转标记}
BODY_JOINTS = ["shoulder_pan", "shoulder_lift", "elbow_flex", "wrist_flex", "wrist_roll"]
JOINT_KEYS = BODY_JOINTS + ["gripper"]
STEP_NORM = 10.0 # 每按一次方向键，归一值±10
```

`BODY_JOINTS`：手臂 5 个旋转关节；加上`gripper`夹爪，一共 6 个可控关节。

### 类 KeyControl：键盘监听（独立线程）
```python
class KeyControl:
    def __init__(self):
        self.target = {j:0.0 ...}
        self.sel = 0 # 当前选中关节下标
        self.lock = threading.Lock()
        self._listener = keyboard.Listener(on_press=self._on_press)
        self._listener.start()
```

+ 单独开一个**后台线程监听键盘按键**，不阻塞 MuJoCo 仿真循环；
+ `lock`线程锁：防止按键回调和仿真主线程同时读写`target`字典造成数据错乱。
1. _on_press 按键回调
2. `1~6`：切换选中关节；
3. 左右方向键：调用`_adjust`增减归一目标值；
4. `0`：全部关节重置到默认中点；

### _adjust
对选中关节修改归一目标，**钳位限制范围**：

+ 手臂关节：`[-100,100]`
+ gripper 夹爪：`[0,100]`，0 闭合，100 张开

 这里只是修改**内存里的 target 目标值**，**不会直接写仿真 ctrl**；仿真主循环每一帧读取这个 target。

### 工具函数：归一值 到 弧度转换
```python
def norm_to_rad(v, lo, hi, flip=1):
    """归一化 [-100,100] → 弧度 [lo,hi]。0 = 中点。"""
def gripper_to_rad(v, lo, hi, flip=1):
    """夹爪 [0,100] → 弧度 [lo,hi]。0=闭合,100=张开。"""
def action_to_rad(model, action):
    """action字典（归一化） → 关节弧度字典"""
def joint_target_to_rad(model, name, norm):
    """单个关节归一值转弧度"""
```

核心逻辑：

1. 读取 xml 模型定义的关节范围 `model.jnt_range`（每个关节上下限弧度 lo/hi）
2. 把遥操作 / 键盘的`-100~100`归一值线性映射到关节弧度区间
3. 乘以`FLIP`，用来反转关节旋转方向（真机和仿真方向不一致时使用）

为什么用归一化 - 100~100： 真实 SO101 主臂输出就是这套归一单位，**真机、仿真共用同一套值域**，不用分别调缩放。

### apply_to_sim(model, data, joint_values_rad)
```python
def apply_to_sim(model, data, joint_values_rad):
    """把 {关节: 弧度} 设成对应执行器的 ctrl 目标（位置控制）。"""
```

MuJoCo 位置控制核心：

1. 根据关节名字找到关节 id
2. 遍历执行器`actuator`，找到驱动这个关节的执行器 aid
3. 将目标弧度写入`data.ctrl[aid]`，MuJoCo 的位置控制器会推动关节往这个目标走

### keyboard_step 解决【鼠标滑块和键盘冲突】
```python
def keyboard_step(model, data, sim_d, cur_target, last_target):
    """键盘模式一帧处理
    1）采纳查看器GUI鼠标拖动Control滑块
    2）键盘只更新发生变化的关节，其余关节不覆盖ctrl
    """
```

痛点：如果每帧无脑全部覆盖`data.ctrl`，你在 MuJoCo 查看器里拖动 Control 滑块会立刻被键盘目标冲掉。 解决办法：

1. 如果 GUI 里滑块修改了`sim_d.ctrl`，同步到仿真`data.ctrl`；
2. **只对本次发生变化的关节，写入 ctrl**，不变的关节保持原有值；

这样：键盘调一部分关节，鼠标滑块可以独立调另外关节，互不冲突。

### step_sim
```python
def step_sim(model, data):
    """按模型时间步长推进物理"""
```

MuJoCo 步进。固定保证至少 16ms 一次画面刷新，内部循环多次 mj_step 完成物理积分。

### main () 主函数入口
**1. argparse 命令行参数**

```python
parser.add_argument("--leader", action="store_true", help="用真实主臂遥操作")
parser.add_argument("--leader-port", default=LEADER_PORT)
parser.add_argument("--demo", action="store_true", help="自动正弦摆动演示")
parser.add_argument("--record", default=None, help="录成npz")
```

4 个参数，对应三种模式，`--record`可以搭配`--leader`一起开启录制。

**2. 加载模型初始化**

```python
model = mujoco.MjModel.from_xml_path(MODEL_DEFAULT)
data = mujoco.MjData(model)
mujoco.mj_resetData(model, data)
mujoco.mj_forward(model, data)
kc = KeyControl() # 启动键盘监听线程
```

###  分支判断（三大模式互斥）
**模式 A：--leader 真实主臂遥操作（支持录制）**

```python
if leader is not None:
    action = leader.get_action()
    if args.record:
        rec.append([时间,6个关节归一值])
    joint_rad = action_to_rad(model, action)
    apply_to_sim(model, data, joint_rad)
```

1. 读取真实主臂（SO101 leader）的 action，得到 6 个关节归一值；
2. 如果开启`--record`，每一帧把 `[t, pan,lift,elbow,wflex,wroll,gripper]` 存入 rec 列表；
3. 归一值转为弧度，写入`data.ctrl`驱动仿真机械臂。

**模式 B：--demo 自动正弦摆动**

```python
elif args.demo:
    t = time.perf_counter() - t0
    action = {各个关节正弦函数生成归一值}
    joint_rad = action_to_rad(model, action)
    apply_to_sim(model, data, joint_rad)
```

纯程序生成正弦信号，各个关节不同频率来回摆动，用来测试模型，不需要外接硬件。

**模式 C：默认键盘模式（不带任何参数）**

```python
else:
    with kc.lock:
        cur = dict(kc.target)
    last_target = keyboard_step(model, data, viewer.d, cur, last_target)
```

读取键盘线程里的目标值，调用`keyboard_step`处理键盘 + 鼠标滑块混合控制。

### 仿真循环主体
```python
with mujoco.viewer.launch_passive(model, data) as viewer:
    while viewer.is_running():
        # 上面三种模式分支
        step_sim(model, data)
        viewer.sync()
        time.sleep(0.005)
```

+ `viewer.sync()`：刷新 MuJoCo 可视化窗口画面
+ `time.sleep`降低循环频率，减少 CPU 占用

### finally 收尾（退出时执行）
```python
except KeyboardInterrupt:
print("\n已停止")
finally:
    kc._listener.stop()
    if leader is not None:
        leader.disconnect()
    if args.record and rec:
        rec = np.array(rec)
        np.savez(args.record, traj=rec, cols=[字段名])
```

1. 关闭键盘监听线程、断开主臂串口；
2. 如果开启录制，把收集的所有帧数组保存成`npz`文件；

npz 内部：`traj`是 N 行 7 列数组：每一行 `[时间t, 6个关节归一值]`，这就是前面`ReplayDemo`加载的轨迹！

### 整条完整链路闭环
1. 运行：`python sim_teleop.py --leader --record demo.npz`
    - 人手拖动真实 SO101 主臂；
    - 主臂输出归一化关节值，仿真从臂跟随运动；
    - 每一帧把时间 + 6 关节归一值存入 rec 数组；
    - 按`Ctrl+C`退出，自动保存`demo.npz`。
2. 回放：`python main.py --replay demo.npz --start 2.0`
    - ReplayDemo 加载 npz；
    - 预处理：找 start_hint、裁剪时间窗口、识别夹爪抓取区间生成 hold_mask；
    - 回放循环，qt 虚拟时间推进，`_row`插值关节角度，驱动机械臂运动。

### 重点区分两个脚本
| 脚本 | 作用 |
| --- | --- |
| sim_teleop.py | **录制**：生成 npz 轨迹（键盘 / 真机 / 正弦） |
| main.py | **回放 / 自动拾取**：加载 npz 回放，或者不带 npz 运行 A 到 B 自动搬运 |


### 关键要点总结
1. **单位设计：归一 [-100,100]**，真机主臂、仿真、录制文件全部统一这套值域；
2. 键盘监听是**独立后台线程**，不阻塞 MuJoCo 仿真循环；
3. 键盘模式做了特殊兼容：**MuJoCo 查看器鼠标滑块和键盘可以一起控制，互不覆盖**；
4. 录制保存的 npz 存储**归一化数值**，不是弧度，回放代码内部再重新映射成关节弧度；
5. 三大模式互斥，命令行参数选择控制源；
6. 这就是整套 SO-ARM101 示教 - 回放系统的**录制端**，main.py 是回放端。

