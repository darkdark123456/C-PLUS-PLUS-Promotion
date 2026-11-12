# -*- coding: utf-8 -*-
"""
sim_teleop.py —— 在 MuJoCo 仿真里"操控 + 看运动"
============================================================
三种用法（键盘 / 鼠标拖动控制已移除）：
  python sim_teleop.py                                 # 拖动真实主臂(COM3) → 仿真从臂跟随
  python sim_teleop.py --leader --record arm.npz       # 跟随的同时把主臂动作录下来
  python sim_teleop.py --record-only --record arm.npz  # 只录主臂，不驱动从臂

注意：固定加载官方 scene.xml；so101_simple.xml 仅作为教学讲解示例（MJCF 结构说明），不在代码中运行。

统一原理：归一化 -100~100 目标 ↔ 弧度 ctrl → 位置执行器 → 物理推进
    （和真实机械臂同一套单位，方向不对改顶部 FLIP）
"""

import argparse
import time

import numpy as np
import mujoco
import mujoco.viewer

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


def get_joint_range(model, name):
    """
    把关节**名字**翻译成 MuJoCo 内部的「关节编号 + 上下限」。

    为什么要翻译？MuJoCo 内部所有东西都放在一维数组里、用**整数下标**访问，
    它根本不认名字；而 XML 里写的是名字。所以每次要用某个关节，
    都得先做一次「名字 → 编号」的查找。

    返回 (关节编号, 限位数组)；模型里没有这个关节名时返回 None。

    调用方通常是这么用的（见下面的 action_to_rad）：
        got = get_joint_range(model, "shoulder_lift")
        if not got:              # 返回 None = 模型里没这个关节，跳过
            continue
        _, (lo, hi) = got        # 编号这里用不上就用 _ 丢掉，只要上下限
        
        
        根本原因mujoco里面的关节是用编号来索引的，名字只是xml里面的标识符，mujoco内部不使用名字
        所以要先通过名字查找编号，然后再通过编号去查找关节的上下限
        
        
    """
    # mj_name2id(模型, 要找哪一类对象, 名字) -> 该类对象数组里的下标
    #   mjtObj.mjOBJ_JOINT 表示「我要找的是一个关节」
    #   还有别的类别：mjOBJ_BODY(刚体) / mjOBJ_GEOM(几何体) /
    #                mjOBJ_ACTUATOR(执行器) / mjOBJ_SITE(标记点) ...
    # 名字不存在时它返回 -1（不会报错），所以必须自己判断
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name) # type: ignore
    if jid < 0:
        return None

    # model.jnt_range 的形状是 (关节总数, 2)，第 jid 行就是 [下限, 上限]：
    #     转轴关节(hinge)：单位是**弧度**（MuJoCo 内部一律用弧度存储，
    #                       和 XML 里写 angle="degree" 无关，编译时就换算了）
    #     滑动关节(slide)：单位是**米**
    # 最后 .copy() 别省：jnt_range[jid] 只是模型内存里的一小块「视图」，
    # 不 copy 的话拿到的还是模型自己的内存；copy 出来才是一份独立的数据副本
    return jid, model.jnt_range[jid].copy()


# =============================================================================
# 单位速查表 —— 单位搞混的时候先看这里
# -----------------------------------------------------------------------------
#   环节                              单位
#   XML 里写的 range                  按 <compiler angle> 解释（degree 或 radian）
#   model.jnt_range（编译后）         弧度 / 米   ← 永远，和 XML 里怎么写无关
#   data.qpos                         弧度 / 米   ← 永远
#   data.ctrl（position 执行器）      弧度 / 米   ← 和 qpos 同单位
#   LeRobot leader.get_action()       归一化：身体 -100~100，夹爪 0~100
#   录制进 npz 的 rec                 归一化值（不是弧度）
#
#   一句话：**MuJoCo 只认弧度，根本不认识「归一化」**；归一化是主臂(LeRobot)
#   那一侧的约定。实测：往 ctrl 写 1.5708，qpos 就落在 1.5708 rad = 90°，不多不少。
#
#   数据只往一个方向流，所以下面三个换算函数全是 归一化 → 弧度：
#       主臂 get_action()      归一化值
#            │  norm_to_rad / gripper_to_rad    ← 唯一的换算，单向
#            ▼
#       仿真 data.ctrl          弧度值
#            │  mj_step()
#            ▼
#       data.qpos               弧度值  →  机械臂动起来
#
#   为什么中间要夹一层归一化？因为主臂的「0 / 100」是它自己标定出来的行程端点，
#   和仿真模型的 jnt_range 不一定相等。归一化把两边都压成百分比，再拉伸到对方身上：
#       主臂拉到一端(-100)  → 从臂下限        主臂拉到另一端(+100) → 从臂上限
#       主臂居中(0)         → 从臂行程中点
#   好处：录下来的数据跟具体模型无关，换台机器、换个模型也能用。
# =============================================================================


