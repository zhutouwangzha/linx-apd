#include "linx_common.h"
#include "scap.h"

const char* plugin_get_required_api_version() {
    return PLUGIN_API_VERSION_STR;
}

const char* plugin_get_version() {
	return "0.1.0";
}

const char* plugin_get_name() {
	return "syscall_sequence_plugin";
}

const char* plugin_get_description() {
	return "该插件加载ebpf程序,采集进程的系统调用序列,并输出到对应的文件或终端中";
}

const char* plugin_get_contact() {
	return "linx";
}
