/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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



#include "debug.hpp"
#include "ignote.hpp"
#include "keybinder.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "prefskeybinder.hpp"
#include "tray.hpp"


#define KEYBINDING_SHOW_NOTE_MENU_DEFAULT "&lt;Alt&gt;F12"
#define KEYBINDING_OPEN_START_HERE_DEFAULT "&lt;Alt&gt;F11"
#define KEYBINDING_CREATE_NEW_NOTE_DEFAULT "disabled"
#define KEYBINDING_OPEN_SEARCH_DEFAULT "disabled"
#define KEYBINDING_OPEN_RECENT_CHANGES_DEFAULT "disabled"


namespace gnote {


  class PrefsKeybinder::Binding
  {
  public:
    Binding(const std::string & pref_path, const std::string & default_binding,
            const sigc::slot<void> & handler, IKeybinder & native_keybinder);
    void set_binding();
    void unset_binding();
  private:
    void on_binding_changed(const Glib::ustring & key);

    std::string m_pref_path;
    std::string m_key_sequence;
    sigc::slot<void> m_handler;
    IKeybinder  & m_native_keybinder;
    guint         m_notify_cnx;
  };

  PrefsKeybinder::Binding::Binding(const std::string & pref_path, 
                                   const std::string & default_binding,
                                   const sigc::slot<void> & handler, 
                                   IKeybinder & native_keybinder)
    : m_pref_path(pref_path)
    , m_key_sequence(default_binding)
    , m_handler(handler)
    , m_native_keybinder(native_keybinder)
    , m_notify_cnx(0)
  {
    Glib::RefPtr<Gio::Settings> keybindings_settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_KEYBINDINGS);
    m_key_sequence = keybindings_settings->get_string(pref_path);
    set_binding();
    keybindings_settings->signal_changed()
      .connect(sigc::mem_fun(*this, &PrefsKeybinder::Binding::on_binding_changed));
  }

  void PrefsKeybinder::Binding::on_binding_changed(const Glib::ustring & key)
  {
    if (key == m_pref_path) {
      std::string value = Preferences::obj().get_schema_settings(Preferences::SCHEMA_KEYBINDINGS)->get_string(key);
      DBG_OUT("Binding for '%s' changed to '%s'!", m_pref_path.c_str(), value.c_str());

      unset_binding ();

      m_key_sequence = value;
      set_binding ();
    }
  }


  void PrefsKeybinder::Binding::set_binding()
  {
    if(m_key_sequence.empty() || (m_key_sequence == "disabled")) {
      return;
    }
    DBG_OUT("Binding key '%s' for '%s'",
            m_key_sequence.c_str(), m_pref_path.c_str());

    m_native_keybinder.bind (m_key_sequence, m_handler);
  }

  void PrefsKeybinder::Binding::unset_binding()
  {
    if(m_key_sequence.empty()) {
      return;
    }
    DBG_OUT("Unbinding key  '%s' for '%s'",
            m_key_sequence.c_str(), m_pref_path.c_str());

    m_native_keybinder.unbind (m_key_sequence);
  }
  

  PrefsKeybinder::PrefsKeybinder(IKeybinder & keybinder)
    : m_native_keybinder(keybinder)
  {
  }


  PrefsKeybinder::~PrefsKeybinder()
  {

  }


  void PrefsKeybinder::bind(const std::string & pref_path, const std::string & default_binding, 
                            const sigc::slot<void> & handler)
  {
    m_bindings.push_back(new Binding(pref_path, default_binding,
                                     handler, m_native_keybinder));
  }

  void PrefsKeybinder::unbind_all()
  {
    for(std::list<Binding*>::const_iterator iter = m_bindings.begin();
        iter != m_bindings.end(); ++iter) {
      delete *iter;
    }
    m_bindings.clear ();
    m_native_keybinder.unbind_all ();
  }


  GnotePrefsKeybinder::GnotePrefsKeybinder(IKeybinder & keybinder,
                                           NoteManager & manager, IGnoteTray & trayicon)
    : PrefsKeybinder(keybinder)
    , m_manager(manager)
    , m_trayicon(trayicon)
  {
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE);
    enable_disable(settings->get_boolean(Preferences::ENABLE_KEYBINDINGS));
    settings->signal_changed()
      .connect(sigc::mem_fun(*this, &GnotePrefsKeybinder::enable_keybindings_changed));
  }


  void GnotePrefsKeybinder::enable_keybindings_changed(const Glib::ustring & key)
  {
    if(key == Preferences::ENABLE_KEYBINDINGS) {
      Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
        .get_schema_settings(Preferences::SCHEMA_GNOTE);
      
      bool enabled = settings->get_boolean(key);
      enable_disable(enabled);
    }
  }


  void GnotePrefsKeybinder::enable_disable(bool enable)
  {
    DBG_OUT("EnableDisable Called: enabling... %s", enable ? "true" : "false");
    if(enable) {
      bind(Preferences::KEYBINDING_SHOW_NOTE_MENU, KEYBINDING_SHOW_NOTE_MENU_DEFAULT,
           sigc::mem_fun(*this, &GnotePrefsKeybinder::key_show_menu));

      bind(Preferences::KEYBINDING_OPEN_START_HERE, KEYBINDING_OPEN_START_HERE_DEFAULT,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_openstart_here));

      bind(Preferences::KEYBINDING_CREATE_NEW_NOTE, KEYBINDING_CREATE_NEW_NOTE_DEFAULT,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_create_new_note));

      bind(Preferences::KEYBINDING_OPEN_SEARCH, KEYBINDING_OPEN_SEARCH_DEFAULT,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_open_search));

      bind(Preferences::KEYBINDING_OPEN_RECENT_CHANGES, KEYBINDING_OPEN_RECENT_CHANGES_DEFAULT,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_open_recent_changes));
    }
    else {
      unbind_all();
    }
  }


  void GnotePrefsKeybinder::key_show_menu()
  {
    // Show the notes menu, highlighting the first item.
    // This matches the behavior of GTK for
    // accelerator-shown menus.
    m_trayicon.show_menu (true);
  }


  void GnotePrefsKeybinder::key_openstart_here()
  {
    Note::Ptr note = m_manager.find_by_uri (m_manager.start_note_uri());
    if (note) {
      IGnote::obj().open_note(note);
    }
  }


  void GnotePrefsKeybinder::key_create_new_note()
  {
    try {
      Note::Ptr new_note = m_manager.create();
      IGnote::obj().open_note(new_note);
    } 
    catch (...) {
      // Fail silently.
    }

  }


  void GnotePrefsKeybinder::key_open_search()
  {
    /* Find dialog is deprecated in favor of searcable ToC */
    /*
      NoteFindDialog find_dialog = NoteFindDialog.GetInstance (manager);
      find_dialog.Present ();
    */
    key_open_recent_changes ();
  }


  void GnotePrefsKeybinder::key_open_recent_changes()
  {
    IGnote::obj().open_search_all();
  }

}

