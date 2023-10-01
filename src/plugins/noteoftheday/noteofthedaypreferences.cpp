/*
 * gnote
 *
 * Copyright (C) 2011-2014,2019,2022-2023 Aurimas Cernius
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

#include <glibmm/i18n.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "noteoftheday.hpp"
#include "noteofthedaypreferences.hpp"
#include "notewindow.hpp"

namespace noteoftheday {

NoteOfTheDayPreferences::NoteOfTheDayPreferences(gnote::IGnote & ignote, gnote::Preferences &, gnote::NoteManager & manager)
  : m_open_template_button(_("_Open Today: Template"), true)
  , m_label(_("Change the <span weight=\"bold\">Today: Template</span> "
              "note to customize the text that new Today notes have."))
  , m_gnote(ignote)
  , m_note_manager(manager)
{
  set_row_spacing(12);
  m_label.set_wrap(true);
  m_label.set_use_markup(true);
  m_label.set_vexpand(true);
  attach(m_label, 0, 0, 1, 1);

  m_open_template_button.set_use_underline(true);
  m_open_template_button.signal_clicked().connect(
      sigc::mem_fun(*this,
                    &NoteOfTheDayPreferences::open_template_button_clicked));
  attach(m_open_template_button, 0, 1, 1, 1);
}

NoteOfTheDayPreferences::~NoteOfTheDayPreferences()
{
}

void NoteOfTheDayPreferences::open_template_button_clicked() const
{
  auto template_note = m_note_manager.find(NoteOfTheDay::s_template_title);

  if(!template_note) {
    try {
      auto & note = m_note_manager.create(Glib::ustring(NoteOfTheDay::s_template_title), NoteOfTheDay::get_template_content(NoteOfTheDay::s_template_title));
      template_note = gnote::NoteBase::Ref(std::ref(note));
      note.queue_save(gnote::CONTENT_CHANGED);
    }
    catch (const sharp::Exception & e) {
      /* TRANSLATORS: first %s is template note title, second is error */
      ERR_OUT(_("NoteOfTheDay could not create %s: %s"),
              NoteOfTheDay::s_template_title.c_str(),
              e.what());
    }
  }

  if(template_note) {
    m_gnote.open_note(template_note.value());
  }
}

}
