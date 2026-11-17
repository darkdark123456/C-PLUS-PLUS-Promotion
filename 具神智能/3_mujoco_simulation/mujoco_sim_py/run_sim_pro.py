# -*- coding: utf-8 -*-
""" run_sim_pro.py —— SO-ARM101 MuJoCo 自动演示【防穿地优化版】
====================================================================
在 run_sim.py 基础上的优化点（解决"夹爪经常深入地下"的问题）：
  1. 启动自动校准抓取姿态：按模型真实STL网格顶点搜索 shoulder_lift 抬高量，
     保证夹爪张开/合爪最低点离地 ≥ --margin(默认1.2cm)。
     实测原硬编码 Q_GRASP 姿态下固定爪最低点在地面以下约3.0cm，
     这就是"夹爪经常插进地下"的根源；校准后不再依赖手调数值，
     换标定文件(so101_new_calib.xml)也能自适应，不会再次戳地。
  2. 逐帧防穿地守卫：每帧插值出关节角后，实测夹爪最低点，若低于安全余量
     立即小步抬高肩关节修正。无论轨迹怎么插值，画面上夹爪都不会插进地面。
  3. 指尖落点：滑块放置点不再用整机爪部网格质心（含腕部叉架、偏差约2cm），
     而是按两片爪指各自的指尖区域来算（本版已升级为"对夹中心"，见第8条）。
  4. 放下滑块不再瞬移：松爪时把滑块放到当前指尖正下方地面，
     而不是回传到启动时算好的固定点位，消除释放瞬间的位置跳变。
  5. 回放模式同样受守卫保护：--replay 回放录制轨迹时若夹爪低于地面，
     也会自动抬高，避免录制高度与仿真地面不匹配导致穿地。
  6. 启动诊断打印：校准前离地高度、自动抬高量、A/B抓取点一目了然。
  7. 【本版新增】自动标定“真正夹得住滑块”的合拢角：
     原来沿用 GRIP_CLOSE=0.95 只是“半合”，动爪(上爪)指尖还在 82mm 高处，
     比固定爪(下爪, 12.6mm)高约 7cm，两爪根本没碰到 34mm×60mm 的滑块，
     画面上就是“下爪不再入地、上爪却夹不住滑块”。
     现在启动时用真实网格做二维搜索（粗搜合拢角 θ × 落点偏移 s 取两爪刚好贴住，
     再精搜到两爪各咬进滑块 0.3mm），标定出上下两爪都实实在在夹住滑块的组合。
  8. 【本版新增】落点改用“两爪各自指尖”算：
     旧 fingertip_spot 把两爪顶点合并后取“全局最低带”，本模型里动爪比固定爪高
     7cm，合并取最低带只会选中固定爪，滑块被放到固定爪正下方（偏差 19.3mm）。
     现在分别求两爪指尖质心，沿“合爪方向”偏移标定出的 s，并把滑块偏航角
     转到正对两爪，A/B 两个姿态都能夹正。

用法（与 run_sim.py 完全兼容）：
  python run_sim_pro.py --replay demo.npz    # 回放真实主臂录制的夹取动作(带守卫)
  python run_sim_pro.py                      # 内置脚本化演示（自动校准+守卫）
  python run_sim_pro.py --margin 0.02        # 自定义离地安全余量(米)，默认1.2cm

操作：左键拖拽旋转视角，滚轮缩放，ESC 退出。
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
GRIP_CLOSE = 0.55                     # 夹爪“合拢”角度初值(弧度)；启动时由 auto_calibrate_grip 自动标定
GRIP_STEP = 0.010                     # 标定合拢角的粗搜步长（弧度）
GRIP_REF_STEP = 0.001                 # 标定合拢角的精搜步长（弧度）
GRIP_OFF_MAX = 0.060                  # 标定落点偏移时，沿“合爪方向”的搜索上限（米）
GRIP_OFF_STEP = 0.0005                # 标定落点偏移的搜索步长（米）
GRIP_PEN = 0.0003                     # 精搜目标：让两爪各“咬进”滑块 0.3mm（保证真的贴上，肉眼看不出）
PAN_SWING = 0.9                       # A点到B点基座旋转角（弧度 ≈52°）
LIFT_RAISE = 0.55                     # “悬停/搬运”姿态，相比抓取姿态抬高的关节角度（弧度）
FLOOR_MARGIN = 0.012                  # 夹爪最低点离地安全余量(米)，自动校准与逐帧守卫共用
GUARD_STEP = 0.004                    # 逐帧守卫每次抬肩的步长（弧度，约抬升1.4mm）
TIP_BAND = 0.012                      # “指尖区域”判定的垂直带宽（米），用于计算抓取落点
# SO-ARM101机械臂全部关节名称，顺序：基座偏航→肩抬升→肘部弯曲→腕弯曲→腕旋转→夹爪
JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex",
               "wrist_flex", "wrist_roll", "gripper"]


# 抓取姿态（弧度）：肘部弯曲(~80°)、腕部下压，像人“弯腰下探”去拿物体。
# ⚠️这只是参考基准值！启动时 main() 会按模型真实网格自动校准
#    （见 auto_calibrate_grasp），保证夹爪最低点离地 ≥ FLOOR_MARGIN，
#    所以这里写多少都不会把夹爪戳进地里。
Q_GRASP = {
    "shoulder_pan": 0.0, "shoulder_lift": 1.36, "elbow_flex": -1.40,
    "wrist_flex": 1.16, "wrist_roll": 0.0, "gripper": GRIP_OPEN,
}
Q_HOME = {  # 起始待机姿态（机械臂收起，安全初始位置）
    "shoulder_pan": 0.0, "shoulder_lift": -0.30, "elbow_flex": -0.55,
    "wrist_flex": 0.35, "wrist_roll": 0.0, "gripper": GRIP_OPEN,
}
# 两片夹爪对应的mesh名称：动爪=上爪(随gripper关节摆动) / 固定爪=下爪(随腕部一起动)
JAW_MOVING = "moving_jaw_so101_v1"            # 动爪（上爪）：gripper 关节越大越张开、越小越合拢
JAW_FIXED = "wrist_roll_follower_so101_v1"    # 固定爪（下爪）：不随 gripper 关节动
JAW_MESHES = (JAW_MOVING, JAW_FIXED)

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
    ⚠️注意：是顶点几何平均，不是物理质心，工程上快速估算抓取位置
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
    ❗不调用mj_forward，后面读取的坐标、位姿全部是旧数据！
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

        # 计算最大的那个虚部分量；注意：矩阵下标i对应的是四元数虚部下标 i+1
        q[i + 1] = 0.5 * np.sqrt(max(0.0, 1 + R[i, i] - R[j, j] - R[k, k]))
        # 由已经算出的 q[i+1]，反推其余三个四元数分量
        q[0] = (R[k, j] - R[j, k]) / (4 * q[i + 1])
        q[j + 1] = (R[j, i] + R[i, j]) / (4 * q[i + 1])
        q[k + 1] = (R[k, i] + R[i, k]) / (4 * q[i + 1])

        # 解包得到 w,x,y,z
        w, x, y, z = q[0], q[1], q[2], q[3]

    return w, x, y, z


def quat_to_rot(q):
    """
    四元数(w,x,y,z) → 3×3旋转矩阵，与 rot_to_quat 互为逆运算。
    抓取瞬间要用它把滑块(qpos里的四元数姿态，含偏航角)转成旋转矩阵，
    否则 hold_T 会把滑块当成"无旋转"，夹起瞬间滑块会突然跳转一个偏航角。
    """
    w, x, y, z = float(q[0]), float(q[1]), float(q[2]), float(q[3])
    n = w * w + x * x + y * y + z * z
    if n < 1e-12:
        return np.eye(3)
    s = 2.0 / n
    return np.array([
        [1 - s * (y * y + z * z), s * (x * y - w * z),     s * (x * z + w * y)],
        [s * (x * y + w * z),     1 - s * (x * x + z * z),  s * (y * z - w * x)],
        [s * (x * z - w * y),     s * (y * z + w * x),      1 - s * (x * x + y * y)],
    ])


def slider_box_pose(data, qadr):
    """
    从qpos读出滑块的完整位姿（4×4齐次矩阵：平移+四元数姿态）
    用于抓取瞬间精确保存"滑块相对于夹爪"的相对位姿
    """
    T = np.eye(4)
    T[:3, 3] = data.qpos[qadr:qadr + 3]
    T[:3, :3] = quat_to_rot(data.qpos[qadr + 3:qadr + 7])
    return T


def slider_qadr(model):
    """
    获取red_slider自由关节在qpos数组内的起始偏移下标
    freejoint占用qpos连续7个位置：[x,y,z, qw,qx,qy,qz] 平移+四元数姿态
    """
    return model.jnt_qposadr[mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_JOINT, "red_slider")]


def place_slider(data, qadr, pos, yaw=0.0):
    """
    设置红色滑块在地面的位置与偏航角，其余姿态分量为0（只绕世界z轴旋转）
    参数 pos：(x,y,z) 世界坐标，z=0放在地面
         yaw：绕世界z轴的偏航角（弧度），让滑块一对面正对两爪，夹持时上下爪都能贴住
    qadr：滑块freejoint在qpos的起始偏移
    """
    data.qpos[qadr:qadr + 3] = pos
    # 绕z轴偏航的四元数 [cos(yaw/2), 0, 0, sin(yaw/2)]；yaw=0 即 [1,0,0,0] 无旋转
    h = 0.5 * float(yaw)
    data.qpos[qadr + 3:qadr + 7] = [np.cos(h), 0.0, 0.0, np.sin(h)]


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

# ================= 2.5) 防穿地安全层（自动校准 + 逐帧守卫 + 指尖落点） =================
def jaw_cache(model):
    """
    预缓存两片夹爪geom的编号和STL局部顶点数组（启动时调用一次）
    避免每帧重复扫描全部geom查找mesh，守卫计算更快
    返回：[(geom_id, 顶点数组N×3), ...]
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


