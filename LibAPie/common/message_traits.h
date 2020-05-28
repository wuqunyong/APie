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

DEFINE_TYPE_TRAIT(HasInsertToDb, InsertToDb)
DEFINE_TYPE_TRAIT(HasDeleteFromDb, DeleteFromDb)
DEFINE_TYPE_TRAIT(HasUpdateToDb, UpdateToDb)
DEFINE_TYPE_TRAIT(HasLoadFromDb, LoadFromDb)

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
typename std::enable_if<HasInsertToDb<T>::value, bool>::type InsertToDb(T& message) {
  return message.InsertToDb();
}


template <typename T>
typename std::enable_if<HasDeleteFromDb<T>::value, bool>::type
DeleteFromDb(T& message) {
  return message.DeleteFromDb();
}


template <typename T>
typename std::enable_if<HasUpdateToDb<T>::value, bool>::type
UpdateToDb(T& message) {
  return message.UpdateToDb();
}


template <typename T>
typename std::enable_if<HasLoadFromDb<T>::value, bool>::type
LoadFromDb(const T& message, const std::string& str) {
  return message.LoadFromDb(str);
}


}  // namespace message

