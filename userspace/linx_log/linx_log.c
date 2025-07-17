#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

#include "linx_log.h"
#include "linx_thread_pool.h"

static linx_log_t *g_linx_log_instance = NULL;

static linx_thread_pool_t *g_log_thread_pool = NULL;

static char *linx_log_level_str[LINX_LOG_LEVEL_MAX] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};

/**
 * @brief 将日志级别字符串转换为对应的枚举值
 * 
 * 该函数用于把字符串形式的日志级别（如"DEBUG"、"INFO"等）转换为对应的枚举值。
 * 当输入字符串为NULL或不能匹配任何已知日志级别时，会返回LINX_LOG_LEVEL_MAX表示错误。
 * 
 * @param level_str 日志级别的字符串表示（区分大小写），可以是NULL
 * 
 * @return 返回匹配的linx_log_level_t枚举值；
 *         如果没有匹配项则返回LINX_LOG_LEVEL_MAX表示错误
 */
static linx_log_level_t linx_log_level_str_to_level(const char *level_str)
{
    /* 立即处理NULL输入的情况 */
    if (!level_str) {
        return LINX_LOG_LEVEL_MAX;
    }

    /* 将输入字符串与所有已知日志级别字符串进行比较 */
    for (int i = 0; i < LINX_LOG_LEVEL_MAX; i++) {
        if (strcmp(level_str, linx_log_level_str[i]) == 0) {
            return (linx_log_level_t)i;
        }
    }

    /* 没有找到匹配的日志级别 */
    return LINX_LOG_LEVEL_MAX;
}

/**
 * @brief 将日志消息推入日志队列
 * 
 * 该函数负责将新的日志消息添加到全局日志实例的队列中。如果队列已满，
 * 会自动扩展队列容量。操作是线程安全的，使用互斥锁保护共享资源。
 * 
 * @param message 要推入队列的日志消息，包含日志内容和相关信息。
 *                如果队列扩展失败，该消息会被释放。
 */
static void linx_log_queue_push(linx_log_message_t *message)
{
    int new_capacity;
    linx_log_message_t **new_queue;

    /* 获取互斥锁保护共享资源 */
    pthread_mutex_lock(&g_linx_log_instance->lock);

    /* 检查队列容量，必要时进行扩展 */
    if (g_linx_log_instance->queue_size >= g_linx_log_instance->queue_capacity) {
        /* 计算新容量：初始为16，之后每次翻倍 */
        new_capacity = g_linx_log_instance->queue_capacity == 0 
                        ? 16 : g_linx_log_instance->queue_capacity * 2;
        
        /* 尝试扩展队列内存 */
        new_queue = realloc(g_linx_log_instance->queue, sizeof(linx_log_message_t *) * new_capacity);
        if (!new_queue) {
            /* 内存分配失败处理：释放消息并返回 */
            fprintf(stderr, "Failed to expand log queue\n");
            free(message->message);
            free(message);
            pthread_mutex_unlock(&g_linx_log_instance->lock);
            return;
        }

        /* 更新队列指针和容量 */
        g_linx_log_instance->queue = new_queue;
        g_linx_log_instance->queue_capacity = new_capacity;
    }

    /* 消息入队 */
    g_linx_log_instance->queue[g_linx_log_instance->queue_size++] = message;

    /* 释放互斥锁 */
    pthread_mutex_unlock(&g_linx_log_instance->lock);
}

/**
 * @brief 从日志队列中弹出消息（FIFO方式）
 * 
 * 该函数从全局日志实例的队列中取出最早的一条日志消息。
 * 操作是线程安全的，使用互斥锁保护共享资源。
 * 
 * @return 成功时返回 linx_log_message_t* 类型的消息指针；
 *         如果队列为空则返回NULL
 */
static linx_log_message_t *linx_log_queue_pop(void)
{
    linx_log_message_t *msg;

    /* 加锁确保线程安全 */
    pthread_mutex_lock(&g_linx_log_instance->lock);

    /* 检查队列是否为空 */
    if (g_linx_log_instance->queue_size == 0) {
        pthread_mutex_unlock(&g_linx_log_instance->lock);
        return NULL;
    }

    /* 获取队列首元素（FIFO） */
    msg = g_linx_log_instance->queue[0];
    
    /* 将后续元素前移一位实现出队操作 */
    for (int i = 1; i < g_linx_log_instance->queue_size; ++i) {
        g_linx_log_instance->queue[i - 1] =
            g_linx_log_instance->queue[i];
    }

    /* 更新队列大小 */
    g_linx_log_instance->queue_size--;

    /* 解锁 */
    pthread_mutex_unlock(&g_linx_log_instance->lock);

    return msg;
}

