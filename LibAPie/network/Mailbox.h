#pragma once

#include <stddef.h>
#include <deque>
#include <mutex>

#include "../network/Signaler.h"

namespace APie
{
	typedef void (event_cb)(evutil_socket_t, short, void*);

	template<typename T>
    class Mailbox
    {
    public:

        Mailbox():
		  ev(NULL)
		{
			
		}

        ~Mailbox()
		{
			this->release();
		}

		void release()
		{
			if (this->ev != NULL)
			{
				event_free(this->ev);
				this->ev = NULL;
			}
		}

        void registerFd(struct event_base* base, event_cb* tfn, void* arg)
		{
			evutil_socket_t iFd = signaler.getFd();
			this->ev = event_new(base, iFd, EV_READ|EV_PERSIST, tfn, arg);
			event_add(this->ev, NULL);
		}

        void send(const T &cmd)
		{
			sync.lock();
			this->pipe.push_back(cmd);
			sync.unlock();

			signaler.send();
		}

		int size()
		{
			int iCount = 0;
			sync.lock();
			iCount = this->pipe.size();
			sync.unlock();
			return iCount;
		}

        int recv(T &cmd)
		{
			signaler.recv();

			sync.lock();
			if (!this->pipe.empty())
			{
				cmd = this->pipe.front();
				this->pipe.pop_front();

				sync.unlock();
				return 0;
			}

			errno = EAGAIN;
			sync.unlock();
			return -1;
		}
        
    private:

		std::deque<T> pipe;
        Signaler signaler;
		std::mutex sync;

		struct event* ev;

        //  Disable copying of mailbox_t object.
        Mailbox (const Mailbox&);
        const Mailbox &operator = (const Mailbox&);
    };

}

