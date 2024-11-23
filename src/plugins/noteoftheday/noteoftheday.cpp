/*
 * gnote
 *
 * Copyright (C) 2013-2014,2017,2019-2020,2022-2024 Aurimas Cernius
 * Copyright (C) 2009-2010 Debarshi Ray
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
#include <glibmm/stringutils.h>

#include "sharp/datetime.hpp"
#include "debug.hpp"
#include "noteoftheday.hpp"
#include "utils.hpp"

namespace noteoftheday {

const Glib::ustring NoteOfTheDay::s_template_title
                                  = _("Today: Template");
const Glib::ustring NoteOfTheDay::s_title_prefix
                                  = _("Today: ");

gnote::NoteBase::ORef NoteOfTheDay::create(gnote::NoteManagerBase & manager, const Glib::Date & date)
{
  Glib::ustring title = get_title(date);
  Glib::ustring xml = get_content(date, manager);

  gnote::NoteBase::ORef notd;
  try {
    notd = manager.create(Glib::ustring(title), std::move(xml));
  }
  catch (const sharp::Exception & e) {
    /* TRANSLATORS: first %s is note title, second is error */
    ERR_OUT(_("NoteOfTheDay could not create %s: %s"),
            title.c_str(),
            e.what());
    return gnote::NoteBase::ORef();
  }

  // Automatically tag all new Note of the Day notes
  notd.value().get().add_tag(manager.tag_manager().get_or_create_system_tag("NoteOfTheDay"));

  return notd;
}

void NoteOfTheDay::cleanup_old(gnote::NoteManagerBase & manager)
{
  std::vector<gnote::NoteBase::Ref> kill_list;

  Glib::Date date;
  date.set_time_current(); // time set to 00:00:00

  manager.for_each([&kill_list, date](gnote::NoteBase & note) {
    const Glib::ustring & title = note.get_title();
    const auto & date_time = note.create_date();

    if (true == Glib::str_has_prefix(title, s_title_prefix)
        && s_template_title != title
        && Glib::Date(
             date_time.get_day_of_month(),
             static_cast<Glib::Date::Month>(date_time.get_month()),
             date_time.get_year()) != date
        && !has_changed(note)) {
      kill_list.push_back(note);
    }
  });

  for(const auto & note : kill_list) {
    DBG_OUT("NoteOfTheDay: Deleting old unmodified '%s'", note.get().get_title().c_str());
    manager.delete_note(note);
  }
}

Glib::ustring NoteOfTheDay::get_content(const Glib::Date & date, const gnote::NoteManagerBase & manager)
{
  const Glib::ustring title = get_title(date);

  // Attempt to load content from template
  auto template_note = manager.find(s_template_title);

  if(template_note) {
    Glib::ustring xml_content = template_note.value().get().xml_content();
    return xml_content.replace(xml_content.find(s_template_title, 0),
                               s_template_title.length(),
                               title);
  }
  else {
    return get_template_content(title);
  }
}

Glib::ustring NoteOfTheDay::get_content_without_title(const Glib::ustring & content)
{
  const Glib::ustring::size_type nl = content.find("\n");

  if (Glib::ustring::npos != nl)
    return Glib::ustring(content, nl, Glib::ustring::npos);
  else
    return Glib::ustring();
}

gnote::NoteBase::ORef NoteOfTheDay::get_note_by_date(gnote::NoteManagerBase & manager, const Glib::Date & date)
{
  return manager.search([date](gnote::NoteBase & note, gnote::NoteBase::ORef & ret) {
    const Glib::ustring & title = note.get_title();
    const auto & date_time = note.create_date();

    if (true == Glib::str_has_prefix(title, s_title_prefix)
        && s_template_title != title
        && Glib::Date(
             date_time.get_day_of_month(),
             static_cast<Glib::Date::Month>(date_time.get_month()),
             date_time.get_year()) == date) {
      ret = note;
      return false;
    }
    return true;
  }, gnote::NoteBase::ORef());
}

Glib::ustring NoteOfTheDay::get_template_content(const Glib::ustring & title)
{
  return Glib::ustring::compose(
    "<note-content xmlns:size=\"http://beatniksoftware.com/tomboy/size\">"
    "<note-title>%1</note-title>\n\n\n\n"
    "<size:huge>%2</size:huge>\n\n\n"
    "<size:huge>%3</size:huge>\n\n\n"
    "</note-content>",
    title,
    _("Tasks"),
    _("Appointments"));
}

Glib::ustring NoteOfTheDay::get_title(const Glib::Date & date)
{
  // Format: "Today: Friday, July 01 2005"
  return s_title_prefix + date.format_string(_("%A, %B %d %Y"));
}

bool NoteOfTheDay::has_changed(gnote::NoteBase & note)
{
  const auto & date_time = note.create_date();
  const Glib::ustring original_xml
    = get_content(Glib::Date(
                    date_time.get_day_of_month(),
                    static_cast<Glib::Date::Month>(date_time.get_month()),
                    date_time.get_year()),
                    note.manager());

  return get_content_without_title(note.text_content()) != get_content_without_title(gnote::utils::XmlDecoder::decode(original_xml));
}

}
