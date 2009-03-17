

#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/stock.h>


#include "tray.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "utils.hpp"
#include "note.hpp"
#include "notewindow.hpp"
#include "tag.hpp"
#include "preferences.hpp"
#include "sharp/datetime.hpp"

namespace gnote {
	bool                      NoteMenuItem::s_static_inited = false;
	Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::s_note_icon;
	Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::s_pinup;
	Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::s_pinup_active;
	Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::s_pindown;
	
	NoteMenuItem::NoteMenuItem(const Note::Ptr & note, bool show_pin)
		: Gtk::ImageMenuItem(get_display_name(note))
		, m_note(note)
		, m_pin_img(NULL)
		, m_pinned(false)
		, m_inhibit_activate(false)
	{
		_init_static();
		set_image(*manage(new Gtk::Image(s_note_icon)));
		if(show_pin) {
			Gtk::HBox *box = manage(new Gtk::HBox(false, 0));
			Gtk::Widget *child = get_child();
			Gtk::Container::remove(*child);
			box->pack_start(*child, true, true, 0);
			add(*box);
			box->show();

			m_pinned = note->is_pinned();
			m_pin_img = manage(new Gtk::Image(m_pinned ? s_pindown : s_pinup));
			m_pin_img->show();
			box->pack_start(*m_pin_img, false, false, 0);
		}
	}

	void NoteMenuItem::on_activate()
	{
		if(!m_inhibit_activate) {
			if(m_note) {
				//m_note->window()->present();
			}
		}
	}

	bool NoteMenuItem::on_button_press_event(GdkEventButton *ev)
	{
		if (m_pin_img && (ev->x >= m_pin_img->get_allocation().get_x()) 
				&& (ev->x < m_pin_img->get_allocation().get_x() 
						+ m_pin_img->get_allocation().get_width())) {
			m_pinned = !m_pinned;
			m_note->set_pinned(m_pinned);
			m_pin_img->set(m_pinned ? s_pindown : s_pinup);
			m_inhibit_activate = true;
			return true;
		}

		return Gtk::ImageMenuItem::on_button_press_event(ev);
	}


	bool NoteMenuItem::on_button_release_event(GdkEventButton *ev)
	{
		if (m_inhibit_activate) {
			m_inhibit_activate = false;
			return true;
		}
		return Gtk::ImageMenuItem::on_button_release_event(ev);
	}


	bool NoteMenuItem::on_motion_notify_event(GdkEventMotion *ev)
	{
		if (!m_pinned && m_pin_img) {
			if (ev->x >= m_pin_img->get_allocation().get_x() &&
					ev->x < m_pin_img->get_allocation().get_x() + m_pin_img->get_allocation().get_width()) {
				if (m_pin_img->get_pixbuf() != s_pinup_active) {
					m_pin_img->set(s_pinup_active);
				}
			} else if (m_pin_img->get_pixbuf() != s_pinup) {
				m_pin_img->set(s_pinup);
			}
		}

		return Gtk::ImageMenuItem::on_motion_notify_event(ev);
	}


	bool NoteMenuItem::on_leave_notify_event(GdkEventCrossing *ev)
	{
		if (!m_pinned && m_pin_img) {
			m_pin_img->set(s_pinup);
		}
		return Gtk::ImageMenuItem::on_leave_notify_event(ev);
	}

	std::string NoteMenuItem::format_for_label (const std::string & name)
	{
		// Replace underscores ("_") with double-underscores ("__")
		// so Note menuitems are not created with mnemonics.
		return boost::replace_all_copy(name, "-", "--");
	}


	std::string NoteMenuItem::get_display_name(const Note::Ptr & note)
	{
		std::string display_name = note->title();
		int max_length = 100;
		
		if (note->is_new()) {
			std::string new_string = _(" (new)");
			max_length -= new_string.size();
			display_name = ellipsify (display_name, max_length)	+ new_string;
		} else {
			display_name = ellipsify (display_name, max_length);
		}

		return format_for_label(display_name);
	}

