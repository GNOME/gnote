

#include <iostream>

#include <boost/format.hpp>
#include <boost/bind.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <libxml++/parsers/textreader.h>

#include "utils.hpp"
#include "debug.hpp"

namespace gnote {
	namespace utils {

		namespace {
			void get_menu_position (Gtk::Menu * menu,
															int & x,
															int & y,
															bool & push_in)
			{
				if (menu->get_attach_widget() == NULL ||
						menu->get_attach_widget()->get_window() == NULL) {
					// Prevent null exception in weird cases
					x = 0;
					y = 0;
					push_in = true;
					return;
				}
			
				menu->get_attach_widget()->get_window()->get_origin(x, y);
				x += menu->get_attach_widget()->get_allocation().get_x();
				
				Gtk::Requisition menu_req = menu->size_request();
				if (y + menu_req.height >= menu->get_attach_widget()->get_screen()->get_height()) {
					y -= menu_req.height;
				}
				else {
					y += menu->get_attach_widget()->get_allocation().get_height();
				}
				
				push_in = true;
			}


			void deactivate_menu(Gtk::Menu *menu)
			{
				menu->popdown();
				if(menu->get_attach_widget()) {
					menu->get_attach_widget()->set_state(Gtk::STATE_NORMAL);
				}
			}
		}

		void popup_menu(Gtk::Menu &menu, const GdkEventButton * ev)
		{
			popup_menu(menu, ev, boost::bind(&get_menu_position, &menu, _1, _2, _3));
		}


		void popup_menu(Gtk::Menu &menu, const GdkEventButton * ev, 
										Gtk::Menu::SlotPositionCalc calc)
		{
			menu.signal_deactivate().connect(sigc::bind(&deactivate_menu, &menu));
			menu.popup(calc, (ev ? ev->button : 0), 
									(ev ? ev->time : gtk_get_current_event_time()));
			if(menu.get_attach_widget()) {
				menu.get_attach_widget()->set_state(Gtk::STATE_SELECTED);
			}
		}


		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & name, int size)
		{
			try {
				return Gtk::IconTheme::get_default()->load_icon(name, size);
			}
			catch(const Glib::Exception & e)
			{
				std::cout << e.what().c_str() << std::endl;
			}
			return Glib::RefPtr<Gdk::Pixbuf>();
		}

		void show_help(const std::string & filename, const std::string & link_id,
									 GdkScreen *screen, Gtk::Window *parent)
		{
			std::string uri = "ghelp:" + filename;
			if(!link_id.empty()) {
				uri += link_id;
			}
			GError *error = NULL;

			if(!gtk_show_uri (screen, uri.c_str(), gtk_get_current_event_time (), &error)) {
				
				std::string message = _("The \"GNote Notes Manual\" could "
																"not be found.  Please verify "
																"that your installation has been "
																"completed successfully.");
				HIGMessageDialog dialog(parent,
																GTK_DIALOG_DESTROY_WITH_PARENT,
																Gtk::MESSAGE_ERROR,
																Gtk::BUTTONS_OK,
																_("Help not found"),
																message);
				dialog.run();
				if(error) {
					g_error_free(error);
				}
			}
		}



