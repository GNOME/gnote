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
      if(intermediate_results.empty()) {
        result = TransferResult::SUCCESS;
      }
      else {
        result = intermediate_results.front();
        intermediate_results.pop();
      }

      completion(result);
    }

    mutable TransferResult result;
    mutable std::queue<TransferResult> intermediate_results;
    std::function<void(std::function<void()>)> submit_for_completion;
  };


  struct TestLimiterNoLimit
  {
    TestLimiterNoLimit()
      : claims(0)
      , releases(0)
    {}

    void claim()
    {
      ++claims;
    }

    void release()
    {
      ++releases;
    }

    int claims;
    int releases;
  };


  struct TestLimiterFixed
  {
    explicit TestLimiterFixed(unsigned max)
      : m_current(0)
      , m_limiter(max)
    {
      s_max = 0;
    }

    void claim()
    {
      m_limiter.claim();
      std::unique_lock lock(m_control);
      if(++m_current > s_max) {
        s_max = m_current;
      }
    }

    void release()
    {
      {
        std::unique_lock lock(m_control);
        --m_current;
      }
      m_limiter.release();
    }

    static unsigned s_max;

    std::mutex m_control;
    std::condition_variable m_total_reached;
    unsigned m_current;
    gnote::sync::TransferLimiterFixed m_limiter;
  };


  template <typename LimiterT = gnote::sync::TransferLimiterNoLimit>
  struct TestGvfsTransfer : gnote::sync::GvfsTransfer<std::vector<TestTransfer>, TestTransfer, LimiterT>
  {
    template <typename... LimiterArgs>
    explicit TestGvfsTransfer(const std::vector<TestTransfer> &transfers, const LimiterArgs... args)
      : gnote::sync::GvfsTransfer<std::vector<TestTransfer>, TestTransfer, LimiterT>(transfers, args...)
    {}

    using gnote::sync::GvfsTransfer<std::vector<TestTransfer>, TestTransfer, LimiterT>::m_limiter;
  };

  struct Fixture
  {
    gnote::Monitor monitor;
    std::queue<std::function<void()>> async_queue;
    bool transfers_complete;
    std::function<void(std::function<void()>)> async_transfer;

    Fixture()
      : async_transfer([this](std::function<void()> completion) { submit_transfer(completion); })
    {
    }

    void submit_transfer(std::function<void()> completion)
    {
      gnote::Monitor::Lock guard(monitor);
      async_queue.push(completion);
      monitor.notify_one();
    };

    template <typename LimiterT>
    void perform_transfers(TestGvfsTransfer<LimiterT> &transfer)
    {
      std::thread tfer_thread([this, &transfer] {
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
    }
  };


  TEST_FIXTURE(Fixture, all_files_are_transfered)
  {
    std::vector<TestTransfer> transfers;
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);

    TestGvfsTransfer transfer(transfers);
    perform_transfers(transfer);

    CHECK(TransferResult::SUCCESS == transfers[0].result);
    CHECK(TransferResult::SUCCESS == transfers[1].result);
  }

  TEST_FIXTURE(Fixture, failures_are_retried)
  {
    std::queue<TransferResult> transfer_results;
    transfer_results.push(TransferResult::FAILURE);
    std::vector<TestTransfer> transfers;
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);

    // transfer that fails once
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.back().intermediate_results = transfer_results;

    // transfer that fials twice
    transfer_results.push(TransferResult::FAILURE);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.back().intermediate_results = transfer_results;

    TestGvfsTransfer transfer(transfers);
    perform_transfers(transfer);
    CHECK(TransferResult::SUCCESS == transfers[0].result);
    CHECK(TransferResult::SUCCESS == transfers[1].result);
    CHECK(transfers[1].intermediate_results.empty());
    CHECK(TransferResult::SUCCESS == transfers[2].result);
    CHECK(transfers[2].intermediate_results.empty());
  }

  TEST_FIXTURE(Fixture, limiter_is_claimed_and_released)
  {
    std::vector<TestTransfer> transfers;
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    TestGvfsTransfer<TestLimiterNoLimit> transfer(transfers);
    perform_transfers(transfer);
    CHECK(TransferResult::SUCCESS == transfers[0].result);
    CHECK_EQUAL(1, transfer.m_limiter.claims);
    CHECK_EQUAL(1, transfer.m_limiter.releases);
  }

  TEST_FIXTURE(Fixture, limiter_is_not_exceeded)
  {
    const unsigned MAX_TRANSFERS = 2;

    std::vector<TestTransfer> transfers;
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    transfers.emplace_back(async_transfer, TransferResult::NOT_STARTED);
    TestGvfsTransfer<TestLimiterFixed> transfer(transfers, MAX_TRANSFERS);
    perform_transfers(transfer);
    CHECK(TransferResult::SUCCESS == transfers[0].result);
    CHECK(TransferResult::SUCCESS == transfers[1].result);
    CHECK(TransferResult::SUCCESS == transfers[2].result);
    CHECK(TransferResult::SUCCESS == transfers[3].result);
    CHECK_EQUAL(MAX_TRANSFERS, TestLimiterFixed::s_max);
  }
}

unsigned SuiteGvfsTransferTests::TestLimiterFixed::s_max = 0;

