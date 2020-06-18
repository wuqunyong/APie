#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <atomic>

#include "../singleton/threadsafe_singleton.h"
#include "../event/dispatched_thread.h"



namespace APie
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

		uint32_t generatorTId();


		std::shared_ptr<Event::DispatchedThreadImpl> chooseIOThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getLogicThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getLogThread();
		std::shared_ptr<Event::DispatchedThreadImpl> getThreadById(uint32_t id);

    private:
		typedef std::vector<std::shared_ptr<Event::DispatchedThreadImpl>> ThreadVec;
		std::map<Event::EThreadType, ThreadVec> thread_;

		std::shared_ptr<Event::DispatchedThreadImpl> logic_thread_;
		std::shared_ptr<Event::DispatchedThreadImpl> log_thread_;
		std::map<uint32_t, std::shared_ptr<Event::DispatchedThreadImpl>> thread_id_;

		std::atomic<uint32_t> tid_ = 0;

        Ctx (const Ctx&) = delete;
        const Ctx &operator = (const Ctx&) = delete;
    };
    

	typedef APie::ThreadSafeSingleton<Ctx> CtxSingleton;

	//usage: Envoy::CtxSingleton::get();
}

