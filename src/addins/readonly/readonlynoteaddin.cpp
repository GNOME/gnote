/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#include "iactionmanager.hpp"
#include "itagmanager.hpp"
#include "notewindow.hpp"
#include "readonlynoteaddin.hpp"
#include "tag.hpp"


namespace {
  class ReadOnlyAction
    : public gnote::utils::CheckAction
    , public gnote::NoteWindow::NonModifyingNoteAction
  {
  public:
    typedef Glib::RefPtr<ReadOnlyAction> Ptr;
    static Ptr create()
      {
        return Ptr(new ReadOnlyAction);
      }
    virtual void reference() const override
    {
      gnote::utils::CheckAction::reference();
    }
    virtual void unreference() const override
    {
      gnote::utils::CheckAction::unreference();
    }
  private:
    ReadOnlyAction()
      : gnote::utils::CheckAction("ReadOnlyAction")
    {
      set_label(_("Read Only"));
      set_tooltip(_("Make this note read-only"));
    }
  };
}


namespace readonly {

DECLARE_MODULE(ReadOnlyModule);

ReadOnlyModule::ReadOnlyModule()
  : sharp::DynamicModule()
{
  ADD_INTERFACE_IMPL(ReadOnlyNoteAddin);
  enabled(false);
}


ReadOnlyNoteAddin::ReadOnlyNoteAddin()
  : NoteAddin()
{
}

ReadOnlyNoteAddin::~ReadOnlyNoteAddin()
{
}

void ReadOnlyNoteAddin::initialize()
{
}

void ReadOnlyNoteAddin::shutdown()
{
}

void ReadOnlyNoteAddin::on_note_opened()
{
  m_action = ReadOnlyAction::create();
  add_note_action(m_action, gnote::READ_ONLY_ORDER);
  m_action->signal_activate().connect(
    sigc::mem_fun(*this, &ReadOnlyNoteAddin::on_menu_item_toggled));

  gnote::ITagManager & m = gnote::ITagManager::obj();
  const gnote::Tag::Ptr ro_tag = m.get_or_create_system_tag("read-only");
  if(get_note()->contains_tag(ro_tag)) {
    ReadOnlyAction::Ptr::cast_dynamic(m_action)->checked(true);
    on_menu_item_toggled();
  }
}

void ReadOnlyNoteAddin::on_menu_item_toggled()
{
  gnote::ITagManager & m = gnote::ITagManager::obj();
  const gnote::Tag::Ptr ro_tag = m.get_or_create_system_tag("read-only");
  if(ReadOnlyAction::Ptr::cast_dynamic(m_action)->checked()) {
    get_note()->enabled(false);
    get_note()->add_tag(ro_tag);
  }
  else {
    get_note()->enabled(true);
    get_note()->remove_tag(ro_tag);
  }
}

}
