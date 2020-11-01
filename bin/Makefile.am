AM_CXXFLAGS = -Wall -std=c++17

# name=@PACKAGE_NAME@

INCLUDES = -I /usr/local/include \
			-I /usr/include/mysql \
			-I /root/yaml-cpp-master/include \
			-I /root/protobuf-3.11.4/src \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp \
			-I $(top_srcdir)/PBMsg \
			-I $(top_srcdir)/LibAPie
			

apie_sources = \
	$(top_srcdir)/PBMsg/common.pb.cc  \
	$(top_srcdir)/PBMsg/login_msg.pb.cc  \
	$(top_srcdir)/PBMsg/mysql_proxy_msg.pb.cc  \
	$(top_srcdir)/PBMsg/opcodes.pb.cc  \
	$(top_srcdir)/PBMsg/pubsub.pb.cc  \
	$(top_srcdir)/PBMsg/route_register.pb.cc  \
	$(top_srcdir)/PBMsg/rpc_msg.pb.cc  \
	$(top_srcdir)/PBMsg/service_discovery.pb.cc  \
	$(top_srcdir)/ThirdParty/http-parser/http_parser.c  \
	$(top_srcdir)/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_writer.cpp  \
	$(top_srcdir)/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_reader.cpp  \
	$(top_srcdir)/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_value.cpp  \
	$(top_srcdir)/LibAPie/api/api_impl.cc  \
	$(top_srcdir)/LibAPie/api/forward_handler.cc  \
	$(top_srcdir)/LibAPie/api/hook.cc  \
	$(top_srcdir)/LibAPie/api/os_sys_calls.cc  \
	$(top_srcdir)/LibAPie/api/pb_handler.cc  \
	$(top_srcdir)/LibAPie/api/pubsub.cc  \
	$(top_srcdir)/LibAPie/common/exception_trap.cpp  \
	$(top_srcdir)/LibAPie/common/file.cc  \
	$(top_srcdir)/LibAPie/common/string_utils.cpp  \
	$(top_srcdir)/LibAPie/common/utility.cc  \
	$(top_srcdir)/LibAPie/crypto/crypto_utility.cc  \
	$(top_srcdir)/LibAPie/event/dispatched_thread.cc  \
	$(top_srcdir)/LibAPie/event/dispatcher_impl.cc  \
	$(top_srcdir)/LibAPie/event/event_impl_base.cc  \
	$(top_srcdir)/LibAPie/event/file_event_impl.cc  \
	$(top_srcdir)/LibAPie/event/libevent.cc  \
	$(top_srcdir)/LibAPie/event/libevent_scheduler.cc  \
	$(top_srcdir)/LibAPie/event/real_time_system.cc  \
	$(top_srcdir)/LibAPie/event/signal_impl.cc  \
	$(top_srcdir)/LibAPie/event/timer_impl.cc  \
	$(top_srcdir)/LibAPie/filesystem/directory.cpp  \
	$(top_srcdir)/LibAPie/http/http_parser_impl.cpp  \
	$(top_srcdir)/LibAPie/http/http_parser_settings.cpp  \
	$(top_srcdir)/LibAPie/http/http_request.cpp  \
	$(top_srcdir)/LibAPie/http/http_request_decoder.cpp  \
	$(top_srcdir)/LibAPie/http/http_response.cpp  \
	$(top_srcdir)/LibAPie/http/http_response_decoder.cpp  \
	$(top_srcdir)/LibAPie/http/http_util.cpp  \
	$(top_srcdir)/LibAPie/http/string_util.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/dao_factory.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_connector.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_field.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_orm.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_table.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/result_set.cpp  \
	$(top_srcdir)/LibAPie/network/address.cc  \
	$(top_srcdir)/LibAPie/network/client_connection.cpp  \
	$(top_srcdir)/LibAPie/network/client_proxy.cpp  \
	$(top_srcdir)/LibAPie/network/command.cpp  \
	$(top_srcdir)/LibAPie/network/ctx.cpp  \
	$(top_srcdir)/LibAPie/network/end_point.cpp  \
	$(top_srcdir)/LibAPie/network/listener_impl.cc  \
	$(top_srcdir)/LibAPie/network/logger.cpp  \
	$(top_srcdir)/LibAPie/network/object.cpp  \
	$(top_srcdir)/LibAPie/network/output_stream.cc  \
	$(top_srcdir)/LibAPie/network/server_connection.cpp  \
	$(top_srcdir)/LibAPie/network/signaler.cpp  \
	$(top_srcdir)/LibAPie/rpc/client/rpc_client.cpp  \
	$(top_srcdir)/LibAPie/rpc/server/rpc_server.cpp  \
	$(top_srcdir)/LibAPie/rpc/init.cpp  \
	$(top_srcdir)/LibAPie/serialization/byte_buffer.cpp  \
	$(top_srcdir)/LibAPie/serialization/protocol_head.cpp


lib_LTLIBRARIES = libapie.la
libapie_la_SOURCES = $(apie_sources)
libapie_la_LIBADD = -lmysqlclient
libapie_la_LDFLAGS = -L/usr/lib64/mysql -L/usr/local/lib64 -L/usr/local/lib -L/usr/local/protobuf/lib -Wl,-rpath,/usr/local/apie/lib -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl


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
