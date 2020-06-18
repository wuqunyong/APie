#pragma once


#include <array>
#include <cstdint>
#include <string>

#include "../network/address.h"
#include "../network/windows_platform.h"

#include <event2/util.h>



namespace APie {
namespace Network {

std::shared_ptr<sockaddr_in> addressFromFd(evutil_socket_t fd);
std::shared_ptr<sockaddr_in> peerAddressFromFd(evutil_socket_t fd);

std::string makeFriendlyAddress(sockaddr_in addr);

int getInAddr(struct in_addr * dst, const char *address);

} // namespace Network
} // namespace Envoy
