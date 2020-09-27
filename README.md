# Welcome to APie!


## 依赖

 - libevent
 - protobuf
 - yaml

## 安装依赖
```shell
yum install -y mysql-devel mysql-server lrzsz curl-devel openssl openssl-devel readline-devel pcre pcre-devel zlib zlib-devel libevent libevent-devel gcc gcc-c++ rpm-build automake libtool
```
### 安装cmake
```shell
tar -zxvf cmake-3.18.1-Linux-x86_64.tar.gz
cd cmake-3.18.1-Linux-x86_64
```
### 升级GCC
```shell
yum install centos-release-scl -y
yum install devtoolset-8 -y
scl enable devtoolset-8 bash
gcc --version
```
### 安装protobuf
```shell
unzip protobuf-3.11.4.zip
cd protobuf-3.11.4
./autogen.sh
./configure --prefix=/usr/local/protobuf/
make
make check
make install
ldconfig
```
### 安装yaml
```shell
unzip yaml-cpp-master.zip
cd yaml-cpp-master
mkdir build
cd build
cmake ..
make
make test
make install
```

## 编译
```shell
./bootstrap.sh
./configure
make rpm
```

## Demo
```cpp
#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"

#include "service_init.h"

std::tuple<uint32_t, std::string> initHook()
{
	//TODO
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> startHook()
{
	//TODO
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

std::tuple<uint32_t, std::string> exitHook()
{
	//TODO
	return std::make_tuple(Hook::HookResult::HR_Ok, "");
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fatalExit("usage: exe <ConfFile>");
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Init, initHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Start, startHook);
	APie::Hook::HookRegistrySingleton::get().appendHook(APie::Hook::HookPoint::HP_Exit, exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();

    return 0;
}
```