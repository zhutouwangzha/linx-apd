# 简介

该仓库存放的代码是Falco 插件技术 + 自研eBPF对系统全量系统调用进行进程上下文和参数采集，并以固定日志格式输出到文件中。

# 目录结构

```tex
.
├── docs                  /* 存放成果演示文档 */
├── ebpf									/* ebpf内核代码存放处 */
│   ├── include
│   │   ├── skel							/* 编译时会在该目录生成骨架代码 */
│   │   └── vmlinux							/* 编译时会在该目录生成vmlinux.h文件 */
│   └── tail_calls							/* 该目录下存放所有的系统调用采集代码 */
├── include
│   └── macro
│       └── linx_syscalls_macro.h			/* 通过 scripts/get_syscalls_macro.sh 脚本生成的结构化系统调用宏定义文件 */
├── json_config								/* 存放该插件运行中需要的json配置文件 */
├── Makefile
├── plugin									/* 插件代码，该目录下所有代码最终会编译为一个动态库 */
│   ├── ebpf_user							/* 用于加载ebpf内核程序以及填充ebpf内核中全局变量 */
│   ├── linx_event_putfile					/* 以固定格式日志写入文件中 */
│   ├── linx_event_rich						/* 对ebpf内核中采集的数据进行丰富 */
│   ├── linx_log							/* 带日志等级的日志输出接口 */
│   ├── linx_syscall_table.c				/* 定义所有系统调用的名称、调用号、参数等等信息 */
│   ├── Makefile
│   ├── plugin								/* 开发falco插件所必须的接口 */
│   └── plugin_config						/* 用于解析传递给插件的JSON配置 */
├── plugin_yaml								/* 存放该插件的测试规则文件 */
├── README.md
├── scripts
│   ├── generate_bpf_files.py				/* 根据 linx_syscalls_macro.h 宏定义文件输出ebpf内核采集代码 */
│   ├── generate_interesting_syscalls.sh	/* 根据 linx_syscalls_macro.h 宏定义文件输出JSON配置文件 */
│   └── get_syscalls_macro.sh				/* 解析系统内核头文件，输出 linx_syscalls_macro.h 宏定义文件 */
└── test									/* 测试代码 */
```

# 快速开始

## 编译

由于该插件没有使用官方提供的SDK进行开发，所以编译的过程需要依赖falco仓库相关的头文件或者编译好的静态库，编译该插件应该优先编译好falco。

创建一个目录，后续将falco和插件存放在同一级目录下。

```shell
mkdir falco-plugin
cd falco-plugin
```

### 编译falco

```
git clone -b ZTtask#20267 https://gitlab.linx-info.cd/security-self-innovate/behavior_detection/linx-behavior-detection.git
cd linx-behavior-detection
git submodule init
git submodule update
./build.sh
```

### 编译插件

插件内部依赖`cjson`，所以在编译前优先按照`cjson`依赖库：

```shell
yum install cjson-devel
```

```shell
git clone -b syscall_sequence_plugin https://gitlab.linx-info.cd/security-self-innovate/behavior_detection/linx-behavior-detection.git syscall_sequence_plugin
cd syscall_sequence_plugin
make
```

编译成功会在`build`目录下生成`linx_bpf.ll`链接文件、`linx_bpf.bpf`ebpf内核文件、`syscall_test`测试文件以及`libsyscall_sequence_plugin.so`插件动态库文件。

## 运行插件

### 修改falco规则文件

插件编译完成后，falco是不会默认加载的，需要修改falco规则文件指定插件名称以及插件路径才会加载该插件，插件加载规则文件位于`linx-behavior-detection/build/falco.yaml`。

首先修改文件中的`load_plugins`字段，表示falco在运行中需要加载哪些插件，可以加载多个插件，插件之间使用`，`隔开，这里我们让`falco`只加载该插件：

```yaml
load_plugins: [syscall_sequence_plugin]
```

随后修改`plugins`字段，表示各插件名称、插件路径、插件的初始化与打开参数等：

```yaml
plugins:
	- name: syscall_sequence_plugin
    library_path: /xxx/falco-plugin/syscall_sequence_plugin/build/libsyscall_sequence_plugin.so
    init_config: '{
                    "rich_value_size": 1024, 
                    "str_max_size": 100, 
                    "putfile": "true", 
                    "path": "./1.txt", 
                    "format": "normal", 
                    "log_level": "ERROR",
                    "log_file": "/xxx/falco-plugin/linx-behavior-detection/1.txt"
                  }'
    open_params: '{
                    "filter_own": "true", "filter_falco": "true", "filter_pids": [815],
                    "filter_comms": ["gmain", "sshd"], "drop_mode": "false", "drop_failed": "true", 
                    "interest_syscall_file": "/xxx/falco-plugin/syscall_sequence_plugin/json_config/interesting_syscalls.json"
                  }'
```

