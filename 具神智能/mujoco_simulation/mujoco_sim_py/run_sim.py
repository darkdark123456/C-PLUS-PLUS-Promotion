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



##################弧度和角度之间的转换公式##################


            #度=角度*Π/180
            
            
######################################################3##

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
    return mujoco.MjModel.from_xml_string(scene_xml, assets=assets) # type: ignore


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
    return mujoco.MjModel.from_xml_string(xml, assets=assets) # type: ignore

# ================= 2) 小工具函数（坐标变换、点云、位姿相关） =================
def mesh_block(model, mesh_id):
    """
    读取指定mesh_id网格的全部【STL局部顶点】
    输入：model模型对象，mesh_id网格编号
    返回：N×3 numpy数组，N个顶点，每行[x,y,z]，是CAD导出的原始局部坐标（固定不变）
    原理：通过mesh_vertadr找到该mesh顶点在mesh_vert大数组的起始偏移，mesh_vertnum读取顶点数量
    """
    a = int(np.asarray(model.mesh_vertadr[mesh_id]).reshape(-1)[0]) #该顶点的起始偏移
    n = int(np.asarray(model.mesh_vertnum[mesh_id]).reshape(-1)[0])# 往右偏移了多少个顶点
    return model.mesh_vert[a:a + n].reshape(-1, 3) #转换为N*3的矩阵形式，用于旋转移动求解


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
        if model.geom(g).type == mujoco.mjtGeom.mjGEOM_MESH: # type: ignore
            # 判断该geom引用的mesh名字匹配目标mesh_name
            if model.mesh(model.geom(g).dataid).name == mesh_name:
                mid = model.geom(g).dataid  # 获取mesh编号mesh_id
                v = mesh_block(model, mid)  # 取出stl局部顶点 N×3
                b = data.geom_xpos[g]       # geom当前世界平移向量 shape(3,)
                R = data.geom_xmat[g].reshape(3, 3) # geom世界旋转矩阵3×3
                return b + v @ R.T          # 全部顶点转换到世界坐标系  
            
        """
        在这里为什么不用四元数的方式进行计算？？？
        """
            
            
            
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
    C = np.vstack(parts) # type: ignore
    # axis=0：沿着所有行求均值，每一列(x,y,z)分别计算平均值
    return C.mean(0)


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
        jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name) # type: ignore
        # jnt_qposadr：该关节在广义坐标qpos数组内的起始下标
        data.qpos[model.jnt_qposadr[jid]] = v
    # 正向运动学求解，求解的是旋转角度和移动距离，更新所有body、geom的世界位置姿态
    mujoco.mj_forward(model, data) # type: ignore


def body_pose(model, data, name="gripper"):
    """
    获取指定body刚体的4×4齐次变换矩阵（世界坐标系下位姿）
    T = [[R, b],
         [0, 1]]
    R:3×3旋转矩阵；b:3维平移向量[x,y,z]
    输入body名称，返回4×4齐次矩阵，用于坐标变换
    """
    # 根据body名字查询body的id编号
    bid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_BODY, name) # type: ignore
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
    return model.jnt_qposadr[mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, "red_slider")] # type: ignore


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
    """缓动插值函数，0<=t<=1；平滑启停，避免机械臂运动生硬、突变冲击
    输出也在 0 <= f(t) <= 1
    """
    return t * t * (3 - 2 * t)


