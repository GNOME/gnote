/*
 * gnote
 *
 * Copyright (C) 2025 Aurimas Cernius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MONITOR_HPP_
#define _MONITOR_HPP_

#include <condition_variable>


namespace gnote {

class Monitor
{
public:
  class Lock
  {
  public:
    friend class Monitor;
    explicit Lock(Monitor &monitor)
      : m_lock(monitor.m_lock)
    {}
  private:
    std::unique_lock<std::mutex> m_lock;
  };

  void wait(Lock &lock);
  void notify_one();
  void notify_all();
private:
  std::mutex m_lock;
  std::condition_variable m_cond;
};


class CompletionMonitor
{
public:
  class WaitLock
  {
  public:
    explicit WaitLock(CompletionMonitor &monitor);
    ~WaitLock();
  private:
    CompletionMonitor &m_monitor;
    Monitor::Lock m_lock;
  };

  class NotifyLock
  {
  public:
    explicit NotifyLock(CompletionMonitor &monitor);
    ~NotifyLock();
  private:
    CompletionMonitor &m_monitor;
    Monitor::Lock m_lock;
  };
private:
  void check_monitor();

  Monitor m_monitor;
  bool m_completed{false};
};

}

#endif

