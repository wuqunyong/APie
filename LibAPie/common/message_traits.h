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
	using ReplyCallback = std::function<void(rpc_msg::STATUS, T&)>;
}; 

template <typename T>
using LoadFromDbReplyCB = typename LoadFromDbCallback_<T>::ReplyCallback;


template <typename T>
struct LoadFromDbByFilterCallback_
{
	using ReplyCallback = std::function<void(rpc_msg::STATUS, std::vector<T>&)>;
};

template <typename T>
using LoadFromDbByFilterCB = typename LoadFromDbByFilterCallback_<T>::ReplyCallback;

template <typename T>
typename std::enable_if<HasInsertToDb<T>::value, bool>::type 
InsertToDb(T& message) {
  return message.insertToDb();
}


template <typename T>
typename std::enable_if<HasDeleteFromDb<T>::value, bool>::type
DeleteFromDb(T& message) {
  return message.deleteFromDb();
}


template <typename T>
typename std::enable_if<HasUpdateToDb<T>::value, bool>::type
UpdateToDb(T& message) {
  return message.updateToDb();
}


template <typename T>
typename std::enable_if<HasLoadFromDb<T>::value && std::is_base_of<DeclarativeBase, T>::value, bool>::type
LoadFromDb(::rpc_msg::CHANNEL server, T& dbObj, LoadFromDbReplyCB<T> cb)
{

	mysql_proxy_msg::MysqlQueryRequest queryRequest;
	queryRequest = dbObj.generateQuery();

	//::rpc_msg::CHANNEL server;
	//server.set_type(common::EPT_DB_Proxy);
	//server.set_id(1);

	auto queryCB = [dbObj, cb](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			cb(status, dbObj);
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlQueryResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			cb(newStatus, dbObj);
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_query", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		bool bResult = dbObj.loadFromPb(response);
		if (!bResult)
		{
			newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
		}

		cb(newStatus, dbObj);
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
			cb(status, result);
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlQueryResponse response;
		if (!response.ParseFromString(replyData))
		{
			hasError = true;
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			cb(newStatus, result);
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_query", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());

		if (!response.result())
		{
			hasError = true;
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			cb(newStatus, result);
			return;
		}


		uint32_t iRowCount = 0;
		for (auto& rowData : response.table().rows())
		{
			decltype(dbObj) newObj;

			bool bResult = newObj.loadFromPb(rowData);
			if (!bResult)
			{
				hasError = true;
				newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
				cb(newStatus, result);
				return;
			}
			else
			{
				result.push_back(newObj);
			}
		}

		if (!status.has_more())
		{
			cb(newStatus, result);
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRouteWithServerStream(server, ::rpc_msg::RPC_MysqlQueryByFilter, queryRequest, queryCB);
	
	//return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlQueryByFilter, queryRequest, queryCB);
}

}  // namespace message

