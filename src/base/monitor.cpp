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

#include <stdexcept>

#include "debug.hpp"
#include "monitor.hpp"


namespace gnote {

void Monitor::wait(Lock &lock)
{
  m_cond.wait(lock.m_lock);
}

void Monitor::notify_one()
{
  m_cond.notify_one();
}

void Monitor::notify_all()
{
  m_cond.notify_all();
}


CompletionMonitor::WaitLock::WaitLock(CompletionMonitor &monitor)
  : m_monitor(monitor)
  , m_lock(monitor.m_monitor)
{
}

CompletionMonitor::WaitLock::~WaitLock()
{
  while(!m_monitor.m_completed) {
    m_monitor.m_monitor.wait(m_lock);
  }
}

CompletionMonitor::NotifyLock::NotifyLock(CompletionMonitor &monitor)
  : m_monitor(monitor)
  , m_lock(monitor.m_monitor)
{
  monitor.check_monitor();
}

CompletionMonitor::NotifyLock::~NotifyLock()
{
  m_monitor.m_completed = true;
  m_monitor.m_monitor.notify_all();
}

void CompletionMonitor::check_monitor()
{
  DBG_ASSERT(!m_completed, "CompletionMonitor already completed");
  if(m_completed) {
    throw std::logic_error("CompletionMonitor already completed");
  }
}

}