	std::string NoteMenuItem::ellipsify (const std::string & str, size_t max)
	{
		if(str.size() > max) {
			std::string new_str = str;
			new_str.resize(max);
			return new_str + "...";
		}
		return str;
	}



	void NoteMenuItem::_init_static()
	{
		if(s_static_inited)
			return;
		s_note_icon = utils::get_icon("note", 16);
		s_pinup = utils::get_icon("pin-up", 16);
		s_pinup_active = utils::get_icon("pin-active", 16);
		s_pindown = utils::get_icon("pin-down", 16);
		s_static_inited = true;
	}

	Tray::Tray(const boost::shared_ptr<NoteManager> & manager, TrayIcon & trayicon)
		: m_manager(manager)
		, m_trayicon(trayicon)
		, m_menu_added(false)
	{

		m_tray_menu = make_tray_notes_menu();
	}

	Gtk::Menu * Tray::make_tray_notes_menu()
	{
		Gtk::Menu *menu;

		ActionManager * am = ActionManager::get_manager();
		
		menu = (Gtk::Menu*)am->get_widget("/TrayIconMenu");
		DBG_ASSERT(menu, "menu not found");
		
		bool enable_keybindings = Preferences::get_preferences()->get<bool>(Preferences::ENABLE_KEYBINDINGS);
		if (enable_keybindings) {
			Gtk::MenuItem *item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");
			if(item) {
				GConfKeybindingToAccel::add_accelerator(
					*item, Preferences::KEYBINDING_CREATE_NEW_NOTE);
			}
			item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/ShowSearchAllNotes");
			if(item) {
				GConfKeybindingToAccel::add_accelerator(
					*item, Preferences::KEYBINDING_OPEN_RECENT_CHANGES);
			}
			item = (Gtk::MenuItem*)am->get_widget("/TrayIconMenu/OpenStartHereNote");
			if(item) {
				GConfKeybindingToAccel::add_accelerator(
					*item, Preferences::KEYBINDING_OPEN_START_HERE);
			}
		}

		return menu;
	}

	void Tray::update_tray_menu(Gtk::Widget * parent)
	{
		if (!m_menu_added) {
			if (parent)
// Can't call the Gtkmm version as it is protected.
//				m_tray_menu->attach_to_widget(*parent);
				gtk_menu_attach_to_widget(m_tray_menu->gobj(), parent->gobj(), NULL);
				m_menu_added = true;
			}

			add_recently_changed_notes();

			m_tray_menu->show_all();
	}

	void Tray::remove_recently_changed_notes()
	{
		std::list<Gtk::MenuItem*>::iterator iter;
		for(iter = m_recent_notes.begin();
				iter != m_recent_notes.end(); ++iter) {
			m_tray_menu->remove(**iter);
		}
		m_recent_notes.clear();
	}

