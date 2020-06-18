#include "../network/address.h"


#include <array>
#include <cstdint>
#include <string>
#include <assert.h>


namespace APie {
namespace Network {

std::shared_ptr<sockaddr_in> addressFromSockAddr(const sockaddr_storage& ss, socklen_t ss_len)
{
	switch (ss.ss_family) {
	case AF_INET: {
		assert(ss_len == 0 || ss_len == sizeof(sockaddr_in));
		const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(&ss);
		assert(AF_INET == sin->sin_family);
		return std::make_shared<sockaddr_in>(*sin);
	}
	}

	return nullptr;
}

std::shared_ptr<sockaddr_in> addressFromFd(evutil_socket_t fd) {
  sockaddr_storage ss;
  socklen_t ss_len = sizeof ss;
  int rc = ::getsockname(fd, reinterpret_cast<sockaddr*>(&ss), &ss_len);
  if (rc != 0) {
	  return nullptr;
  }

  return addressFromSockAddr(ss, ss_len);
}

std::shared_ptr<sockaddr_in> peerAddressFromFd(evutil_socket_t fd) {
  sockaddr_storage ss;
  socklen_t ss_len = sizeof ss;
  const int rc = ::getpeername(fd, reinterpret_cast<sockaddr*>(&ss), &ss_len);
  if (rc != 0) {
    return nullptr;
  }

  return addressFromSockAddr(ss, ss_len);
}


std::string makeFriendlyAddress(sockaddr_in addr) {
	char str[INET_ADDRSTRLEN];
	const char* ptr = inet_ntop(AF_INET, &addr.sin_addr, str, INET_ADDRSTRLEN);
	if (ptr == nullptr)
	{
		return "";
	}

	return ptr;
}

int getInAddr(struct in_addr * dst, const char *address)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // Only IPv4 address
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* ai_list;
	if (getaddrinfo(address, NULL, &hints, &ai_list) != 0)
	{
		dst->s_addr = INADDR_NONE;
		return -1;
	}

	/* Return sin_addr for the first address returned */
	struct sockaddr_in* sin = (struct sockaddr_in*)ai_list->ai_addr;
	memcpy(dst, &sin->sin_addr, sizeof(struct in_addr));

	freeaddrinfo(ai_list);
	return 0;
}

} // namespace Network
} // namespace Envoy
