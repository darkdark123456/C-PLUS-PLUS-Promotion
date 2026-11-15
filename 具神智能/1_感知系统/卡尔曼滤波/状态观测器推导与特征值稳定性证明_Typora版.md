# 状态观测器：从状态方程到误差收敛的完整推导

> 本笔记整理了从真实系统、观测器方程、误差方程，到矩阵指数解和特征值稳定性条件的完整逻辑，并回答学习过程中容易混淆的问题。







![image-20261115150222778](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115150222778.png)



![image-20261115150442880](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115150442880.png)



![image-20261115150557797](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115150557797.png)



![image-20261115150735821](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115150735821.png)





![image-20261115151337621](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115151337621.png)



### ！！！**要想eobs(t)观测误差尽可能趋近于零，在t->∞时，只需矩阵A-KC的特征值的实部<0,下面有具体证明**

## 1. 先弄清楚：系统内部的方程是什么

控制理论通常先建立一个系统的数学模型。对于连续时间线性定常系统（LTI），常写成：

$$
\boxed{\dot{x}=Ax+Bu}
$$

$$
\boxed{y=Cx}
$$

各符号含义：

| 符号 | 含义 |
|---|---|
| \(x\) | 系统状态向量，描述系统当前内部状态 |
| \(\dot{x}\) | 状态向量对时间的一阶导数，即状态变化率 |
| \(u\) | 外部输入或控制输入 |
| \(y\) | 系统实际输出、可测量量 |
| \(A\) | 系统矩阵，描述状态自身的动态关系 |
| \(B\) | 输入矩阵，描述输入如何影响状态 |
| \(C\) | 输出矩阵，描述状态如何映射为可测量输出 |





### 1.1 这些方程从哪里来？

![image-20261115150242020](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115150242020.png)

它们不是凭空规定的，而是通过具体系统的物理规律建立，或通过实验数据进行系统辨识得到。

例如，一个简化小车模型，令位置为 \(p\)、速度为 \(v\)，输入力为 \(u\)，质量为 \(m\)：

$$
\dot p=v,\qquad m\dot v=u
$$

定义状态向量：

$$
 x=\begin{bmatrix}p\\v\end{bmatrix}
$$

则：

$$
\dot x=\begin{bmatrix}\dot p\\\dot v\end{bmatrix}
=\begin{bmatrix}v\\u/m\end{bmatrix}
=\underbrace{\begin{bmatrix}0&1\\0&0\end{bmatrix}}_{A}
\underbrace{\begin{bmatrix}p\\v\end{bmatrix}}_{x}
+\underbrace{\begin{bmatrix}0\\1/m\end{bmatrix}}_{B}u
$$

因此得到 \(\dot{x}=Ax+Bu\)。若传感器只测量位置，则：

$$
y=p=\begin{bmatrix}1&0\end{bmatrix}x=Cx
$$

这说明：状态方程描述系统内部状态如何变化；输出方程描述我们能测量到什么。

> 注意：这个小车只是说明状态空间模型如何从物理规律整理出来的简化例子。真实系统的模型可能还包含摩擦、阻尼、弹性、负载等因素。

## 2. 为什么需要状态观测器

有些状态无法直接测量。例如小车有位置和速度两个状态，但传感器可能只测量位置。我们希望利用：

- 已知的系统模型 \(A,B,C\)；
- 已知的输入 \(u\)；
- 实际测量输出 \(y\)；

来估计无法直接测量的状态。

观测器内部维护一个估计状态 \(\hat{x}\)，并根据模型预测系统状态。

## 3. 观测器方程为什么这样写

先按系统模型建立一个预测部分：

$$
\dot{\hat{x}}=A\hat{x}+Bu
$$

但只靠模型预测，估计可能与真实系统有偏差。因此把实际输出与预测输出比较：

真实输出：

$$
y=Cx
$$

观测器预测输出：

$$
\hat{y}=C\hat{x}
$$

输出误差：

$$
y-\hat{y}
$$

把这个误差乘以观测器增益矩阵 \(K\)，作为修正项加入预测模型：

$$
\boxed{\dot{\hat{x}}=A\hat{x}+Bu+K(y-\hat{y})}
$$

其中：

- \(A\hat{x}+Bu\)：依据系统模型进行状态预测；
- \(y-\hat{y}\)：实际输出与预测输出之间的误差；
- \(K(y-\hat{y})\)：把输出误差反馈到状态估计中，修正预测结果；
- \(K\)：观测器增益，决定误差反馈对状态估计的影响程度。

