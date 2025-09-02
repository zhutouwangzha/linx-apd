#!/bin/bash

mkdir -p /tmp/linx_apd_rules
rm -rf /tmp/linx_apd_rules/*

cp ./yaml_config/linx_apd_rules/CVE-2023-22809.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/find.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/Illegal_users_add.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/illgal_cron_add.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/Prev_udf_exec.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/Prev_udf_funtion_create.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/Prev_udf_so_create.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/read_filesystem_info.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/read_grep_password.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/read_history_or_last.yaml /tmp/linx_apd_rules   # 只测过了last，read未测
cp ./yaml_config/linx_apd_rules/read_passwd.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/read_password.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/Reverse_normal.yaml /tmp/linx_apd_rules         # 输出的%proc.stdin.name %proc.stdout.name未做
cp ./yaml_config/linx_apd_rules/rm_auth_log.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/SSH_faild.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/SSH_success.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/sshkey_add.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/sudo_chroot.yaml /tmp/linx_apd_rules
cp ./yaml_config/linx_apd_rules/suid_cat.yaml /tmp/linx_apd_rules

gdb ./build/bin/linx-apd --args \
    ./build/bin/linx-apd -c yaml_config/linx_apd_config/linx_apd.yaml \
    -r /tmp/linx_apd_rules
