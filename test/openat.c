#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

// 获取文件描述符对应的文件名（通过/proc）
char* get_filename_from_fd(int fd) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

    // 获取路径长度
    ssize_t len = readlink(path, NULL, 0);
    if (len < 0) {
        perror("readlink(get size)");
        return NULL;
    }

    // 分配内存并读取链接目标
    char *filename = malloc(len + 1);
    if (!filename) {
        perror("malloc");
        return NULL;
    }

    ssize_t bytes = readlink(path, filename, len);
    if (bytes < 0) {
        perror("readlink");
        free(filename);
        return NULL;
    }
    
    filename[bytes] = '\0';  // 确保字符串终止
    return filename;
}

// 将文件标志转换为可读字符串
const char* flags_to_string(int flags) {
    static char buffer[256];
    buffer[0] = '\0';
    
    // 访问模式
    switch (flags & O_ACCMODE) {
        case O_RDONLY: strcat(buffer, "O_RDONLY"); break;
        case O_WRONLY: strcat(buffer, "O_WRONLY"); break;
        case O_RDWR:   strcat(buffer, "O_RDWR"); break;
        default:       strcat(buffer, "UNKNOWN_ACCESS");
    }
    
    // 其他标志
    if (flags & O_APPEND)    strcat(buffer, " | O_APPEND");
    if (flags & O_ASYNC)     strcat(buffer, " | O_ASYNC");
    if (flags & __O_CLOEXEC)   strcat(buffer, " | O_CLOEXEC");
    if (flags & O_CREAT)     strcat(buffer, " | O_CREAT");
    if (flags & __O_DIRECT)    strcat(buffer, " | O_DIRECT");
    if (flags & __O_DIRECTORY) strcat(buffer, " | O_DIRECTORY");
    if (flags & __O_DSYNC)     strcat(buffer, " | O_DSYNC");
    if (flags & O_EXCL)      strcat(buffer, " | O_EXCL");
    if (flags & __O_NOATIME)   strcat(buffer, " | O_NOATIME");
    if (flags & O_NOCTTY)    strcat(buffer, " | O_NOCTTY");
    if (flags & __O_NOFOLLOW)  strcat(buffer, " | O_NOFOLLOW");
    if (flags & O_NONBLOCK)  strcat(buffer, " | O_NONBLOCK");
    if (flags & __O_PATH)      strcat(buffer, " | O_PATH");
    if (flags & O_SYNC)      strcat(buffer, " | O_SYNC");
    if (flags & __O_TMPFILE)   strcat(buffer, " | O_TMPFILE");
    if (flags & O_TRUNC)     strcat(buffer, " | O_TRUNC");
    
    return buffer;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [mode]\n", argv[0]);
        fprintf(stderr, "Modes: create (default), append, truncate, read-only\n");
        return EXIT_FAILURE;
    }
    
    const char *filename = argv[1];
    int flags = O_RDWR;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // rw-r--r--
    
    // 解析打开模式
    if (argc > 2) {
        if (strcmp(argv[2], "append") == 0) {
            flags |= O_APPEND | O_CREAT;
        } else if (strcmp(argv[2], "truncate") == 0) {
            flags |= O_TRUNC | O_CREAT;
        } else if (strcmp(argv[2], "read-only") == 0) {
            flags = O_RDONLY;
        } else if (strcmp(argv[2], "create") == 0) {
            flags |= O_CREAT | O_EXCL;
        }
    } else {
        flags |= O_CREAT;
    }
    
    printf("Opening file: %s\n", filename);
    printf("Flags used:   %s\n", flags_to_string(flags));
    
    // 打开文件
    int fd = open(filename, flags, mode);
    if (fd == -1) {
        perror("open failed");
        return EXIT_FAILURE;
    }
    
    printf("\nFile opened successfully!\n");
    printf("File descriptor: %d\n", fd);
    
    // 获取实际使用的标志（可能与应用请求的不同）
    int actual_flags = fcntl(fd, F_GETFL);
    if (actual_flags == -1) {
        perror("fcntl failed");
        close(fd);
        return EXIT_FAILURE;
    }
    
    printf("Actual flags:    %s\n", flags_to_string(actual_flags));
    
    // 获取文件路径
    char *resolved_path = get_filename_from_fd(fd);
    if (resolved_path) {
        printf("Resolved path:   %s\n", resolved_path);
        free(resolved_path);
    } else {
        printf("Could not resolve path for fd %d\n", fd);
    }
    
    sleep(10);

    // 关闭文件
    if (close(fd) == -1) {
        perror("close failed");
        return EXIT_FAILURE;
    }
    
    printf("\nFile closed successfully\n");
    return EXIT_SUCCESS;
}