其中`library_path`表示插件的路径，需要根据存放位置进行修改，需要写绝对路径，否则会从`/usr/share/falco/plugins/`路径下查找。

`init_config`用于控制插件行为，`open_params`用于控制`ebpf`程序行为，其中各个字段的含义在后续章节介绍。

只需要留意`log_file`和`interest_syscall_file`需要根据不同环境修改以下绝对路径。

### 编写插件规则文件

该规则文件位于`plugin_yaml/test.yaml`，内容如下：

```yaml
# Your custom rules!

- rule: Test linx_ebpf Event Source Output
  desc: Test linx_ebpf Event Source Output
  source: linx_ebpf
  condition: >
    evt.pluginname = syscall_sequence_plugin
  output: >
    %linx.time : %linx.dir %linx.type[value] %linx.args %linx.comm %linx.fullpath %linx.cmdline %linx.fds 
  priority: NOTICE
```

该规则完成逻辑是：当插件名为`syscall_sequence_plugin`时，输出：时间 : 系统调用进入还是退出 系统调用名 系统调用的参数 命令 命令的绝对路径 执行的命令 该进程打开的所有文件

该插件提供能使用的字段将在后续章节介绍。

### 运行

```shell
./linx-behavior-detection/build/userspace/falco/falco -c ./linx-behavior-detection/build/falco.yaml -r ./syscall_sequence_plugin/plugin_yaml/test.yaml --enable-source=linx_ebpf
```

成功运行后，一般会在`stderr`持续输出如下信息：

```tex
16:06:57.159273132: Notice 2025-06-11 16:06:57.159273132 : (<) (close) (fd=43) (systemd-journal) (/usr/lib/systemd/systemd-journald) (/usr/lib/systemd/systemd-journald) (0(/dev/null), 1(/dev/null), 2(/dev/null), 3(), 4(), 5(), 6(/dev/kmsg), 7(), 8(/dev/kmsg), 9(), 10(/proc/sys/kernel/hostname), 11())
16:06:57.159399920: Notice 2025-06-11 16:06:57.159399920 : (<) (close) (fd=43) (systemd-journal) (/usr/lib/systemd/systemd-journald) (/usr/lib/systemd/systemd-journald) (0(/dev/null), 1(/dev/null), 2(/dev/null), 3(), 4(), 5(), 6(/dev/kmsg), 7(), 8(/dev/kmsg), 9(), 10(/proc/sys/kernel/hostname), 11())
16:06:57.659023052: Notice 2025-06-11 16:06:57.659023052 : (>) (close) (fd=0) (in:imjournal) (/usr/sbin/rsyslogd) (/usr/sbin/rsyslogd -n -i/var/run/rsyslogd.pid) (0(/dev/null), 1(/dev/null), 2(/dev/null), 3(/dev/urandom), 4(), 5(/var/log/messages), 6(), 7(/var/log/syslog), 8(/var/log/kern.log), 9(/run/log/journal/aefaf3cab9b948199b7bb546770936b2/system.journal), 10(/var/log/debug), 11(/var/log/daemon.log))
......
```

# JSON配置

当前存在三个不同的JSON字符串用于控制插件与`ebpf`内核程序的执行状态。

**注意**：编写这些配置时，字段的顺序不能修改，否则在读取配置时会出错。

## init_config

该JSON字符串位于`build/falco.yaml`文件中，主要用于控制插件本身的执行逻辑。可用字段及相关描述如下：

| 序号 | 名称            | 取值类型 | 取值范围                           | 描述                                                         |
| ---- | --------------- | -------- | ---------------------------------- | ------------------------------------------------------------ |
| 1    | rich_value_size | 整数     | 0 ~ 2^32 - 1                       | 控制丰富事件时，buffer的最大大小                             |
| 2    | str_max_size    | 整数     | 0 ~ 2^32 - 1                       | 控制输出系统调用参数时，字符串的最大长度，当字符串超出设定值时，用...标识 |
| 3    | putfile         | 字符串   | true、false                        | 控制是否按照特定消息格式输出到文件中                         |
| 4    | path            | 字符串   | 文件路径，最长256                  | 存放消息的文件路径                                           |
| 5    | format          | 字符串   | normal、json(目前未支持)           | 输出格式                                                     |
| 6    | log_level       | 字符串   | DEBUG、INFO、WARNING、ERROR、FATAL | 控制日志的输出等级                                           |
| 7    | log_file        | 字符串   | 文件路径，最长256                  | 存放日志的文件                                               |

## open_params

该JSON字符串位于`build/falco.yaml`文件中，主要用于控制`ebpf`内核层的执行逻辑。可用字段及相关描述如下：