def jaw_lowest_z(model, data, cache):
    """
    计算夹爪网格在世界坐标系下的最低点Z值（地面在z=0）
    顶点世界坐标 = geom平移b + 局部顶点v @ geom旋转R转置
    ❗已实测验证：该算法与MuJoCo碰撞引擎geomDistance(夹爪,地面)结果一致
    返回：float，最低点z；正值=离地高于地面，负值=已穿入地面
    """
    m = 1e9
    for g, v in cache:
        b = data.geom_xpos[g]
        R = data.geom_xmat[g].reshape(3, 3)
        m = min(m, float((b + v @ R.T)[:, 2].min()))
    return m


def _clearance_of(model, data, cache, q):
    """临时把机械臂设到姿态q，返回夹爪最低点离地高度（内部辅助函数）"""
    set_arm(data, model, q)
    return jaw_lowest_z(model, data, cache)


def clearance_worst(model, data, cache, q):
    """
    姿态q在“张爪/合爪”两种状态下的最差离地余量（取两者较小值）
    抓取流程中夹爪会在该姿态开合，所以两种状态都必须安全
    """
    return min(
        _clearance_of(model, data, cache, {**q, "gripper": GRIP_OPEN}),
        _clearance_of(model, data, cache, {**q, "gripper": GRIP_CLOSE}),
    )


