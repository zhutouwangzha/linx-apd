#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "linx_machine_status.h"
#include "linx_hash_map.h"
#include "linx_log.h"

static user_t s_user = {0};
static group_t s_group = {0};

static int64_t get_login_uid(void)
{
    int64_t loginuid = -1;
    FILE *fp = fopen("/proc/self/loginuid", "r");
    if (!fp) {
        return loginuid;
    }

    if (fscanf(fp, "%ld", &loginuid) != 1) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return loginuid;
}

static void get_current_user(user_t *user)
{
    uid_t uid = getuid();
    int64_t loginuid = get_login_uid();
    struct passwd *pw = getpwuid(uid);

    user->uid = uid;
    user->loginuid = (loginuid >= 0) ? loginuid : (int64_t)uid;

    if (pw) {
        strncpy(user->name, pw->pw_name, sizeof(user->name) - 1);
        strncpy(user->homedir, pw->pw_dir, sizeof(user->homedir) - 1);
        strncpy(user->shell, pw->pw_shell, sizeof(user->shell) - 1);
    } else {
        user->name[0] = '\0';
        user->homedir[0] = '\0';
        user->shell[0] = '\0';
    }

    user->name[sizeof(user->name) - 1] = '\0';
    user->homedir[sizeof(user->homedir) - 1] = '\0';
    user->shell[sizeof(user->shell) - 1] = '\0';

    pw = getpwuid(user->loginuid);
    if (pw) {
        strncpy(user->loginname, pw->pw_name, sizeof(user->loginname) - 1);
    } else {
        user->loginname[0] = '\0';
    }
    
    user->loginname[sizeof(user->loginname) - 1] = '\0';
}

static void get_current_group(group_t *group)
{
    gid_t gid = getgid();
    struct group *gr = getgrgid(gid);

    group->gid = gr->gr_gid;

    if (gr) {
        strncpy(group->name, gr->gr_name, sizeof(group->name) - 1);
    } else {
        group->name[0] = '\0';
    }

    group->name[sizeof(group->name) - 1] = '\0';
}

static int bind_user_field(void)
{
    int ret;

    BEGIN_FIELD_MAPPINGS(user)
        FIELD_MAP(user_t, uid, LINX_FIELD_TYPE_UINT32)
        FIELD_MAP(user_t, name, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(user_t, homedir, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(user_t, shell, LINX_FIELD_TYPE_CHARBUF)
        FIELD_MAP(user_t, loginuid, LINX_FIELD_TYPE_INT64)
        FIELD_MAP(user_t, loginname, LINX_FIELD_TYPE_CHARBUF)
    END_FIELD_MAPPINGS(user)

    ret = linx_hash_map_add_field_batch("user", user_mappings, user_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
    }

    return ret;
}

static int bind_group_field(void)
{
    int ret;

    BEGIN_FIELD_MAPPINGS(group)
        FIELD_MAP(group_t, gid, LINX_FIELD_TYPE_UINT32)
        FIELD_MAP(group_t, name, LINX_FIELD_TYPE_CHARBUF)
    END_FIELD_MAPPINGS(group)

    ret = linx_hash_map_add_field_batch("group", group_mappings, group_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
        return -1;
    }

    return ret;
}

static int linx_machine_status_bind_field(void)
{
    int ret = -1;

    ret = bind_user_field();
    ret = ret ? : bind_group_field();

    return ret;
}

int linx_machine_status_init(void)
{
    int ret = linx_machine_status_bind_field();
    if (ret) {
        return ret;
    }

    get_current_user(&s_user);
    get_current_group(&s_group);

    return ret;
}

void linx_machine_status_deinit(void)
{

}

user_t *linx_machine_status_get_user(void)
{
    return &s_user;
}

group_t *linx_machine_status_get_group(void)
{
    return &s_group;
}
