#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define TEST_DIR_NAME   "/tmp/test_dir"
#define TEST_FILE_NAME  "test_file.txt"

static int unlinkat_test(void)
{
    int ret = 0, dir_fd, file_fd;
    char file_path[256];

    ret = mkdir(TEST_DIR_NAME, 0755);
    if (ret < 0) {
        perror("mkdir");
        return ret;
    }

    dir_fd = open(TEST_DIR_NAME, O_DIRECTORY | O_RDONLY);
    if (dir_fd < 0) {
        perror("open");
        ret = -1;
        goto out;
    }

    snprintf(file_path, sizeof(file_path), "%s/%s", TEST_DIR_NAME, TEST_FILE_NAME);
    file_fd = open(file_path, O_CREAT | O_WRONLY, 0644);
    if (file_fd < 0) {
        perror("open");
        ret = -1;
        goto out;
    }

    close(file_fd);

    ret = unlinkat(dir_fd, TEST_FILE_NAME, 0);
    if (ret < 0) {
        perror("unlinkat");
    }

    printf("unlinkat(%d, %s, %d) = %d\n", dir_fd, TEST_FILE_NAME, 0, ret);

out:
    close(dir_fd);
    ret = rmdir(TEST_DIR_NAME);
    if (ret < 0) {
        perror("rmdir");
    }

    return ret;
}

static int read_test(void)
{
    char buf[128];

    int fd = open("./Makefile", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return errno;
    }

    read(fd, buf, sizeof(buf));

    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    ret = ret ? : unlinkat_test();
    ret = ret ? : read_test();

    return ret;
}