def norm_to_rad(v, lo, hi, flip=1):
    """
    
    因为lerobot和mujoco的单位区间长度不一定相同，
    需要先计算当前的v在lerobot的范围内所占的百分比，然后再映射到mujoco的范围内，就能找到
    v在mujoco中的[lo,hi]的具体位置x，再通过lo+x就是v在[lo,hi]中的实际位置
    这样就能保证两者的动作是一致的。
    
    
    归一化 [-100,100] → 弧度 [lo,hi]。0 = 中点。

    推导就是「把百分比换成实际区间」2步：
        ② 除以 200 变成 0~1 的百分比：               (v - -100) / 200
        ③ 乘区间长度再放下限，落到 [lo,hi] 上：      lo + 百分比 * (hi - lo)
        
        百分比*(hi-lo) 能计算出v落在[lo,hi]哪一段位置上，然后再+lo，就是v的实际位置

    flip 是方向翻转：FLIP 表里把某个关节设成 -1，整个区间就会镜像，
    用于「真机方向和仿真相反」的情况。

    以 shoulder_pan（lo=-1.9199, hi=+1.9199）为例：
        v = -100  ->  -1.9199 rad  =  -110.00 度   (主臂拉到一端 → 从臂下限)
        v =    0  ->   0.0000 rad  =     0.00 度   (主臂居中    → 从臂行程中点)
        v =  +50  ->  +0.9599 rad  =   +55.00 度
        v = +100  ->  +1.9199 rad  =  +110.00 度   (主臂拉到另一端 → 从臂上限)
        
    """
    
    """
    为什么将两者的长度换成百分比进行计算？
    


    Returns:
        _type_: _description_
    """
    
    return flip * (lo + (v + 100.0) / 200.0 * (hi - lo))


def gripper_to_rad(v, lo, hi, flip=1):
    """
    夹爪 [0,100] → 弧度 [lo,hi]。0=闭合,100=张开。

    和上面公式一模一样，只是夹爪用的是 [0,100] 而不是 [-100,100]，
    所以少了「平移到 0~200」那一步（"+100"），直接除以 100。

    注意夹爪区间是**非对称**的（lo=-0.1745, hi=+1.7453）：
        v =   0  ->  -0.1745 rad  =   -10.00 度   (闭合)
        v =  50  ->  +0.7854 rad  =   +45.00 度   (一半)
        v = 100  ->  +1.7453 rad  =  +100.00 度   (张开)
    """
    return flip * (lo + v / 100.0 * (hi - lo))


def action_to_rad(model, action):
    """把 action 字典（"关节.pos": 归一化值）转成 {关节: 弧度}。"""
    out = {}
    for jname in BODY_JOINTS:
        key = f"{jname}.pos"
        if key not in action:
            continue
        #通过编号得到该运动关节的上下限
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




