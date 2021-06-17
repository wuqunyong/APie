AM_CXXFLAGS = -Wall -std=c++17

# name=@PACKAGE_NAME@

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /usr/local/protobuf/include \
			-I $(top_srcdir)/third_party/astar \
			-I $(top_srcdir)/third_party/cpp_redis/includes \
			-I $(top_srcdir)/third_party/cpp_redis/tacopie/includes \
			-I $(top_srcdir)/third_party/http-parser \
			-I $(top_srcdir)/third_party/JPSPlusGoalBounding \
			-I $(top_srcdir)/third_party/jsoncpp/include \
			-I $(top_srcdir)/third_party/tinyxml2 \
			-I $(top_srcdir)/third_party/influxdb-cpp \
			-I $(top_srcdir)/pb_msg \
			-I $(top_srcdir)/libapie
			

apie_sources = \
	$(top_srcdir)/pb_msg/core/common.pb.cc  \
	$(top_srcdir)/pb_msg/core/mysql_proxy_msg.pb.cc  \
	$(top_srcdir)/pb_msg/core/nats_msg.pb.cc  \
	$(top_srcdir)/pb_msg/core/opcodes.pb.cc  \
	$(top_srcdir)/pb_msg/core/pubsub.pb.cc  \
	$(top_srcdir)/pb_msg/core/route_register.pb.cc  \
	$(top_srcdir)/pb_msg/core/rpc_msg.pb.cc  \
	$(top_srcdir)/pb_msg/core/service_discovery.pb.cc  \
	$(top_srcdir)/third_party/http-parser/http_parser.c  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/BucketPriorityQueue.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/DijkstraFloodfill.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/Entry.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/FastStack.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/FPUtil.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/GenericHeap.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/JPSPlus.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/Map.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/PrecomputeMap.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/ScenarioLoader.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/SimpleUnsortedPriorityQueue.cpp  \
	$(top_srcdir)/third_party/JPSPlusGoalBounding/UnsortedPriorityQueue.cpp  \
	$(top_srcdir)/third_party/jsoncpp/src/lib_json/json_writer.cpp  \
	$(top_srcdir)/third_party/jsoncpp/src/lib_json/json_reader.cpp  \
	$(top_srcdir)/third_party/jsoncpp/src/lib_json/json_value.cpp  \
	$(top_srcdir)/third_party/tinyxml2/tinyxml2.cpp  \
	$(top_srcdir)/libapie/api/api_impl.cc  \
	$(top_srcdir)/libapie/api/forward_handler.cc  \
	$(top_srcdir)/libapie/api/hook.cc  \
	$(top_srcdir)/libapie/api/logiccmd_handler.cc  \
	$(top_srcdir)/libapie/api/os_sys_calls.cc  \
	$(top_srcdir)/libapie/api/pb_handler.cc  \
	$(top_srcdir)/libapie/api/pubsub.cc  \
	$(top_srcdir)/libapie/common/exception_trap.cpp  \
	$(top_srcdir)/libapie/common/file.cc  \
	$(top_srcdir)/libapie/common/graphics_utility.cpp  \
	$(top_srcdir)/libapie/common/string_utils.cpp  \
	$(top_srcdir)/libapie/common/utility.cc  \
	$(top_srcdir)/libapie/compressor/lz4_compressor_impl.cc  \
	$(top_srcdir)/libapie/crypto/crypto_utility.cc  \
	$(top_srcdir)/libapie/decompressor/lz4_decompressor_impl.cc  \
	$(top_srcdir)/libapie/event/dispatched_thread.cc  \
	$(top_srcdir)/libapie/event/dispatcher_impl.cc  \
	$(top_srcdir)/libapie/event/event_impl_base.cc  \
	$(top_srcdir)/libapie/event/file_event_impl.cc  \
	$(top_srcdir)/libapie/event/libevent.cc  \
	$(top_srcdir)/libapie/event/libevent_scheduler.cc  \
	$(top_srcdir)/libapie/event/nats_proxy.cc  \
	$(top_srcdir)/libapie/event/real_time_system.cc  \
	$(top_srcdir)/libapie/event/signal_impl.cc  \
	$(top_srcdir)/libapie/event/timer_impl.cc  \
	$(top_srcdir)/libapie/filesystem/directory.cpp  \
	$(top_srcdir)/libapie/http/http_parser_impl.cpp  \
	$(top_srcdir)/libapie/http/http_parser_settings.cpp  \
	$(top_srcdir)/libapie/http/http_request.cpp  \
	$(top_srcdir)/libapie/http/http_request_decoder.cpp  \
	$(top_srcdir)/libapie/http/http_response.cpp  \
	$(top_srcdir)/libapie/http/http_response_decoder.cpp  \
	$(top_srcdir)/libapie/http/http_util.cpp  \
	$(top_srcdir)/libapie/http/string_util.cpp  \
	$(top_srcdir)/libapie/mysql_driver/dao_factory.cpp  \
	$(top_srcdir)/libapie/mysql_driver/mysql_connector.cpp  \
	$(top_srcdir)/libapie/mysql_driver/mysql_field.cpp  \
	$(top_srcdir)/libapie/mysql_driver/mysql_orm.cpp  \
	$(top_srcdir)/libapie/mysql_driver/mysql_table.cpp  \
	$(top_srcdir)/libapie/mysql_driver/result_set.cpp  \
	$(top_srcdir)/libapie/network/address.cc  \
	$(top_srcdir)/libapie/network/client_connection.cpp  \
	$(top_srcdir)/libapie/network/client_proxy.cpp  \
	$(top_srcdir)/libapie/network/command.cpp  \
	$(top_srcdir)/libapie/network/ctx.cpp  \
	$(top_srcdir)/libapie/network/end_point.cpp  \
	$(top_srcdir)/libapie/network/listener_impl.cc  \
	$(top_srcdir)/libapie/network/logger.cpp  \
	$(top_srcdir)/libapie/network/object.cpp  \
	$(top_srcdir)/libapie/network/output_stream.cc  \
	$(top_srcdir)/libapie/network/server_connection.cpp  \
	$(top_srcdir)/libapie/network/signaler.cpp  \
	$(top_srcdir)/libapie/redis_driver/redis_client.cpp  \
	$(top_srcdir)/libapie/rpc/client/rpc_client.cpp  \
	$(top_srcdir)/libapie/rpc/server/rpc_server.cpp  \
	$(top_srcdir)/libapie/rpc/init.cpp  \
	$(top_srcdir)/libapie/serialization/byte_buffer.cpp  \
	$(top_srcdir)/libapie/serialization/protocol_head.cpp