def lerp_q(q0, q1, t):
    """
    关节字典线性插值
    q0：起始关节字典，q1目标关节字典，t∈[0,1]
    返回插值后的关节状态字典，实现平滑过渡

    q0 = {
    # 起始待机姿态（机械臂收起，安全初始位置）
    "shoulder_pan": 0.0,
    "shoulder_lift": -0.30,
    "elbow_flex": -0.55,
    "wrist_flex": 0.35,
    "wrist_roll": 0.0,
    "gripper": GRIP_OPEN,
    }
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
                '''
                np.array([sx*hx,sy*hy,sz])构造1维数组，数组3个元素，形状为(3,)
                R是3*3矩阵
                R@np.array([sx*hx,sy*hy,sz]), 会自动将np.array([sx*hx,sy*hy,sz])转化为(3,1)
                也就是3行1列的用矩阵，那这样最后计算得到结果也是(3,1)
                python会压缩输出，输出为(3,), 表示1维数组，3个元素
                qp[:3] 和刚才计算的一维数组相加，得到1维数组，3个元素
                基于最后计算的1维数组获取下标为[2],也就是z坐标
                '''
                m = min(m, float((qp[:3] + R @ np.array([sx*hx, sy*hy, sz]))[2]))
    return m

# ================= 3) 演示编排（状态机） =================
class PickPlace:
    """
    自动拾取放置状态机：A点夹取→搬运到B放下，B夹取搬运回A，循环往复
    原理：分多阶段phase，缓动插值控制机械臂关节；抓取时滑块绑定在夹爪坐标系跟随运动
    触地即放逻辑：下降阶段滑块底部碰到地面，自动释放，防止物体穿透地面
    """

    '''
        # 夹爪常量
        GRIP_OPEN = 0.0
        GRIP_CLOSE = 0.8
        PAN_SWING = 0.65   # shoulder_pan旋转目标值（弧度）
        SLIDER_HALF = (0.02, 0.02, 0.01) # 滑块box半尺寸 hx,hy,hz
        
        # HOME待机姿态（就是你最早的Q_HOME）
        q_home = {
            "shoulder_pan": 0.0,
            "shoulder_lift": -0.30,
            "elbow_flex": -0.55,
            "wrist_flex": 0.35,
            "wrist_roll": 0.0,
            "gripper": GRIP_OPEN,
        }
        
        # q_hover：抓取前的悬停姿态，物体正上方安全高度
        q_hover = {
            "shoulder_pan": 0.0,
            "shoulder_lift": -0.20,
            "elbow_flex": -0.40,
            "wrist_flex": 0.20,
            "wrist_roll": 0.0,
            "gripper": GRIP_OPEN,
        }
        
        # q_grasp_a：A点下探，到达滑块上方准备抓取的关节姿态
        q_grasp_a = {
            "shoulder_pan": 0.0,
            "shoulder_lift": -0.45,
            "elbow_flex": -0.70,
            "wrist_flex": 0.10,
            "wrist_roll": 0.0,
            "gripper": GRIP_OPEN,
        }
        
        # q_grasp_b：B点下探抓取姿态（基座pan已经转到另一边）
        q_grasp_b = {
            "shoulder_pan": PAN_SWING,
            "shoulder_lift": -0.45,
            "elbow_flex": -0.70,
            "wrist_flex": 0.10,
            "wrist_roll": 0.0,
            "gripper": GRIP_OPEN,
        }
        
        # spots：A、B地面放置点【世界坐标xyz】
        spots = [
            [0.25, 0.0, 0.01],   # spot 0：A点世界坐标
            [0.25, 0.3, 0.01],   # spot 1：B点世界坐标
        ]

    '''


#home 的实际姿态a
    """
                弧度	角度	 该轴限位	        处于行程