| 序号 | 名称                  | 取值类型   | 取值范围                                     | 描述                                               |
| ---- | --------------------- | ---------- | -------------------------------------------- | -------------------------------------------------- |
| 1    | filter_own            | 字符串     | true、false                                  | 在ebpf内核采集时，是否过滤掉插件本身               |
| 2    | filter_falco          | 字符串     | true、false                                  | 在ebpf内核采集时，是否过滤掉falco本身              |
| 3    | filter_pids           | 整数数组   | 最多过滤64个pid，每个pid取值在0 ~ 2 ^ 22 - 1 | 在ebpf内核采集时，过滤哪些PID                      |
| 4    | filter_comms          | 字符串数组 | 最多过滤16个命令，每个命令最多16个字符长度   | 在ebpf内核采集时，过滤哪些命令                     |
| 5    | drop_mode             | 字符串     | true、false                                  | 在ebpf内核采集时，是否放弃采集所有系统调用         |
| 6    | drop_failed           | 字符串     | true、false                                  | 在ebpf内核采集时，是否放弃采集失败的系统调用       |
| 7    | interest_syscall_file | 字符串     | 文件路径，最长256                            | 指定特定json格式文件，文件内标识要采集哪些系统调用 |

## json_config/interesting_syscalls.json

该文件主要描述了哪些系统调用需要在`ebpf`内核中进行读取，具体格式如下：

```json
{
  "read": {
    "syscall_id": 0,
    "interesting": 0
  },
  "write": {
    "syscall_id": 1,
    "interesting": 0
  },
  "open": {
    "syscall_id": 2,
    "interesting": 1
  },
  "close": {
    "syscall_id": 3,
    "interesting": 1
  },
  ......
}
```

只要`interesting`字段为1，就表示需要采集该系统调用，否则不采集。

# 规则字段

当前插件提供了众多规则字段，所有前缀都为`linx`。可用字段描述如下：

| 序号 | 名称     | 是否可判断           | 参数                                     | 输出示例                                                     | 描述                                   |
| ---- | -------- | -------------------- | ---------------------------------------- | ------------------------------------------------------------ | -------------------------------------- |
| 1    | type     | 使用 in 进行子集判断 | 可以不跟，也可从后面选一个：[key\|value] | %linx.type: (3(close))<br />%linx.type[key]: (3)<br />%linx.type[value]: (close) | 触发事件的系统调用                     |
| 2    | user     | 使用 in 进行子集判断 | 可以不跟，也可从后面选一个：[key\|value] | %linx.user: (0(root))<br />%linx.user[key]: (0)<br />%linx.user[value]: (root) | 触发事件的用户                         |
| 3    | group    | 使用 in 进行子集判断 | 可以不跟，也可从后面选一个：[key\|value] | %linx.group: (0(root))<br />%linx.group[key]: (0)<br />%linx.group[value]: (root) | 触发事件的组                           |
| 4    | fds      | 使用 in 进行子集判断 | 可以不跟，也可从后面选一个：[key\|value] | %linx.fds: (0(/dev/null), 1(/dev/null))<br />%linx.fds[key]: (0, 1)<br />%linx.fds[value]: (/dev/null, /dev/null) | 当前进程打开的所有文件                 |
| 5    | args     | 不能                 | 不能跟参数                               | 不同的系统调用输出不同，这里以close为例<br />%linx.args: (fd=43) | 当前系统调用的所有参数                 |
| 6    | time     | 不能                 | 不能跟参数                               | %linx.time: 2025-06-12 09:43:49.410583558                    | 触发事件的时间                         |
| 7    | dir      | 使用 in 进行子集判断 | 不能跟参数                               | %linx.dir: (<)                                               | 标识进入系统调用(>)还是退出系统调用(<) |
| 8    | comm     | 使用 in 进行子集判断 | 不能跟参数                               | %linx.comm: (in:imjournal)                                   | 执行的命令                             |
| 9    | pid      | 使用 in 进行子集判断 | 不能跟参数                               | %linx.pid: (814)                                             | 触发事件任务的pid                      |
| 10   | tid      | 使用 in 进行子集判断 | 不能跟参数                               | %linx.tid: (814)                                             | 触发事件任务的tid                      |
| 11   | ppid     | 使用 in 进行子集判断 | 不能跟参数                               | %linx.ppid: (1)                                              | 触发事件任务的ppid                     |
| 12   | cmdline  | 使用 in 进行子集判断 | 不能跟参数                               | %linx.cmdline: (/usr/sbin/rsyslogd -n -i/var/run/rsyslogd.pid) | 在终端执行的命令                       |
| 13   | fullpath | 使用 in 进行子集判断 | 不能跟参数                               | %linx.fullpath: (/usr/sbin/rsyslogd)                         | 命令的绝对路径                         |

