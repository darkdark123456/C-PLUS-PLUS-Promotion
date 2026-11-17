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
            if name in "123456": # type: ignore
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
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name) # type: ignore
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