# 通过关节id和执行器的id进行比较，如果该关节id存在对应的执行器，就将该关节的弧度值写入控制器
# 如果关节id不存在对应的执行器，就跳过该关节
def apply_to_sim(model, data, joint_values_rad):
    """
    把 {关节: 弧度} 设成对应执行器的 ctrl 目标（位置控制）。
    运动是通过执行器管理的，将关节弧度信息转化为控制器ctrl的参数
    """
    for name, rad in joint_values_rad.items():
        # 获取关节id
        jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name) # type: ignore
        if jid < 0:
            continue
        aid = None
        # a 遍历的是【执行器】编号 0,1,2...
        for a in range(model.nu):
            '''
            actuator_trnid这是一个执行器数组，通过id关联对应的关节 每一个关节都对应一个执行器
            
            
            actuator_trnid[i]=[主目标id, 附属目标id]
            
            
            array([
                 0   [0, -1],    # 执行器0 → 驱动关节0 (shoulder_pan)
                 1   [1, -1],    # 执行器1 → 驱动关节1 (shoulder_lift)
                 2   [2, -1],    # 执行器2 → 驱动关节2 (elbow_flex)
                 3   [3, -1],    # 执行器3 → 驱动关节3 (wrist_flex)
                 4   [4, -1],    # 执行器4 → 驱动关节4 (wrist_roll)
                 5   [5, -1],    # 执行器5 → 驱动关节5 (gripper)
                 n   [?, ?]]    # 如果还有额外执行器...
                ])
                
            每一个执行器都对应两个关节id，主目标id和附属目标id，附属目标id一般为-1，表示没有附属目标
            一个关节要想执行，必须自己的id和执行器的id相同，才能够被正确执行
            
            
            
                
                
            '''
            # 根据这个执行器编号判断是否是目标关节
            if model.actuator_trnid[a, 0] == jid:
                aid = a
                break

        '''
        主臂归一化值(-100~100)
            ↓ action_to_rad（按jnt_range线性映射）
        弧度 rad
            ↓ apply_to_sim
        找到关节jid → 反查执行器aid → 写入 data.ctrl[aid] = rad
            ↓ mj_step
        物理引擎驱动关节转到这个角度
        '''
        if aid is not None:
            lo, hi = model.jnt_range[jid]
            # 把弧度写入控制器 所运行的弧度不能超过关节的最大值，不能低于关节的最小值
            data.ctrl[aid] = min(max(rad, lo), hi)



#
def step_sim(model, data):
    """按模型时间步长推进物理（让 ctrl 目标真的驱动机械臂）。"""
    # 模型自己的物理步长，比如 0.002秒(2ms)
    dt = model.opt.timestep
    # 一帧要跑几个物理步 ≈ 16ms/2ms = 8步
    n = max(1, int(round(0.016 / dt)))
    for _ in range(n):
        # 每步推进2ms物理
        mujoco.mj_step(model, data) # type: ignore


