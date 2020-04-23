#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "../singleton/threadsafe_singleton.h"
#include "../event/dispatched_thread.h"



namespace Envoy
{
    //  Context object encapsulates all the global state associated with
    //  the library.
    
    class Ctx
    {
    public:
		Ctx();
		~Ctx();

		void init();
		void start();
		void destroy();

		std::shared_ptr<Event::DispatchedThreadImpl> chooseIOThread();

    private:
		typedef std::vector<std::shared_ptr<Event::DispatchedThreadImpl>> ThreadVec;
		std::map<Event::EThreadType, ThreadVec> thread_;

		std::shared_ptr<Event::DispatchedThreadImpl> logic_thread_;

        Ctx (const Ctx&) = delete;
        const Ctx &operator = (const Ctx&) = delete;
    };
    

	typedef Envoy::ThreadSafeSingleton<Ctx> CtxSingleton;

	//usage: Envoy::CtxSingleton::get();
}

