// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: pubsub.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_pubsub_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_pubsub_2eproto

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
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_pubsub_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_pubsub_2eproto {
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
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_pubsub_2eproto;
namespace pubsub {
class LOGIC_CMD;
class LOGIC_CMDDefaultTypeInternal;
extern LOGIC_CMDDefaultTypeInternal _LOGIC_CMD_default_instance_;
}  // namespace pubsub
PROTOBUF_NAMESPACE_OPEN
template<> ::pubsub::LOGIC_CMD* Arena::CreateMaybeMessage<::pubsub::LOGIC_CMD>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace pubsub {

// ===================================================================

class LOGIC_CMD :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:pubsub.LOGIC_CMD) */ {
 public:
  LOGIC_CMD();
  virtual ~LOGIC_CMD();

  LOGIC_CMD(const LOGIC_CMD& from);
  LOGIC_CMD(LOGIC_CMD&& from) noexcept
    : LOGIC_CMD() {
    *this = ::std::move(from);
  }

  inline LOGIC_CMD& operator=(const LOGIC_CMD& from) {
    CopyFrom(from);
    return *this;
  }
  inline LOGIC_CMD& operator=(LOGIC_CMD&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const LOGIC_CMD& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const LOGIC_CMD* internal_default_instance() {
    return reinterpret_cast<const LOGIC_CMD*>(
               &_LOGIC_CMD_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(LOGIC_CMD& a, LOGIC_CMD& b) {
    a.Swap(&b);
  }
  inline void Swap(LOGIC_CMD* other) {
    if (other == this) return;
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline LOGIC_CMD* New() const final {
    return CreateMaybeMessage<LOGIC_CMD>(nullptr);
  }

  LOGIC_CMD* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<LOGIC_CMD>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const LOGIC_CMD& from);
  void MergeFrom(const LOGIC_CMD& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(LOGIC_CMD* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "pubsub.LOGIC_CMD";
  }
  private:
  inline ::PROTOBUF_NAMESPACE_ID::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_pubsub_2eproto);
    return ::descriptor_table_pubsub_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kParamsFieldNumber = 2,
    kCmdFieldNumber = 1,
  };
  // repeated string params = 2;
  int params_size() const;
  private:
  int _internal_params_size() const;
  public:
  void clear_params();
  const std::string& params(int index) const;
  std::string* mutable_params(int index);
  void set_params(int index, const std::string& value);
  void set_params(int index, std::string&& value);
  void set_params(int index, const char* value);
  void set_params(int index, const char* value, size_t size);
  std::string* add_params();
  void add_params(const std::string& value);
  void add_params(std::string&& value);
  void add_params(const char* value);
  void add_params(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& params() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_params();
  private:
  const std::string& _internal_params(int index) const;
  std::string* _internal_add_params();
  public:

  // string cmd = 1;
  void clear_cmd();
  const std::string& cmd() const;
  void set_cmd(const std::string& value);
  void set_cmd(std::string&& value);
  void set_cmd(const char* value);
  void set_cmd(const char* value, size_t size);
  std::string* mutable_cmd();
  std::string* release_cmd();
  void set_allocated_cmd(std::string* cmd);
  private:
  const std::string& _internal_cmd() const;
  void _internal_set_cmd(const std::string& value);
  std::string* _internal_mutable_cmd();
  public:

  // @@protoc_insertion_point(class_scope:pubsub.LOGIC_CMD)
 private:
  class _Internal;

  ::PROTOBUF_NAMESPACE_ID::internal::InternalMetadataWithArena _internal_metadata_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> params_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr cmd_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_pubsub_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// LOGIC_CMD

// string cmd = 1;
inline void LOGIC_CMD::clear_cmd() {
  cmd_.ClearToEmptyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}
inline const std::string& LOGIC_CMD::cmd() const {
  // @@protoc_insertion_point(field_get:pubsub.LOGIC_CMD.cmd)
  return _internal_cmd();
}
inline void LOGIC_CMD::set_cmd(const std::string& value) {
  _internal_set_cmd(value);
  // @@protoc_insertion_point(field_set:pubsub.LOGIC_CMD.cmd)
}
inline std::string* LOGIC_CMD::mutable_cmd() {
  // @@protoc_insertion_point(field_mutable:pubsub.LOGIC_CMD.cmd)
  return _internal_mutable_cmd();
}
inline const std::string& LOGIC_CMD::_internal_cmd() const {
  return cmd_.GetNoArena();
}
inline void LOGIC_CMD::_internal_set_cmd(const std::string& value) {
  
  cmd_.SetNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value);
}
inline void LOGIC_CMD::set_cmd(std::string&& value) {
  
  cmd_.SetNoArena(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:pubsub.LOGIC_CMD.cmd)
}
inline void LOGIC_CMD::set_cmd(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  cmd_.SetNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:pubsub.LOGIC_CMD.cmd)
}
inline void LOGIC_CMD::set_cmd(const char* value, size_t size) {
  
  cmd_.SetNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:pubsub.LOGIC_CMD.cmd)
}
inline std::string* LOGIC_CMD::_internal_mutable_cmd() {
  
  return cmd_.MutableNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}
inline std::string* LOGIC_CMD::release_cmd() {
  // @@protoc_insertion_point(field_release:pubsub.LOGIC_CMD.cmd)
  
  return cmd_.ReleaseNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}
inline void LOGIC_CMD::set_allocated_cmd(std::string* cmd) {
  if (cmd != nullptr) {
    
  } else {
    
  }
  cmd_.SetAllocatedNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), cmd);
  // @@protoc_insertion_point(field_set_allocated:pubsub.LOGIC_CMD.cmd)
}

