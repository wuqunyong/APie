#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <msgpack.hpp>

struct your_type {
    int a;
    int b;
    std::string c;
    MSGPACK_DEFINE(a, b, c);
};

bool operator==(your_type const& lhs, your_type const& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b;
}

int main() {
    // packing
    std::stringstream ss;
    std::vector<std::map<std::string, your_type>> v 
    { 
        { 
            { "key1", {1,2,"a"} },
            { "key2", {3,4,"b"} }
        },
        {
            {"key3", {5, 6,"c"} } 
        }
    };
    msgpack::pack(ss, v);
    
    // unpacking
    msgpack::object_handle oh = msgpack::unpack(ss.str().data(), ss.str().size());
    msgpack::object const& obj = oh.get();
    std::cout << "object: " << obj << std::endl;

    // converting
    auto v2 = obj.as<std::vector<std::map<std::string, your_type>>>();
    if (v == v2)
    {
    std::cout << "v == v2 " << std::endl;
    }
    else
    {
    std::cout << "v ！= v2 " << std::endl;
    }

    assert(v == v2);
}