def auto_calibrate_grasp(model, data, cache, q_ref, margin):
    """
    【核心优化1】启动自动校准抓取姿态
    问题：Q_GRASP是手调固定数值，原值夹爪固定爪最低点其实在地面以下约3cm，
          且是否穿地取决于so101_new_calib.xml的标定参数，换模型就会失效。
    方案：纯运动学搜索（不碰物理引擎，瞬间完成）——
          每个关节正反两个方向都试（不同构型下“抬臂”的方向不同！），
          优先肩、再肘、再腕，每步5mrad，直到张爪/合爪最低点都 ≥ margin。
    返回：(校准后姿态dict, 校准动作描述str, 校准后余量float)
    """
    # 先量原姿态（张爪/合爪取最差）
    c0 = clearance_worst(model, data, cache, q_ref)
    if c0 >= margin:
        return dict(q_ref), "无需校准", c0
    # 搜索计划：每个关节先试“更有希望”的方向，不行再试反方向
    plans = [("shoulder_lift", (-1, +1)), ("elbow_flex", (+1, -1)), ("wrist_flex", (+1, -1))]
    for joint, signs in plans:
        for sign in signs:
            for d in np.arange(0.005, 0.60, 0.005):
                q = {**q_ref, joint: q_ref[joint] + sign * d}
                c = clearance_worst(model, data, cache, q)
                if c >= margin:
                    s = "-" if sign < 0 else "+"
                    return q, f"{joint} {s}{d:.3f} rad", float(c)
    # 所有方向都不达标（极端情况），返回原姿态，交给逐帧守卫兜底
    return dict(q_ref), "搜索失败(由逐帧守卫兜底)", c0


