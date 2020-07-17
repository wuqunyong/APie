// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: opcodes.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_opcodes_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_opcodes_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3011000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3011004 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_opcodes_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_opcodes_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_opcodes_2eproto;
PROTOBUF_NAMESPACE_OPEN
PROTOBUF_NAMESPACE_CLOSE
namespace opcodes {

enum OPCODE_ID : int {
  OP_None = 0,
  OP_RPC_REQUEST = 1,
  OP_RPC_RESPONSE = 2,
  OP_MSG_REQUEST_ADD_INSTANCE = 3,
  OP_MSG_RESP_ADD_INSTANCE = 4,
  OP_MSG_NOTICE_INSTANCE = 5,
  OPCODE_ID_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  OPCODE_ID_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool OPCODE_ID_IsValid(int value);
constexpr OPCODE_ID OPCODE_ID_MIN = OP_None;
constexpr OPCODE_ID OPCODE_ID_MAX = OP_MSG_NOTICE_INSTANCE;
constexpr int OPCODE_ID_ARRAYSIZE = OPCODE_ID_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* OPCODE_ID_descriptor();
template<typename T>
inline const std::string& OPCODE_ID_Name(T enum_t_value) {
  static_assert(::std::is_same<T, OPCODE_ID>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function OPCODE_ID_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    OPCODE_ID_descriptor(), enum_t_value);
}
inline bool OPCODE_ID_Parse(
    const std::string& name, OPCODE_ID* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<OPCODE_ID>(
    OPCODE_ID_descriptor(), name, value);
}
enum StatusCode : int {
  SC_Ok = 0,
  SC_Rpc_Timeout = 1000,
  SC_Discovery_AuthError = 2000,
  StatusCode_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  StatusCode_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool StatusCode_IsValid(int value);
constexpr StatusCode StatusCode_MIN = SC_Ok;
constexpr StatusCode StatusCode_MAX = SC_Discovery_AuthError;
constexpr int StatusCode_ARRAYSIZE = StatusCode_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* StatusCode_descriptor();
template<typename T>
inline const std::string& StatusCode_Name(T enum_t_value) {
  static_assert(::std::is_same<T, StatusCode>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function StatusCode_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    StatusCode_descriptor(), enum_t_value);
}
inline bool StatusCode_Parse(
    const std::string& name, StatusCode* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<StatusCode>(
    StatusCode_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace opcodes

PROTOBUF_NAMESPACE_OPEN

template <> struct is_proto_enum< ::opcodes::OPCODE_ID> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::opcodes::OPCODE_ID>() {
  return ::opcodes::OPCODE_ID_descriptor();
}
template <> struct is_proto_enum< ::opcodes::StatusCode> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::opcodes::StatusCode>() {
  return ::opcodes::StatusCode_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_opcodes_2eproto
