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
typename std::enable_if<HasLoadFromDb<T>::value, bool>::type LoadFromDb(::rpc_msg::CHANNEL server, T& dbObj, LoadFromDbReplyCB<T> cb)
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


}  // namespace message