		HIGMessageDialog::HIGMessageDialog(Gtk::Window *parent,
																			 GtkDialogFlags flags, Gtk::MessageType msg_type, 
																			 Gtk::ButtonsType btn_type, const Glib::ustring & header,
																			 const Glib::ustring & msg)
			: Gtk::Dialog()
		{
			set_has_separator(false);
			set_border_width(5);
			set_resizable(false);
			set_title("");

			get_vbox()->set_spacing(12);
			get_action_area()->set_layout(Gtk::BUTTONBOX_END);

			m_accel_group = Glib::RefPtr<Gtk::AccelGroup>(Gtk::AccelGroup::create());
			add_accel_group(m_accel_group);

			Gtk::HBox *hbox = manage(new Gtk::HBox (false, 12));
			hbox->set_border_width(5);
			hbox->show();
			get_vbox()->pack_start(*hbox, false, false, 0);

			switch (msg_type) {
			case Gtk::MESSAGE_ERROR:
				m_image = new Gtk::Image (Gtk::Stock::DIALOG_ERROR,
																	Gtk::ICON_SIZE_DIALOG);
				break;
			case Gtk::MESSAGE_QUESTION:
				m_image = new Gtk::Image (Gtk::Stock::DIALOG_QUESTION,
																	Gtk::ICON_SIZE_DIALOG);
				break;
			case Gtk::MESSAGE_INFO:
				m_image = new Gtk::Image (Gtk::Stock::DIALOG_INFO,
																	Gtk::ICON_SIZE_DIALOG);
				break;
			case Gtk::MESSAGE_WARNING:
				m_image = new Gtk::Image (Gtk::Stock::DIALOG_WARNING,
																	Gtk::ICON_SIZE_DIALOG);
				break;
			default:
				m_image = new Gtk::Image ();
				break;
			}

			if (m_image) {
				Gtk::manage(m_image);
				m_image->show();
				m_image->property_yalign().set_value(0);
				hbox->pack_start(*m_image, false, false, 0);
			}

			Gtk::VBox *label_vbox = manage(new Gtk::VBox (false, 0));
			label_vbox->show();
			hbox->pack_start(*label_vbox, true, true, 0);

			std::string title = str(boost::format("<span weight='bold' size='larger'>%1%"
																						"</span>\n") % header);

			Gtk::Label *label;

			label = manage(new Gtk::Label (title));
			label->set_use_markup(true);
			label->set_justify(Gtk::JUSTIFY_LEFT);
			label->set_line_wrap(true);
			label->set_alignment (0.0f, 0.5f);
			label->show();
			label_vbox->pack_start(*label, false, false, 0);

			label = manage(new Gtk::Label(msg));
			label->set_use_markup(true);
			label->set_justify(Gtk::JUSTIFY_LEFT);
			label->set_line_wrap(true);
			label->set_alignment (0.0f, 0.5f);
			label->show();
			label_vbox->pack_start(*label, false, false, 0);
			
			m_extra_widget_vbox = manage(new Gtk::VBox (false, 0));
			m_extra_widget_vbox->show();
			label_vbox->pack_start(*m_extra_widget_vbox, true, true, 12);

			switch (btn_type) {
			case Gtk::BUTTONS_NONE:
				break;
			case Gtk::BUTTONS_OK:
				add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
				break;
			case Gtk::BUTTONS_CLOSE:
				add_button (Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE, true);
				break;
			case Gtk::BUTTONS_CANCEL:
				add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, true);
				break;
			case Gtk::BUTTONS_YES_NO:
				add_button (Gtk::Stock::NO, Gtk::RESPONSE_NO, false);
				add_button (Gtk::Stock::YES, Gtk::RESPONSE_YES, true);
				break;
			case Gtk::BUTTONS_OK_CANCEL:
				add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, false);
				add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
				break;
			}

			if (parent){
				set_transient_for(*parent);
			}

			if ((flags & GTK_DIALOG_MODAL) != 0) {
				set_modal(true);
			}

			if ((flags & GTK_DIALOG_DESTROY_WITH_PARENT) != 0) {
				property_destroy_with_parent().set_value(true);
			}
		}


		void HIGMessageDialog::add_button(const Gtk::BuiltinStockID& stock_id, 
																			 Gtk::ResponseType resp, bool is_default)
		{
			Gtk::Button *button = manage(new Gtk::Button (stock_id));
			button->property_can_default().set_value(true);
			
			add_button(button, resp, is_default);
		}

		void HIGMessageDialog::add_button (const Glib::RefPtr<Gdk::Pixbuf> & pixbuf, 
																			 const Glib::ustring & label_text, 
																			 Gtk::ResponseType resp, bool is_default)
		{
			Gtk::Button *button = manage(new Gtk::Button());
			Gtk::Image *image = manage(new Gtk::Image(pixbuf));
			// NOTE: This property is new to GTK+ 2.10, but we don't
			//       really need the line because we're just setting
			//       it to the default value anyway.
			//button.ImagePosition = Gtk::PositionType.Left;
			button->set_image(*image);
			button->set_label(label_text);
			button->set_use_underline(true);
			button->property_can_default().set_value(true);
			
			add_button (button, resp, is_default);
		}
		
		void HIGMessageDialog::add_button (Gtk::Button *button, Gtk::ResponseType resp, bool is_default)
		{
			button->show();

			add_action_widget (*button, resp);

			if (is_default) {
				set_default_response(resp);
				button->add_accelerator ("activate", m_accel_group,
																 GDK_Escape, (Gdk::ModifierType)0,
																 Gtk::ACCEL_VISIBLE);
			}
		}




		const std::string XmlDecoder::decode(const std::string & source)
		{
			// TODO there is probably better than a std::string for that.
			// this will do for now.
			std::string builder;

			xmlpp::TextReader xml(source);

			while (xml.read ()) {
				switch (xml.get_node_type()) {
				case xmlpp::TextReader::Text:
				case xmlpp::TextReader::Whitespace:
					builder += xml.get_value();
					break;
				default:
					break;
				}
			}

			xml.close ();

			return builder;
		}


		InterruptableTimeout::~InterruptableTimeout()
		{
			cancel();
		}


		bool InterruptableTimeout::callback(InterruptableTimeout* self)
		{
			if(self)
				return self->timeout_expired();
			return false;
		}

		void InterruptableTimeout::reset(guint timeout_millis)
		{
			cancel();
			m_timeout_id = g_timeout_add(timeout_millis, (GSourceFunc)callback, this);
		}

		void InterruptableTimeout::cancel()
		{
			if(m_timeout_id != 0) {
				g_source_remove(m_timeout_id);
				m_timeout_id = 0;
			}
		}

		bool InterruptableTimeout::timeout_expired()
		{
			signal_timeout();
			m_timeout_id = 0;
			return false;
		}

		ToolMenuButton::ToolMenuButton(Gtk::Toolbar& toolbar, const Gtk::BuiltinStockID& stock_image, 
																	 const Glib::ustring & label, Gtk::Menu & menu)
			: Gtk::ToggleToolButton(label)
			,	m_menu(menu)
		{
			set_icon_widget(*manage(new Gtk::Image(stock_image, toolbar.get_icon_size())));
			property_can_focus().set_value(true);
			gtk_menu_attach_to_widget(menu.gobj(), static_cast<Gtk::Widget*>(this)->gobj(),
																NULL);
//			menu.attach_to_widget(*this);
			menu.signal_deactivate().connect(sigc::mem_fun(*this, &ToolMenuButton::release_button));
			show_all();
		}


		bool ToolMenuButton::on_button_press_event(GdkEventButton *ev)
		{
			popup_menu(m_menu, ev);
			return true;
		}

		void ToolMenuButton::on_clicked()
		{
			m_menu.select_first(true);
			popup_menu(m_menu, NULL);
		}

		bool ToolMenuButton::on_mnemonic_activate(bool group_cycling)
		{
			// ToggleButton always grabs focus away from the editor,
			// so reimplement Widget's version, which only grabs the
			// focus if we are group cycling.
			if (!group_cycling) {
				activate();
			} 
			else if (can_focus()) {
				grab_focus();
			}

			return true;
		}


		void ToolMenuButton::release_button()
		{
			set_active(false);
		}
		
		

	}
}