def main():
    
    
    #0 配置参数
    
    parser = argparse.ArgumentParser(description="在 MuJoCo 仿真里操控 SO-ARM101（官方模型）")
    # 带leader选项，将来args.leader就为True,后面的help是说明
    parser.add_argument("--leader", action="store_true", help="用真实主臂遥操作（默认）")
    parser.add_argument("--leader-port", default=LEADER_PORT, help="主臂端口")
    parser.add_argument("--record-only", action="store_true",
                        help="只录制主臂动作，不驱动仿真从臂（需配合 --record）")
    '''
        - `--record`：参数名
        - `default=None`：命令行里不写 `--record` 时，`args.record` 就是 `None`
        - `metavar="FILE.npz"`：只影响帮助文档显示，提示这里该填一个文件名
    '''

    parser.add_argument("--record", default=None, help="把主臂动作录成 npz(归一化值)",
                        metavar="FILE.npz")
    args = parser.parse_args()

    # 键盘控制已移除，所以不给任何模式参数时默认走「主臂跟随」
    if not (args.leader or args.record_only):
        args.leader = True
    if args.record_only and not args.record:
        parser.error("--record-only 必须同时给出 --record FILE.npz")




    #1 读取模型和数据
    '''
    ① MjModel：加载模型（只读静态结构，来自XML）
    包含：连杆几何、关节定义、执行器、质量、摩擦等"模型规则"
    MODEL_DEFAULT = "scene.xml"，固定加载官方精确场景，不加载教学示例 so101_simple.xml
    '''
    model = mujoco.MjModel.from_xml_path(MODEL_DEFAULT) # type: ignore
    '''
    ② MjData：按模型尺寸分配一块"运行时数据"内存
        此时里面是全零：关节角qpos、速度qvel、受力、碰撞信息都还是0
        model是"说明书"，data是"当前这一帧的状态"
    '''
    data = mujoco.MjData(model) # type: ignore
    '''
        ③ mj_resetData：用XML里定义的初始姿态(qpos0)覆盖data
        把机械臂摆到XML设定的初始位置，同时清零速度、受力、接触
    '''
    mujoco.mj_resetData(model, data) # type: ignore
    '''
        ④ mj_forward：正向运动学求解
        根据当前qpos，算出所有body/geom的世界坐标xpos/xmat
        如果不调用它，后面读到的世界位置都是旧值/零值
    '''
    
    #2 获取当前机械臂的世界坐标 偏移量等数据
    mujoco.mj_forward(model, data) # type: ignore


    #3 连接主臂
    leader = None
    if args.leader or args.record_only:
        try:
            from lerobot.teleoperators.so101_leader import SO101Leader, SO101LeaderConfig
            leader = SO101Leader(SO101LeaderConfig(port=args.leader_port, id=LEADER_ID))
            leader.connect()
            if args.record_only:
                print(f"主臂已连接: {args.leader_port} —— 只录制，不驱动从臂")
            else:
                print(f"主臂已连接: {args.leader_port} —— 拖动主臂，仿真从臂跟随")
        except Exception as e:
            print(f"主臂连接失败: {type(e).__name__}: {e}")
            print("请检查 --leader-port 端口号（可用 lerobot-find-port 查找真实端口）")
            return

    # 4 启动渲染窗口
    with mujoco.viewer.launch_passive(model, data) as viewer:
        # 可以理解为返回当前的时间戳，单位 s
        t0 = time.perf_counter()
        rec = []           # 录制: [t, pan,lift,elbow,wflex,wroll,gripper] 归一化
        try:
            # 如果窗口打开则更新各个关节运动
            while viewer.is_running():
                if leader is not None:
                    
                    # 5 获取主臂的关节lerobot归一化值
                    action = leader.get_action()
                    #if debug use record = true
                    args.record = True
                    
                    
                    if args.record:
                        '''
                            - `leader.get_action()` 拿到主臂所有关节当前值，是归一化值
                            - 身体关节 `-100~100`，夹爪 `0~100`
                            - 每帧 `append` 一行到 `rec`，7 列 = [时间，5 个身体关节，夹爪]
                            
                            rec = [
                                    #  t(秒)   pan    lift   elbow  wflex  wroll  gripper
                                    [0.000,    0.0,    0.0,    0.0,   0.0,   0.0,   50.0 ],
                                    [0.020,    1.0,   -5.0,   10.0,   0.0,   0.0,   45.0 ],
                                    [0.040,    2.0,  -12.0,   22.0,   1.0,   0.0,   38.0 ],
                                    [0.060,    3.0,  -20.0,   35.0,   2.0,   0.0,   30.0 ],
                                ]
                                
                                
                            rec 存储的是两个时间点差值的运动归一化值，但是这两点的运动不是平缓的
                            之后要通过在这两点之间插值进行平滑过渡，如果要存储运动数据，启用它
                        '''
                        rec.append([
                            time.perf_counter() - t0,
                            action["shoulder_pan.pos"], action["shoulder_lift.pos"],
                            action["elbow_flex.pos"], action["wrist_flex.pos"],
                            action["wrist_roll.pos"], action["gripper.pos"],
                        ])
                        
                    # 6 将归一化的值转化为弧度
                    joint_rad = action_to_rad(model, action)
                    # 7 根据弧度改写为执行器的控制值
                    # --record-only 时只录制、不驱动从臂，所以跳过这一步
                    if not args.record_only:
                        apply_to_sim(model, data, joint_rad)

                # 8让物理引擎按 ctrl 目标推进机械臂运动
                # 通过自动插值平缓运行到ctrl中的弧度目标值
                step_sim(model, data)
                # 把新位置刷新到3D窗口，你才看到机械臂动了
                viewer.sync()
                time.sleep(0.005)
        except KeyboardInterrupt:
            print("\n已停止")
        finally:
            if leader is not None:
                leader.disconnect()
                print("主臂已断开")
            if args.record and rec:
                # list → numpy二维数组 (N,7)
                rec = np.array(rec, dtype=np.float64)
                # 存成npz，数据名叫"traj"
                np.savez(args.record, traj=rec, # type: ignore
                         cols=["t", "shoulder_pan", "shoulder_lift", "elbow_flex",
                               "wrist_flex", "wrist_roll", "gripper"])
                print(f"已录制 {len(rec)} 帧 -> {args.record}")


if __name__ == "__main__":
    main()
