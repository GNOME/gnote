/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
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


#include <queue>
#include <thread>

#include <glibmm/dispatcher.h>
#include <UnitTest++/UnitTest++.h>

#include "base/monitor.hpp"
#include "synchronization/gvfstransfer.hpp"

using TransferResult = gnote::sync::TransferResult;


SUITE(GvfsTransferTests)
{
  struct TestTransfer
  {
    TestTransfer(const std::function<void(std::function<void()>)> &submit_for_completion, TransferResult initial_value)
      : result(initial_value)
      , submit_for_completion(submit_for_completion)
    {
    }

    void transfer_async(const std::function<void(TransferResult)> &completion, const Glib::RefPtr<Gio::Cancellable> &cancel_op) const
    {
      submit_for_completion([this, completion] { transfer_complete(completion); });
    }

    void transfer_complete(const std::function<void(TransferResult)> &completion) const
    {
      result = TransferResult::SUCCESS;
      completion(result);
    }

    mutable TransferResult result;
    std::function<void(std::function<void()>)> submit_for_completion;
  };


  typedef gnote::sync::GvfsTransfer<std::vector<TestTransfer>, TestTransfer> TestGvfsTransfer;


  TEST(all_files_are_transfered)
  {
    gnote::Monitor monitor;
    std::queue<std::function<void()>> async_queue;
    bool transfers_complete = false;
    auto async_transfer = [&monitor, &async_queue](std::function<void()> completion) {
      gnote::Monitor::Lock guard(monitor);
      async_queue.push(completion);
      monitor.notify_one();
    };

    std::vector<TestTransfer> transfers;
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    TestGvfsTransfer transfer(transfers);

    std::thread tfer_thread([&monitor, &transfer, &transfers_complete] {
      transfer.transfer();
      transfers_complete = true;
      gnote::Monitor::Lock lock(monitor);
      monitor.notify_one();
    });

    while(!transfers_complete) {
      std::function<void()> func;
      {
        gnote::Monitor::Lock guard(monitor);
        while(async_queue.empty() && !transfers_complete) {
          monitor.wait(guard);
        }
        if(!async_queue.empty()) {
          func = async_queue.front();
          async_queue.pop();
        }
      }
      if(func) {
        func();
       }
    }

    tfer_thread.join();
    CHECK(TransferResult::SUCCESS == transfers[0].result);
    CHECK(TransferResult::SUCCESS == transfers[1].result);
  }
}

