

#ifndef _GNOTE_UTILS_HPP__
#define _GNOTE_UTILS_HPP__

#include <sigc++/signal.h>

#include <gdkmm/pixbuf.h>
#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textiter.h>
#include <gtkmm/textmark.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>

#include "sharp/exception.hpp"
#include "sharp/uri.hpp"
#include "libtomboy/tomboyutil.h"

namespace sharp {
  class DateTime;
}

namespace gnote {
	namespace utils {

		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & , int );

		
		void popup_menu(Gtk::Menu &menu, const GdkEventButton *);
		void popup_menu(Gtk::Menu &menu, const GdkEventButton *, Gtk::Menu::SlotPositionCalc calc);

		void show_help(const std::string & filename, const std::string & link_id,
									 GdkScreen *screen, Gtk::Window *parent);

    std::string get_pretty_print_date(const sharp::DateTime &, bool show_time);

		class GlobalKeybinder
		{
		public:
			
			GlobalKeybinder(const Glib::RefPtr<Gtk::AccelGroup> & accel_group)
				: m_accel_group(accel_group)
				{
					m_fake_menu.set_accel_group(accel_group);
				}

			void add_accelerator(const sigc::slot<void> & , guint, Gdk::ModifierType, 
													 Gtk::AccelFlags);
			
		private:
			Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
			Gtk::Menu m_fake_menu;
		};


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


    class UriList
      : public std::list<sharp::Uri>
    {
    public:
//      UriList(const NoteList & notes);
      UriList(const std::string & data);
      UriList(const Gtk::SelectionData & selection);
      std::string to_string();
      std::list<std::string> get_local_paths();
      
    private:
      void load_from_string(const std::string & data);
      
    };

    class XmlEncoder
    {
    public:
      static std::string encode(const std::string & source);
    };

		class XmlDecoder
		{
		public:
			static std::string decode(const std::string & source);

		};


		class TextRange
		{
		public:
			TextRange(const Gtk::TextIter & start,
								const Gtk::TextIter & end) throw(sharp::Exception);
			const Glib::RefPtr<Gtk::TextBuffer> & buffer() const
				{
					return m_buffer;
				}
			const std::string text() const
				{
					return start().get_text(end());
				}
			int length() const
				{
					return text().size();
				}
			Gtk::TextIter start() const;
			void set_start(const Gtk::TextIter &);
			Gtk::TextIter end() const;
			void set_end(const Gtk::TextIter &);	
			void erase();
			void destroy();
			void remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag);
		private:
			Glib::RefPtr<Gtk::TextBuffer> m_buffer;
			Glib::RefPtr<Gtk::TextMark>   m_start_mark;
			Glib::RefPtr<Gtk::TextMark>   m_end_mark;
		};


		class TextTagEnumerator
		{
		public:
			TextTagEnumerator(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
												const Glib::RefPtr<Gtk::TextTag> & tag);
			const TextRange & current() const
				{
					return m_range;
				}
			bool move_next();
			void reset()
				{
					m_buffer->move_mark(m_mark, m_buffer->begin());
				}
		private:
			Glib::RefPtr<Gtk::TextBuffer> m_buffer;
			Glib::RefPtr<Gtk::TextTag>    m_tag;
			Glib::RefPtr<Gtk::TextMark>   m_mark;
			TextRange                     m_range;
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
