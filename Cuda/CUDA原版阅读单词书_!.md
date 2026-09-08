# CUDA Programming Guide 英文原版阅读单词书

> **素材来源**：CUDA Programming Guide, Release 13.3（第 401–600 页）
> **目标**：扫清 CUDA 官方手册的英文阅读障碍，对照 PDF 边读边查
> **覆盖章节**：L2 Cache Control · Memory Synchronization Domains · Interprocess Communication · Virtual Memory Management · Extended GPU Memory · CUDA Dynamic Parallelism · CUDA Interoperability with APIs · Technical Appendices（C++ 语言支持等）

## 使用说明

| 标记 | 含义 |
|---|---|
| ★ | 极高频词，务必熟记 |
| △ | 次高频词，文档反复出现 |
| ◇ | 低频专业术语，特定章节出现 |
| 音标 | 美式音标（IPA） |
| 语境 | 例句均摘录/改写自本文档原文，还原真实行文 |

---

## 一、基础通用技术词汇（贯穿全书）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| architecture ★ | /ˈɑːrkɪtektʃər/ | n. | 架构 | The probability distribution depends upon the hardware **architecture**. |
| granularity ★ | /ˌɡrænjuˈlærəti/ | n. | 粒度 | VMM exposes fine-grained control over memory management. |
| fine-grained △ | /ˈfaɪn ɡreɪnd/ | adj. | 细粒度的 | **fine-grained** data sharing and zero-copy transfers |
| coarse-grained ◇ | /ˈkɔːrs ɡreɪnd/ | adj. | 粗粒度的 | coarse-grained synchronization is less flexible |
| alignment △ | /əˈlaɪnmənt/ | n. | 对齐 | memory **alignment** requirements |
| contiguous △ | /kənˈtɪɡjuəs/ | adj. | 连续的 | reserves a **contiguous** range of virtual addresses |
| fragment △ | /ˈfræɡment/ | v./n. | 碎片化 / 片段 | reduce GPU memory **fragmentation** |
| coalesce △ | /ˌkoʊəˈles/ | v. | 合并 | **coalesced** memory access improves bandwidth |
| semantics ★ | /sɪˈmæntɪks/ | n. | 语义 | CUDA Stream ordering **semantics** |
| asynchronous ★ | /eɪˈsɪŋkrənəs/ | adj. | 异步的 | **asynchronous** kernel launches |
| synchronous ★ | /ˈsɪŋkrənəs/ | adj. | 同步的 | implicit **synchronization** between parent and child |
| implicit ★ | /ɪmˈplɪsɪt/ | adj. | 隐式的 | **implicitly** synchronize new grids |
| explicit ★ | /ɪkˈsplɪsɪt/ | adj. | 显式的 | **explicit**, low-level control over this process |
| subsequent △ | /ˈsʌbsɪkwənt/ | adj. | 随后的 | handles **subsequently** used to obtain device pointers |
| preceding ◇ | /prɪˈsiːdɪŋ/ | adj. | 前述的 | as described in the **preceding** section |
| persist ★ | /pərˈsɪst/ | v. | 持久驻留 | data may **persist** in L2 cache |
| persisting △ | /pərˈsɪstɪŋ/ | adj. | 驻留型的 | **persisting** L2 cache lines |
| evict ★ | /ɪˈvɪkt/ | v. | 驱逐、置换 | cache lines will be **evicted** |
| eviction △ | /ɪˈvɪkʃn/ | n. | 驱逐、置换 | cache line **eviction** policy |
| thrash △ | /θræʃ/ | v. | 抖动、颠簸 | avoid **thrashing** of cache lines |
| throughput ★ | /ˈθruːpʊt/ | n. | 吞吐量 | improve **throughput** and latency |
| latency ★ | /ˈleɪtənsi/ | n. | 延迟 | reducing **latency** of memory operations |
| overhead ★ | /ˌoʊvərˈhed/ | n. | 开销 | reduce data transfer **overhead** |
| entail ◇ | /ɪnˈteɪl/ | v. | 需要、牵涉 | the operation **entails** a synchronization |
| alleviate ◇ | /əˈliːvieɪt/ | v. | 缓解 | provides a way to **alleviate** such interference |
| facilitate ★ | /fəˈsɪlɪteɪt/ | v. | 促进、使容易 | **facilitates** efficient access to all system memory |
| necessitate △ | /nəˈsesɪteɪt/ | v. | 使成为必要 | more transactions than those **necessitated** by the model |
| adhere ◇ | /ədˈhɪr/ | v. | 遵守、坚持 | code must **abide by** the rule（遵守规则） |
| abide by △ | /əˈbaɪd baɪ/ | v. | 遵守 | code must **abide by** the rule |
| whereby ◇ | /werˈbaɪ/ | adv. | 借此、凭此 | a mechanism **whereby** memory is shared |
| whereas ◇ | /ˌwerˈæz/ | conj. | 然而、鉴于 | **Whereas** CPU memory is... |
| hence △ | /hens/ | adv. | 因此 | **Hence**, a reset to normal is important |
| thereby △ | /ˌðerˈbaɪ/ | adv. | 由此 | **thereby** improving communication efficiency |
| accordingly ◇ | /əˈkɔːrdɪŋli/ | adv. | 相应地 | adjust the policy **accordingly** |

