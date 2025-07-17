# 固域3.0内核模块

## 1）概述

内核模块采用tracepoint方法，完成进程信息收集，以及与应用层数据交互。

### 1）目录结构

```
├── include							
│   ├── kernel					// 内核头文件
│   │   ├── linx_consumer.h		// 消费者数据结构定义头文件
│   │   ├── linx_fillers.h		// 实际参数填充函数头文件
│   │   └── sysmon_events.h		// 实际参数填充头文件
│   ├── kmod_msg.h				// 公用数据结构（内核与应用层公用）
│   └── user
│       ├── linx_devset.h		// 驱动操作
│       ├── linx_logs.h			// 日志输出
│       └── linx_syscall.h		// 系统调用
├── kernel
│   ├── linx_event_table.c		// 系统调用定义
│   ├── linx_filler.c			// 参数解析
│   ├── linx_fillers_table.c	// 参数解析函数映射
│   ├── linx_syscall_table.c	// 系统调用映射信息
│   ├── Makefile				// 内核模块编译文件
│   └── sysmon_main.c			// 内核模块主函数
├── Makefile
├── README.md
├── script
│   └── test.sh
└── user
    ├── linx_devset.c			// 驱动操作含
    ├── linx_logs.c				// 日志序列化输出
    ├── main.c					
    └── Makefile
```



## 2）当前状态

```
1）可实现系统调用采集，以及数据发送，数据采集包括：时间戳、pid、gid、系统调用号及名称、命令、文件操作符等信息。
2）通过mmap映射内存（当前32*1024B），应用层可直接读取。
3）支持实际参数解析(当前仅在内核中实现open)
```

## 3）待完善功能

```
1）系统调用暂时覆盖较少，缓冲区分配较小，未实现多核独立采集
2）系统调用实际参数解析未覆盖所有系统调用。
3）缓冲区输出信息的序列化有待完善。
4）应用层暂未支持JSON配置功能。
5）丢弃模式功能，内核简单过滤功能待开发。
```

## 4）操作

```
1）编译
$>	make

2）执行用户层序
$> ./user/app

3）编译内核模块
$> make kernel

4）编译应用层序
$> make user

5）加载/卸载内核模块
$> insmod sysmon.ko  && rmmod sysmon.ko

6）默认输出日志文件名称：linx_kmod.txt
```

