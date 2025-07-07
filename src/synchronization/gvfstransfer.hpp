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

#ifndef _SYNCHRONIZATION_GVFSTRANSFER_HPP_
#define _SYNCHRONIZATION_GVFSTRANSFER_HPP_

#include <atomic>

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

struct FileTransfer
{
  FileTransfer(const Glib::RefPtr<Gio::File> &src, const Glib::RefPtr<Gio::File> &dest)
    : source(src)
    , destination(dest)
    , result(TransferResult::NOT_STARTED)
  {}

  Glib::RefPtr<Gio::File> source;
  Glib::RefPtr<Gio::File> destination;
  TransferResult result;
};

class GvfsTransferBase
{
protected:
  static TransferResult finish_single_transfer(const Glib::RefPtr<Gio::File> &source, const Glib::RefPtr<Gio::AsyncResult> &result)
  {
    try {
      if(source->copy_finish(result)) {
        return TransferResult::SUCCESS;
      }
      else {
        //ERR_OUT(_("Failed to copy file"));
      }
    }
    catch(std::exception & e) {
      //ERR_OUT(_("Exception when finishing note copy: %s"), e.what());
    }
    catch(...) {
      //ERR_OUT(_("Exception when finishing note copy"));
    }

    return TransferResult::FAILURE;
  }
};

template <typename TransferT>
class GvfsTransfer
  : public GvfsTransferBase
{
public:
  explicit GvfsTransfer(std::vector<TransferT> &transfers)
    : m_transfers(transfers)
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
    }
    while(failures > 0 && !m_cancel_op->is_cancelled());

    return failures;
  }
private:
  unsigned try_file_transfer()
  {
    std::atomic<unsigned> remaining(m_transfers.size());
    std::atomic<unsigned> failures(0);

    for(FileTransfer &transfer : m_transfers) {
      if(transfer.result == TransferResult::SUCCESS) {
        --remaining;
        continue;
      }
      transfer.result = TransferResult::NOT_STARTED;
      transfer.source->copy_async(transfer.destination, [this, &transfer, &remaining, &failures](Glib::RefPtr<Gio::AsyncResult> &result) {
        transfer.result = finish_single_transfer(transfer.source, result);
        if(transfer.result == TransferResult::FAILURE) {
          ++failures;
        }

        if(--remaining == 0 || transfer.result == TransferResult::FAILURE) {
          Monitor::Lock lock(m_finished);
          m_finished.notify_one();
        }
      }, m_cancel_op);
    }

    unsigned failure_margin = m_transfers.size() / 4;
    if(failure_margin < 10) {
      failure_margin = 10;
    }

    Monitor::Lock lock(m_finished);
    while(remaining > 0) {
      m_finished.wait(lock);
      if(failures > failure_margin) {
        m_cancel_op->cancel();
      }
    }

    return failures;
  }

  std::vector<TransferT> &m_transfers;
  const Glib::RefPtr<Gio::Cancellable> m_cancel_op = Gio::Cancellable::create();
  Monitor m_finished;
};

}
}

#endif