直观地说：模型负责预测，测量误差负责纠偏，\(K\) 决定纠偏的方式和力度。

## 4. 从头推导观测误差方程

### 第一步：定义状态估计误差

定义真实状态与估计状态之差：

$$
\boxed{e_{obs}=x-\hat{x}}
\tag{1}
$$

这里的 \(e_{obs}\) 是误差变量（或误差向量），不是自然常数。

### 第二步：两边对时间求导

$$
\boxed{\dot e_{obs}=\dot{x}-\dot{\hat{x}}}
\tag{2}
$$

符号上方的小点表示对时间求一阶导数：

$$
\dot e_{obs}=\frac{d e_{obs}}{dt}
$$

### 第三步：代入真实系统和观测器方程

真实系统：

$$
\dot{x}=Ax+Bu
$$

观测器：

$$
\dot{\hat{x}}=A\hat{x}+Bu+K(y-\hat{y})
$$

代入式 (2)：

$$
\begin{aligned}
\dot e_{obs}
&=Ax+Bu-[A\hat{x}+Bu+K(y-\hat{y})]\\
&=Ax+Bu-A\hat{x}-Bu-K(y-\hat{y})\\
&=A(x-\hat{x})-K(y-\hat{y})
\end{aligned}
$$

因为 \(+Bu-Bu=0\)，输入项抵消。

再利用误差定义 \(x-\hat{x}=e_{obs}\)：

$$
\boxed{\dot e_{obs}=Ae_{obs}-K(y-\hat{y})}
\tag{3}
$$

### 第四步：推导输出误差关系

由真实输出方程和观测器输出方程：

$$
y=Cx,\qquad \hat{y}=C\hat{x}
$$

两式相减：

$$
\begin{aligned}
y-\hat{y}
&=Cx-C\hat{x}\\
&=C(x-\hat{x})\\
&=Ce_{obs}
\end{aligned}
$$

因此：

$$
\boxed{y-\hat{y}=Ce_{obs}}
\tag{4}
$$

### 第五步：代回式 (3)

$$
\begin{aligned}
\dot e_{obs}
&=Ae_{obs}-KCe_{obs}\\
&=(A-KC)e_{obs}
\end{aligned}
$$

最终得到观测误差方程：

$$
\boxed{\dot e_{obs}=(A-KC)e_{obs}}
\tag{5}
$$

**关键理解：** 真实系统和观测器使用相同输入 \(u\)，所以相减时 \(Bu\) 抵消。误差如何变化由矩阵 \(A-KC\) 决定。

## 5. 为什么微分方程的解突然出现指数

容易混淆的地方是：式 (5) 右边没有指数，为什么解中出现指数？

原因是：**指数不是原方程中突然多出来的项，而是求解常系数线性微分方程后得到的函数形式。**

### 5.1 先看一维标量例子

考虑：

$$
\dot e=ae
$$

也就是：

$$
\frac{de}{dt}=ae
$$

分离变量：

$$
\frac{1}{e}de=a\,dt
$$

两边积分：

$$
\int\frac{1}{e}de=\int a\,dt
$$

得到：

$$
\ln|e|=at+C
$$

两边取指数，并把常数合并：

$$
\boxed{e(t)=C_1e^{at}}
$$

若给定初始条件 \(e(0)\)，则 \(C_1=e(0)\)，所以：

$$
\boxed{e(t)=e^{at}e(0)}
$$

例如 \(\dot e=-2e\)，且 \(e(0)=10\)，则：

$$
e(t)=10e^{-2t}
$$

误差会随时间衰减。

### 5.2 推广到矩阵方程

观测误差方程为：

$$
\dot e_{obs}=(A-KC)e_{obs}
$$

令：

$$
M=A-KC
$$

则：

$$
\dot e_{obs}=Me_{obs}
$$

这是常系数线性微分方程的矩阵形式。其解为：

$$
\boxed{e_{obs}(t)=e^{Mt}e_{obs}(0)}
$$

代回 \(M=A-KC\)：

$$
\boxed{e_{obs}(t)=e^{(A-KC)t}e_{obs}(0)}
\tag{6}
$$

这里的 \(e^{Mt}\) 是**矩阵指数**，不是把矩阵当成普通数字直接做幂运算。

矩阵指数定义为幂级数：

$$
\boxed{e^{Mt}=I+Mt+\frac{(Mt)^2}{2!}+\frac{(Mt)^3}{3!}+\cdots}
$$

其中 \(I\) 是单位矩阵。

可以验证这个解：

