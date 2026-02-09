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

#include <glibmm/i18n.h>

#include "debug.hpp"
#include "gvfstransfer.hpp"


namespace gnote {
namespace sync {

void GioFileWrapper::copy_to_async(const GioFileWrapper &dest, const std::function<void(TransferResult)> &completion, const Glib::RefPtr<Gio::Cancellable> &cancel_op) const
{
  m_file->copy_async(dest.m_file, [this, completion](Glib::RefPtr<Gio::AsyncResult> &result) {
    auto res = finish_single_transfer(m_file, result);
    completion(res);
  }, cancel_op);
}


TransferResult GioFileWrapper::finish_single_transfer(const Glib::RefPtr<Gio::File> &source, const Glib::RefPtr<Gio::AsyncResult> &result) const
{
  try {
    if(source->copy_finish(result)) {
      return TransferResult::SUCCESS;
    }
    else {
      ERR_OUT(_("Failed to copy file"));
    }
  }
  catch(std::exception & e) {
    ERR_OUT(_("Exception when finishing note copy: %s"), e.what());
  }
  catch(...) {
    ERR_OUT(_("Exception when finishing note copy"));
  }

  return TransferResult::FAILURE;
}

unsigned GvfsTransferBase::calculate_failure_margin(std::size_t transfers)
{
  unsigned failure_margin = transfers / 4;
  if(failure_margin < 10) {
    failure_margin = 10;
  }

  return failure_margin;
}


TransferLimiterFixed::TransferLimiterFixed(unsigned max)
{
  sem_init(&m_semaphore, 0, max);
}

TransferLimiterFixed::~TransferLimiterFixed()
{
  sem_destroy(&m_semaphore);
}

void TransferLimiterFixed::claim()
{
  sem_wait(&m_semaphore);
}

void TransferLimiterFixed::release()
{
  sem_post(&m_semaphore);
}


}
}