---

## 二、L2 Cache Control（L2 缓存控制）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| set-aside ★ | /ˈset əˌsaɪd/ | n./adj. | 预留（的）、划分（的） | the L2 **set-aside** cache size |
| carve out △ | /kɑːrv aʊt/ | v. | 划出、分出 | memory **carved out** for persisting accesses |
| hit ratio △ | /hɪt ˈreɪʃioʊ/ | n. | 命中率 | With a **hitRatio** of 0.5... |
| access policy window ★ | /ˈækses ˈpɑːləsi ˈwɪndoʊ/ | n. | 访问策略窗口 | the **num_bytes** in the **accessPolicyWindow** |
| streaming property △ | /ˈstriːmɪŋ ˈprɑːpərti/ | n. | 流式属性 | 40% of the accesses have the **streaming property** |
| designate △ | /ˈdezɪɡneɪt/ | v. | 指定、指派 | **designated** as persisting and cached |
| utilization ★ | /ˌjuːtələˈzeɪʃn/ | n. | 利用率 | manage **utilization** of L2 set-aside cache |
| concurrent ★ | /kənˈkɜːrənt/ | adj. | 并发的 | kernels executing **concurrently** in different streams |
| reset to normal △ | /ˈriːset tə ˈnɔːrml/ | v. | 恢复为普通状态 | reset a persisting region **to normal** status |
| untouched ◇ | /ʌnˈtʌtʃt/ | adj. | 未碰过的、未使用的 | eventually **untouched** lines are reset |
| discourage △ | /dɪsˈkɜːrɪdʒ/ | v. | 劝阻、不鼓励 | reliance on automatic reset is strongly **discouraged** |
| undetermined ◇ | /ˌʌndɪˈtɜːrmɪnd/ | adj. | 不确定的 | the **undetermined** length of time |
| property ★ | /ˈprɑːpərti/ | n. | 属性 | query L2 cache **properties** |
| maximum allowed △ | /ˈmæksɪməm əˈlaʊd/ | n. | 允许的最大值 | the **max allowed** size |
| eviction policy ◇ | /ɪˈvɪkʃn ˈpɑːləsi/ | n. | 驱逐策略 | the hardware eviction policy |

---

