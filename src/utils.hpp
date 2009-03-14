

#ifndef _GNOTE_UTILS_HPP__
#define _GNOTE_UTILS_HPP__

#include <gdkmm/pixbuf.h>
#include <gtkmm/dialog.h>
#include <gtkmm/menu.h>
#include <gtkmm/messagedialog.h>


namespace gnote {
	namespace utils {

		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & , int );

		void popup_menu(Gtk::Menu *menu, const GdkEventButton *, Gtk::Menu::SlotPositionCalc calc);

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

	}
}

#endif
