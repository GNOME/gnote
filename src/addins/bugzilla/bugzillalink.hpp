/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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

#ifndef __BUGZILLA_LINK_HPP_
#define __BUGZILLA_LINK_HPP_

#include <gtkmm/textiter.h>

#include "base/macros.hpp"
#include "noteeditor.hpp"
#include "notetag.hpp"

namespace bugzilla {


class BugzillaLink
  : public gnote::DynamicNoteTag
{
public:
  typedef Glib::RefPtr<BugzillaLink> Ptr;
  static gnote::DynamicNoteTag::Ptr create()
    {
      return gnote::DynamicNoteTag::Ptr(new BugzillaLink);
    }
  BugzillaLink();
  Glib::ustring get_bug_url() const;
  void set_bug_url(const Glib::ustring & );
protected:
  virtual void initialize(const Glib::ustring & element_name) override;
  virtual bool on_activate(const gnote::NoteEditor & , const Gtk::TextIter &, 
                           const Gtk::TextIter &) override;
  virtual void on_attribute_read(const Glib::ustring &) override;
private:
  void make_image();
};

}

#endif
