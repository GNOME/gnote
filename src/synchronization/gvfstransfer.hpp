/*
 * gnote
 *
 * Copyright (C) 2025-2026 Aurimas Cernius
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

#ifndef _SYNCHRONIZATION_GVFSTRANSFER_HPP_
#define _SYNCHRONIZATION_GVFSTRANSFER_HPP_

#include <atomic>

#include <semaphore.h>
#include <giomm/file.h>

#include "base/monitor.hpp"


namespace gnote {
namespace sync {

enum class TransferResult
{
  NOT_STARTED,
  SUCCESS,
  FAILURE,
};

class GioFileWrapper
{
public:
  GioFileWrapper(const Glib::RefPtr<Gio::File> &file)
    : m_file(file)
  {}

  void copy_to_async(const GioFileWrapper &dest, const std::function<void(TransferResult)> &completion, const Glib::RefPtr<Gio::Cancellable> &cancel_op) const;
private:
  TransferResult finish_single_transfer(const Glib::RefPtr<Gio::File> &source, const Glib::RefPtr<Gio::AsyncResult> &result) const;

  Glib::RefPtr<Gio::File> m_file;
};

template <typename FileT>
struct FileTransferBase
{
  FileTransferBase(const FileT &src, const FileT &dest)
    : source(src)
    , destination(dest)
    , result(TransferResult::NOT_STARTED)
  {}

  void transfer_async(const std::function<void(TransferResult)> &completion, const Glib::RefPtr<Gio::Cancellable> &cancel_op) const
  {
    source.copy_to_async(destination, completion, cancel_op);
  }

  FileT source;
  FileT destination;
  mutable TransferResult result;
};

struct FileTransfer
  : FileTransferBase<GioFileWrapper>
{
  FileTransfer(const Glib::RefPtr<Gio::File> &src, const Glib::RefPtr<Gio::File> &dest)
    : FileTransferBase(src, dest)
  {}
};


struct TransferLimiterNoLimit
{
  void claim() {}
  void release() {}
};


class TransferLimiterFixed
{
public:
  explicit TransferLimiterFixed(unsigned max);
  TransferLimiterFixed(const TransferLimiterFixed&) = delete;
  TransferLimiterFixed &operator=(const TransferLimiterFixed&) = delete;
  ~TransferLimiterFixed();

  void claim();
  void release();
private:
  sem_t m_semaphore;
};


class GvfsTransferBase
{
protected:
  static unsigned calculate_failure_margin(std::size_t transfers);
};

template <typename ContainerT, typename FileTransferT = FileTransfer, typename TransferLimiterT = TransferLimiterNoLimit>
class GvfsTransfer
  : public GvfsTransferBase
{
public:
  template <typename... LimiterArgs>
  explicit GvfsTransfer(const ContainerT &transfers, const LimiterArgs... limiter_args)
    : m_limiter(limiter_args...)
    , m_transfers(transfers)
    , m_failure_margin(calculate_failure_margin(transfers.size()))
  {}

  unsigned transfer()
  {
    unsigned failures = 0;
    do {
      unsigned fails = try_file_transfer();
      if(fails > 0) {
        bool no_progress = fails == failures;
        failures = fails;
        if(no_progress) {
          break;
        }
      }
      else {
        failures = 0;
      }
    }
    while(failures > 0 && !m_cancel_op->is_cancelled());

    return failures;
  }
protected:
  TransferLimiterT m_limiter;
private:
  unsigned try_file_transfer()
  {
    std::atomic<unsigned> remaining(m_transfers.size());
    std::atomic<unsigned> failures(0);

    for(const FileTransferT &transfer : m_transfers) {
      if(transfer.result == TransferResult::SUCCESS) {
        --remaining;
        continue;
      }
      transfer.result = TransferResult::NOT_STARTED;
      m_limiter.claim();
      transfer.transfer_async([this, &remaining, &failures](TransferResult result) {
        m_limiter.release();
        if(result == TransferResult::FAILURE) {
          ++failures;
        }

        if(--remaining == 0 || result == TransferResult::FAILURE) {
          Monitor::Lock lock(m_finished);
          m_finished.notify_one();
        }
      }, m_cancel_op);
    }

    Monitor::Lock lock(m_finished);
    while(remaining > 0) {
      m_finished.wait(lock);
      if(failures > m_failure_margin) {
        m_cancel_op->cancel();
      }
    }

    return failures;
  }

  const ContainerT &m_transfers;
  const unsigned m_failure_margin;
  const Glib::RefPtr<Gio::Cancellable> m_cancel_op = Gio::Cancellable::create();
  Monitor m_finished;
};

}
}

#endif