def floor_guard(model, data, cache, q, margin, step=GUARD_STEP, max_iter=200):
    """
    【核心优化2】逐帧防穿地守卫（最后一道防线）
    先把q设进data，实测夹爪最低点；若低于margin则小步“抬臂”直到安全：
      1. 先探测方向：shoulder_lift ±3步各试一次，哪边离地更高往哪边抬
         （前倾构型是减小shoulder_lift抬臂；后仰/回折构型是增大才抬臂，
          只会单方向抬的守卫在回放数据上会越修越深——实测教训）
      2. 肩关节两个方向都不行，再依次试肘、腕
    成功即返回修正后的关节字典（data停在修正后状态）；全失败则尽力返回。
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


def jaw_tips(model, data, cache, band=TIP_BAND):
    """
    【核心优化8a】分别求两片爪指【各自】指尖区域（自身最低点上方band内的顶点）
    的质心与最低点高度。
    ⚠️必须分开算：旧 fingertip_spot 是把两爪顶点合并后取“全局最低带”，
       而本模型里动爪(上爪)最低点比固定爪(下爪)高约7cm，合并取最低带只会
       选中固定爪 → 滑块被放到固定爪正下方(实测偏19.3mm)，动爪悬在上方够不着。
    返回：{mesh名: (最低点z, 指尖质心[x,y,z])}
    """
    out = {}
    for g, v in cache:
        nm = model.mesh(model.geom(g).dataid).name
        b = data.geom_xpos[g]
        R = data.geom_xmat[g].reshape(3, 3)
        w = b + v @ R.T
        sel = w[w[:, 2] < w[:, 2].min() + band]
        out[nm] = (float(w[:, 2].min()), sel.mean(0))
    return out


def jaw_collide_ids(model):
    """
    【核心优化8b】取出两片爪指的【碰撞geom】编号（测距/接触检测用）。
    注意：同一个STL在XML里被声明了 visual 和 collision 两个geom，
          visual 的 contype=0（手臂默认不参与碰撞），必须挑 contype!=0 的那个。
    返回：{mesh名: geom编号}
    """
    out = {}
    for g in range(model.ngeom):
        if model.geom(g).type != mujoco.mjtGeom.mjGEOM_MESH:
            continue
        if int(model.geom_contype[g]) == 0:
            continue
        nm = model.mesh(model.geom(g).dataid).name
        if nm in JAW_MESHES:
            out.setdefault(nm, g)
    return out


def grip_spot(model, data, cache, offset=None, band=TIP_BAND):
    """
    【核心优化8c】由当前机械臂姿态算出“对夹中心”的地面落点与滑块偏航角：
      原点 = 固定爪(下爪)指尖的地面投影；
      方向u = 固定爪指尖→动爪指尖的水平单位向量（即合爪方向，随姿态一起转）；
      落点 = 原点 + offset×u；
      offset=None 时取两爪尖间距的一半（中点），用于回放这类合爪角未知的场合。
    返回 (x, y, yaw)：yaw 让滑块一对面正对两爪，使上下两爪同时贴住滑块；
                     z 由调用方置0（滑块底面贴地）。
    """
    tips = jaw_tips(model, data, cache, band)
    f = tips[JAW_FIXED][1][:2]
    m = tips[JAW_MOVING][1][:2]
    d = m - f
    n = float(np.hypot(d[0], d[1]))
    u = d / n if n > 1e-9 else np.array([1.0, 0.0])
    s = (n / 2.0) if offset is None else float(offset)
    p = f + u * s
    return float(p[0]), float(p[1]), float(np.arctan2(u[1], u[0]))


def signed_gap(model, data, g1, g2, far=0.12):
    """
    两geom的带符号距离：>0 分离、≈0 贴合、<0 穿透（MuJoCo mj_geomDistance）
    far：超过该距离就按 far 返回，用于加速
    """
    return float(mujoco.mj_geomDistance(model, data, g1, g2, far, np.zeros(6)))


def auto_calibrate_grip(model, data, cache, q_grasp):
    """
    【核心优化7】启动自动标定“真正能夹住滑块”的合拢角 + 落点偏移。
    问题：GRIP_CLOSE=0.95 只是“半合”——此时动爪(上爪)指尖在82mm高处、
          固定爪(下爪)在12.6mm，两爪纵向差约7cm，谁都没碰到34mm×60mm的滑块，
          画面上就是“下爪不再入地、上爪却夹不住”。
    方案：不猜数值，用真实网格做二维几何搜索（不跑物理，瞬间完成）：
          ① 粗搜：合爪角 θ 从 GRIP_OPEN 向关节下限逐档扫，每个θ沿合爪方向扫
             落点偏移 s，取 |d_fixed|+|d_moving| 最小者（两爪刚好同时贴住滑块）；
          ② 精搜：在粗搜结果附近用细步长再扫一遍，目标改成让两爪各“咬进”
             GRIP_PEN(=0.3mm)，确保上下两爪都实实在在贴住（肉眼看不出这点重叠）。
    返回：(合拢角θ, 落点偏移s(米), d_fixed, d_moving, 描述str)
    """
    coll = jaw_collide_ids(model)
    jr = joint_range(model, "gripper")
    if len(coll) < 2 or jr is None:
        return GRIP_CLOSE, 0.0, np.nan, np.nan, "缺少夹爪碰撞体/关节限位，沿用默认合拢角"
    sg = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_GEOM, "slider_geom")
    qadr = slider_qadr(model)
    lo = float(jr[0])

    def scan(angs, svals, target):
        """给定若干合爪角与落点偏移，返回最接近 target 距离的 (cost,θ,s,d_fixed,d_moving)"""
        best = None
        for ang in angs:
            set_arm(data, model, {**q_grasp, "gripper": float(ang)})
            tips = jaw_tips(model, data, cache)
            f = tips[JAW_FIXED][1][:2]
            d = tips[JAW_MOVING][1][:2] - f
            n = float(np.hypot(d[0], d[1]))
            if n < 1e-9:
                continue           # 两爪尖重合，方向不可用，跳过该角度
            u = d / n
            yaw = float(np.arctan2(u[1], u[0]))   # 让滑块一对面正对两爪
            for s in svals:
                p = f + u * s
                place_slider(data, qadr, (float(p[0]), float(p[1]), 0.0), yaw)
                mujoco.mj_forward(model, data)
                df = signed_gap(model, data, coll[JAW_FIXED], sg)
                dm = signed_gap(model, data, coll[JAW_MOVING], sg)
                cost = abs(df - target) + abs(dm - target)
                if best is None or cost < best[0]:
                    best = (cost, float(ang), float(s), df, dm)
        return best

    # ① 粗搜：两爪刚好同时贴住滑块(目标距离0)
    coarse = scan(np.arange(GRIP_OPEN, lo - 1e-9, -GRIP_STEP),
                  np.arange(0.0, GRIP_OFF_MAX + 1e-9, GRIP_OFF_STEP), 0.0)
    if coarse is None:
        return GRIP_CLOSE, 0.0, np.nan, np.nan, "搜索失败，沿用默认合拢角"
    a1, s1 = coarse[1], coarse[2]
    # ② 精搜：在粗搜结果邻域细扫，目标改成两爪各咬进滑块 GRIP_PEN，确保都贴住
    fine = scan(np.arange(a1 + GRIP_STEP, a1 - 2 * GRIP_STEP - 1e-9, -GRIP_REF_STEP),
                np.arange(max(0.0, s1 - 3 * GRIP_OFF_STEP), s1 + 3 * GRIP_OFF_STEP + 1e-9,
                          GRIP_OFF_STEP / 2.5),
                -GRIP_PEN)
    best = fine if fine is not None else coarse
    _cost, ang, s, df, dm = best
    how = (f"合拢角 θ={ang:.3f}rad（张开{GRIP_OPEN:.3f}→合拢{ang:.3f}）"
           f"；落点沿合爪方向偏移 s={s * 1000:.1f}mm"
           f"；标定处 下爪/上爪距滑块 {df * 1000:+.2f}/{dm * 1000:+.2f}mm（0=刚好贴住）")
    return ang, s, df, dm, how


# ================= 3) 演示编排（状态机） =================
class PickPlacePro:
    """
    自动拾取放置状态机【Pro】：A点夹取→搬运到B放下，B夹取搬运回A，循环往复
    原理：分多阶段phase，缓动插值控制机械臂关节；抓取时滑块绑定在夹爪坐标系跟随运动
    相比run_sim.py的PickPlace：
      1. 每帧过一遍防穿地守卫floor_guard，夹爪永远不会插进地面
      2. 释放滑块时放到“当前指尖正下方”，而不是固定点位，消除释放瞬移
      3. 合爪角用标定出来的 grip_ang、落点用标定出来的 grip_s，上下两爪都能夹住滑块
    触地即放逻辑：下降阶段滑块底部碰到地面，自动释放，防止物体穿透地面
    """

    def __init__(self, model, data, q_hover, q_grasp_a, q_grasp_b, q_home, spots,
                 margin=FLOOR_MARGIN, grip_ang=GRIP_CLOSE, grip_s=0.0):
        self.model, self.data = model, data
        self.q_home = q_home
        self.q = dict(q_home)  # 当前关节角度状态
        self.q_hover = q_hover # 悬停姿态（抓取前抬起来的安全高度）
        self.q_grasp = [q_grasp_a, q_grasp_b] # A、B两点抓取姿态
        self.spots = spots # A,B两个地面落点 (x, y, yaw)
        self.qadr = slider_qadr(model) # 滑块freejoint在qpos中的偏移下标
        self.slider_half = SLIDER_HALF # 滑块半尺寸保存
        self.jaw_cache = jaw_cache(model) # 夹爪网格缓存（防穿地守卫用）
        self.margin = margin              # 夹爪离地安全余量
        self.grip_ang = grip_ang          # 标定出的合拢角（真正能夹住滑块）
        self.grip_s = grip_s              # 标定出的落点沿合爪方向的偏移

        U0 = q_hover
        U1 = {**q_hover, "shoulder_pan": PAN_SWING} # 旋转后的悬停姿态
        A, B = q_grasp_a, q_grasp_b
        G, GO = grip_ang, GRIP_OPEN
        # 动作阶段列表：(名称,目标关节q,目标夹爪角度,阶段持续时间,事件标记)
        leg = [
            ("desc_A", A, GO, 2.6, "approach"),    # 靠近A点，下探准备抓取
            ("close_A", A, G, 0.9, "grab"),        # 合爪抓取（合到标定角，上下爪贴住滑块）
            ("lift_A", U0, G, 2.2, None),          # 抬起，离开地面
            ("pan_AB", U1, G, 2.8, None),          # 基座旋转，移动到B上方
            ("down_B", B, G, 2.2, "place"),        # 下降到B点，place标记：触地释放
            ("open_B", B, GO, 0.9, None),          # 张开夹爪放下物体
            ("up_B", U1, GO, 2.0, None),           # 抬升离开
            ("desc_B", B, GO, 2.2, "approach"),    # 靠近B点准备抓取
            ("close_B", B, G, 0.9, "grab"),        # B点合爪抓取
            ("lift_B", U1, G, 2.0, None),          # B点抬起
            ("pan_BA", U0, G, 2.8, None),          # 旋转回到A上方
            ("down_A", A, G, 2.2, "place"),        # 下降放到A点
            ("open_A", A, GO, 0.9, None),          # 张开夹爪释放
            ("up_A", U0, GO, 2.0, None),           # 抬升离开
        ]
        self.phases = leg
        self.i = 0  # 当前阶段索引
        self.t = 0.0 # 当前阶段已经流逝的时间
        self.holding = False # 是否正在抓取、吸附滑块
        self.hold_T = None   # 抓取保存的齐次变换矩阵：滑块相对于gripper的相对位姿
        self.cur_spot = 0    # 当前滑块所在点位索引0:A，1:B

    def _release(self):
        """
        释放滑块：取消吸附，把滑块放到【当前对夹中心】的地面
        对比旧版放回固定点位：避免下降途中提前触地释放时滑块横向瞬移
        落点用 grip_spot（固定爪尖沿合爪方向偏移 grip_s）+ 正对两爪的偏航角，
        这样下次来夹时上下两爪仍然正好夹住滑块
        """
        self.holding = False
        self.hold_T = None
        self.cur_spot = 1 - self.cur_spot
        x, y, yaw = grip_spot(self.model, self.data, self.jaw_cache, offset=self.grip_s)
        place_slider(self.data, self.qadr, (x, y, 0.0), yaw)
        mujoco.mj_forward(self.model, self.data)

    def update(self, dt):
        """
        每帧更新状态机，dt是单帧时间步长
        1. 时间累加，缓动插值生成当前目标关节角度
        2. 过防穿地守卫（低于安全余量自动抬高肩关节）后驱动机械臂
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
        # 【防穿地守卫】本帧姿态若使夹爪低于安全余量，自动抬高肩关节修正
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
            # 未抓取，滑块固定放在地面落点(x, y, yaw)——yaw让它正对两爪
            sx, sy, syaw = self.spots[self.cur_spot]
            place_slider(data, self.qadr, (sx, sy, 0.0), syaw)
            mujoco.mj_forward(model, data)

        # 当前阶段时间跑完，切换下一阶段
        if self.t >= dur:
            if ev == "grab":
                # grab事件：抓取，记录滑块相对于夹爪的相对变换矩阵hold_T
                # 用滑块的完整位姿（含偏航角），保证夹起瞬间不跳转、搬运中夹持关系不变
                T_body = body_pose(model, data, "gripper")
                self.hold_T = np.linalg.inv(T_body) @ slider_box_pose(data, self.qadr)
                self.holding = True
            elif ev == "place":
                self._release()
            self.i = (self.i + 1) % len(self.phases)
            self.t = 0.0


