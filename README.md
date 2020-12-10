# Welcome to APie!https://github.com/wuqunyong/APie


## 依赖

 - libevent
 - protobuf
 - yaml
 - lz4

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

## 架构图
![架构图](https://github.com/wuqunyong/APie/blob/master/docs/topology.png)

## 框架图
 [nio reactor](http://gee.cs.oswego.edu/dl/cpjslides/nio.pdf)

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


## MysqlORM Demo
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