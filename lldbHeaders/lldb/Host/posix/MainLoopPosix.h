//===-- MainLoopPosix.h -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef lldb_Host_posix_MainLoopPosix_h_
#define lldb_Host_posix_MainLoopPosix_h_

#include "lldb/Host/MainLoopBase.h"

#include "llvm/ADT/DenseMap.h"

namespace lldb_private {

// Posix implementation of the MainLoopBase class. It can monitor file
// descriptors for
// readability using pselect. In addition to the common base, this class
// provides the ability to
// invoke a given handler when a signal is received.
//
// Since this class is primarily intended to be used for single-threaded
// processing, it does not
// attempt to perform any internal synchronisation and any concurrent accesses
// must be protected
// externally. However, it is perfectly legitimate to have more than one
// instance of this class
// running on separate threads, or even a single thread (with some limitations
// on signal
// monitoring).
// TODO: Add locking if this class is to be used in a multi-threaded context.
class MainLoopPosix : public MainLoopBase {
private:
  class SignalHandle;

public:
  typedef std::unique_ptr<SignalHandle> SignalHandleUP;

  ~MainLoopPosix() override;

  ReadHandleUP RegisterReadObject(const lldb::IOObjectSP &object_sp,
                                  const Callback &callback,
                                  Error &error) override;

  // Listening for signals from multiple MainLoopPosix instances is perfectly
  // safe as long as they
  // don't try to listen for the same signal. The callback function is invoked
  // when the control
  // returns to the Run() function, not when the hander is executed. This means
  // that you can
  // treat the callback as a normal function and perform things which would not
  // be safe in a
  // signal handler. However, since the callback is not invoked synchronously,
  // you cannot use
  // this mechanism to handle SIGSEGV and the like.
  SignalHandleUP RegisterSignal(int signo, const Callback &callback,
                                Error &error);

  Error Run() override;

  // This should only be performed from a callback. Do not attempt to terminate
  // the processing
  // from another thread.
  // TODO: Add synchronization if we want to be terminated from another thread.
  void RequestTermination() override { m_terminate_request = true; }

protected:
  void UnregisterReadObject(IOObject::WaitableHandle handle) override;

  void UnregisterSignal(int signo);

private:
  class SignalHandle {
  public:
    ~SignalHandle() { m_mainloop.UnregisterSignal(m_signo); }

  private:
    SignalHandle(MainLoopPosix &mainloop, int signo)
        : m_mainloop(mainloop), m_signo(signo) {}

    MainLoopPosix &m_mainloop;
    int m_signo;

    friend class MainLoopPosix;
    DISALLOW_COPY_AND_ASSIGN(SignalHandle);
  };

  struct SignalInfo {
    Callback callback;
    struct sigaction old_action;
    bool was_blocked : 1;
  };

  llvm::DenseMap<IOObject::WaitableHandle, Callback> m_read_fds;
  llvm::DenseMap<int, SignalInfo> m_signals;
  bool m_terminate_request : 1;
};

} // namespace lldb_private

#endif // lldb_Host_posix_MainLoopPosix_h_