# ================= 4) 回放录制轨迹（带防穿地守卫） =================
class ReplayDemoPro:
    """
    回放演示类【Pro】：回放sim_teleop录制的npz轨迹文件 + 逐帧防穿地守卫
    原理：读取录制的归一化关节信号，转换为弧度，按时间插值驱动机械臂
    自动识别合爪/张爪：合爪吸附滑块跟随夹爪；张开时将滑块放置在当前指尖下方
    相比run_sim.py的ReplayDemo：
      1. 每帧过防穿地守卫：录制数据的高度和仿真地面不匹配时自动抬高，不会穿地
      2. 释放落点用指尖质心，更贴近真实夹持位置
    """

    JOINT_NAMES = ["shoulder_pan", "shoulder_lift", "elbow_flex",
                   "wrist_flex", "wrist_roll", "gripper"]

    def __init__(self, model, data, path, speed=1.0, start_sec=0.0, len_sec=None,
                 use_slider=True, guard=True, margin=FLOOR_MARGIN):
        z = np.load(path)
        traj = z["traj"]                       # traj数组(N,7): 时间t + 6个关节归一化值
        self.model, self.data = model, data
        self.use_slider = use_slider # 是否启用红色滑块模拟抓取
        self.speed = speed           # 回放速度倍率
        self.guard = guard           # 是否启用逐帧防穿地守卫
        self.margin = margin         # 夹爪离地安全余量
        self.jaw_cache = jaw_cache(model) # 夹爪网格缓存（守卫用）
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

    def _put(self, spot):
        """把滑块按落点 spot=(x, y, yaw) 放到地面上"""
        place_slider(self.data, self.qadr, (spot[0], spot[1], 0.0), spot[2])

    def _reset(self):
        """回放循环重置，初始化滑块位置"""
        if not self.use_slider:
            return
        s = self.holds[0][0]
        self.last_spot = self._finger_spot(self.rad[s])
        self.holding = False
        self.hold_T = None
        self._put(self.last_spot)
        mujoco.mj_forward(self.model, self.data)

    def _row(self, qt):
        """根据当前回放时间qt，插值得到6个关节弧度值，返回一维数组"""
        return np.array([np.interp(qt, self.t, self.rad[:, c]) for c in range(6)])

    def _finger_spot(self, row):
        """
        输入一组关节弧度，返回滑块地面落点 (x, y, yaw)
        落点取两片爪指指尖质心的中点（回放时合爪角由录制数据决定，用中点最稳），
        yaw 让滑块一对面正对两爪，便于看"上下爪都夹住"
        注意：会临时改变data中的手臂姿态（用于测量），调用方负责恢复
        """
        set_arm(self.data, self.model, dict(zip(self.JOINT_NAMES, row)))
        return grip_spot(self.model, self.data, self.jaw_cache)

    def update(self, dt, qt):
        """回放每帧更新逻辑，qt当前回放时间，dt时间步"""
        model, data = self.model, self.data
        row = self._row(qt)
        if self.guard:
            # 【防穿地守卫】回放角度若使夹爪低于地面，自动抬高肩关节修正
            qd = floor_guard(model, data, self.jaw_cache,
                             dict(zip(self.JOINT_NAMES, row)), self.margin)
            row = np.array([qd[n] for n in self.JOINT_NAMES])
        else:
            set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))
        if not self.use_slider:
            # 纯回放手臂，不加载滑块
            mujoco.mj_forward(model, data)
            return

        idx = int(np.searchsorted(self.t, qt, side="right") - 1)
        idx = max(0, min(idx, len(self.t) - 1))
        want_hold = bool(self.hold_mask[idx])

        # 状态切换：从松开→抓取，记录滑块相对夹爪的位姿（含偏航角，避免夹起瞬间跳转）
        if want_hold and not self.holding:
            T_body = body_pose(model, data, "gripper")
            self.hold_T = np.linalg.inv(T_body) @ slider_box_pose(data, self.qadr)
            self.holding = True
        # 状态切换：抓取→松开，把滑块放到当前对夹中心，并恢复本帧手臂姿态
        elif (not want_hold) and self.holding:
            self.last_spot = self._finger_spot(row)
            self.holding = False
            self._put(self.last_spot)
            set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))

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
                set_arm(data, model, dict(zip(self.JOINT_NAMES, row)))  # 恢复手臂姿态
                self._put(self.last_spot)
                mujoco.mj_forward(model, data)
        else:
            # 未抓取，固定在地面落点
            self._put(self.last_spot)
            mujoco.mj_forward(model, data)


