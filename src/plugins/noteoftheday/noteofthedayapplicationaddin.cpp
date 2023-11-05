/*
 * gnote
 *
 * Copyright (C) 2010,2013,2017,2023 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
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

#include <glibmm/main.h>

#include "noteoftheday.hpp"
#include "noteofthedayapplicationaddin.hpp"
#include "noteofthedaypreferencesfactory.hpp"

namespace noteoftheday {

DECLARE_MODULE(NoteOfTheDayModule);

NoteOfTheDayModule::NoteOfTheDayModule()
{
  ADD_INTERFACE_IMPL(NoteOfTheDayApplicationAddin);
  ADD_INTERFACE_IMPL(NoteOfTheDayPreferencesFactory);
  enabled(false);
}

const char * NoteOfTheDayApplicationAddin::IFACE_NAME
               = "gnote::ApplicationAddin";

NoteOfTheDayApplicationAddin::NoteOfTheDayApplicationAddin()
  : ApplicationAddin()
  , m_initialized(false)
  , m_timeout()
{
}

NoteOfTheDayApplicationAddin::~NoteOfTheDayApplicationAddin()
{
}

void NoteOfTheDayApplicationAddin::check_new_day() const
{
  Glib::Date date;
  date.set_time_current();

  if(!NoteOfTheDay::get_note_by_date(note_manager(), date)) {
    NoteOfTheDay::cleanup_old(note_manager());

    // Create a new NotD if the day has changed
    NoteOfTheDay::create(note_manager(), date);
  }
}

void NoteOfTheDayApplicationAddin::initialize()
{
  if (!m_timeout) {
    m_timeout
      = Glib::signal_timeout().connect_seconds(
          sigc::bind_return(
            sigc::mem_fun(
              *this,
              &NoteOfTheDayApplicationAddin::check_new_day),
            true),
          60,
          Glib::PRIORITY_DEFAULT);
  }

  Glib::signal_idle().connect_once(
    sigc::mem_fun(*this,
                  &NoteOfTheDayApplicationAddin::check_new_day),
    Glib::PRIORITY_DEFAULT);

  m_initialized = true;
}

void NoteOfTheDayApplicationAddin::shutdown()
{
  if (m_timeout)
    m_timeout.disconnect();

  m_initialized = false;
}

bool NoteOfTheDayApplicationAddin::initialized()
{
  return m_initialized;
}

}
