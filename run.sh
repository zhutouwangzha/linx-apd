#!/bin/bash
set -e

current_name=$0
script_name=$(basename $current_name)
script_path=$(readlink -f $current_name)
script_dir=$(dirname $script_path)

read_timeout=10

# http://10.1.43.167/ci_data_linx_apd/ci/os992403_x86_64.tar
func_docker_run()
{
    docker run --rm -v $PWD:/builds --workdir /builds  --privileged --network host -it os992403_x86_64:latest bash
    return
}

func_install_pkgs()
{
    local spec_path=packaging/linx-apd.spec
    yum makecache
    yum remove -y kernel-devel
    rm -rf /lib/modules/*
    yum-builddep -y $spec_path
    return
}

func_compile_apd()
{
    export KERNELDIR=/lib/modules/$(ls /lib/modules)/build
    export KDIR=${KERNELDIR}
    make || echo yes
    return
}

func_rpm_apd()
{
    local pkg_name=$(cat packaging/linx-apd.spec | grep pkg_name | head -n 1 | awk '{ print $3 }')
    local pkg_version=$(cat packaging/linx-apd.spec | grep pkg_version | head -n 1 | awk '{ print $3 }')
    local rpmbuild_root_dir=$(pwd)/rpmbuild
    local spec_path=packaging/${pkg_name}.spec

    rm -rf $rpmbuild_root_dir
    mkdir -p ${rpmbuild_root_dir}/{SOURCES,SPECS}
    rm -rf /tmp/${pkg_name}-${pkg_version}
    mkdir -p /tmp/${pkg_name}-${pkg_version}
    cp -rf * /tmp/${pkg_name}-${pkg_version}
    pushd /tmp/
    tar -czf ${rpmbuild_root_dir}/SOURCES/${pkg_name}-${pkg_version}.tar.gz ${pkg_name}-${pkg_version}
    rm -rf ${pkg_name}-${pkg_version}
    popd
    cp -rf $spec_path ${rpmbuild_root_dir}/SPECS/${pkg_name}.spec
    rpmbuild -ba --define "_topdir ${rpmbuild_root_dir}" ${rpmbuild_root_dir}/SPECS/${pkg_name}.spec

    return
}

func_rpm_apd_test()
{
    local spec_path=packaging/linx-apd.spec
    yum install -y kernel zlib zstd bzip2 xz-libs elfutils-libelf
    find rpmbuild/RPMS/ ! -name *debugsource* ! -name "*debuginfo*" -name *.rpm -exec rpm -Kv {} \;
    find rpmbuild/RPMS/ ! -name *debugsource* ! -name "*debuginfo*" -name *.rpm -exec rpm -q --info {} \;
    find rpmbuild/RPMS/ ! -name *debugsource* ! -name "*debuginfo*" -name *.rpm -exec rpm -ivh {} \;
    modinfo linxapd -k $(ls /lib/modules)
    return
}

func_upload_apd()
{
    find . -type f -name *.rpm -exec export env_find_rpm=1 \; &> /dev/null || echo yes
    if ! find . -type f -name *.rpm | grep -q rpm; then exit 0; fi
    if ! test -z $GITLAB_USER_LOGIN; then
        local user_login=$GITLAB_USER_LOGIN
    else
        exit 0
    fi
    local artifacts_path=artifacts
    local upload_date=$(date +'%Y-%m-%d_%H.%M.%S')
    local upload_tmp_dir=/root/ci_data_linx_apd/$user_login/linx-apd/$upload_date
    local upload_hostname=root@10.1.43.167
    local upload_passwd=root
    sshpass -p $upload_passwd ssh -o StrictHostKeyChecking=no $upload_hostname "[ ! -d $upload_tmp_dir ]; mkdir -p $upload_tmp_dir"
    find . -type f ! -name *debugsource* ! -name "*debuginfo*" ! -name *.src.rpm -name *.rpm -exec sshpass -p $upload_passwd scp -o StrictHostKeyChecking=no -r {} $upload_hostname:$upload_tmp_dir \;
    return
}

func_build_apd()
{
    func_install_pkgs
    func_rpm_apd
    func_rpm_apd_test
    func_upload_apd
    return
}

func_cloc_info()
{
    local cloc_info_dir=artifacts_cloc_info
    local code_txt_dir=${cloc_info_dir}/code
    local commit_txt_dir=${cloc_info_dir}/commit
    local code_txt=${code_txt_dir}/linx-apd.txt
    local commit_txt=${commit_txt_dir}/linx-apd.txt

    mkdir -pv $code_txt_dir $commit_txt_dir
    git config --global --add safe.directory $(pwd)
    git log > $commit_txt
    cat $commit_txt | grep ^commit | wc -l > ${commit_txt}.commit.number.txt
    echo "$ cloc kernel/kmod" > ${code_txt}
    cloc kernel/kmod >> ${code_txt}
    echo "$ cloc --not-match-f='vmlinux.h$' kernel/ebpf" >> ${code_txt}
    cloc --not-match-f="vmlinux.h$" kernel/ebpf >> ${code_txt}
    echo "$ cloc --not-match-f='vmlinux.h$' kernel" >> ${code_txt}
    cloc --not-match-f='vmlinux.h$' kernel >> ${code_txt}
    echo "$ cloc --not-match-f='vmlinux.h$' ." >> ${code_txt}
    cloc --not-match-f='vmlinux.h$' . >> ${code_txt}
    find $cloc_info_dir -type f -exec cat {} \;

    return
}

func_exit()
{
    return
}

function func_main()
{
    case $1 in
        "--help")
            echo "Usage: cmd [--help|func_<op>]"
            ;;
        *)
            if [[ -z "$1" ]]; then
                local op_arry=($(cat $script_path | grep '^func_' | grep '()' | grep -v 'func_main' | sed s/'()'//g | xargs))
                local op_arry_len=${#op_arry[@]}
                echo "Operation list:"
                for((i=0;$i<${op_arry_len};i=$[$i+1]))
                do
                    echo  "  [$[$i + 1]] ${op_arry[$i]}"
                done
                echo "Input operation:"
                read op_idx
                if test -z $op_idx; then
                    func_main
                fi
                ${op_arry[$[$op_idx-1]]}
                exit 0
            fi

            func_$1
            ;;
    esac

    return
}

func_main $@

exit 0