/**
 * @brief LINX日志处理线程
 * 
 * 该线程负责从日志队列中取出消息并写入日志文件，直到收到停止信号。
 * 
 * @param arg 未使用的线程参数（保留参数）
 * @param should_stop 停止标志指针：
 *                    - 2: 立即停止
 *                    - 1: 优雅停止（处理完队列中所有消息后停止）
 *                    - 0: 继续运行
 * @return void* 总是返回NULL
 */
static void *linx_log_thread(void *arg, int *should_stop)
{
    (void)arg;

    linx_log_message_t *msg = NULL;

    /* 主消息处理循环 */
    while (1) {
        char time_buf[64];
        struct tm *tm_info;

        /* 从队列获取下一条日志消息 */
        msg = linx_log_queue_pop();

        /* 检查停止条件：
         * 2 = 立即停止
         * 1 = 队列为空时停止
         */
        if (*should_stop == 2) {
            break;
        } else if (*should_stop == 1 && msg == NULL) {
            break;
        } else if (msg == NULL) {
            sched_yield();
            continue;
        }

        /* 格式化时间戳 */
        tm_info = localtime(&msg->tv.tv_sec);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

        /* 将日志写入文件（如果文件已打开） */
        if (g_linx_log_instance->log_file) {
            fprintf(g_linx_log_instance->log_file, "[%s.%03ld] [%s] %s\n",
                    time_buf, msg->tv.tv_usec / 1000,
                    linx_log_level_str[msg->level], msg->message);
            fflush(g_linx_log_instance->log_file);
        }

        /* 释放消息资源 */
        free(msg->message);
        free(msg);
    }
    
    return NULL;
}

/**
 * @brief 初始化LINX日志系统
 * 
 * 该函数负责初始化全局日志实例，包括：
 * 1. 根据传入的日志级别字符串设置日志过滤等级
 * 2. 分配日志实例内存并初始化互斥锁
 * 3. 设置日志输出目标（文件或stderr）
 * 4. 初始化日志队列和线程池
 * 
 * @param log_file 日志文件路径：
 *                - "stderr"：输出到标准错误
 *                - NULL：默认输出到标准错误
 *                - 其他字符串：作为文件路径打开并追加日志
 * @param log_level 日志级别字符串（如"DEBUG"、"INFO"等）
 * 
 * @return 成功返回0，失败返回-1（可能原因：无效日志级别、内存分配失败、
 *         锁初始化失败、文件打开失败或线程池创建失败）
 */
int linx_log_init(const char *log_file, const char *log_level)
{
    int ret = 0;
    // 将字符串形式的日志级别转换为枚举值
    linx_log_level_t level = linx_log_level_str_to_level(log_level);

    // 校验日志级别有效性
    if (level < 0 || level >= LINX_LOG_LEVEL_MAX) {
        return -1;
    }

    // 分配全局日志实例内存
    g_linx_log_instance = (linx_log_t *)malloc(sizeof(linx_log_t));
    if (g_linx_log_instance == NULL) {
        return -1;
    }

    // 初始化互斥锁用于线程安全
    if (pthread_mutex_init(&g_linx_log_instance->lock, NULL) != 0) {
        free(g_linx_log_instance);
        return -1;
    }

    // 设置日志输出目标
    if (log_file != NULL) {
        if (strcmp(log_file, "stderr") == 0) {
            g_linx_log_instance->log_file = stderr;  // 输出到标准错误
        } else {
            g_linx_log_instance->log_file = fopen(log_file, "a");  // 追加模式打开文件
            if (g_linx_log_instance->log_file == NULL) {
                return -1;
            }
        }
    } else {
        g_linx_log_instance->log_file = stderr;  // 默认输出到标准错误
    }

    // 初始化日志队列参数
    g_linx_log_instance->level = level;
    g_linx_log_instance->queue = NULL;
    g_linx_log_instance->queue_capacity = 0;
    g_linx_log_instance->queue_size = 0;

    // 创建单线程的日志线程池
    g_log_thread_pool = linx_thread_pool_create(1);
    if (g_log_thread_pool == NULL) {
        linx_log_deinit();  // 失败时清理资源
        return -1;
    }

    // 添加日志处理任务到线程池
    ret = linx_thread_pool_add_task(g_log_thread_pool, linx_log_thread, NULL);
    if (ret) {
        linx_log_deinit();  // 失败时清理资源
        return -1;
    }

    return 0; 
}