shoulder_pan	0.0 	0.0°	 ±110°	            中位
shoulder_lift	1.36	77.9°	 ±100°	            正方向 78%
elbow_flex	    −1.4	−80.2°	 ±96.8°	            负方向 83%
wrist_flex	    1.16	66.5°	 ±95°	            正方向 70%
wrist_roll	    0.0	    0.0°	−157° ~ +163°	    中位
gripper	      1.7453	100.0°	−10° ~ +100°	    完全张开
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
        mujoco.mj_forward(self.model, self.data) # type: ignore
        '''
            # self.model 是 mjModel 对象，加载xml之后就固定了，仿真不会自动改动
            # 整数，场景总共有多少个geom，比如22
            self.model.ngeom 
            # 总关节数量，比如7（机械臂6关节 + red_slider freejoint）       
            self.model.njnt
            # 数组：每个关节在qpos里的起始下标         
            self.model.jnt_qposadr  
            # 所有刚体名字列表 ["world", "base", "shoulder_link", ... , "red_slider"]
            self.model.body_names   
            # 第0号geom的类型，比如mjGEOM_PLANE地面
            self.model.geom(0).type 
            # 场景里全部STL mesh名字
            self.model.mesh_names   
        
            # self.data 是 mjData 对象，每 mj_step / mj_forward 刷新
            # 一维数组，广义坐标。包含机械臂关节弧度 + red_slider的[x,y,z,qw,qx,qy,qz]
            self.data.qpos 
            # 广义速度，关节角速度、滑块线速度角速度     
            self.data.qvel
            # 所有body的世界坐标，shape [nbody,3] 平移向量    
            self.data.xpos 
            # 所有body的旋转矩阵，shape [nbody,9]（摊平3x3）     
            self.data.xmat
            # 当前所有碰撞接触信息（地面、夹爪碰滑块）      
            self.data.contact   

        '''

    def update(self, dt):
        """
        每帧更新状态机，dt是单帧时间步长
        1. 时间累加，缓动插值生成当前目标关节角度
        2. set_arm驱动机械臂
        3. 如果处于holding抓取状态：跟随夹爪同步更新滑块位置姿态
        4. 检测触地，自动释放物体；阶段时间结束切换下一动作阶段
        """
        model, data = self.model, self.data
        '''
            取出当前第 i 个阶段的完整元组，
            数据格式("阶段名称", 目标关节姿态字典, 目标夹爪角度, 阶段持续时间, 事件标记)
            [1:] 跳过第一个元素（阶段名称），
            把剩下 4 个参数按顺序赋值给左边 4 个变量。
            
            
            很奇怪，这里的g_target的弧度是如何拿到的？？？？？
            
        '''
        q_target, g_target, dur, ev = self.phases[self.i][1:]
        '''
            q0 当前阶段的起始关节姿态字典
              q0 = {
                "shoulder_pan": 0.0,
                "shoulder_lift": -0.45,
                "elbow_flex": -0.70,
                "wrist_flex": 0.10,
                "wrist_roll": 0.0,
                "gripper": GRIP_OPEN,
            }
            g0 当前夹爪关节的起始角度
        '''
        q0, g0 = self.q, self.q["gripper"]

        self.t = min(self.t + dt, dur)
        '''
        self.t / dur  ,  把绝对时间转换成 0~1 的线性进度。
        如果不做ease线性处理，那么就会出现剧烈抖动。y' = x
        ease(x)=3x²-2x³, 通过ease 变成 S 型的 0~1 平滑进度
        ease'(x) = 6x-6x² 当x = 0, 速度为0，缓慢启动
        当x = 1 速度为0 缓慢停止
        '''
        t = ease(self.t / dur) # 归一化+缓动函数
        qm = lerp_q(q0, q_target, t) # 插值得到当前目标关节
        '''
        qm = {
            # 基座不动
            "shoulder_pan": 0.0,
            # 从-0.20降到-0.45，走了一半       
            "shoulder_lift": -0.325,  
            # 从-0.40弯到-0.70，走了一半 
            "elbow_flex": -0.55,
            # 从0.20收到0.10，走了一半       
            "wrist_flex": 0.15, 
            # 滚转不动       
            "wrist_roll": 0.0, 
            # 夹爪保持张开        
            "gripper": 0.0             
        }
        '''
        qm["gripper"] = g0 + (g_target - g0) * t # 夹爪角度单独插值
        set_arm(data, model, qm)
        self.q = qm

        if self.holding:
            # 抓取状态：滑块跟随gripper刚体运动
            '''
            body_pose(model, data, "gripper") 为夹爪局部坐标系到世界坐标系的齐次变换矩阵
            self.hold_T 为滑块局部坐标系到夹爪局部坐标系的齐次变换矩阵
            T_box 为滑块局部坐标系到世界坐标系的齐次变换矩阵
            '''
            T_box = body_pose(model, data, "gripper") @ self.hold_T # type: ignore
            # 更新滑块freejoint的平移和姿态
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3]
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3])
            mujoco.mj_forward(model, data) # type: ignore
            # place阶段触地检测：滑块底部低于阈值，立刻释放，防止扎进地面
            if ev == "place" and slider_bottom_min(data, self.qadr, self.slider_half) <= 0.0012:
                self._release()
                self.t = 0.0
                self.i = (self.i + 1) % len(self.phases)
                return
        else:
            # 未抓取，滑块固定放在地面点位
            place_slider(data, self.qadr, self.spots[self.cur_spot])
            mujoco.mj_forward(model, data) # type: ignore

        # 当前阶段时间跑完，切换下一阶段
        if self.t >= dur:
            if ev == "grab":
                # grab事件：抓取，读取夹爪的世界齐次变换矩阵（夹爪局部 → 世界）
                T_body = body_pose(model, data, "gripper")
                # 构造滑块的世界齐次变换矩阵（滑块局部 → 世界），姿态默认摆正
                T_box = np.eye(4)
                T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy()
                # 求相对变换：hold_T = 夹爪矩阵的逆 × 滑块世界矩阵
                #    得到 滑块局部 → 夹爪局部 的相对变换矩阵，抓取后固定不变
                """
                让滑块跟随夹爪移动
                """
                self.hold_T = np.linalg.inv(T_body) @ T_box
                self.holding = True
            elif ev == "place": 
                self._release()
            self.i = (self.i + 1) % len(self.phases)
            self.t = 0.0