## 三、Memory Synchronization Domains（内存同步域）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| fence ★ | /fens/ | n./v. | 内存栅栏 | memory **fence**/flush operations |
| flush ★ | /flʌʃ/ | v./n. | 刷新、冲刷 | the local kernel will implicitly **flush** its writes |
| ordering ★ | /ˈɔːrdərɪŋ/ | n. | 排序、顺序 | the memory **ordering** provided by release and acquire |
| release △ | /rɪˈliːs/ | n./v. | 释放（内存序） | **release** and acquire operations |
| acquire △ | /əˈkwaɪər/ | n./v. | 获取（内存序） | release and **acquire** of a |
| scope ★ | /skoʊp/ | n. | 作用域、范围 | a device-**scope** operation |
| system scope △ | /ˈsɪstəm skoʊp/ | n. | 系统作用域 | system-**scope** ordering |
| domain ★ | /doʊˈmeɪn/ | n. | 域 | memory synchronization **domains** |
| interference ★ | /ˌɪntərˈfɪrəns/ | n. | 干扰 | may see degraded performance due to **interference** |
| isolate △ | /ˈaɪsəleɪt/ | v. | 隔离 | **isolating** traffic with domains |
| traffic ★ | /ˈtræfɪk/ | n. | 流量、通信 | cross-domain **traffic** is flushed to the system scope |
| tag △ | /tæɡ/ | v./n. | 打标签 / 标签 | writes and fences are **tagged** with the ID |
| encompass △ | /ɪnˈkʌmpəs/ | v. | 包含、覆盖 | one kernel's writes will not be **encompassed** by a fence |
| cumulativity ◇ | /ˌkjuːmjələˈtɪvəti/ | n. | 累积性 | This is known as **cumulativity**. |
| conservatively ◇ | /kənˈsɜːrvətɪvli/ | adv. | 保守地 | it must cast a **conservatively** wide net |
| in-flight △ | /ˈɪn flaɪt/ | adj. | 在途的、进行中的 | for **in-flight** memory operations |
| intrinsic △ | /ɪnˈtrɪnzɪk/ | n./adj. | 内建函数 / 固有的 | fences may occur explicitly as **intrinsics** |
| backward compatibility △ | /ˈbækwərd kəmˌpætəˈbɪləti/ | n. | 向后兼容 | **backward compatibility** is maintained |
| logical domain △ | /ˈlɑːdʒɪkl doʊˈmeɪn/ | n. | 逻辑域 | selects between **logical domains** |
| physical domain △ | /ˈfɪzɪkl doʊˈmeɪn/ | n. | 物理域 | mapping from logical to **physical domains** |
| remote domain △ | /rɪˈmoʊt doʊˈmeɪn/ | n. | 远程域 | the **remote domain** is intended for kernels... |
| portable △ | /ˈpɔːrtəbl/ | adj. | 可移植的 | To facilitate **portable** code |
| bracket △ | /ˈbrækɪt/ | v. | 包围、限定范围 | bracketing a section of stream use |
| graph capture △ | /ɡræf ˈkæptʃər/ | n. | 图捕获 | attributes are copied to graph nodes during **stream capture** |
| semantics of streams ◇ | /sɪˈmæntɪks əv striːmz/ | n. | 流语义 | follows CUDA Stream ordering **semantics** |

---

## 四、Interprocess Communication（进程间通信 IPC）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| interprocess ★ | /ˌɪntərˈprɑːses/ | adj. | 进程间的 | **interprocess** communication (IPC) |
| process-portable △ | /ˈprɑːses ˈpɔːrtəbl/ | adj. | 跨进程可移植的 | creating **process-portable** handles |
| shareable △ | /ˈʃerəbl/ | adj. | 可共享的 | IPC-**shareable** memory buffers |
| handle ★ | /ˈhændl/ | n. | 句柄、操作符 | handles are **opaque** identifiers |
| opaque ★ | /oʊˈpeɪk/ | adj. | 不透明的 | handles are **opaque** identifiers |
| device pointer ★ | /dɪˈvaɪs ˈpɔɪntər/ | n. | 设备指针 | obtain process-local **device pointers** |
| peer ★ | /pɪr/ | n./adj. | 对等（设备） | device memory on **peer** GPUs |
| exporter △ | /ɪkˈspɔːrtər/ | n. | 导出方 | an **exporter** and an importer |
| importer △ | /ɪmˈpɔːrtər/ | n. | 导入方 | the **importer** process |
| exchange △ | /ɪksˈtʃeɪndʒ/ | v./n. | 交换 | exchanging shareable handles |
| mechanism ★ | /ˈmekənɪzəm/ | n. | 机制 | using a **multicast mechanism** |
| facilitate sharing ◇ | /fəˈsɪlɪteɪt ˈʃerɪŋ/ | v. | 促进共享 | **facilitating** memory sharing across devices |

---

