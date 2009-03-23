


#ifndef __NOTE_EDITOR_HPP_
#define __NOTE_EDITOR_HPP_

#include <glibmm/refptr.h>
#include <gtkmm/textview.h>

#include "preferences.hpp"

namespace gnote {

class NoteEditor
	: public Gtk::TextView
{
public:
	typedef Glib::RefPtr<NoteEditor> Ptr;

	NoteEditor(const Glib::RefPtr<Gtk::TextBuffer> & buffer);
	~NoteEditor();
	static int default_margin()
		{
			return 8;
		}

protected:
	virtual void on_drag_data_received (Glib::RefPtr<Gdk::DragContext> & context,
																			int x, int y,
																			const Gtk::SelectionData & selection_data,
																			guint info,	guint time);

private:
	Pango::FontDescription get_gnome_document_font_description();
	void on_font_setting_changed (Preferences*, GConfEntry* entry);
	static void on_font_setting_changed_gconf (GConfClient *, guint cnxid, GConfEntry* entry, gpointer data);
	void update_custom_font_setting();
	void modify_font_from_string (const std::string & fontString);
	bool key_pressed (GdkEventKey * ev);
	bool button_pressed (GdkEventButton * ev);

	guint                         m_gconf_notify;
};


}

#endif