	void Tray::add_recently_changed_notes()
	{
		int min_size = Preferences::get_preferences()->get<int>(Preferences::MENU_NOTE_COUNT);
		int max_size = 18;
		int list_size = 0;
		bool menuOpensUpward = m_trayicon.menu_opens_upward();
		NoteMenuItem *item;

		// Remove the old dynamic items
		remove_recently_changed_notes();

		// Assume menu opens downward, move common items to top of menu
		ActionManager & am = *ActionManager::get_manager();
		Gtk::MenuItem* newNoteItem = (Gtk::MenuItem*)am.get_widget(
			"/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");
		Gtk::MenuItem* searchNotesItem = (Gtk::MenuItem*)am.get_widget(
			"/TrayIconMenu/ShowSearchAllNotes");
		m_tray_menu->reorder_child (*newNoteItem, 0);
		int insertion_point = 1; // If menu opens downward
			
		// Find all child widgets under the TrayNewNotePlaceholder
		// element.  Make sure those added by add-ins are
		// properly accounted for and reordered.
		std::list<Gtk::Widget*> newNotePlaceholderWidgets;
		std::list<Gtk::Widget*> allChildWidgets(am.get_placeholder_children("/TrayIconMenu/TrayNewNotePlaceholder"));
		for(std::list<Gtk::Widget*>::const_iterator iter = allChildWidgets.begin();
				iter != allChildWidgets.end(); ++iter) {
			Gtk::MenuItem * menuitem = dynamic_cast<Gtk::MenuItem*>(*iter);
			if (menuitem && (menuitem != newNoteItem)) {
				newNotePlaceholderWidgets.push_back(menuitem);
				m_tray_menu->reorder_child (*menuitem, insertion_point);
				insertion_point++;
			}
		}
			
		m_tray_menu->reorder_child (*searchNotesItem, insertion_point);
		insertion_point++;

		sharp::DateTime days_ago(sharp::DateTime::now());
		days_ago.add_days(-3);

		// Prevent template notes from appearing in the menu
		//TODO
		Tag::Ptr template_tag;// = TagManager.GetOrCreateSystemTag (TagManager.TemplateNoteSystemTag);

		// List the most recently changed notes, any currently
		// opened notes, and any pinned notes...
		const Note::List & notes = m_manager->get_notes();
		for(Note::List::const_iterator iter = notes.begin();
				iter != notes.end(); ++iter) {
			Note::Ptr note(*iter);
			
			if (note->is_special())
				continue;
			
			// Skip template notes
			if (note->contains_tag (template_tag))
				continue;

			bool show = false;

			// Test for note.IsPinned first so that all of the pinned notes
			// are guaranteed to be included regardless of the size of the
			// list.
			if (note->is_pinned()) {
					show = true;
			} else if ((note->is_opened() && note->get_window()->is_mapped()) ||
								 (note->change_date() > days_ago) ||
								 (list_size < min_size)) {
				if (list_size <= max_size)
					show = true;
			}

			if (show) {
				item = Gtk::manage(new NoteMenuItem (note, true));
				// Add this widget to the menu (+insertion_point to add after new+search+...)
				m_tray_menu->insert(*item, list_size + insertion_point);
				// Keep track of this item so we can remove it later
				m_recent_notes.push_back(item);
				
				list_size++;
			}
		}

		Note::Ptr start = m_manager->find_by_uri(m_manager->start_note_uri());
		if (start) {
			item = Gtk::manage(new NoteMenuItem(start, false));
			if (menuOpensUpward) {
				m_tray_menu->insert (*item, list_size + insertion_point);
			}
			else {
				m_tray_menu->insert (*item, insertion_point);
			}
			m_recent_notes.push_back(item);

			list_size++;

			bool enable_keybindings = Preferences::get_preferences()->get<bool>(Preferences::ENABLE_KEYBINDINGS);
			if (enable_keybindings) {
				GConfKeybindingToAccel::add_accelerator (
					*item, Preferences::KEYBINDING_OPEN_START_HERE);
			}
		}


		// FIXME: Rearrange this stuff to have less wasteful reordering
		if (menuOpensUpward) {
			// Relocate common items to bottom of menu
			insertion_point -= 1;
			m_tray_menu->reorder_child (*searchNotesItem, list_size + insertion_point);
			for(std::list<Gtk::Widget*>::iterator widget = newNotePlaceholderWidgets.begin();
					widget != newNotePlaceholderWidgets.end(); ++widget) {
				Gtk::MenuItem *menuitem = dynamic_cast<Gtk::MenuItem*>(*widget);
				if(menuitem) {
					m_tray_menu->reorder_child (*menuitem, list_size + insertion_point);
				}
			}
			m_tray_menu->reorder_child (*newNoteItem, list_size + insertion_point);
			insertion_point = list_size;
		}

		Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem());
		m_tray_menu->insert(*separator, insertion_point);
		m_recent_notes.push_back(separator);
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
		set_tooltip_text(get_tooltip_text());
		