## 五、Virtual Memory Management（虚拟内存管理 VMM）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| virtual memory ★ | /ˈvɜːrtʃuəl ˈmeməri/ | n. | 虚拟内存 | CUDA provides a **virtual memory management** (VMM) API |
| reserve ★ | /rɪˈzɜːrv/ | v. | 预留 | the OS **reserves** a contiguous range of virtual addresses |
| commit ★ | /kəˈmɪt/ | v. | 提交（分配物理页） | the OS **commits** the virtual addresses |
| map ★ | /mæp/ | v. | 映射 | map it to physical GPU memory |
| unmap ◇ | /ˌʌnˈmæp/ | v. | 解除映射 | **unmap** the memory |
| remap ◇ | /ˌriːˈmæp/ | v. | 重新映射 | **remap** allocations |
| allocation ★ | /ˌæləˈkeɪʃn/ | n. | 分配 | memory **allocation** calls |
| deallocation △ | /diːˌæləˈkeɪʃn/ | n. | 释放 | efficient allocation and **deallocation** |
| allocate △ | /ˈæləkeɪt/ | v. | 分配 | **allocate** and export |
| reallocate ◇ | /riˈæləkeɪt/ | v. | 重新分配 | without costly memory copies or **reallocations** |
| reservation △ | /ˌrezərˈveɪʃn/ | n. | 预留 | memory **reservation**, mapping, and access permissions |
| fragmentation ★ | /ˌfræɡmenˈteɪʃn/ | n. | 碎片化 | reduce GPU memory **fragmentation** |
| MMU ◇ | /ˌem em ˈjuː/ | n. | 内存管理单元 | the Memory Management Unit (**MMU**) |
| page ★ | /peɪdʒ/ | n. | 页 | assigning physical storage to the virtual **pages** |
| fabric memory ★ | /ˈfæbrɪk ˈmeməri/ | n. | 织构内存（互联内存） | **Fabric memory** refers to memory accessible over a fabric |
| interconnect ★ | /ˌɪntərkəˈnekt/ | n./v. | 互连 | high-speed **interconnect** fabric |
| multicast ★ | /ˈmʌltikæst/ | n./v. | 多播 | **multicast** memory access |
| unicast ◇ | /ˈjuːnikæst/ | n./v. | 单播 | **Unicast** memory access refers to... |
| one-to-many ◇ | /wʌn tə ˈmeni/ | adj. | 一对多的 | shared in a **one-to-many** fashion |
| redundant △ | /rɪˈdʌndənt/ | adj. | 冗余的 | reducing **redundant** data transfers |
| decouple △ | /diːˈkʌpl/ | v. | 解耦 | **decoupling** virtual addresses from physical memory |
| unified ★ | /ˈjuːnɪfaɪd/ | adj. | 统一的 | a **unified** virtual address space |
| aliasing ◇ | /ˈeɪliəsɪŋ/ | n. | 别名、混叠 | virtual **aliasing** support |
| compressible ◇ | /kəmˈpresəbl/ | adj. | 可压缩的 | **compressible** memory |
| physical storage ◇ | /ˈfɪzɪkl ˈstɔːrɪdʒ/ | n. | 物理存储 | assigning **physical storage** to the virtual pages |
| export ★ | /ɪkˈspɔːrt/ | v. | 导出 | Allocate and **Export** |
| import ★ | /ɪmˈpɔːrt/ | v. | 导入 | Share and **Import** |
| bind △ | /baɪnd/ | v. | 绑定 | **binds** together physical memory allocations |
| opaque identifier △ | /oʊˈpeɪk aɪˈdentɪfaɪər/ | n. | 不透明标识符 | handles are **opaque identifiers** |
| dynamically ★ | /daɪˈnæmɪkli/ | adv. | 动态地 | grow GPU memory allocations **dynamically** |
| on demand △ | /ɑːn dɪˈmænd/ | adv. | 按需 | map physical memory **on demand** |
| zero-copy △ | /ˈzɪroʊ ˈkɑːpi/ | adj. | 零拷贝的 | **zero-copy** transfers |
| key-value cache ◇ | /ki ˈvæljuː kæʃ/ | n. | 键值缓存 | managing **key-value caches** in LLMs |
| sophisticated △ | /səˈfɪstɪkeɪtɪd/ | adj. | 复杂的、高级的 | building **sophisticated** memory allocators |
| scalable ★ | /ˈskeɪləbl/ | adj. | 可扩展的 | efficient, flexible, and **scalable** GPU memory management |
| prerequisite ◇ | /ˌpriːˈrekwəzɪt/ | n. | 前提条件 | **prerequisites** for virtual memory management |
| throughput ◇ | /ˈθruːpʊt/ | n. | 吞吐量 | improving **throughput** and latency |
| utilization ◇ | /ˌjuːtələˈzeɪʃn/ | n. | 利用率 | improve memory **utilization** |
| variable-sized ◇ | /ˈveriəbl saɪzd/ | adj. | 变长的 | **variable-sized** memory allocations |
| IMEX ◇ | /ˈaɪmeks/ | n. | 节点间内存交换 | **IMEX** channels are a GPU driver feature |
| domain (IMEX) ◇ | /doʊˈmeɪn/ | n. | （IMEX）域 | within an **IMEX domain** |

---

