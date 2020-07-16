protoc login_msg.proto --cpp_out=./
protoc mysql_proxy_msg.proto --cpp_out=./
protoc pubsub.proto --cpp_out=./

protoc rpc_msg.proto --cpp_out=./
protoc opcodes.proto --cpp_out=./
protoc service_discovery.proto --cpp_out=./

pause