		// TODO
		// 			Tomboy.ExitingEvent += OnExit;
		signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::on_activate));
		signal_popup_menu().connect(sigc::mem_fun(*this, &TrayIcon::on_popup_menu));
	}

	void TrayIcon::show_menu(bool select_first_item)
	{
		if(m_context_menu) {
			DBG_OUT("context menu found");
		}
		m_tray->update_tray_menu(NULL);
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
			
//			Glib::RefPtr<Gdk::Screen> screen;
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
		DBG_OUT("get right click menu");
		if(m_context_menu) {
			DBG_OUT("menu already exists");
			return m_context_menu;
		}
		m_context_menu = new Gtk::Menu();
		DBG_OUT("creating menu");

		Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();
		m_context_menu->set_accel_group(accel_group);

		Gtk::ImageMenuItem * item;

		item = manage(new Gtk::ImageMenuItem(_("_Preferences"), true));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::PREFERENCES, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_preferences));
		m_context_menu->append(*item);

		item = manage(new Gtk::ImageMenuItem(_("_Help"), true));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_help_contents));
		m_context_menu->append(*item);

		item = manage(new Gtk::ImageMenuItem(_("_About GNote"), true));
		item->set_image(*manage(new Gtk::Image(Gtk::Stock::ABOUT, Gtk::ICON_SIZE_MENU)));
		item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_about));
		m_context_menu->append(*item);

		m_context_menu->append(*manage(new Gtk::SeparatorMenuItem()));

		item = manage(new Gtk::ImageMenuItem(_("_Quit"), true));
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

	bool TrayIcon::menu_opens_upward()
	{
			bool open_upwards = false;
			int val = 0;
//			Glib::RefPtr<Gdk::Screen> screen;
			GdkScreen *cscreen;

			Gdk::Rectangle area;
			GtkOrientation orientation;
//			get_geometry(screen, area, orientation);
			gtk_status_icon_get_geometry(gobj(), &cscreen, area.gobj(), &orientation);
			val = area.get_y();

			Gtk::Requisition menu_req = m_tray->tray_menu()->size_request();
			if (val + menu_req.height >= gdk_screen_get_height(cscreen))
				open_upwards = true;

			return open_upwards;
	}

	std::string get_tooltip_text()
	{
		std::string tip_text = _("GNote Notes");
		
		if (Preferences::get_preferences()->get<bool>(Preferences::ENABLE_KEYBINDINGS)) {
			std::string shortcut =
				GConfKeybindingToAccel::get_shortcut (
					Preferences::KEYBINDING_SHOW_NOTE_MENU);
			if (!shortcut.empty())
				tip_text += str(boost::format(" (%1%)") % shortcut);
		}
			
		return tip_text;
	}

	//
	// This is a helper to take the XKeybinding string from GConf, and
	// convert it to a widget accelerator label, so note menu items can
	// display their global X keybinding.
	//
	// FIXME: It would be totally sweet to allow setting the accelerator
	// visually through the menuitem, and have the new value be stored in
	// GConf.
	//
	Glib::RefPtr<Gtk::AccelGroup> GConfKeybindingToAccel::s_accel_group;

	
	Glib::RefPtr<Gtk::AccelGroup>
	GConfKeybindingToAccel::get_accel_group()
	{
		if(!s_accel_group) {
			s_accel_group = Gtk::AccelGroup::create();
		}
		return s_accel_group;
	}

	std::string GConfKeybindingToAccel::get_shortcut (const std::string & gconf_path)
	{
		try {
			std::string binding = Preferences::get_preferences()->get<std::string>(gconf_path);
			if (binding.empty() ||
					binding == "disabled") {
				return "";
			}
			
			boost::algorithm::replace_all(binding, "<", "");
			boost::algorithm::replace_all(binding, ">", "-");				
			
			return binding;
		} 
		catch(...)
		{
		}
		return "";
	}

	void GConfKeybindingToAccel::add_accelerator (Gtk::MenuItem & item, const std::string & gconf_path)
	{
		guint keyval;
		Gdk::ModifierType mods;
		
// TODO
//		if (Services.Keybinder.GetAccelKeys (gconf_path, out keyval, out mods)) {
//			item.add_accelerator ("activate",
//				                     get_accel_group(),
//				                     keyval,
//				                     mods,
//				                     Gtk::ACCEL_VISIBLE);
//		}
	}

	
}