## 六、Extended GPU Memory（扩展 GPU 内存 EGM）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| high-bandwidth ★ | /ˌhaɪ ˈbændwɪdθ/ | adj. | 高带宽的 | the **high-bandwidth** NVLink-C2C |
| integrated ★ | /ˈɪntɪɡreɪtɪd/ | adj. | 集成的 | applies to **integrated** CPU-GPU systems |
| topology ★ | /təˈpɑːlədʒi/ | n. | 拓扑、布局 | currently supported **topologies** |
| socket ◇ | /ˈsɑːkɪt/ | n. | 插槽、Socket | **socket** identifiers |
| NUMA ◇ | /ˈnuːmə/ | n. | 非均匀内存访问 | **NUMA** (Non-Uniform Memory Access) |
| ordinal ◇ | /ˈɔːrdɪnl/ | n. | 序号 | different from the **ordinal** of a device |
| allocator △ | /ˈæləkeɪtər/ | n. | 分配器 | **cuMemCreate** and cudaMemPoolCreate **allocators** |
| routing △ | /ˈruːtɪŋ/ | n. | 路由 | traffic is guaranteed to be **routed** over NVLinks |
| fabric ◇ | /ˈfæbrɪk/ | n. | 织构（互联网络） | over the NVSwitch **fabric** |
| chip-to-chip ◇ | /tʃɪp tə tʃɪp/ | adj. | 芯片到芯片的 | a high bandwidth C2C (Chip-to-Chip) **interconnect** |
| attached memory ◇ | /əˈtætʃt ˈmeməri/ | n. | 挂载内存 | CPU **attached** memory and HBM3 |
| identifier assignment ◇ | /aɪˈdentɪfaɪər əˈsaɪnmənt/ | n. | 标识符分配 | identifier **assignment** |
| device ordinal ◇ | /dɪˈvaɪs ˈɔːrdɪnl/ | n. | 设备序号 | get the identifier via **deviceOrdinal** |
| remote ◇ | /rɪˈmoʊt/ | adj. | 远程的 | for **remote** memory accesses |

---

## 七、CUDA Dynamic Parallelism（CUDA 动态并行）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| dynamic parallelism ★ | /daɪˈnæmɪk ˈpærəlelɪzəm/ | n. | 动态并行 | CUDA **dynamic parallelism** enables device-side launches |
| parent grid △ | /ˈperənt ɡrɪd/ | n. | 父网格 | belongs to the **parent grid** |
| child grid △ | /tʃaɪld ɡrɪd/ | n. | 子网格 | the new grid is called a **child grid** |
| nested ★ | /ˈnestɪd/ | adj. | 嵌套的 | invocation is properly **nested** |
| recursion ★ | /rɪˈkɜːrʒn/ | n. | 递归 | modifications to eliminate **recursion** |
| recursive △ | /rɪˈkɜːrsɪv/ | adj. | 递归的 | may not be directly or indirectly **recursive** |
| irregular △ | /ɪˈreɡjələr/ | adj. | 不规则的 | **irregular** loop structure |
| invocation ★ | /ˌɪnvəˈkeɪʃn/ | n. | 调用、启用 | the specific **invocation** of that kernel |
| invoke △ | /ɪnˈvoʊk/ | v. | 调用 | **invoke** a kernel from device code |
| dependency ★ | /dɪˈpendənsi/ | n. | 依赖 | tracking **dependencies** between launches |
| concurrency ★ | /kənˈkɜːrənsi/ | n. | 并发 | ordering and **concurrency** |
| primitive ★ | /ˈprɪmətɪv/ | n./adj. | 原语 | scope of CUDA **primitives** |
| footprint ★ | /ˈfʊtprɪnt/ | n. | 占用空间 | memory **footprint** |
| pending △ | /ˈpendɪŋ/ | adj. | 待处理的 | **pending** kernel launches |
| overhead △ | /ˌoʊvərˈhed/ | n. | 开销 | dynamic-parallelism-enabled kernel **overhead** |
| fire-and-forget ◇ | /ˈfaɪər ənd fərˈɡet/ | adj. | 发射后不管的 | the Fire-and-Forget Stream |
| coherence ★ | /koʊˈhɪrəns/ | n. | 一致性 | memory **coherence** and consistency |
| consistency ★ | /kənˈsɪstənsi/ | n. | 一致性 | memory **consistency** model |
| distinguishable ◇ | /dɪˈstɪŋɡwɪʃəbl/ | adj. | 可区分的 | the **distinction** between a kernel and a grid |
| syntactically ◇ | /sɪnˈtæktɪkli/ | adv. | 句法上地 | APIs **syntactically** similar to the runtime API |
| counterpart ★ | /ˈkaʊntərpɑːrt/ | n. | 对应物、对应方 | similar to their host **counterparts** |
| interoperability ★ | /ˌɪntərˌɑːpərəˈbɪləti/ | n. | 互操作性 | compatibility and **interoperability** |
| device runtime △ | /dɪˈvaɪs ˈrʌntaɪm/ | n. | 设备运行时 | relies on the CUDA **Device Runtime** |
| grid scope ◇ | /ɡrɪd skoʊp/ | n. | 网格作用域 | exist within **grid scope** |
| undefined behavior ★ | /ˌʌndɪˈfaɪnd bɪˈheɪvjər/ | n. | 未定义行为 | have **undefined behavior** when used outside |
| satisfied △ | /ˈsætɪsfaɪd/ | adj. | 被满足的 | once stream dependencies are **satisfied** |
| invocation and completion ◇ | /ˌɪnvəˈkeɪʃn ənd kəmˈpliːʃn/ | n. | 调用与完成 | the **invocation and completion** of child grids |