lib_LTLIBRARIES = libapie.la
libapie_la_SOURCES = $(apie_sources)
libapie_la_LIBADD = -lmysqlclient
libapie_la_LDFLAGS = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -lcpp_redis -ltacopie -lnats


# apie_module_ldflags = -L/usr/lib64/mysql/ -L/root/protobuf-3.11.4/build -L/root/yaml-cpp-master/build -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl

# route_proxy_sources = \
# 	$(top_srcdir)/RouteProxy/main.cpp \
# 	$(top_srcdir)/RouteProxy/service_init.cpp \
# 	$(top_srcdir)/RouteProxy/logic/route_proxy.cpp 

# scene_server_sources = \
# 	$(top_srcdir)/SceneServer/main.cpp \
# 	$(top_srcdir)/SceneServer/service_init.cpp \
# 	$(top_srcdir)/SceneServer/logic/scene_mgr.cpp 

# service_registry_sources = \
# 	$(top_srcdir)/ServiceRegistry/main.cpp \
# 	$(top_srcdir)/ServiceRegistry/service_init.cpp \
# 	$(top_srcdir)/ServiceRegistry/logic/service_registry.cpp 


# bin_PROGRAMS = route_proxy scene_server service_registry

# route_proxy_SOURCES = $(route_proxy_sources)
# route_proxy_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
# route_proxy_LDFLAGS = $(apie_module_ldflags)

# scene_server_SOURCES = $(scene_server_sources)
# scene_server_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
# scene_server_LDFLAGS = $(apie_module_ldflags)

# service_registry_SOURCES = $(service_registry_sources)
# service_registry_LDADD = -lapie -lmysqlclient -lprotobuf -lyaml-cpp
# service_registry_LDFLAGS = $(apie_module_ldflags)