def main():
    parser = argparse.ArgumentParser(description="SO-ARM101 MuJoCo 自动演示（防穿地优化版）")
    parser.add_argument("--replay", default=None,
                        help="回放 --leader --record 录下的动作文件(.npz)，用真实关节角夹取")
    parser.add_argument("--speed", type=float, default=1.0, help="回放速度倍率")
    parser.add_argument("--start", type=float, default=0.0,
                        help="从录制文件的第几秒开始回放(可跳过开头静止段)")
    parser.add_argument("--len", type=float, default=None,
                        help="回放时长(秒)，默认到结尾")
    parser.add_argument("--margin", type=float, default=FLOOR_MARGIN,
                        help="夹爪最低点离地安全余量(米)，默认1.2cm")
    args = parser.parse_args()

    model = load_model_with_slider()
    data = mujoco.MjData(model)
    mujoco.mj_resetData(model, data)
    mujoco.mj_forward(model, data)

    # ---------- 回放真实录制（带防穿地守卫，只展示真实手臂动作） ----------
    if args.replay:
        path = args.replay
        if not os.path.exists(path):
            alt = os.path.join(SIM_DIR, os.path.basename(path))
            if os.path.exists(alt):
                path = alt
            else:
                print(f"[run_sim_pro] 找不到录制文件 {args.replay}。")
                print("  请确认在 mujoco_sim 目录下运行，或先录制：")
                print("    python sim_teleop.py --leader --record demo.npz")
                return
        model = load_model_plain()            # 回放模式：不注入红色滑块
        data = mujoco.MjData(model)
        mujoco.mj_resetData(model, data)
        mujoco.mj_forward(model, data)
        anim = ReplayDemoPro(model, data, path, speed=args.speed,
                             start_sec=args.start, len_sec=args.len,
                             use_slider=False, guard=True, margin=args.margin)
        print(f"[run_sim_pro] 回放录制 {path}：{anim.duration:.1f}s（守卫开启，防穿地）"
              f"（录制开头约有 {anim.start_hint:.1f}s 静止段；想跳过可加 --start {anim.start_hint:.0f}）")
        set_arm(data, model, dict(zip(ReplayDemoPro.JOINT_NAMES, anim.rad[0])))
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

    # ---------- 内置脚本化演示（自动校准 + 逐帧守卫） ----------
    cache = jaw_cache(model)
    # 校准前原姿态余量（张爪/合爪取最差）
    c_before = clearance_worst(model, data, cache, Q_GRASP)
    # 【核心优化】启动自动校准抓取姿态，保证不穿地
    q_grasp, how, clr = auto_calibrate_grasp(model, data, cache, Q_GRASP, args.margin)
    print(f"[run_sim_pro] 校准前抓取姿态夹爪最低点离地 {c_before * 100:+.1f}cm"
          f"（目标余量 {args.margin * 100:.1f}cm）")
    print(f"[run_sim_pro] 自动校准：{how}；校准后余量 {clr * 100:.1f}cm")

    # 【核心优化7】标定“真正夹得住滑块”的合拢角与落点偏移（修“上爪夹不住滑块”）
    grip_ang, grip_s, gdf, gdm, ghow = auto_calibrate_grip(model, data, cache, q_grasp)
    print(f"[run_sim_pro] 夹爪对夹自动标定：{ghow}")

    # 悬停姿态：抓取姿态基础上抬高LIFT_RAISE
    Q_HOVER = {**q_grasp, "shoulder_lift": q_grasp["shoulder_lift"] - LIFT_RAISE, "gripper": GRIP_OPEN}
    Q_GRASP_A = {**q_grasp}
    Q_GRASP_B = {**q_grasp, "shoulder_pan": PAN_SWING}

    def spot_of(q):
        """
        输入抓取姿态q，返回该点的地面落点(x, y, yaw)
        合爪角取标定值grip_ang，落点=固定爪尖+标定偏移grip_s×合爪方向，
        此时上下两爪正好夹住滑块（不再只把滑块塞在下爪正下方）
        """
        set_arm(data, model, {**q, "gripper": grip_ang})
        return grip_spot(model, data, cache, offset=grip_s)

    # 计算A、B两个抓取点位
    spotA = spot_of(Q_GRASP_A)
    spotB = spot_of(Q_GRASP_B)
    print(f"[run_sim_pro] 抓取点 A=({spotA[0]:.3f}, {spotA[1]:.3f}, yaw={np.degrees(spotA[2]):+.1f}°)"
          f"  B=({spotB[0]:.3f}, {spotB[1]:.3f}, yaw={np.degrees(spotB[2]):+.1f}°)  (两爪对夹中心)")

    qadr = slider_qadr(model)
    place_slider(data, qadr, (spotA[0], spotA[1], 0.0), spotA[2])
    mujoco.mj_forward(model, data)

    # 初始化拾取放置状态机（每帧自带防穿地守卫，合爪角/落点用标定值）
    anim = PickPlacePro(model, data, Q_HOVER, Q_GRASP_A, Q_GRASP_B, Q_HOME,
                        spots=[spotA, spotB], margin=args.margin,
                        grip_ang=grip_ang, grip_s=grip_s)
    print("[run_sim_pro] 演示开始：夹取 A → 放到 B → 夹回 A → ...（ESC 退出）")
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