---

## 八、CUDA Interoperability with APIs（API 互操作）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| interoperability ★ | /ˌɪntərˌɑːpərəˈbɪləti/ | n. | 互操作性 | CUDA **interoperability** with APIs |
| graphics ★ | /ˈɡræfɪks/ | n. | 图形 | **graphics** interoperability |
| external resource △ | /ɪkˈstɜːrnl ˈriːsɔːrs/ | n. | 外部资源 | **external resource** interoperability |
| legacy ★ | /ˈleɡəsi/ | adj. | 遗留的、旧的 | the **legacy** interprocess communication API |
| UUID ◇ | /juː juː aɪ ˈdiː/ | n. | 通用唯一标识符 | matching device **UUIDs** |
| initialize ★ | /ɪˈnɪʃəlaɪz/ | v. | 初始化 | **initializing** CUDA with matching device UUIDs |
| matching △ | /ˈmætʃɪŋ/ | adj. | 匹配的 | with **matching** device UUIDs |
| scalable link ◇ | /ˈskeɪləbl lɪŋk/ | n. | 可扩展链路 | in a **Scalable Link Interface** (SLI) configuration |
| OpenGL ◇ | /ˌoʊpən dʒi ˈel/ | n. | OpenGL（图形API） | **OpenGL** interoperability |
| Direct3D ◇ | /dəˈrekt θriː diː/ | n. | Direct3D（图形API） | **Direct3D** interoperability |
| Vulkan ◇ | /ˈvʊlkæn/ | n. | Vulkan（图形API） | **Vulkan** interoperability |
| device matching ◇ | /dɪˈvaɪs ˈmætʃɪŋ/ | n. | 设备匹配 | **device matching** using UUIDs |
| enumerate ◇ | /ɪˈnuːməreɪt/ | v. | 枚举 | **enumerate** available devices |

---

## 九、Technical Appendices：C++ Language Support（C++ 语言支持）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| lambda ★ | /ˈlæmbdə/ | n. | 匿名函数、Lambda 表达式 | **lambda** expressions |
| constexpr ★ | /ˌkɑːnstɪkˈsprɛs/ | n./adj. | 常量表达式 | **constexpr** variables and functions |
| template ★ | /ˈtemplɪt/ | n. | 模板 | **template** parameters |
| deduction △ | /dɪˈdʌkʃn/ | n. | 推导 | functions with **deduced** return type |
| structured binding ◇ | /ˈstrʌktʃərd ˈbaɪndɪŋ/ | n. | 结构化绑定 | C++17 **structured binding** |
| three-way comparison ◇ | /θriː weɪ kəmˈpærɪsn/ | n. | 三路比较 | **three-way comparison** operator |
| annotation ★ | /ˌænəˈteɪʃn/ | n. | 注解、标注 | function and variable **annotations** |
| specifier ★ | /ˈspesɪfaɪər/ | n. | 限定符 | execution space **specifiers** |
| inlining ◇ | /ˈɪnlaɪnɪŋ/ | n. | 内联 | **inlining** specifiers |
| built-in ★ | /ˌbɪlt ˈɪn/ | adj. | 内建的 | **built-in** types and variables |
| restriction ★ | /rɪˈstrɪkʃn/ | n. | 限制 | additional **restrictions** in tile code |
| unsupported ★ | /ˌʌnsəˈpɔːrtɪd/ | adj. | 不支持的 | the constructs below are **unsupported** |
| ellipsis ◇ | /ɪˈlɪpsɪs/ | n. | 省略号（变参…） | using the C **ellipsis** syntax ... |
| bitfield ◇ | /ˈbɪtfiːld/ | n. | 位域 | types containing **bitfields** |
| virtual base class ◇ | /ˈvɜːrtʃuəl beɪs klæs/ | n. | 虚基类 | types with **virtual base classes** |
| translation unit ★ | /trænzˈleɪʃn ˈjuːnɪt/ | n. | 翻译单元 | a function body in the same **translation unit** |
| anonymous ◇ | /əˈnɑːnəməs/ | adj. | 匿名的 | unnamed type |
| unnamed ◇ | /ˌʌnˈneɪmd/ | adj. | 未命名的 | **unnamed** struct |
| private ◇ | /ˈpraɪvət/ | adj. | 私有的 | **private** type |
| member variable ◇ | /ˈmembər ˈveriəbl/ | n. | 成员变量 | pointer to **member variable** |
| standard library ★ | /ˈstændərd ˈlaɪbreri/ | n. | 标准库 | CUDA C++ **Standard Library** |
| type trait ◇ | /taɪp treɪt/ | n. | 类型萃取 | extended lambda **type traits** |
| namespace ★ | /ˈneɪmspeɪs/ | n. | 命名空间 | inline **namespaces** |
| derive ◇ | /dɪˈraɪv/ | v. | 派生 | **derived** classes |
| overload △ | /ˌoʊvərˈloʊd/ | v./n. | 重载 | function **overloading** |

