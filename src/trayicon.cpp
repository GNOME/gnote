

#include <glibmm/i18n.h>
#include <gtkmm/stock.h>
#include <gtkmm/menuitem.h>

#include "trayicon.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "utils.hpp"

namespace gnote {

	Tray::Tray(const boost::shared_ptr<NoteManager> & manager, TrayIcon & trayicon)
		: m_manager(manager)
		, m_trayicon(trayicon)
	{

		m_tray_menu = make_tray_notes_menu();
	}

	Gtk::Menu * Tray::make_tray_notes_menu()
	{
		Gtk::Menu *menu;

		ActionManager * am = ActionManager::get_manager();
		
		menu = (Gtk::Menu*)am->get_widget("/TrayIconMenu");
		DBG_ASSERT(menu, "menu not found");
		
		// TODO
		bool enable_keybindings = true; // Preferences.Get (Preferences.ENABLE_KEYBINDINGS);
		if (enable_keybindings) {
			Gtk::MenuItem *item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");
			if(item) {
//	GConfKeybindingToAccel.AddAccelerator (
//					        item,
//					        Preferences.KEYBINDING_CREATE_NEW_NOTE);
			}
			item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/ShowSearchAllNotes");
			if(item) {
//					GConfKeybindingToAccel.AddAccelerator (
//					        item,
//					        Preferences.KEYBINDING_OPEN_RECENT_CHANGES);
			}
			item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/OpenStartHereNote");
			if(item) {
//					GConfKeybindingToAccel.AddAccelerator (
//					        item,
//					        Preferences.KEYBINDING_OPEN_START_HERE);
			}
		}

		return menu;
	}


	TrayIcon::TrayIcon(const boost::shared_ptr<NoteManager> & manager)
		: Gtk::StatusIcon()
		, m_tray(new Tray(manager, *this))
		, m_keybinder(new PrefsKeybinder(manager, *this))
		, m_context_menu(NULL)
	{
		int panel_size = 32;
		Glib::RefPtr<Gdk::Pixbuf> pixbuf = utils::get_icon("gnote", panel_size);
		set(pixbuf);
		set_tooltip_text("");
		
		// TODO
		// 			Tomboy.ExitingEvent += OnExit;
		signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::on_activate));
		signal_popup_menu().connect(sigc::mem_fun(*this, &TrayIcon::on_popup_menu));
	}

	void TrayIcon::show_menu(bool select_first_item)
	{
		if(m_context_menu) {
			m_context_menu->hide();
			DBG_OUT("context menu found");
		}
		// UpdateTrayMenu
		if(select_first_item) {
			m_tray->tray_menu()->select_first(false);
		}
		utils::popup_menu(m_tray->tray_menu(), NULL, sigc::mem_fun(*this, &TrayIcon::get_tray_menu_pos));
	}

	TrayIcon::~TrayIcon()
	{
		delete m_context_menu;
	}

	void TrayIcon::on_activate()
	{
		DBG_OUT("activated");
		show_menu(false);
	}

	void TrayIcon::on_popup_menu(guint button, guint32 /*activate_time*/)
	{
		DBG_OUT("popup");
		if(button == 3) {
			Gtk::Menu *menu = get_right_click_menu();
			utils::popup_menu(menu, NULL, sigc::mem_fun(*this, &TrayIcon::get_tray_menu_pos));
		}
	}	


	void TrayIcon::get_tray_menu_pos(int & x, int &y, bool & push_in)
	{
			push_in = true;
			x = 0;
			y = 0;
			
			Glib::RefPtr<Gdk::Screen> screen;
			GdkScreen *cscreen = NULL;
			Gdk::Rectangle area;
			GtkOrientation orientation;
// using the C++ API seems to crash here on the Gdk::Screen.
//			get_geometry (screen, area, orientation);
			gtk_status_icon_get_geometry(gobj(), &cscreen, area.gobj(), &orientation);
			x = area.get_x();
			y = area.get_y();

			Gtk::Requisition menu_req;
			get_right_click_menu()->size_request (menu_req);
			if (y + menu_req.height >= gdk_screen_get_height(cscreen)/*screen->get_height()*/) {
				y -= menu_req.height;
			}
			else {
				y += area.get_height();
			}
			DBG_OUT("x = %d, y = %d, push_in = %d", x, y, push_in);
	}

	Gtk::Menu * TrayIcon::get_right_click_menu()
	{
		if(m_tray->tray_menu()) {
			m_tray->tray_menu()->hide();
		}
		if(m_context_menu) {
			m_context_menu->hide();
			return m_context_menu;
		}
		m_context_menu = new Gtk::Menu();
		
		Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();
		m_context_menu->set_accel_group(accel_group);

		Gtk::ImageMenuItem * item;

		item = manage(new Gtk::ImageMenuItem(_("_Preferences")));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::PREFERENCES, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_preferences));
		m_context_menu->append(*item);

		item = manage(new Gtk::ImageMenuItem(_("_Help")));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_help_contents));
		m_context_menu->append(*item);

		item = manage(new Gtk::ImageMenuItem(_("_About GNote")));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::ABOUT, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_about));
		m_context_menu->append(*item);

		m_context_menu->append(*manage(new Gtk::SeparatorMenuItem()));

		item = manage(new Gtk::ImageMenuItem(_("_Quit")));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::QUIT, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::quit));
		m_context_menu->append(*item);

		m_context_menu->show_all();

		return m_context_menu;
	}

	void TrayIcon::show_preferences()
	{
		(*ActionManager::get_manager())["ShowPreferencesAction"]->activate();
	}

	void TrayIcon::show_help_contents()
	{
		(*ActionManager::get_manager())["ShowHelpAction"]->activate();
	}

	void TrayIcon::show_about()
	{
		(*ActionManager::get_manager())["ShowAboutAction"]->activate();
	}

	void TrayIcon::quit()
	{
		(*ActionManager::get_manager())["QuitGNoteAction"]->activate();
	}

	void TrayIcon::on_exit()
	{
		set_visible(false);
	}
}
