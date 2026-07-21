# Architecture Design

## 1. 项目目标

本项目用于构建一个模块化、可扩展、跨平台的 C 语言工程框架。

设计目标：

* 使用 C 作为主要开发语言
* 使用 SCons 作为构建系统
* 支持 Linux、Windows 等平台
* 模块化设计，降低耦合
* 支持静态库、动态库和 SDK 导出
* 支持 Python 工具链扩展
* 适合作为长期维护和持续迭代的工程基础

---

# 2. 项目目录结构

```text
project
│
├── app/
│
├── src/
│   ├── platform/
│   ├── utils/
│   ├── storage/
│   ├── crypto/
│   └── vault/
│
├── docs/
│
├── tools/
│
├── target/
│
├── SConstruct
│
└── README.md
```

---

# 3. 目录职责

## app

应用程序入口。

职责：

* 示例程序
* 测试程序
* 调试工具

原则：

* 不实现核心逻辑
* 只调用业务模块接口

---

## src

核心源码目录。

所有业务模块均位于此目录。

---

## docs

项目文档。

包括：

* 架构设计
* API 文档
* 开发规范
* 发布说明

---

## tools

辅助工具。

主要用于：

* Python 脚本
* 自动生成代码
* 自动打包
* 自动发布

---

## target

构建输出目录。

包含：

```text
target
│
├── obj/
│
├── output/
│
└── package/
```

说明：

* obj：中间目标文件
* output：可执行文件或库文件
* package：发布包

---

# 4. 模块划分

## platform

平台抽象层。

职责：

* 文件系统
* 时间接口
* 线程接口
* 锁接口
* 日志输出

示例：

```c
platform_file_open();
platform_mutex_lock();
platform_get_tick();
```

特点：

* 与操作系统相关
* 为上层提供统一接口

---

## utils

基础工具模块。

职责：

* 字符串处理
* 链表
* 哈希表
* 队列
* 通用算法

示例：

```c
list_push();
hash_put();
str_trim();
```

特点：

* 不依赖业务模块
* 所有模块均可使用

---

## storage

存储层。

职责：

* 数据保存
* 数据加载
* 文件读写
* 配置管理

示例：

```c
storage_save();
storage_load();
```

特点：

* 不关心业务逻辑
* 不关心加密逻辑

---

## crypto

加密层。

职责：

* CRC
* MD5
* SHA256
* AES

示例：

```c
crypto_crc32();
crypto_sha256();
crypto_encrypt();
```

特点：

* 只负责数据处理
* 不负责存储

---

## vault

核心业务层。

职责：

* 数据管理
* 对象管理
* 统一访问接口

示例：

```c
vault_put();
vault_get();
vault_remove();
```

特点：

* 对外提供统一 API
* 内部可调用 storage 和 crypto

---

# 5. 模块依赖关系

```text
app
 |
 v
vault
 |
 +------------+
 |            |
 v            v
storage     crypto
 |
 v
platform

utils
 ^
 |
所有模块均可使用
```

依赖规则：

* 依赖方向只能向下
* 禁止循环依赖
* 禁止跨层反向调用

例如：

允许：

```text
vault -> storage
vault -> crypto
```

禁止：

```text
storage -> vault
crypto -> vault
```

---

# 6. 设计原则

## 单一职责

每个模块只负责一类功能。

例如：

* storage 只负责存储
* crypto 只负责加密
* vault 只负责业务管理

---

## 最小依赖

模块之间尽量减少依赖关系。

---

## 接口优先

优先设计头文件接口。

实现可以后续迭代。

推荐开发顺序：

```text
1. 设计接口
2. 编译通过
3. 添加实现
4. 编写测试
```

---

## 实现可替换

对外暴露稳定接口。

内部实现允许随时替换。

例如：

```text
链表实现
    ↓
哈希表实现
    ↓
数据库实现
```

外部代码无需修改。

---

# 7. 开发阶段规划

## Phase 1

完成工程框架。

目标：

* SCons 构建系统
* 模块目录
* 基础接口

---

## Phase 2

实现 utils 模块。

目标：

* list
* hash
* string

---

## Phase 3

实现 platform 模块。

目标：

* file
* log
* time

---

## Phase 4

实现 storage 模块。

目标：

* save
* load

---

## Phase 5

实现 crypto 模块。

目标：

* crc32
* sha256

---

## Phase 6

实现 vault 模块。

目标：

* 数据管理
* 持久化管理
* 加密存储

---

# 8. 当前状态

当前项目处于：

```text
Phase 1
```

重点工作：

* 完善目录结构
* 完善构建系统
* 确定模块职责
* 建立开发规范

暂不进行复杂业务实现。