---

## 十、高频动词与抽象名词（阅读必备）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| denote ★ | /dɪˈnoʊt/ | v. | 表示、指代 | X **denotes** the window size |
| comprise △ | /kəmˈpraɪz/ | v. | 包含、由…组成 | the API **comprises** several functions |
| pertain to ◇ | /pərˈteɪn tə/ | v. | 与…相关 | **pertaining to** EGM functionalities |
| pertain ◇ | /pərˈteɪn/ | v. | 涉及、关于 | the rules **pertaining to** memory |
| assert △ | /əˈsɜːrt/ | v. | 断言 | **assert** the condition is true |
| guarantee ★ | /ˌɡærənˈtiː/ | v./n. | 保证 | the model **guarantees** that... |
| ensure ★ | /ɪnˈʃʊr/ | v. | 确保 | **ensure** that all GPUs can access... |
| verify △ | /ˈverɪfaɪ/ | v. | 核实、验证 | users should **verify** two things |
| query ★ | /ˈkwɪri/ | v./n. | 查询 | **query** for support |
| indicate △ | /ˈɪndɪkeɪt/ | v. | 表明、指出 | **indicating** that the operation failed |
| assume △ | /əˈsuːm/ | v. | 假设、假定 | **assume** the hardware supports it |
| derive from ◇ | /dɪˈraɪv frəm/ | v. | 源自 | performance **derived from** the cache |
| constitute ◇ | /ˈkɑːnstɪtuːt/ | v. | 构成 | **constitutes** a security mechanism |
| sufficient ★ | /səˈfɪʃnt/ | adj. | 足够的 | device-scope fencing remains **sufficient** |
| insufficient ◇ | /ˌɪnsəˈfɪʃnt/ | adj. | 不足的 | **insufficient** memory |
| compatible △ | /kəmˈpætəbl/ | adj. | 兼容的 | **compatible** with the driver |
| incompatible ◇ | /ˌɪnkəmˈpætəbl/ | adj. | 不兼容的 | **incompatible** versions |
| comply with ◇ | /kəmˈplaɪ wɪð/ | v. | 遵从 | **comply with** the specification |
| deviation ◇ | /ˌdiːviˈeɪʃn/ | n. | 偏差 | any **deviation** from the rules |
| deteriorate ◇ | /dɪˈtɪriəreɪt/ | v. | 恶化 | performance may **deteriorate** |
| degrade △ | /dɪˈɡreɪd/ | v. | 降低、退化 | **degraded** performance |
| mitigate ◇ | /ˈmɪtɪɡeɪt/ | v. | 减轻、缓解 | **mitigate** the impact |
| remedy ◇ | /ˈremədi/ | n./v. | 补救（措施） | a possible **remedy** is... |
| exploit △ | /ɪkˈsplɔɪt/ | v. | 利用 | **exploit** the full bandwidth |
| harness ◇ | /ˈhɑːrnɪs/ | v. | 利用（力量） | **harness** the capabilities |
| circumvent ◇ | /ˌsɜːrkəmˈvent/ | v. | 绕过、规避 | **circumvent** the restriction |
| endeavor ◇ | /ɪnˈdevər/ | n./v. | 努力 | a worthwhile **endeavor** |
| ambiguity ◇ | /ˌæmbɪˈɡjuːəti/ | n. | 歧义 | avoid **ambiguity** in semantics |
| clarify △ | /ˈklærəfaɪ/ | v. | 澄清、阐明 | to **clarify** the behavior |
| stipulate ◇ | /ˈstɪpjuleɪt/ | v. | 规定、明确要求 | the API **stipulates** that... |
| suffice ◇ | /səˈfaɪs/ | v. | 足够、满足 | a simple check will **suffice** |
| overridden ◇ | /ˌoʊvərˈrɪdn/ | v. | 覆盖、推翻 | default values may be **overridden** |
| rationale ◇ | /ˌræʃəˈnæl/ | n. | 理由、依据 | the **rationale** behind this design |
| prudent ◇ | /ˈpruːdnt/ | adj. | 谨慎的、明智的 | it is **prudent** to check support first |

