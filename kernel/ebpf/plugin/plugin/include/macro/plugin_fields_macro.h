#ifndef __PLUGIN_FIELDS_MACRO_H__
#define __PLUGIN_FIELDS_MACRO_H__

/**
 * PLUGIN_FIELDS_MACRO(id, up_name, low_name, type, islist, isRequired, isIndex, isKey, addOutput, display, desc)
 * 每个成员含义不同：
 *  - id: 字段的标号，从0开始累增
 *  - up_name: 大写名称
 *  - low_name: 小写名称
 *  - type: 字段的属性，可选"string", "uint64", "bool", "reltime", "abstime", "ipaddr", "ipnet"
 *  - name: 字段的名称，在实际定义时会在前面加上(linx.)，即在使用时通过linx.xxx获取
 *  - islist: 可选，若为true，则该字段可以传出多个值，并且在yaml文件中可以使用in进行子集判断
 *  - isRequired: 若为true，则字段必须跟上参数，例如linx.xxx[1]、linx.xxx[key]，并且isIndex和isKey必须有一项为true
 *  - isIndex: 若为true，则参数为数组类型
 *  - isKey: 若为true，则参数为字符串类型
 *  - addOutput: 可选，将采集的输出添加到合适的事件源中
 *  - display: 可选，若存在，则会替代原始名称在工具界面上的显示
 *  - desc: 描述性文字
 */

/*  PLUGIN_FIELDS_MACRO(idx, up_name,   low_name,       type,       islist, isreq,  isindx, iskey,  add,    dis,    desc) */
#define PLUGIN_FIELDS_MACRO_ALL \
    PLUGIN_FIELDS_MACRO(0,  TYPE,        type,           string,     true,   false,  false,  true,   false,  "", "以字符串形式输出当前触发的系统调用,可以跟上参数：[key]表示系统调用号,[value]表示系统调用名;不跟参数时默认输出:号(名)") \
    PLUGIN_FIELDS_MACRO(1,  USER,        user,           string,     true,   false,  false,  true,   false,  "", "触发系统调用的UID,可以跟上参数：[key]表示uid,[value]表示uid对应的用户名;不跟参数时默认输出:号(名)") \
    PLUGIN_FIELDS_MACRO(2,  GROUP,       group,          string,     true,   false,  false,  true,   false,  "", "触发系统调用的GID,可以跟上参数：[key]表示gid,[value]表示gid对应的组名;不跟参数时默认输出:号(名)") \
    PLUGIN_FIELDS_MACRO(3,  FDS,         fds,            string,     true,   false,  false,  true,   false,  "", "触发系统调用时关联的文件,可以跟上参数：[key]表示fd,[value]表示fd对应的文件名;不跟参数时默认输出:号(名)") \
    PLUGIN_FIELDS_MACRO(4,  ARGS,        args,           string,     false,  false,  false,  false,  false,  "", "按照一定格式输出的关于系统调用的所有相关信息") \
    PLUGIN_FIELDS_MACRO(5,  TIME,        time,           string,     false,  false,  false,  false,  false,  "", "触发系统调用的时间") \
    PLUGIN_FIELDS_MACRO(6,  DIR,         dir,            string,     true,   false,  false,  false,  false,  "", "标识是进入还是退出系统调用,'>'表示进入,'<' 表示退出") \
    PLUGIN_FIELDS_MACRO(7,  COMM,        comm,           string,     true,   false,  false,  false,  false,  "", "触发系统调用的命令") \
    PLUGIN_FIELDS_MACRO(8,  PID,         pid,            uint64,     true,   false,  false,  false,  false,  "", "触发系统调用的PID") \
    PLUGIN_FIELDS_MACRO(9,  TID,         tid,            uint64,     true,   false,  false,  false,  false,  "", "触发系统调用的TID") \
    PLUGIN_FIELDS_MACRO(10, PPID,        ppid,           uint64,     true,   false,  false,  false,  false,  "", "触发系统调用的PPID") \
    PLUGIN_FIELDS_MACRO(11, CMDLINE,     cmdline,        string,     true,   false,  false,  false,  false,  "", "触发系统调用的cmdline") \
    PLUGIN_FIELDS_MACRO(12, FULLPATH,    fullpath,       string,     true,   false,  false,  false,  false,  "", "触发系统调用的命令的绝对路径")

#endif /* __PLUGIN_FIELDS_MACRO_H__ */