# 现存问题

## scripts/get_syscalls_macro.sh脚本问题

目前从脚本获取的系统调用参数不是很齐全，当前没有以下系统调用的参数：

- uname、uselib、_sysctl、get_kernel_syms、query_module、nfsservctl、getpmsg、putpmsg、afs_syscall、tuxcall、security、set_thread_area、get_thread_area、lookup_dcookie、vserver、landlock_create_ruleset、landlock_add_rule、landlock_restrict_self 

后续寻找其他内核文件中是否存在这些系统调用的参数，或者手动添加？

优化脚本，通过 /sys/kernel/debug/tracing/events/syscalls/sys_enter_$syscall/format 文件
找不到系统调用签名，则通过 man 手册查找，两个地方都找不到则不将该系统调用写入到宏定义文件中，
减少代码冗余。

## 参数提取问题

当前在`ebpf`内核中只将`char *`字符串指针类型的实际字符串提出了出来，其余所有的指针类型都只提取了其地址，并没有提取其值。

主要问题还是在于结构体指针的提取，不同结构体内部成员不同，并且可能存在成员又是指针的情况。

支持基础类型的指针提取，例如(int *, uint64_t *等等)

## 系统调用采集ebpf代码

由于所有`ebpf`代码都是通过脚本生成的，并且数量众多，当前并没有进行完备的测试，某些系统调用的参数提取逻辑可能还存在问题。

# 版本跟踪

## V0.1

完成基本功能，可采集所有系统调用，并通过`json_config/interesting_syscalls.json`文件配置要加载以及采集哪些系统调用。

存在问题：

1. 目前脚本`scripts/get_syscalls_macro.sh`获取的系统调用参数不是很齐全，当前没有以下系统调用的参数：

   uname、uselib、_sysctl、get_kernel_syms、query_module、nfsservctl、getpmsg、putpmsg、afs_syscall、tuxcall、security、set_thread_area、get_thread_area、lookup_dcookie、vserver、landlock_create_ruleset、landlock_add_rule、landlock_restrict_self 

2. 当前在`ebpf`内核中只将`char *`字符串指针类型的实际字符串提出了出来，其余所有的指针类型都只提取了其地址，并没有提取其值。主要问题还是在于结构体指针的提取，不同结构体内部成员不同，并且可能存在成员又是指针的情况。

3. 由于所有`ebpf`代码都是通过脚本生成的，并且数量众多，当前并没有进行完备的测试，某些系统调用的参数提取逻辑可能还存在问题。

## V0.2

新增模块：

1. 新增`xdp`类型的`ebpf`内核程序用于获取`http`头信息，文件位置`ebpf/xdp/xdp_pass.bpf.c`。

模块优化：

1. 上传`ebpf/include/vmlinux/vmlinux.h`，不再每次编译时生成，方便开发过程查看对应结构体。
2. 优化`linx_event_putfile`写入逻辑，使用滑动窗口`mmap`替代原本的`write`，降低写入延迟，提升写入效率。
3. 优化`scripts/get_syscalls_macro.sh`生成`linx_syscalls_macro.h`文件脚本，原本解析`unistd_64.h`中所有系统调用到`linx_syscalls_macro.h`文件，导致某些不存在的系统调用解析不到参数名称以及参数格式；现在通过`/sys/kernel/debug/tracing/events/syscalls/sys_exit_{系统调用名}/format`文件和`man`手册进行双重判断，没有的系统调用直接丢弃，减少代码冗余。
4. 优化`scripts/generate_interesting_syscalls.sh`生成要采集的系统调用`json`文件脚本，从原来解析`/usr/include/asm/unistd_64.h`文件替换为解析`include/macro/linx_syscalls_macro.h`文件。因为`unistd_64.h`为了版本兼容等问题保留了一些实际并没有使用的系统调用，而`linx_syscalls_macro.h`是经过处理，仅保留了当前系统在使用的系统调用。所以生成的`json`文件比上一版本更小。
5. 优化`scripts/generate_bpf_files.py`根据`linx_syscalls_macro.h`文件生成`ebpf`内核程序脚本，支持基本类型指针的值提取，例如(int *, uint64_t *等等)，结构体指针暂未支持。由于`linx_syscalls_macro.h`更细致化，所以生成的`ebpf`内核程序文件更少（减少当前系统没有的系统调用文件），更精细（对基础类型的指针也可以进行采集了）。

现存问题：

1. 新增的`xdp`网络采集消息格式和之前系统调用采集消息格式不同，所以当前并没有将实际消息传递到应用层。
2. 新增的`xdp`暂时没有字段控制其加载与否。
3. 系统调用采集不支持结构体指针值的采集。