---

## 十一、文档结构高频用语（读目录/标题必备）

| 英文 | 音标 | 词性 | 释义 | 文档语境 |
|---|---|---|---|---|
| release ★ | /rɪˈliːs/ | n. | 发布版 | CUDA Programming Guide, **Release** 13.3 |
| appendix ★ | /əˈpendɪks/ | n. | 附录 | Technical **Appendices** |
| chapter ★ | /ˈtʃæptər/ | n. | 章 | **Chapter** 4. CUDA Features |
| section ★ | /ˈsekʃn/ | n. | 节 | see **Section** 4.16 |
| subsection ◇ | /ˈsʌbsekʃn/ | n. | 小节 | in the **subsection** below |
| table of contents ◇ | /ˈteɪbl əv ˈkɑːntents/ | n. | 目录 | the **table of contents** |
| figure ★ | /ˈfɪɡjər/ | n. | 图 | **Figure** 57: Parent-Child Launch Nesting |
| example ★ | /ɪɡˈzæmpl/ | n. | 示例 | consider the **example** above |
| overview ★ | /ˈoʊvərvjuː/ | n. | 概述 | an **overview** of the feature |
| preliminaries △ | /prɪˈlɪməneriz/ | n. | 预备知识 | 4.17.1 **Preliminaries** |
| note ★ | /noʊt/ | n./v. | 注意 | **Note** that... |
| guidelines △ | /ˈɡaɪdlaɪnz/ | n. | 指南、指导原则 | Programming **Guidelines** |
| summary ★ | /ˈsʌməri/ | n. | 总结 | In **summary**, the API gives... |
| conclude ◇ | /kənˈkluːd/ | v. | 总结、得出结论 | to **conclude** this section |
| refer to ★ | /rɪˈfɜːr tə/ | v. | 参见、指代 | **refer to** Section 5.3 |
| aforementioned ◇ | /əˈfɔːrmenʃnd/ | adj. | 前述的 | the **aforementioned** limitations |
| following ★ | /ˈfɑːloʊɪŋ/ | adj. | 以下的 | the **following** code sample |
| below ★ | /bɪˈloʊ/ | adv. | 下面 | as described **below** |

---

## 附：本章节高频缩略语速查

| 缩写 | 全称 | 中文 |
|---|---|---|
| SM | Streaming Multiprocessor | 流式多处理器 |
| VMM | Virtual Memory Management | 虚拟内存管理 |
| IPC | Interprocess Communication | 进程间通信 |
| CDP | CUDA Dynamic Parallelism | CUDA 动态并行 |
| EGM | Extended GPU Memory | 扩展 GPU 内存 |
| MMU | Memory Management Unit | 内存管理单元 |
| NUMA | Non-Uniform Memory Access | 非均匀内存访问 |
| C2C | Chip-to-Chip | 芯片到芯片（互连） |
| UVA | Unified Virtual Addressing | 统一虚拟寻址 |
| API | Application Programming Interface | 应用程序编程接口 |
| PTX | Parallel Thread Execution | 并行线程执行（中间语言） |
| IMEX | Internode Memory Exchange | 节点间内存交换 |
| LLM | Large Language Model | 大语言模型 |
| HBM | High Bandwidth Memory | 高带宽内存 |
| UUID | Universally Unique Identifier | 通用唯一标识符 |
| SLI | Scalable Link Interface | 可扩展链路接口 |

---

*本单词书词条均取自 CUDA Programming Guide Release 13.3（第401–600页）原文语境，音标为美式音标。*
