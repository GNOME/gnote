

/* ported from */
/***************************************************************************
 *  ActionManager.cs
 *
 *  Copyright (C) 2005-2006 Novell, Inc.
 *  Written by Aaron Bockover <aaron@abock.org>
 ****************************************************************************/

/*  THIS FILE IS LICENSED UNDER THE MIT LICENSE AS OUTLINED IMMEDIATELY BELOW:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */


#include <glibmm/i18n.h>
#include <gtkmm/window.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "actionmanager.hpp"
#include "utils.hpp"

namespace gnote {

	ActionManager * ActionManager::s_instance = NULL;


	ActionManager::ActionManager()
		: m_ui(Gtk::UIManager::create())
		, m_main_window_actions(Gtk::ActionGroup::create("MainWindow"))
	{
		populate_action_groups();
		m_newNote = utils::get_icon("note-new", 16);
	}


	void ActionManager::load_interface()
	{
		m_ui->add_ui_from_file("UIManagerLayout.xml");
		Gtk::Window::set_default_icon_name("tomboy");


		Gtk::ImageMenuItem *imageitem = (Gtk::ImageMenuItem*)m_ui->get_widget(
			"/MainWindowMenubar/FileMenu/FileMenuNewNotePlaceholder/NewNote");
		if (imageitem) {
			imageitem->set_image(*manage(new Gtk::Image(m_newNote)));
		}
			
		imageitem = (Gtk::ImageMenuItem*)m_ui->get_widget (
			"/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");

		if (imageitem) {
			imageitem->set_image(*manage(new Gtk::Image(m_newNote)));
		}
	}

	void ActionManager::populate_action_groups()
	{
		Glib::RefPtr<Gtk::Action> action;
		action = Gtk::Action::create("FileMenuAction", _("_File"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create("NewNoteAction", 
																 Gtk::Stock::NEW, _("_New"),
																 _("Create a new note"));
		m_main_window_actions->add(action, Gtk::AccelKey("<Control>N"));

		action = Gtk::Action::create(
			"OpenNoteAction", Gtk::Stock::OPEN,
			_("_Open..."),_("Open the selected note"));
		action->set_sensitive(false);
		m_main_window_actions->add(action, Gtk::AccelKey( "<Control>O"));

		action = Gtk::Action::create(
			"DeleteNoteAction", Gtk::Stock::DELETE,
			_("_Delete"),	_("Delete the selected note"));
		action->set_sensitive(false);
		m_main_window_actions->add(action, Gtk::AccelKey("Delete"));

		action = Gtk::Action::create(
			"CloseWindowAction", Gtk::Stock::CLOSE,
			_("_Close"), _("Close this window"));
		m_main_window_actions->add(action, Gtk::AccelKey("<Control>W"));

		action = Gtk::Action::create(
			"QuitGNoteAction", Gtk::Stock::QUIT,
			_("_Quit"), _("Quit GNote"));
		m_main_window_actions->add(action, Gtk::AccelKey("<Control>Q"));

		action = Gtk::Action::create("EditMenuAction", _("_Edit"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create(
			"ShowPreferencesAction", Gtk::Stock::PREFERENCES,
			_("_Preferences"), _("GNote Preferences"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create("HelpMenuAction", _("_Help"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create("ShowHelpAction", Gtk::Stock::HELP,
			_("_Contents"), _("GNote Help"));
		m_main_window_actions->add(action, Gtk::AccelKey("F1"));

		action = Gtk::Action::create(
			"ShowAboutAction", Gtk::Stock::ABOUT,
			_("_About"), _("About GNote"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create(
			"TrayIconMenuAction",	_("TrayIcon"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create(
			"TrayNewNoteAction", Gtk::Stock::NEW,
			_("Create _New Note"), _("Create a new note"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create(
			"ShowSearchAllNotesAction", Gtk::Stock::FIND,
			_("_Search All Notes"),	_("Open the Search All Notes window"));
		m_main_window_actions->add(action);

		action = Gtk::Action::create(
			"NoteSynchronizationAction",
			_("S_ynchronize Notes"), _("Start synchronizing notes"));
		m_main_window_actions->add(action);		

		m_ui->insert_action_group(m_main_window_actions);
	}

	Glib::RefPtr<Gtk::Action> ActionManager::find_action_by_name(const std::string & n)
	{
		Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> > actiongroups = m_ui->get_action_groups();
		for(Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> >::const_iterator iter(actiongroups.begin()); 
				iter != actiongroups.end(); ++iter) {
			Glib::ListHandle<Glib::RefPtr<Gtk::Action> > actions = (*iter)->get_actions();
			for(Glib::ListHandle<Glib::RefPtr<Gtk::Action> >::const_iterator iter2(actions.begin()); 
					iter2 != actions.end(); ++iter2) {
				if((*iter2)->get_name() == n) {
					return *iter2;
				}
			}
		}
		return Glib::RefPtr<Gtk::Action>();			
	}

}