$$
\frac{d}{dt}\left(e^{Mt}e_{obs}(0)\right)
=Me^{Mt}e_{obs}(0)
=Me_{obs}(t)
$$

它满足原方程，并且在 \(t=0\) 时：

$$
e_{obs}(0)=e^{M\cdot0}e_{obs}(0)=Ie_{obs}(0)
$$

因此它是满足初始条件的解。

> 以上直接使用 \(e^{Mt}\) 的形式，前提是 M 为常数矩阵。若 M 随时间变化，一般需要使用状态转移矩阵，不能直接套用这个简单形式。

## 6. 为什么看矩阵特征值就能判断误差是否收敛

由式 (6)：

$$
e_{obs}(t)=e^{Mt}e_{obs}(0)
$$

初始误差 \(e_{obs}(0)\) 已经给定。因此，要让误差趋于零，关键是让：

$$
\boxed{e^{Mt}\rightarrow0\quad(t\rightarrow\infty)}
$$

矩阵指数的长期变化由矩阵 M 的特征值决定。

### 6.1 先看一个容易计算的对角矩阵

令：

$$
M=\begin{bmatrix}-2&0\\0&-3\end{bmatrix}
$$

它的特征值为：

$$
\lambda_1=-2,\qquad\lambda_2=-3
$$

因为它是对角矩阵，矩阵指数为：

$$
e^{Mt}=\begin{bmatrix}e^{-2t}&0\\0&e^{-3t}\end{bmatrix}
$$

假设初始误差：

$$
e(0)=\begin{bmatrix}10\\20\end{bmatrix}
$$

则：

$$
\begin{aligned}
e(t)&=e^{Mt}e(0)\\
&=\begin{bmatrix}e^{-2t}&0\\0&e^{-3t}\end{bmatrix}
\begin{bmatrix}10\\20\end{bmatrix}\\
&=\begin{bmatrix}10e^{-2t}\\20e^{-3t}\end{bmatrix}
\end{aligned}
$$

当 \(t\rightarrow\infty\) 时：

$$
e^{-2t}\rightarrow0,\qquad e^{-3t}\rightarrow0
$$

所以：

$$
\boxed{e(t)\rightarrow\begin{bmatrix}0\\0\end{bmatrix}}
$$

这个例子说明：负特征值对应的指数项会衰减。

### 6.2 一般矩阵为什么也看特征值

若 M 可对角化，可写成：

$$
M=V\Lambda V^{-1}
$$

其中 \(\Lambda\) 是由特征值组成的对角矩阵：

$$
\Lambda=\operatorname{diag}(\lambda_1,\lambda_2,\ldots,\lambda_n)
$$

于是：

$$
e^{Mt}=Ve^{\Lambda t}V^{-1}
$$

而：

$$
e^{\Lambda t}=\operatorname{diag}(e^{\lambda_1t},e^{\lambda_2t},\ldots,e^{\lambda_nt})
$$

每个特征值对应一个动态模式。若所有特征值实部都严格小于零，各指数模式都会衰减。

即使 M 不可对角化，只要所有特征值实部严格小于零，矩阵指数仍然趋于零；Jordan 块可能带来多项式因子，但负指数衰减仍会使整体趋于零。

### 6.3 为什么要求“实部小于零”而不是特征值都为负实数

特征值可能是复数。例如：

$$
\lambda=-2+3i
$$

则：

$$
e^{\lambda t}=e^{(-2+3i)t}=e^{-2t}e^{3it}
$$

利用欧拉公式：

$$
e^{\lambda t}=e^{-2t}(\cos3t+i\sin3t)
$$

其中：

- \(e^{-2t}\) 决定振幅，随时间衰减到零；
- 正弦、余弦部分决定振荡。

所以系统可以一边振荡，一边振幅越来越小。只要实部为负，振荡包络就会衰减。

因此一般条件写成：

$$
\boxed{\operatorname{Re}(\lambda_i(M))<0,\quad i=1,\ldots,n}
$$

这表示 M 是 Hurwitz 矩阵。

## 7. 回到观测器：选择 K 的目的

在观测器误差方程中：

$$
M=A-KC
$$

因此要让观测误差渐近收敛，需要选择 K，使：

$$
\boxed{\operatorname{Re}[\lambda_i(A-KC)]<0,\quad\forall i}
$$

于是：

$$
e^{(A-KC)t}\rightarrow0
$$

进而：

$$
e_{obs}(t)=e^{(A-KC)t}e_{obs}(0)\rightarrow0
$$

又因为：

$$
e_{obs}=x-\hat{x}
$$

所以：

$$
x-\hat{x}\rightarrow0
$$

