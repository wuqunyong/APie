// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: opcodes.proto

#include "opcodes.pb.h"

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
namespace opcodes {
}  // namespace opcodes
static constexpr ::PROTOBUF_NAMESPACE_ID::Metadata* file_level_metadata_opcodes_2eproto = nullptr;
static const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* file_level_enum_descriptors_opcodes_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_opcodes_2eproto = nullptr;
const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_opcodes_2eproto::offsets[1] = {};
static constexpr ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema* schemas = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::Message* const* file_default_instances = nullptr;

const char descriptor_table_protodef_opcodes_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\ropcodes.proto\022\007opcodes*\217\003\n\tOPCODE_ID\022\013"
  "\n\007OP_None\020\000\022\037\n\033OP_MSG_REQUEST_ADD_INSTAN"
  "CE\020e\022\034\n\030OP_MSG_RESP_ADD_INSTANCE\020f\022\032\n\026OP"
  "_MSG_NOTICE_INSTANCE\020g\022&\n\"OP_DISCOVERY_M"
  "SG_REQUEST_HEARTBEAT\020h\022#\n\037OP_DISCOVERY_M"
  "SG_RESP_HEARTBEAT\020i\022\035\n\030OP_MSG_REQUEST_AD"
  "D_ROUTE\020\311\001\022\032\n\025OP_MSG_RESP_ADD_ROUTE\020\312\001\022#"
  "\n\036OP_ROUTE_MSG_REQUEST_HEARTBEAT\020\313\001\022 \n\033O"
  "P_ROUTE_MSG_RESP_HEARTBEAT\020\314\001\022\023\n\016OP_RPC_"
  "REQUEST\020\255\002\022\024\n\017OP_RPC_RESPONSE\020\256\002\022 \n\033OP_M"
  "SG_REQUEST_CLIENT_LOGIN\020\320\017*\204\005\n\nStatusCod"
  "e\022\t\n\005SC_Ok\020\000\022\033\n\026SC_Discovery_AuthError\020\350"
  "\007\022\037\n\032SC_Discovery_DuplicateNode\020\351\007\022\036\n\031SC"
  "_Discovery_Unregistered\020\352\007\022\032\n\025SC_Route_I"
  "nvalidPoint\020\320\017\022\027\n\022SC_Route_AuthError\020\321\017\022"
  "\032\n\025SC_Route_Unregistered\020\322\017\022\023\n\016SC_Rpc_Ti"
  "meout\020\270\027\022\026\n\021SC_Rpc_RouteEmpty\020\271\027\022!\n\034SC_R"
  "pc_RouteEstablishedEmpty\020\272\027\022!\n\034SC_RPC_Ro"
  "uteSerialNumInvalid\020\273\027\022\032\n\025SC_RPC_RouteSe"
  "ndError\020\274\027\022\034\n\027SC_RPC_NotReceivedReply\020\275\027"
  "\022\023\n\016SC_RPC_NotSend\020\276\027\022$\n\037SC_RPC_InvalidA"
  "rgs_MethodsEmpty\020\277\027\022&\n!SC_ClientProxy_Se"
  "rialNumEqualZero\020\240\037\022)\n$SC_ClientProxy_Se"
  "rialNumNotEqualZero\020\241\037\022\037\n\032SC_ClientProxy"
  "_Established\020\242\037\022\"\n\035SC_ClientProxy_NotEst"
  "ablished\020\243\037\022\034\n\027SC_ClientProxy_BadAlloc\020\244"
  "\037\022\036\n\031SC_ClientProxy_NoIOThread\020\245\037b\006proto"
  "3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_opcodes_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_opcodes_2eproto_sccs[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_opcodes_2eproto_once;
static bool descriptor_table_opcodes_2eproto_initialized = false;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_opcodes_2eproto = {
  &descriptor_table_opcodes_2eproto_initialized, descriptor_table_protodef_opcodes_2eproto, "opcodes.proto", 1081,
  &descriptor_table_opcodes_2eproto_once, descriptor_table_opcodes_2eproto_sccs, descriptor_table_opcodes_2eproto_deps, 0, 0,
  schemas, file_default_instances, TableStruct_opcodes_2eproto::offsets,
  file_level_metadata_opcodes_2eproto, 0, file_level_enum_descriptors_opcodes_2eproto, file_level_service_descriptors_opcodes_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_opcodes_2eproto = (  ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_opcodes_2eproto), true);
namespace opcodes {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* OPCODE_ID_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_opcodes_2eproto);
  return file_level_enum_descriptors_opcodes_2eproto[0];
}
bool OPCODE_ID_IsValid(int value) {
  switch (value) {
    case 0:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 201:
    case 202:
    case 203:
    case 204:
    case 301:
    case 302:
    case 2000:
      return true;
    default:
      return false;
  }
}

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* StatusCode_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_opcodes_2eproto);
  return file_level_enum_descriptors_opcodes_2eproto[1];
}
bool StatusCode_IsValid(int value) {
  switch (value) {
    case 0:
    case 1000:
    case 1001:
    case 1002:
    case 2000:
    case 2001:
    case 2002:
    case 3000:
    case 3001:
    case 3002:
    case 3003:
    case 3004:
    case 3005:
    case 3006:
    case 3007:
    case 4000:
    case 4001:
    case 4002:
    case 4003:
    case 4004:
    case 4005:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace opcodes
PROTOBUF_NAMESPACE_OPEN
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
