[# Welcome to APie!](https://github.com/wuqunyong/APie)

# CentOS 7 x64安装
## 依赖

 - libevent
 - protobuf
 - yaml
 - lz4
 - [cpp_redis](https://github.com/cpp-redis/cpp_redis)

## 安装依赖
```shell
yum install -y mysql-devel mysql-server lrzsz curl-devel openssl openssl-devel readline-devel pcre pcre-devel zlib zlib-devel libevent libevent-devel gcc gcc-c++ rpm-build automake libtool lz4-devel
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
## 安装git
```
yum install -y git
git --version
```
## 安装cpp_redis
```
# Clone the project
git clone https://github.com/cpp-redis/cpp_redis.git
# Go inside the project directory
cd cpp_redis
# Get tacopie submodule
git submodule init && git submodule update
# Create a build directory and move into it
mkdir build && cd build
# Generate the Makefile using CMake
cmake .. -DCMAKE_BUILD_TYPE=Release
# Build the library
make
# Install the library
make install
```
## 编译
```shell
./bootstrap.sh
./configure
make rpm
```

## 生成的rpm包
apie-1.0.0-1.x86_64.rpm 
apie-debuginfo-1.0.0-1.x86_64.rpm

## 安装rpm包
```shell
rpm -ivh --nodeps apie-1.0.0-1.x86_64.rpm
```

## 服务操作
### 状态查看
```shell
service status apie
```
### 启动
```shell
service start apie
```
### 关闭
```shell
service stop apie
```

## CentOS7 x64安装MySQL
[# How To Install MySQL on CentOS 7](https://www.digitalocean.com/community/tutorials/how-to-install-mysql-on-centos-7)
In a web browser, visit:
```
https://dev.mysql.com/downloads/repo/yum/
```
###  Step 1 — Installing MySQL
```
wget https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm
md5sum mysql57-community-release-el7-9.noarch.rpm
rpm -ivh mysql57-community-release-el7-9.noarch.rpm
yum install mysql-server
```

### Step 2 — Starting MySQL
```
systemctl start mysqld
systemctl status mysqld
grep 'temporary password' /var/log/mysqld.log
```

### Step 3 — Configuring MySQL
```
mysql_secure_installation
```
### Step 4 — Testing MySQL
```
mysqladmin -u root -p version
```

# Windows安装

# 架构图
![架构图](https://github.com/wuqunyong/APie/blob/master/docs/topology.png)

# 框架图
 [nio reactor](http://gee.cs.oswego.edu/dl/cpjslides/nio.pdf)

# Demo
```cpp
#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

#include "apie.h"

std::tuple<uint32_t, std::string> initHook()
{
	return //TODO;
}

std::tuple<uint32_t, std::string> startHook()
{
	return //TODO;
}

std::tuple<uint32_t, std::string> readyHook()
{
	return //TODO;
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
		PANIC_ABORT("usage: exe <ConfFile>, Expected: %d, got: %d", 2, argc);
	}

	std::string configFile = argv[1];

	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Init, APie::initHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Start, APie::startHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Ready, APie::readyHook);
	APie::Hook::HookRegistrySingleton::get().registerHook(APie::Hook::HookPoint::HP_Exit, APie::exitHook);

	APie::CtxSingleton::get().init(configFile);
	APie::CtxSingleton::get().start();
	APie::CtxSingleton::get().waitForShutdown();

    return 0;
}
```


# MysqlORM Demo
```cpp
namespace APie {

	class ModelUser : public DeclarativeBase {
	public:
		PACKED_STRUCT(struct db_fields {
			uint64_t user_id;
			uint64_t game_id = 1;
			uint32_t level = 2;
			int64_t register_time = 1;
			int64_t login_time = 2;
			int64_t offline_time = 3;
			std::string name = "hello";
			std::string role_info;
		});

		virtual void* layoutAddress() override
		{
			return &fields;
		}

		virtual std::vector<uint32_t> layoutOffset() override
		{
			std::vector<uint32_t> layout = {
				offsetof(db_fields, user_id),
				offsetof(db_fields, game_id),
				offsetof(db_fields, level),
				offsetof(db_fields, register_time),
				offsetof(db_fields, login_time),
				offsetof(db_fields, offline_time),
				offsetof(db_fields, name),
				offsetof(db_fields, role_info),
			};

			return layout;
		}

		virtual std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layoutType() override
		{
			std::vector<std::set<MysqlField::DB_FIELD_TYPE>> layout = {
				get_field_type(fields.user_id),
				get_field_type(fields.game_id),
				get_field_type(fields.level),
				get_field_type(fields.register_time),
				get_field_type(fields.login_time),
				get_field_type(fields.offline_time),
				get_field_type(fields.name),
				get_field_type(fields.role_info),
			};

			return layout;
		}

		static std::shared_ptr<DeclarativeBase> createMethod()
		{
			return std::make_shared<ModelUser>();
		}

		static std::string getFactoryName() 
		{ 
			return "role_base"; 
		}

	public:
		db_fields fields;
	};


}


///////////////////////////////

	if (command.cmd() == "load_from_db")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);

		ModelUser user;
		user.fields.user_id = userId;

		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, ModelUser user, uint32_t iRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		LoadFromDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "load_from_db_by_filter")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t gameId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.game_id = gameId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
		user.markFilter({ 1, 2 });

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, std::vector<ModelUser>& userList) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		LoadFromDbByFilter<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_insert_orm")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.user_id = userId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows, uint64_t insertId) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		InsertToDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_update_orm")
	{
		if (command.params_size() < 2)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);
		uint32_t level = std::stoull(command.params()[1]);


		ModelUser user;
		user.fields.user_id = userId;
		user.fields.level = level;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());
		user.markDirty({ 2 });

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		UpdateToDb<ModelUser>(server, user, cb);
	}
	else if (command.cmd() == "mysql_delete_orm")
	{
		if (command.params_size() < 1)
		{
			return;
		}

		uint64_t userId = std::stoull(command.params()[0]);

		ModelUser user;
		user.fields.user_id = userId;
		bool bResult = user.bindTable(DeclarativeBase::DBType::DBT_Role, ModelUser::getFactoryName());

		::rpc_msg::CHANNEL server;
		server.set_type(common::EPT_DB_Proxy);
		server.set_id(1);

		auto cb = [](rpc_msg::STATUS status, bool result, uint64_t affectedRows) {
			if (status.code() != ::rpc_msg::CODE_Ok)
			{
				return;
			}
		};
		DeleteFromDb<ModelUser>(server, user, cb);
	}

```

# 协议注册

 - GatewayServer(直连)
```
 	Api::PBHandler& serverPB = Api::OpcodeHandlerSingleton::get().server;
	serverPB.setDefaultFunc(GatewayMgr::handleDefaultOpcodes);
	serverPB.bind(::opcodes::OP_MSG_REQUEST_CLIENT_LOGIN, GatewayMgr::handleRequestClientLogin, ::login_msg::MSG_REQUEST_CLIENT_LOGIN::default_instance());
```
- SceneServer(非直连，默认转发目的)
```
	auto& forwardHandler = APie::Api::ForwardHandlerSingleton::get();
	forwardHandler.server.bind(::opcodes::OP_MSG_REQUEST_ECHO, SceneMgr::Forward_handlEcho, ::login_msg::MSG_REQUEST_ECHO::default_instance());
```
- 其他
通过RPC转发