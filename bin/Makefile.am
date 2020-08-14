AM_CXXFLAGS = -Wall -std=c++11

name=@PACKAGE_NAME@

INCLUDES = -I /usr/local/include/ \
			-I /usr/include/mysql/ \
			-I /opt/libevent-2.0.22/include \
			-I $(top_srcdir)/ThirdParty/http-parser \
			-I $(top_srcdir)/ThirdParty/influxdb-cpp
			

apie_sources = \
	$(top_srcdir)/LibAPie/ThirdParty/http-parser/http_parser.c  \
	$(top_srcdir)/LibAPie/ThirdParty/inih/ini.c  \
	$(top_srcdir)/LibAPie/ThirdParty/inih/cpp/INIReader.cpp  \
	$(top_srcdir)/LibAPie/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_writer.cpp  \
	$(top_srcdir)/LibAPie/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_reader.cpp  \
	$(top_srcdir)/LibAPie/ThirdParty/jsoncpp-0.y.z/src/lib_json/json_value.cpp  \
	$(top_srcdir)/LibAPie/api/api_impl.cc  \
	$(top_srcdir)/LibAPie/api/hook.cc  \
	$(top_srcdir)/LibAPie/api/pb_handler.cc  \
	$(top_srcdir)/LibAPie/api/pubsub.cc  \
	$(top_srcdir)/LibAPie/common/exception_trap.cpp  \
	$(top_srcdir)/LibAPie/common/file.cpp  \
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
	$(top_srcdir)/LibAPie/mysql_driver/mysql_connector.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_field.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_orm.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/mysql_table.cpp  \
	$(top_srcdir)/LibAPie/mysql_driver/result_set.cpp  \
	$(top_srcdir)/LibAPie/network/address.cc  \
	$(top_srcdir)/LibAPie/network/client_connection.cpp  \
	$(top_srcdir)/LibAPie/network/client_proxy.cpp  \
	$(top_srcdir)/LibAPie/network/command.cpp  \
	$(top_srcdir)/LibAPie/network/connection.cpp  \
	$(top_srcdir)/LibAPie/network/ctx.cpp  \
	$(top_srcdir)/LibAPie/network/end_point.cpp  \
	$(top_srcdir)/LibAPie/network/listener_impl.cc  \
	$(top_srcdir)/LibAPie/network/logger.cpp  \
	$(top_srcdir)/LibAPie/network/object.cpp  \
	$(top_srcdir)/LibAPie/network/output_stream.cc  \
	$(top_srcdir)/LibAPie/network/signaler.cpp  \
	$(top_srcdir)/LibAPie/rpc/client/rpc_client.cpp  \
	$(top_srcdir)/LibAPie/rpc/server/rpc_server.cpp  \
	$(top_srcdir)/LibAPie/serialization/byte_buffer.cpp  \
	$(top_srcdir)/LibAPie/serialization/protocol_head.cpp



lib_LIBRARIES = libapie.a
apie_a_SOURCES = $(apie_sources)
apie_a_LIBADD = /usr/lib64/mysql/libmysqlclient.a

