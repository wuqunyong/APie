// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: common.proto

#include "common.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
namespace common {
}  // namespace common
static constexpr ::PROTOBUF_NAMESPACE_ID::Metadata* file_level_metadata_common_2eproto = nullptr;
static const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* file_level_enum_descriptors_common_2eproto[1];
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_common_2eproto = nullptr;
const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_common_2eproto::offsets[1] = {};
static constexpr ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema* schemas = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::Message* const* file_default_instances = nullptr;

const char descriptor_table_protodef_common_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\014common.proto\022\006common*\303\001\n\014EndPointType\022"
  "\014\n\010EPT_None\020\000\022\030\n\024EPT_Service_Registry\020\001\022"
  "\023\n\017EPT_Route_Proxy\020\002\022\024\n\020EPT_Scene_Server"
  "\020\003\022\026\n\022EPT_Gateway_Server\020\004\022\020\n\014EPT_DB_Pro"
  "xy\020\005\022\024\n\020EPT_Login_Server\020\006\022\023\n\017EPT_Test_C"
  "lient\020\007\022\013\n\007EPT_Max\020\010b\006proto3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_common_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_common_2eproto_sccs[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_common_2eproto_once;
static bool descriptor_table_common_2eproto_initialized = false;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_common_2eproto = {
  &descriptor_table_common_2eproto_initialized, descriptor_table_protodef_common_2eproto, "common.proto", 228,
  &descriptor_table_common_2eproto_once, descriptor_table_common_2eproto_sccs, descriptor_table_common_2eproto_deps, 0, 0,
  schemas, file_default_instances, TableStruct_common_2eproto::offsets,
  file_level_metadata_common_2eproto, 0, file_level_enum_descriptors_common_2eproto, file_level_service_descriptors_common_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_common_2eproto = (  ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_common_2eproto), true);
namespace common {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* EndPointType_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_common_2eproto);
  return file_level_enum_descriptors_common_2eproto[0];
}
bool EndPointType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace common
PROTOBUF_NAMESPACE_OPEN
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
