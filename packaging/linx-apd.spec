%define pkg_mark zlx1
%define pkg_release 1
%define pkg_dist_suffix v6_2403
%define pkg_version 0.0.1
%define pkg_name linx-apd
%define pkg_kver %(ls /lib/modules)
%define pkg_kdir /lib/modules/%{pkg_kver}/build

Name: %{pkg_name}
Version: %{pkg_version}
Release: %{pkg_release}.%{pkg_mark}.%{pkg_dist_suffix}
Summary: linx apd
License: Copyright (C) 2025 Beijing Linx Software Corp. All Rights Reserved.
URL: http://www.linx-info.com
Source0: %{name}-%{version}.tar.gz
BuildRequires: musl-libc gcc make zlib zstd kernel-devel bzip2 xz-libs elfutils-libelf libbpf-devel cjson libyaml pcre2
Requires: kernel zlib zstd bzip2 xz-libs elfutils-libelf

%description
linx apd

%prep
%setup -q
export LC_ALL=C

%build
export KDIR=%{pkg_kdir} ; make

%install
mkdir -pv %{buildroot}/{usr/bin,lib/modules/%{pkg_kver}/kernel/security,etc/linx_apd_config,etc/linx_apd_rules}
cp -rf build/bin/linx-apd %{buildroot}/usr/bin
cp -rf build/kmod/linxapd.ko %{buildroot}/lib/modules/%{pkg_kver}/kernel/security
cp -rf json_config/interesting_syscalls.json %{buildroot}/etc/linx_apd_config
cp -rf yaml_config/linx_apd_config/linx_apd.yaml %{buildroot}/etc/linx_apd_config
cp -rf yaml_config/linx_apd_rules/SSH_success.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/sudo_chroot.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/Prev_udf_so_create.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/Reverse_normal.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/suid_cat.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/read_passwd.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/read_filesystem_info.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/rm_auth_log.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/find.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/read_history_or_last.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/CVE-2018-1000861.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/illgal_cron_add.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/CVE-2023-22809.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/read_password.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/Prev_udf_exec.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/SSH_faild.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/Illegal_users_add.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/read_grep_password.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/Prev_udf_funtion_create.yaml %{buildroot}/etc/linx_apd_rules
cp -rf yaml_config/linx_apd_rules/sshkey_add.yaml %{buildroot}/etc/linx_apd_rules

%post
depmod -F /boot/System.map-%{pkg_kver} %{pkg_kver}

%postun
depmod -F /boot/System.map-%{pkg_kver} %{pkg_kver}

%files
/usr/bin/linx-apd
/lib/modules/%{pkg_kver}/kernel/security/linxapd.ko
/etc/linx_apd_config/interesting_syscalls.json
/etc/linx_apd_config/linx_apd.yaml
/etc/linx_apd_rules/SSH_success.yaml
/etc/linx_apd_rules/sudo_chroot.yaml
/etc/linx_apd_rules/Prev_udf_so_create.yaml
/etc/linx_apd_rules/Reverse_normal.yaml
/etc/linx_apd_rules/suid_cat.yaml
/etc/linx_apd_rules/read_passwd.yaml
/etc/linx_apd_rules/read_filesystem_info.yaml
/etc/linx_apd_rules/rm_auth_log.yaml
/etc/linx_apd_rules/find.yaml
/etc/linx_apd_rules/read_history_or_last.yaml
/etc/linx_apd_rules/CVE-2018-1000861.yaml
/etc/linx_apd_rules/illgal_cron_add.yaml
/etc/linx_apd_rules/CVE-2023-22809.yaml
/etc/linx_apd_rules/read_password.yaml
/etc/linx_apd_rules/Prev_udf_exec.yaml
/etc/linx_apd_rules/SSH_faild.yaml
/etc/linx_apd_rules/Illegal_users_add.yaml
/etc/linx_apd_rules/read_grep_password.yaml
/etc/linx_apd_rules/Prev_udf_funtion_create.yaml
/etc/linx_apd_rules/sshkey_add.yaml

%changelog

* Tue Aug 12 2025 Li, Hesong <hsli@linx-info.com> - 0.0.1-1.zlx1
- Init.