def joint_range(model, name):
    """查询指定关节的运动范围 [最小值，最大值]（弧度）"""
    jid = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, name) # type: ignore
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
        # traj[:, 0] = [1670.5, 1670.6, 1670.8, 1671.1, ...]
        # 减去第一帧的 `1670.5` 之后, t_abs = [0.0, 0.1, 0.3, 0.6, ...]
        t_abs = traj[:, 0] - traj[0, 0]
        # 每一行是每一帧，每一列是每一帧对应的不同关节
        rad_full = np.zeros((traj.shape[0], 6))
        # 把录制的归一化数值，逐个关节转换为弧度
        for c, name in enumerate(self.JOINT_NAMES):
            rg = joint_range(model, name)
            if rg is None:
                continue
            lo, hi = rg
            if name == "gripper":
                # 将所有帧的夹爪关节归一化转化为弧度的值存储
                rad_full[:, c] = [norm_grip_to_rad(v, lo, hi) for v in traj[:, 6]]
            else:
                # 将所有帧的每一个关节(除了夹爪关节)归一化转化为弧度的值存储
                rad_full[:, c] = [norm_body_to_rad(v, lo, hi) for v in traj[:, c + 1]]
        # 所有帧的夹爪归一化原始值
        grip_full = traj[:, 6]

        # 自动检测动作起始时间：找到关节有明显运动的时刻，用于提示用户--start参数
        # `axis=0`：沿着**行方向（时间轴）**做差分 输出形状(N-1, 6)
        # abs对差分取绝对值对所有差分值取绝对值
        # `axis=1`：沿着**列方向（关节维度）**取最大值 输出长度为 `N-1` 的一维数组
        d = np.abs(np.diff(rad_full, axis=0)).max(axis=1)
        # 第一个满足条件的位置，就是**动作开始的时刻**
        '''
            d = np.array([0.001, 0.002, 0.005, 0.006, 0.003])
            print(np.where(d > 0.004))
            # 输出：(array([2, 3]),)  ← 这是一个元组

            act = np.where(d > 0.004)[0]
            print(act)
            # 输出：array([2, 3])    ← 这就是纯索引数组
        '''
        act = np.where(d > 0.004)[0]
        self.start_hint = float(t_abs[act[0]]) if len(act) else 0.0

        # 根据start、len参数裁剪回放时间窗口
        # **在有序数组中，找到把目标值插进去后数组仍然保持有序的位置下标**，底层用二分查找实现，效率远高于遍历
        # 参数 `side="left"`（也是默认值）的含义：
        # 当数组里存在和目标值相等的元素时，插入点放在「第一个相等元素的左边」否则放在大小相邻的两个数中间。
        i0 = int(np.searchsorted(t_abs, start_sec, side="left"))
        i0 = max(0, min(i0, len(t_abs) - 1))
        if len_sec is None:
            i1 = len(t_abs) - 1
        else:
            i1 = int(np.searchsorted(t_abs, start_sec + len_sec, side="left"))
            i1 = max(i0 + 1, min(i1, len(t_abs) - 1))

        '''
            原始 t_abs = [0.0, 0.1, 0.2, ... , 2.0, 2.1, ... , 5.0, ... , 10.0]
                i0 对应 2.0 秒的帧索引，i1 对应 5.0 秒的帧索引
            1. 切出时间片段：`[2.0, 2.1, ... , 5.0]`
            2. 归零后：`[0.0, 0.1, ... , 3.0]`
            3. `duration = 3.0` 秒
            4. 关节弧度、夹爪数据同步切出对应帧
        '''

        self.full_start = float(t_abs[i0])
        self.t = t_abs[i0:i1 + 1]
        self.t = self.t - self.t[0]
        self.duration = float(self.t[-1])
        # 记录 i0 到 i1 时间段所有的归一化值
        self.grip_norm = grip_full[i0:i1 + 1]
        # 记录 i0 到 i1 时间段所有的弧度制
        self.rad = rad_full[i0:i1 + 1]

        # 根据夹爪归一化数值，自动识别抓取片段（合爪段标记为hold_mask=True）
        self.holds = []
        self.hold_mask = np.zeros(len(self.t), dtype=bool)
        if self.use_slider:
            # 获取夹爪开合最小值和最大值
            '''
            gmin = 20
            gmax = 90
            '''
            gmin, gmax = float(self.grip_norm.min()), float(self.grip_norm.max())
            '''
            thr = 20 + 0.3*(70) = 41, 阈值为41，大于41认为开合，小于41认为抓取
            '''
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

            '''
                 self.holds = [(200, 500), (800, 1100)]
                 # 含义：第200~500帧是第一次抓取，第800~1100帧是第二次抓取
             '''
            for s, e in self.holds:
                # 抓取段全部标为True，就是以帧索引为下标记录值为 true 或者 false 的数组
                self.hold_mask[s:e + 1] = True
            self._reset()

    def _reset(self):
        """回放循环重置，初始化滑块位置"""

        # 如果回放模式没启用滑块（纯手臂回放），直接跳过
        if not self.use_slider:
            return

        '''
            - `self.holds[0]`：第一次抓取动作的片段
            - `[0]`：取该抓取段的**起始帧索引**
            - 含义：找到「第一次合爪抓取」开始的那一帧
        '''
        s = self.holds[0][0]
        '''
            - 输入第 `s` 帧的关节弧度，驱动机械臂运动到对应姿态
            - `_finger_spot` 计算夹爪几何中心点的地面投影坐标（z=0 放在地面）
            - 得到滑块的初始放置点，存入 `self.last_spot`
        '''
        self.last_spot = self._finger_spot(self.rad[s])
        self.holding = False
        self.hold_T = None
        '''
            - 把滑块写入到计算好的地面初始位置
            - 调用正向运动学刷新世界位姿，保证初始状态准确、画面显示正确
            
            `place_slider` 做的事，
            是直接修改 `data.qpos` 里滑块自由关节的 7 个值（平移坐标 + 四元数姿态），
            也就是修改了**正向运动学的输入**。
            
            mj_forward 更新输出世界位姿
        '''
        place_slider(self.data, self.qadr, self.last_spot)
        mujoco.mj_forward(self.model, self.data) # type: ignore

    def _row(self, qt):
        '''
          假设两个关节， 3 帧数据
            self.t = [0.0, 1.0, 2.0]    # 时间轴
            self.rad = [
                [0.0, -0.5],   # 第0帧：关节1=0，关节2=-0.5
                [0.2, -0.3],   # 第1帧
                [0.5,  0.0],   # 第2帧
            ]
        输出
            查询 `qt = 0.5` 秒（刚好在第 0 帧和第 1 帧中间）：

            - 关节 1：`np.interp(0.5, t, rad[:,0])` → 0.1
            - 关节 2：`np.interp(0.5, t, rad[:,1])` → -0.4
            - 返回结果：`array([0.1, -0.4])`
        '''

        """给定时间点，插值算出对应时刻的 6 个关节角度，实现离散轨迹的连续平滑回放。"""
        return np.array([np.interp(qt, self.t, self.rad[:, c]) for c in range(6)])

    def _finger_spot(self, row):
        '''
        row = [shoulder_pan弧度, shoulder_lift弧度, elbow_flex弧度,
                    wrist_flex弧度, wrist_roll弧度, gripper弧度]
        row = [0.0, -0.30, -0.55, 0.35, 0.0, 1.7453]
        '''
        """输入一组关节弧度，驱动机械臂，返回夹爪中心点的地面放置位置(z=0)"""
        set_arm(self.data, self.model, dict(zip(self.JOINT_NAMES, row)))
        c = finger_centroid(self.model, self.data)
        return (float(c[0]), float(c[1]), 0.0)

    def update(self, dt, qt):
        """回放每帧更新逻辑，qt当前回放时间，dt时间步"""
        model, data = self.model, self.data
        # 获取当前回放时间的六个关节角度
        row = self._row(qt)
        set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))
        if not self.use_slider:
            # 纯回放手臂，不加载滑块
            mujoco.mj_forward(model, data) # type: ignore
            return

        idx = int(np.searchsorted(self.t, qt, side="right") - 1)
        idx = max(0, min(idx, len(self.t) - 1))
        want_hold = bool(self.hold_mask[idx])

        # 状态切换：从松开→抓取，记录滑块相对夹爪的位姿
        if want_hold and not self.holding:
            # 夹爪局部 → 世界 的齐次变换矩阵
            T_body = body_pose(model, data, "gripper")
            # 滑块局部 → 世界 的齐次变换矩阵
            T_box = np.eye(4)
            # 滑块相对于父节点的齐次变换矩阵，默认滑块水平摆正，没有旋转只有平移
            T_box[:3, 3] = data.qpos[self.qadr:self.qadr + 3].copy() # type: ignore
            # 逆矩阵 `inv(T_body)`：世界坐标 → 夹爪局部坐标
            # 相当于把变换方向反过来。
            # self.hold_T  滑块局部坐标系 → 夹爪局部坐标系的齐次变换矩阵
            self.hold_T = np.linalg.inv(T_body) @ T_box
            self.holding = True
        # 状态切换：抓取→松开，把滑块放到当前夹爪中心点
        elif (not want_hold) and self.holding:
            self.last_spot = self._finger_spot(row)
            self.holding = False
            place_slider(data, self.qadr, self.last_spot)

        if self.holding:
            # 抓取状态，滑块跟随夹爪同步运动
            # self.hold_T  滑块局部坐标系 → 夹爪局部坐标系的齐次变换矩阵
            # body_pose(model, data, "gripper") 夹爪->到世界的齐次变换矩阵
            # T_box 滑块->世界的齐次变换矩阵
            T_box = body_pose(model, data, "gripper") @ self.hold_T # type: ignore
            data.qpos[self.qadr:self.qadr + 3] = T_box[:3, 3] # type: ignore
            data.qpos[self.qadr + 3:self.qadr + 7] = rot_to_quat(T_box[:3, :3]) # type: ignore
            mujoco.mj_forward(model, data) # type: ignore
            # 兜底检测：滑块穿透地面，强制释放
            if slider_bottom_min(data, self.qadr, SLIDER_HALF) < -0.0006 and self.last_spot is not None:
                self.holding = False
                self.hold_T = None
                self.last_spot = self._finger_spot(row)
                place_slider(data, self.qadr, self.last_spot)
                mujoco.mj_forward(model, data) # type: ignore
        else:
            # 未抓取，固定在地面点位
            place_slider(data, self.qadr, self.last_spot)
            mujoco.mj_forward(model, data) # type: ignore


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

    '''
    加载xml文件生成模型文件，放入内存生成model数据
    整个仿真的全部静态规则与结构定义
    # 内部核心数据示例（对应6轴机械臂+滑块场景）：
    #   nbody = 8          刚体总数：世界根节点 + 机械臂6连杆 + red_slider滑块
    #   njnt = 7           关节总数：6个旋转关节 + 1个滑块自由关节
    #   nq = 13            qpos数组总长度：6个单自由度关节 + 7自由度自由关节
    #   jnt_qposadr[6] = 6 滑块关节在qpos中的起始下标
    #   qpos0              初始位姿数组，如滑块初始位置 [0.25, 0, 0.02]
    #   mesh_vert          全局网格顶点数组，含所有STL模型的顶点坐标
    '''
    model = load_model_with_slider()
    # 按照 model 的尺寸提前分配好了所有数组内存，只是初始值是默认的零值
    data = mujoco.MjData(model) # type: ignore
    '''
    用 model 里定义的初始值覆盖 data 的动态状态，同时把速度、受力、碰撞等全部清零
    主要修改data.qpos，也就是根据model初始化模型所在的相对坐标和相对位姿存储到data.qpos中
    '''
    mujoco.mj_resetData(model, data) # type: ignore
    '''
    根据qpos，和模型规则，通过正向运动学计算结果，算出模型的世界坐标xpos和世界位姿xmat
    '''
    mujoco.mj_forward(model, data) # type: ignore

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
        data = mujoco.MjData(model) # type: ignore
        mujoco.mj_resetData(model, data) # type: ignore
        mujoco.mj_forward(model, data) # type: ignore
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
    #shoulder_lift 往正方向回正
    Q_HOVER = {**Q_GRASP, "shoulder_lift": Q_GRASP["shoulder_lift"] - LIFT_RAISE, "gripper": GRIP_OPEN}
    Q_GRASP_A = {**Q_GRASP}
    Q_GRASP_B = {**Q_GRASP, "shoulder_pan": PAN_SWING}

    def spot_of(q):
        """输入抓取姿态q，驱动机械臂，计算抓取点在地面的投影坐标"""
        set_arm(data, model, {**q, "gripper": GRIP_CLOSE})
        c = finger_centroid(model, data)
        return float(c[0]), float(c[1]), 0.0

    # 计算A、B两个抓取点位 这个投影是基于世界坐标而言的，所以投影坐标(x,y)是
    # 在世界系下的坐标
    spotA = spot_of(Q_GRASP_A)
    spotB = spot_of(Q_GRASP_B)
    print(f"[run_sim] 抓取点 A={tuple(round(v, 3) for v in spotA)}"
          f"  B={tuple(round(v, 3) for v in spotB)}  (爪指质心投影)")

    qadr = slider_qadr(model)
    place_slider(data, qadr, spotA)
    mujoco.mj_forward(model, data) # type: ignore

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