/**
 * @brief 释放并清理日志系统的所有资源
 * 
 * 该函数负责安全地关闭日志系统，包括：
 * 1. 销毁日志线程池
 * 2. 关闭日志文件（非标准错误输出时）
 * 3. 重置队列容量
 * 4. 销毁互斥锁
 * 5. 释放队列内存
 * 6. 释放日志实例内存
 * 
 * @note 该函数会处理所有资源清理工作，调用后日志系统将不可用
 */
void linx_log_deinit(void)
{
    /* 强制销毁日志线程池，等待所有任务完成 */
    linx_thread_pool_destroy(g_log_thread_pool, 1);
    g_log_thread_pool = NULL;

    /* 关闭日志文件（如果是非stderr的独立文件） */
    if (g_linx_log_instance->log_file &&
        g_linx_log_instance->log_file != stderr) 
    {
        fclose(g_linx_log_instance->log_file);
        g_linx_log_instance->log_file = NULL;
    }

    /* 重置队列容量 */
    g_linx_log_instance->queue_capacity = 0;

    /* 销毁用于线程同步的互斥锁 */
    pthread_mutex_destroy(&g_linx_log_instance->lock);

    /* 释放日志队列内存 */
    free(g_linx_log_instance->queue);
    g_linx_log_instance->queue = NULL;

    /* 释放日志实例结构体内存 */
    free(g_linx_log_instance);
}

/**
 * @brief 根据指定的日志级别、文件名、行号和格式字符串输出日志
 *
 * 该函数是一个可变参数函数，用于根据给定的日志级别决定是否输出日志。
 * 如果日志级别低于当前设置的全局日志级别，则直接返回不输出。
 * 否则，将格式化日志消息并输出。
 *
 * @param level 日志级别，用于判断是否满足输出条件
 * @param file  当前源文件名，通常使用__FILE__宏
 * @param line  当前行号，通常使用__LINE__宏
 * @param format 格式化字符串，用于指定日志内容的格式
 * @param ...   可变参数，用于填充格式化字符串中的占位符
 */
void linx_log(linx_log_level_t level, const char *file, int line, const char *format, ...)
{
    /* 检查日志级别是否满足输出条件 */
    if (level < g_linx_log_instance->level) {
        return;
    }

    /* 处理可变参数并调用日志输出函数 */
    va_list args;
    va_start(args, format);
    linx_log_v(level, file, line, format, args);
    va_end(args);
}

/**
 * @brief 记录可变参数的日志消息
 * 
 * 该函数根据指定的日志级别、文件名、行号和格式字符串，生成日志消息并推送到日志队列中。
 * 如果当前日志级别低于配置的全局日志级别，则直接返回不记录。
 * 
 * @param level 日志级别，用于判断是否应该记录该消息
 * @param file 源文件名，用于标识日志来源
 * @param line 源代码行号，用于标识日志来源
 * @param format 格式化字符串，用于构建日志消息内容
 * @param args 可变参数列表，用于填充格式化字符串
 */
void linx_log_v(linx_log_level_t level, const char *file, int line, const char *format, va_list args)
{
    linx_log_message_t *msg;
    char msg_buf[1024];
    int len;

    /* 检查当前日志级别是否低于配置的全局日志级别，如果是则直接返回 */
    if (level < g_linx_log_instance->level) {
        return;
    }

    /* 构建日志消息前缀，包含文件名和行号信息 */
    len = snprintf(msg_buf, sizeof(msg_buf), "[%s:%d]: ",
                   file, line);
    /* 检查前缀构建是否成功，防止缓冲区溢出 */
    if (len < 0 || len >= (int)sizeof(msg_buf)) {
        return;
    }

    /* 将可变参数格式化到消息缓冲区中 */
    len = vsnprintf(msg_buf + len, sizeof(msg_buf) - len,
                    format, args);
    /* 检查格式化是否成功 */
    if (len < 0) {
        return;
    }

    /* 分配日志消息结构体内存 */
    msg = malloc(sizeof(linx_log_message_t));
    if (!msg) {
        return;
    }

    /* 填充日志消息结构体字段 */
    gettimeofday(&msg->tv, NULL);
    msg->level = level;
    msg->message = strdup(msg_buf);
    /* 检查消息字符串复制是否成功 */
    if (!msg->message) {
        free(msg);
        return;
    }

    /* 将日志消息推送到日志队列中 */
    linx_log_queue_push(msg);
}
