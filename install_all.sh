#!/bin/bash -e
#
#  http://free-random.cn
#  

echo
echo This script will install APie and all its dependencies.
echo It has been tested on CentOS 6.8 32位.
echo

set -e   # Exit immediately if a simple command exits with a non-zero status.
set -x   # activate debugging from here

BASE_DIR="$(cd "$(dirname -- "$0")" ; pwd)"
echo "CurDir: $BASE_DIR"

# echo
# echo "List installed packages"
# echo
# yum list installed


# install deps
install_deps() {
    echo "Installing required packages"
    echo
    yum install -y \
        mysql-devel \
        mysql-server \
        lrzsz \
        curl-devel \
        openssl \
        openssl-devel \
        readline-devel \
        pcre \
        pcre-devel \
        zlib \
        zlib-devel \
        libevent \
        libevent-devel \
        gcc \
        gcc-c++ \
        rpm-build \
        automake \
        libtool
}

show_character(){
    mysql <<EOF
    SHOW VARIABLES LIKE '%character%';
EOF
}

edit_mysql() {
    echo
    show_character
    echo

    find / -name my.cnf

    sed -i '$a\[client]\
default-character-set = utf8\
\
[mysqld]\
default-storage-engine = INNODB\
character-set-server = utf8\
collation-server = utf8_general_ci' /etc/my.cnf

    echo
    show_character  
    echo
}

start_mysql() {
    chkconfig --list | grep mysqld
    chkconfig mysqld on
    chkconfig --list | grep mysqld

    echo `service mysqld status`
    service mysqld start
    echo `service mysqld status`
}

restart_mysql() {
    service mysqld restart
}

close_iptables() {
    chkconfig iptables off  #on
    service iptables stop   #start
}

install_lua() {
    cd "${BASE_DIR}/../Downloads"
    pwd
    tar zxvf lua-5.1.5.tar.gz
    cd lua-5.1.5
    make linux test
    make install
}

install_libevent() {
    cd "${BASE_DIR}/../Downloads"
    pwd
    tar zxvf libevent-2.0.22-stable.tar.gz 
    cd libevent-2.0.22-stable
    ./configure --prefix=/opt/libevent-2.0.22
    make 
    make install

    ln -s /usr/local/lib/libevent-2.0.so.5 /usr/lib/libevent-2.0.so.5
}

install_leveldb() {
    cd "${BASE_DIR}/../Downloads"
    pwd
    cp -f leveldb-1.20.zip /opt
    cd /opt/
    pwd
    unzip leveldb-1.20.zip
    cd leveldb-1.20
    make
}

install_nginx() {
    cd "${BASE_DIR}/../Downloads"
    pwd
    tar zxvf nginx-1.10.1.tar.gz 
    cd nginx-1.10.1
    ./configure --with-stream
    make
    make install
}

build_apie() {
    cd "${BASE_DIR}/Builds"
    pwd
    make
}


install_mysql() {
    yum install -y mysql-devel mysql mysql-server 

    # use mysql;
    # grant all privileges  on *.* to root@'%' identified by "password";
    # flush privileges;


    # Access denied for user 'root'@'localhost' (using password: YES)
    # UPDATE user SET Password=PASSWORD('newpassword') where USER='root';
    # FLUSH PRIVILEGES;
}

install_keepalived() {
    yum install -y keepalived

    # tcpdump -vvv -n -i eth0 vrrp
    # 11:30:48.441511 IP (tos 0xc0, ttl 255, id 69, offset 0, flags [none], proto VRRP (112), length 48)
    #     10.8.226.5 > 224.0.0.18: VRRPv2, Advertisement, vrid 51, prio 99, authtype simple, intvl 1s, length 28, addrs(3): 192.168.200.16,192.168.200.17,192.168.200.18 auth "1111^@^@^@^@"

    # cat /var/log/messages | grep Keepalived
}

# now the fun part

#install_deps
#start_mysql
#edit_mysql
#restart_mysql
#install_lua

#install_nginx
#build_apie

install_libevent
install_leveldb


echo
echo 'All done!'
echo

# mysql -h 127.0.0.1 -P 3306 -u root -p123456
# whereis libevent
# LD_DEBUG=libs ./Pie
# ldd ./Pie

