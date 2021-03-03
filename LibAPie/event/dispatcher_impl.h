#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <vector>
#include <mutex>
#include <atomic>
#include <map>

#include "../api/api.h"
#include "../common/time.h"
#include "../event/deferred_deletable.h"
#include "../event/dispatcher.h"
#include "../event/libevent.h"
#include "../event/libevent_scheduler.h"

#include "../network/object.hpp"
#include "../network/mailbox.h"
#include "../network/command.h"
#include "../network/server_connection.h"
#include "../network/client_connection.h"
#include "../network/listener.h"

namespace APie {
namespace Event {

/**
 * libevent implementation of Event::Dispatcher.
 */
class DispatcherImpl : public Dispatcher {
public:
  DispatcherImpl(EThreadType type, uint32_t tid);
  ~DispatcherImpl();

  /**
   * @return event_base& the libevent base.
   */
  event_base& base() { return base_scheduler_.base(); }

  void clearDeferredDeleteList() override;
  Network::ListenerPtr createListener(Network::ListenerCbPtr cb, Network::ListenerConfig config) override;
  TimerPtr createTimer(TimerCb cb) override;
  void deferredDelete(DeferredDeletablePtr&& to_delete) override;

  void start() override;
  void exit() override;
  SignalEventPtr listenForSignal(int signal_num, SignalCb cb) override;
  void post(std::function<void()> callback) override;
  void run(void) override;
  void push(Command& cmd) override;

  std::atomic<bool>& terminating() override;

 public:
	static void addConnection(std::shared_ptr<ServerConnection> ptrConnection);
	static std::shared_ptr<ServerConnection> getConnection(uint64_t iSerialNum);
	static void delConnection(uint64_t iSerialNum);

	static void addClientConnection(std::shared_ptr<ClientConnection> ptrConnection);
	static std::shared_ptr<ClientConnection> getClientConnection(uint64_t iSerialNum);
	static void delClientConnection(uint64_t iSerialNum);

	static void clearAllConnection();

	static uint64_t generatorSerialNum();

private:
  void runPostCallbacks();
  void runIntervalCallbacks();
  void handleCommand();

  void handleNewConnect(PassiveConnect *itemPtr);
  void handlePBRequest(PBRequest *itemPtr);
  void handlePBForward(PBForward *itemPtr);
  void handleSendData(SendData *itemPtr);
  void handleSendDataByFlag(SendDataByFlag *itemPtr);

  void handleAsyncLog(LogCmd* ptrCmd);
  void handleMetric(MetricData* ptrCmd);

  void handleRotate(time_t cutTime);

  void handleDial(DialParameters* ptrCmd);
  void handleDialResult(DialResult* ptrCmd);
  void handleSetServerSessionAttr(SetServerSessionAttr* ptrCmd);
  void handleSetClientSessionAttr(SetClientSessionAttr* ptrCmd);
  void handleLogicCmd(LogicCmd* ptrCmd);
  void handleAsyncCallFunctor(LogicAsyncCallFunctor* ptrCmd);
  void handleCloseLocalClient(CloseLocalClient* ptrCmd);
  void handleCloseLocalServer(CloseLocalServer* ptrCmd);
  void handleClientPeerClose(ClientPeerClose* ptrCmd);
  void handleServerPeerClose(ServerPeerClose* ptrCmd);

  void handleLogicStart(uint32_t iThreadId);
  void handleLogicExit(uint32_t iThreadId);
  void handleStopThread(uint32_t iThreadId);

  void registerEndpoint();

  static void processCommand(evutil_socket_t fd, short event, void *arg);

  EThreadType type_;
  uint32_t tid_;
  LibeventScheduler base_scheduler_;
  TimerPtr deferred_delete_timer_;
  TimerPtr post_timer_;
  TimerPtr interval_timer_;

  std::vector<DeferredDeletablePtr> to_delete_1_;
  std::vector<DeferredDeletablePtr> to_delete_2_;
  std::vector<DeferredDeletablePtr>* current_to_delete_;
  std::mutex post_lock_;
  //std::list<std::function<void()>> post_callbacks_ GUARDED_BY(post_lock_);
  std::list<std::function<void()>> post_callbacks_;
  bool deferred_deleting_{};

  APie::Mailbox<Command> mailbox_;
  time_t i_next_check_rotate;
  time_t i_next_metric_time;

  std::atomic<bool> terminating_ = false;

  static std::atomic<uint64_t> serial_num_;
  static std::mutex connecton_sync_;

  //A server side Socket connection
  static std::map<uint64_t, std::shared_ptr<ServerConnection>> connection_map_;

  //A client side Socket connection.(ClientConnection)
  static std::map<uint64_t, std::shared_ptr<ClientConnection>> client_connection_map_;
};

} // namespace Event
} // namespace Envoy
