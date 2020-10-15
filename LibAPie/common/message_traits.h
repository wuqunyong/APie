/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <string>
#include <cstdint>

#include "macros.h"
#include "../mysql_driver/mysql_orm.h"


namespace APie {

DEFINE_TYPE_TRAIT(HasInsertToDb, insertToDb)
DEFINE_TYPE_TRAIT(HasDeleteFromDb, deleteFromDb)
DEFINE_TYPE_TRAIT(HasUpdateToDb, updateToDb)
DEFINE_TYPE_TRAIT(HasLoadFromDb, loadFromDb)

template <typename T>
class HasDbSerializer {
 public:
  static constexpr bool value =
	  HasInsertToDb<T>::value && HasDeleteFromDb<T>::value &&
	  HasUpdateToDb<T>::value && HasLoadFromDb<T>::value;
};

// avoid potential ODR violation
template <typename T>
constexpr bool HasDbSerializer<T>::value;


template <typename T>
struct LoadFromDbCallback_
{
	using ReplyCallback = std::function<void(rpc_msg::STATUS, T& dbObj, uint32_t iRows)>;
}; 

template <typename T>
using LoadFromDbReplyCB = typename LoadFromDbCallback_<T>::ReplyCallback;


template <typename T>
struct LoadFromDbByFilterCallback_
{
	using ReplyCallback = std::function<void(rpc_msg::STATUS, std::vector<T>& dbObjList)>;
};

template <typename T>
using LoadFromDbByFilterCB = typename LoadFromDbByFilterCallback_<T>::ReplyCallback;



using InsertToDbCB = std::function<void(rpc_msg::STATUS, bool result, uint64_t affectedRows, uint64_t insertId)>;
using UpdateToDbCB = std::function<void(rpc_msg::STATUS, bool result, uint64_t affectedRows)>;
using DeleteFromDbCB = std::function<void(rpc_msg::STATUS, bool result, uint64_t affectedRows)>;

template <typename T>
typename std::enable_if<std::is_base_of<DeclarativeBase, T>::value, bool>::type
InsertToDb(::rpc_msg::CHANNEL server, T& dbObj, InsertToDbCB cb)
{
	dbObj.dirtySet();

	mysql_proxy_msg::MysqlInsertRequest insertRequest = dbObj.generateInsert();
	dbObj.dirtyReset();

	auto insertCB = [cb](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			if (cb)
			{
				cb(status, false, 0, 0);
			}
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlInsertResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);

			if (cb)
			{
				cb(newStatus, false, 0, 0);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_insert", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		if (cb)
		{
			cb(newStatus, response.result(), response.affected_rows(), response.insert_id());
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlInsert, insertRequest, insertCB);
}


template <typename T>
typename std::enable_if<std::is_base_of<DeclarativeBase, T>::value, bool>::type
DeleteFromDb(::rpc_msg::CHANNEL server, T& dbObj, DeleteFromDbCB cb) 
{
	mysql_proxy_msg::MysqlDeleteRequest deleteRequest = dbObj.generateDelete();

	auto deleteCB = [cb](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			if (cb)
			{
				cb(status, false, 0);
			}
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlDeleteResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			if (cb)
			{
				cb(newStatus, false, 0);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_delete", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		if (cb)
		{
			cb(newStatus, response.result(), response.affected_rows());
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlDelete, deleteRequest, deleteCB);
}


template <typename T>
typename std::enable_if<std::is_base_of<DeclarativeBase, T>::value, bool>::type
UpdateToDb(::rpc_msg::CHANNEL server, T& dbObj, UpdateToDbCB cb)
{
	mysql_proxy_msg::MysqlUpdateRequest updateRequest = dbObj.generateUpdate();
	dbObj.dirtyReset();

	if (updateRequest.fields_size() == 0)
	{
		rpc_msg::STATUS status;
		status.set_code(::rpc_msg::CODE_DirtyFlagZero);

		if (cb)
		{
			cb(status, false, 0);
		}
		return false;
	}

	auto updateCB = [cb](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			if (cb)
			{
				cb(status, false, 0);
			}
			
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlUpdateResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			if (cb)
			{
				cb(newStatus, false, 0);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_update", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		if (cb)
		{
			cb(newStatus, response.result(), response.affected_rows());
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlUpdate, updateRequest, updateCB);
}


template <typename T>
typename std::enable_if<HasLoadFromDb<T>::value && std::is_base_of<DeclarativeBase, T>::value, bool>::type
LoadFromDb(::rpc_msg::CHANNEL server, T& dbObj, LoadFromDbReplyCB<T> cb)
{
	mysql_proxy_msg::MysqlQueryRequest queryRequest;
	queryRequest = dbObj.generateQuery();

	auto queryCB = [dbObj, cb](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			if (cb)
			{
				cb(status, dbObj, 0);
			}
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlQueryResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			if (cb)
			{
				cb(newStatus, dbObj, 0);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_query", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		//bool bResult = dbObj.loadFromPb(response);
		//if (!bResult)
		//{
		//	newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
		//}

		bool bResult = dbObj.loadFromPbCheck(response);
		if (!bResult)
		{
			newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
			if (cb)
			{
				cb(newStatus, dbObj, 0);
			}
			return;
		}

		uint32_t iRowCount = response.table().rows_size();
		for (auto& rowData : response.table().rows())
		{
			dbObj.loadFromPb(rowData);
			break;
		}

		if (cb)
		{
			cb(newStatus, dbObj, iRowCount);
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlQuery, queryRequest, queryCB);
}

template <typename T>
typename std::enable_if<HasLoadFromDb<T>::value && std::is_base_of<DeclarativeBase, T>::value, bool>::type
LoadFromDbByFilter(::rpc_msg::CHANNEL server, T& dbObj, LoadFromDbByFilterCB<T> cb)
{
	auto ptrTuple = std::make_shared<std::tuple<std::vector<T>, bool>>();
	std::get<1>(*ptrTuple) = false;

	mysql_proxy_msg::MysqlQueryRequestByFilter queryRequest;
	queryRequest = dbObj.generateQueryByFilter();

	auto queryCB = [dbObj, cb, ptrTuple](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		auto& result = std::get<0>(*ptrTuple);
		auto& hasError = std::get<1>(*ptrTuple);
		if (hasError)
		{
			return;
		}

		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			hasError = true;
			if (cb)
			{
				cb(status, result);
			}
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlQueryResponse response;
		if (!response.ParseFromString(replyData))
		{
			hasError = true;
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			if (cb)
			{
				cb(newStatus, result);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_query", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		bool bResult = dbObj.loadFromPbCheck(response);
		if (!bResult)
		{
			newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
			if (cb)
			{
				cb(newStatus, result);
			}
			return;
		}

		uint32_t iRowCount = 0;
		for (auto& rowData : response.table().rows())
		{
			decltype(dbObj) newObj;

			newObj.loadFromPb(rowData);
			result.push_back(newObj);
		}

		if (!status.has_more())
		{
			if (cb)
			{
				cb(newStatus, result);
			}
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRouteWithServerStream(server, ::rpc_msg::RPC_MysqlQueryByFilter, queryRequest, queryCB);
	
	//return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlQueryByFilter, queryRequest, queryCB);
}

}  // namespace message

