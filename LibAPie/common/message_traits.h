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
#include <type_traits>

#include "macros.h"
#include "../rpc/client/rpc_client.h"

#include "../mysql_driver/mysql_orm.h"
#include "../mysql_driver/dao_factory.h"


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


template <typename ...Ts>
struct LoadFromDbMultiCallback_
{
	using ReplyCallback = std::function<void(const rpc_msg::STATUS& status, std::tuple<Ts...>& tupleData, std::array<uint32_t, sizeof...(Ts)>& rows)>;
};

template <typename ...Ts>
using LoadFromDbMultiReplyCB = typename LoadFromDbMultiCallback_<Ts...>::ReplyCallback;

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
			typename std::remove_reference<decltype(dbObj)>::type newObj;

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

template<class Tuple, std::size_t N>
struct TupleDynamic {
	static void load(Tuple& t, ::mysql_proxy_msg::MysqlMulitQueryResponse& response, std::map<uint32_t, std::tuple<bool, uint32_t>>& loadResult)
	{
		TupleDynamic<Tuple, N - 1>::load(t, response, loadResult);

		std::tuple<uint32_t, uint32_t> result;

		auto &dbObj = std::get<N - 1>(t);
		auto ptrElems = response.mutable_results(N - 1);
		if (nullptr == ptrElems)
		{
			std::get<0>(result) = false;
			std::get<1>(result) = 0;
			loadResult[N - 1] = result;

			return;
		}

		auto &elmes = *ptrElems;
		bool bResult = dbObj.loadFromPbCheck(elmes);
		std::get<0>(result) = bResult;
		std::get<1>(result) = 0;

		if (bResult)
		{
			std::get<1>(result) = elmes.table().rows_size();
			for (auto& rowData : elmes.table().rows())
			{
				dbObj.loadFromPb(rowData);
				break;
			}
		}

		loadResult[N - 1] = result;
	}
};

template<class Tuple>
struct TupleDynamic<Tuple, 1> {
	static void load(Tuple& t, ::mysql_proxy_msg::MysqlMulitQueryResponse& response, std::map<uint32_t, std::tuple<bool, uint32_t>>& loadResult)
	{
		std::tuple<uint32_t, uint32_t> result;

		auto &dbObj = std::get<0>(t);
		auto ptrElems = response.mutable_results(0);
		if (nullptr == ptrElems)
		{
			std::get<0>(result) = false;
			std::get<1>(result) = 0;
			loadResult[0] = result;

			return;
		}

		auto &elmes = *ptrElems;
		bool bResult = dbObj.loadFromPbCheck(elmes);
		std::get<0>(result) = bResult;
		std::get<1>(result) = 0;

		if (bResult)
		{
			std::get<1>(result) = elmes.table().rows_size();;

			for (auto& rowData : elmes.table().rows())
			{
				dbObj.loadFromPb(rowData);
				break;
			}
		}

		loadResult[0] = result;
	}
};

template <typename... Ts>
typename std::enable_if<std::conjunction_v<std::is_base_of<DeclarativeBase, Ts>...>, bool>::type
Multi_LoadFromDb(LoadFromDbMultiReplyCB<Ts...> cb, ::rpc_msg::CHANNEL server, Ts&... args)
{	
	static_assert((sizeof...(Ts)) > 0, "sizeof...(Ts) must > 0");
	static_assert((sizeof...(Ts)) < 20, "sizeof...(Ts) must < 20");

	constexpr uint32_t tupleSize = sizeof...(Ts);

	mysql_proxy_msg::MysqlMultiQueryRequest queryRequest;

	std::vector<mysql_proxy_msg::MysqlQueryRequest> v;
	(v.push_back(args.generateQuery()), ...);

	for (const auto& elem : v)
	{
		auto ptrAdd = queryRequest.add_requests();
		*ptrAdd = elem;
	}

	auto tupleData = std::make_tuple(args...);
	std::array<uint32_t, tupleSize> tupleRows = {0};

	auto queryCB = [cb, tupleData, tupleRows](const rpc_msg::STATUS& status, const std::string& replyData) mutable
	{
		if (status.code() != ::rpc_msg::CODE_Ok)
		{
			if (cb)
			{
				cb(status, tupleData, tupleRows);
			}
			return;
		}

		rpc_msg::STATUS newStatus;
		newStatus.set_code(::rpc_msg::CODE_Ok);

		::mysql_proxy_msg::MysqlMulitQueryResponse response;
		if (!response.ParseFromString(replyData))
		{
			newStatus.set_code(::rpc_msg::CODE_ParseError);
			if (cb)
			{
				cb(newStatus, tupleData, tupleRows);
			}
			return;
		}

		std::stringstream ss;
		ss << response.ShortDebugString();
		ASYNC_PIE_LOG("mysql_multi_query", PIE_CYCLE_DAY, PIE_DEBUG, ss.str().c_str());


		if (response.results_size() != std::tuple_size<decltype(tupleData)>::value)
		{
			newStatus.set_code(::rpc_msg::CODE_NotMatchedResultError);
			if (cb)
			{
				cb(newStatus, tupleData, tupleRows);
			}
			return;
		}

		std::map<uint32_t, std::tuple<bool, uint32_t>> loadResult;  // value: error,rows
		TupleDynamic<decltype(tupleData), std::tuple_size<decltype(tupleData)>::value>::load(tupleData, response, loadResult);

		for (const auto& elems : loadResult)
		{
			tupleRows[elems.first] = std::get<1>(elems.second);
			if (!std::get<0>(elems.second))
			{
				newStatus.set_code(::rpc_msg::CODE_LoadFromDbError);
			}
		}

		if (cb)
		{
			cb(newStatus, tupleData, tupleRows);
		}
	};
	return APie::RPC::RpcClientSingleton::get().callByRoute(server, ::rpc_msg::RPC_MysqlMultiQuery, queryRequest, queryCB);

	return true;
}


//auto multiCb = [](const rpc_msg::STATUS& status, std::tuple<ModelAccount, ModelAccount, ModelAccount>& tupleData, std::array<uint32_t, 3>& tupleRows) {
//};
//bResult = Multi_LoadFromDb(multiCb, server, accountData, accountData, accountData);

}  // namespace message

