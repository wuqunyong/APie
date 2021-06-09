#pragma once

#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/event.h"

namespace APie
{

    //  This is a cross-platform equivalent to signal_fd. However, as opposed
    //  to signal_fd there can be at most one signal in the signaler at any
    //  given moment. Attempt to send a signal before receiving the previous
    //  one will result in undefined behaviour.

    class Signaler
    {
    public:

        Signaler();
        ~Signaler();

        evutil_socket_t getFd();
        void send();
        void recv();
        
    private:

        //  Creates a pair of filedescriptors that will be used
        //  to pass the signals.
        static int makeFdPair(evutil_socket_t *r, evutil_socket_t *w);

        //  Write & read end of the socketpair.
        evutil_socket_t w;
        evutil_socket_t r;

        //  Disable copying of signaler_t object.
        Signaler(const Signaler&);
        const Signaler &operator=(const Signaler&);
    };

}

