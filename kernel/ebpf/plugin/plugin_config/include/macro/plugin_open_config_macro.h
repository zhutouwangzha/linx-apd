#ifndef __PLUGIN_OPEN_CONFIG_MACRO_H__
#define __PLUGIN_OPEN_CONFIG_MACRO_H__

/**
 * OPEN_CONFIG 主要控制eBPF内核程序的执行、采集逻辑等，例如：
 *  需要过滤的pid
 *  需要过滤的系统调用等
 */
#define PLUGIN_OPEN_CONFIG_MACRO_ALL \
    PLUGIN_OPEN_CONFIG_MACRO(0, FILTER_OWN,     filter_own,     "在bpf采集过程中,是否过滤插件自身的系统调用,取值为true或false") \
    PLUGIN_OPEN_CONFIG_MACRO(1, FILTER_FALCO,   filter_falco,   "在bpf采集过程中过滤掉falco的系统调用,取值为ture或false")     \
    PLUGIN_OPEN_CONFIG_MACRO(2, FILTER_PIDS,    filter_pids,    "在bpf采集过程中要过滤掉对应pid的系统调用,取值为实质的pid") \
    PLUGIN_OPEN_CONFIG_MACRO(3, FILTER_COMMS,   filter_comms,   "在bpf采集过程中要过滤掉对应comm的系统调用,取值为ls、cat等") \
    PLUGIN_OPEN_CONFIG_MACRO(4, DROP_MODE,      drop_mode,      "在bpf采集过程中,是否放弃采集所有系统调用,取值为true或false") \
    PLUGIN_OPEN_CONFIG_MACRO(5, DROP_FAILED,    drop_failed,    "在bpf采集过程中是否放弃采集失败的系统调用,取值为treu或false") \
    PLUGIN_OPEN_CONFIG_MACRO(6, INTEREST_SYSCALL_FILE, interest_syscall_file, "在bpf采集过程中要采集的系统调用,取值是指向一个特定格式的json文件路径")

#endif /* __PLUGIN_OPEN_CONFIG_MACRO_H__ */