最终：

$$
\boxed{\hat{x}(t)\rightarrow x(t)}
$$

这就是观测器估计状态逐渐趋近真实状态的数学依据。

### 一个重要前提：可观测性

并不是任意系统都能通过选择 K，让 \(A-KC\) 的所有特征值任意配置到左半平面。对于线性定常系统，若系统对 \((A,C)\) 完全可观测，则可以通过适当选择观测器增益 K 配置观测器极点，使误差系统渐近稳定。

如果系统不可观测，则不能保证所有状态误差都能通过输出反馈被消除。

## 8. 学习过程中几个关键疑问与答案

### Q1：\(e_{obs}\) 上面的小点是什么？

$$
\dot e_{obs}=\frac{de_{obs}}{dt}
$$

小点表示对时间的一阶导数，不是新变量，也不是指数。

- \(e_{obs}\)：误差本身；
- \(\dot e_{obs}\)：误差变化率；
- \(e_{obs}(0)\)：初始误差；
- \(e_{obs}(t)\)：t 时刻的误差。

### Q2：为什么线性方程的解会出现指数？

原方程：

$$
\dot e=Me
$$

右边没有指数。指数是求解常系数线性微分方程后得到的函数形式：

$$
e(t)=e^{Mt}e(0)
$$

“线性”描述的是微分方程中未知函数及其导数的组合方式，不代表解一定是一次函数。

### Q3：\(e_{obs}\) 和指数里的 e 是不是同一个东西？

不是。

- \(e_{obs}\) 是观测误差变量；
- \(e^{Mt}\) 中的 e 是自然常数 \(2.71828\ldots\) 所定义的指数函数，矩阵情况下称为矩阵指数。

### Q4：为什么真实系统减去观测器后，输入 \(Bu\) 消失？

因为真实系统和观测器使用相同输入：

$$
(Ax+Bu)-(A\hat{x}+Bu+K(y-\hat y))
$$

其中 \(+Bu-Bu=0\)，因此输入项抵消。误差方程剩下由 \(A\)、\(K\)、\(C\) 决定的部分。

### Q5：为什么用特征值判断误差是否收敛？

因为矩阵指数的动态模式由矩阵特征值决定。所有特征值实部严格小于零时，各模式都衰减，矩阵指数趋于零，误差也趋于零。

### Q6：教材写 \((A-KC)<0\) 是什么意思？

这是不够严谨的简写。矩阵不能像普通实数一样简单地比较“小于零”。准确条件是：

$$
\operatorname{Re}[\lambda_i(A-KC)]<0
$$

即矩阵 \(A-KC\) 的所有特征值实部都小于零。

## 9. 一页记忆版：整套推导公式链

**真实系统：**

$$
\dot{x}=Ax+Bu,\qquad y=Cx
$$

**观测器：**

$$
\dot{\hat{x}}=A\hat{x}+Bu+K(y-\hat y),\qquad\hat y=C\hat x
$$

**定义误差：**

$$
e_{obs}=x-\hat{x}
$$

**误差求导并代入：**

$$
\begin{aligned}
\dot e_{obs}
&=\dot x-\dot{\hat x}\\
&=Ax+Bu-A\hat x-Bu-K(y-\hat y)\\
&=A(x-\hat x)-K(y-\hat y)
\end{aligned}
$$

**输出误差：**

$$
y-\hat y=Cx-C\hat x=C(x-\hat x)=Ce_{obs}
$$

**误差方程：**

$$
\boxed{\dot e_{obs}=(A-KC)e_{obs}}
$$

**误差解：**

$$
\boxed{e_{obs}(t)=e^{(A-KC)t}e_{obs}(0)}
$$

**稳定条件：**

$$
\boxed{\operatorname{Re}[\lambda_i(A-KC)]<0,\quad\forall i}
$$

**收敛结论：**

$$
\boxed{e_{obs}(t)\rightarrow0\quad\Longrightarrow\quad\hat{x}(t)\rightarrow x(t)}
$$

## 10. 最后用一句话串起来

真实系统由物理规律建立状态方程；观测器用同一个模型预测状态，并利用测量输出误差进行修正。真实系统减去观测器，得到误差方程 \(\dot e_{obs}=(A-KC)e_{obs}\)。求解常系数线性微分方程得到矩阵指数解；选择 K 使 \(A-KC\) 的所有特征值实部小于零，就能使估计误差渐近趋于零。

![image-20261115173433186](C:\Users\mzc228699\AppData\Roaming\Typora\typora-user-images\image-20261115173433186.png)