// repeated string params = 2;
inline int LOGIC_CMD::_internal_params_size() const {
  return params_.size();
}
inline int LOGIC_CMD::params_size() const {
  return _internal_params_size();
}
inline void LOGIC_CMD::clear_params() {
  params_.Clear();
}
inline std::string* LOGIC_CMD::add_params() {
  // @@protoc_insertion_point(field_add_mutable:pubsub.LOGIC_CMD.params)
  return _internal_add_params();
}
inline const std::string& LOGIC_CMD::_internal_params(int index) const {
  return params_.Get(index);
}
inline const std::string& LOGIC_CMD::params(int index) const {
  // @@protoc_insertion_point(field_get:pubsub.LOGIC_CMD.params)
  return _internal_params(index);
}
inline std::string* LOGIC_CMD::mutable_params(int index) {
  // @@protoc_insertion_point(field_mutable:pubsub.LOGIC_CMD.params)
  return params_.Mutable(index);
}
inline void LOGIC_CMD::set_params(int index, const std::string& value) {
  // @@protoc_insertion_point(field_set:pubsub.LOGIC_CMD.params)
  params_.Mutable(index)->assign(value);
}
inline void LOGIC_CMD::set_params(int index, std::string&& value) {
  // @@protoc_insertion_point(field_set:pubsub.LOGIC_CMD.params)
  params_.Mutable(index)->assign(std::move(value));
}
inline void LOGIC_CMD::set_params(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  params_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:pubsub.LOGIC_CMD.params)
}
inline void LOGIC_CMD::set_params(int index, const char* value, size_t size) {
  params_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:pubsub.LOGIC_CMD.params)
}
inline std::string* LOGIC_CMD::_internal_add_params() {
  return params_.Add();
}
inline void LOGIC_CMD::add_params(const std::string& value) {
  params_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:pubsub.LOGIC_CMD.params)
}
inline void LOGIC_CMD::add_params(std::string&& value) {
  params_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:pubsub.LOGIC_CMD.params)
}
inline void LOGIC_CMD::add_params(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  params_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:pubsub.LOGIC_CMD.params)
}
inline void LOGIC_CMD::add_params(const char* value, size_t size) {
  params_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:pubsub.LOGIC_CMD.params)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
LOGIC_CMD::params() const {
  // @@protoc_insertion_point(field_list:pubsub.LOGIC_CMD.params)
  return params_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
LOGIC_CMD::mutable_params() {
  // @@protoc_insertion_point(field_mutable_list:pubsub.LOGIC_CMD.params)
  return &params_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace pubsub

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_pubsub_2eproto
