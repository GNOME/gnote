

#ifndef _GNOTE_UTILS_HPP__
#define _GNOTE_UTILS_HPP__

#include <sigc++/signal.h>

#include <gdkmm/pixbuf.h>
#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>

#include "libtomboy/tomboyutil.h"

namespace gnote {
	namespace utils {

		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & , int );

		
		void popup_menu(Gtk::Menu &menu, const GdkEventButton *);
		void popup_menu(Gtk::Menu &menu, const GdkEventButton *, Gtk::Menu::SlotPositionCalc calc);

		void show_help(const std::string & filename, const std::string & link_id,
									 GdkScreen *screen, Gtk::Window *parent);


		class HIGMessageDialog
			: public Gtk::Dialog 
		{
		public:
			HIGMessageDialog(Gtk::Window *, GtkDialogFlags flags, Gtk::MessageType msg_type, 
											 Gtk::ButtonsType btn_type, const Glib::ustring & header,
											 const Glib::ustring & msg);
			void add_button(const Gtk::BuiltinStockID& stock_id, 
											 Gtk::ResponseType response, bool is_default);
			void add_button(const Glib::RefPtr<Gdk::Pixbuf> & pixbuf, 
											const Glib::ustring & label_text, 
											Gtk::ResponseType response, bool is_default);
			void add_button(Gtk::Button *button, Gtk::ResponseType response, bool is_default);

		private:
			Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
			Gtk::VBox *m_extra_widget_vbox;
			Gtk::Widget *m_extra_widget;
			Gtk::Image *m_image;

		};

		class XmlDecoder
		{
		public:
			static const std::string decode(const std::string & source);

		};


		class InterruptableTimeout
		{
		public:
			InterruptableTimeout()
				: m_timeout_id(0)
				{
				}
			~InterruptableTimeout();
			void reset(guint timeout_millis);
			void cancel();
			sigc::signal<void> signal_timeout;
		private:
			static bool callback(InterruptableTimeout*);
			bool timeout_expired();
			guint m_timeout_id;
		};

		class ForcedPresentWindow 
			: public Gtk::Window
		{
		public:
			ForcedPresentWindow(const Glib::ustring & title)
				: Gtk::Window()
				{
					set_title(title);
				}

			void present()
				{
					::tomboy_window_present_hardcore(gobj());
				}
		};

		class ToolMenuButton
			: public Gtk::ToggleToolButton
		{
		public:
			ToolMenuButton(Gtk::Toolbar& toolbar, const Gtk::BuiltinStockID& stock_image, 
										 const Glib::ustring & label, Gtk::Menu & menu);
			virtual bool on_button_press_event(GdkEventButton *);
			virtual void on_clicked();
			virtual bool on_mnemonic_activate(bool group_cycling);

		private:
			Gtk::Menu &m_menu;
			void release_button();				
		};


	}
}

#endif
