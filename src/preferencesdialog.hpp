/*
 * gnote
 *
 * Copyright (C) 2011-2013,2015-2017,2019-2020,2022-2023 Aurimas Cernius
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




#ifndef __PREFERENCES_DIALOG_HPP_
#define __PREFERENCES_DIALOG_HPP_

#include <map>

#include <gtkmm/dialog.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/spinbutton.h>

#include "sharp/pluginsmodel.hpp"

namespace gnote {

namespace sync { class SyncServiceAddin; }
class AddinManager;
class IGnote;

class PreferencesDialog
  : public Gtk::Dialog
{
public:
  PreferencesDialog(IGnote & ignote, NoteManager & note_manager);
  

  Gtk::Widget *make_editing_pane();
  Gtk::Widget *make_links_pane();
  Gtk::Widget *make_sync_pane();
  Gtk::Widget *make_addins_pane();

private:
  void set_widget_tooltip(Gtk::Widget & widget, Glib::ustring label_text);
  Gtk::Button *make_font_button();
  Gtk::Label *make_label(const Glib::ustring & label_text/*, params object[] args*/);
  Gtk::CheckButton *make_check_button(const Glib::ustring & label_text);

  void enable_addin(bool enable);
  void enable_app_addin(ApplicationAddin *addin, bool enable);
  void enable_sync_addin(sync::SyncServiceAddin *addin, bool enable);

  void open_template_button_clicked();
  void on_font_button_clicked();
  void update_font_button(const Glib::ustring & font_desc);
  void on_sync_addin_combo_changed();
  void on_advanced_sync_config_button();
  void on_reset_sync_addin_button(bool signal);
  void on_save_sync_addin_button();
  void on_sync_settings_saved(const Glib::RefPtr<Glib::Object> & active_sync_service, bool saved, Glib::ustring errorMsg);

  void on_note_rename_behavior_changed();
  void on_autosync_timeout_setting_changed();
  void on_rename_behavior_changed();

  Glib::ustring get_selected_addin();
  void set_module_for_selected_addin(sharp::DynamicModule * module);
  void on_plugin_view_selection_changed(const Glib::RefPtr<sharp::Plugin> & plugin);
  void update_addin_buttons();
  void update_addin_buttons(const Glib::RefPtr<sharp::Plugin> & plugin);
  void load_addins();
  void on_enable_addin_button();
  void on_disable_addin_button();
  void on_addin_prefs_button();
  void addin_pref_dialog_response(const Glib::ustring &, Gtk::Dialog*);
  void on_addin_info_button();
  void addin_info_dialog_response(int, Gtk::Dialog*);
  void on_sync_addin_prefs_changed();
  void update_sync_services();
  void update_timeout_pref();
  void on_autosync_check_toggled();
////

  Gtk::DropDown *m_sync_addin_combo;
  sync::SyncServiceAddin *m_selected_sync_addin;
  Gtk::Grid   *m_sync_addin_prefs_container;
  Gtk::Widget *m_sync_addin_prefs_widget;
  Gtk::Button *m_reset_sync_addin_button;
  Gtk::Button *m_save_sync_addin_button;
  Gtk::CheckButton *m_autosync_check;
  Gtk::SpinButton *m_autosync_spinner;
  Gtk::DropDown *m_rename_behavior_combo;
  IGnote & m_gnote;
  AddinManager &m_addin_manager;
  NoteManager & m_note_manager;
    
  Gtk::Button *font_button;
  Gtk::Label  *font_face;
  Gtk::Label  *font_size;

  Gtk::ColumnView *m_plugin_view;
  sharp::AddinsModel::Ptr m_plugin_model;
  
  Gtk::Button *enable_addin_button;
  Gtk::Button *disable_addin_button;
  Gtk::Button *addin_prefs_button;
  Gtk::Button *addin_info_button;

  /// Keep track of the opened addin prefs dialogs so other windows
  /// can be interacted with (as opposed to opening these as modal
  /// dialogs).
  ///
  /// Key = Mono.Addins.Addin.Id
  std::map<Glib::ustring, Gtk::Dialog* > addin_prefs_dialogs;

  /// Used to keep track of open AddinInfoDialogs.
  /// Key = Mono.Addins.Addin.Id
  std::map<Glib::ustring, Gtk::Dialog* > addin_info_dialogs;

};


}

#endif
