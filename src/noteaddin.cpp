


#include "noteaddin.hpp"
#include "notewindow.hpp"
#include "sharp/foreach.hpp"


namespace gnote {

  void NoteAddin::initialize(const Note::Ptr & note)
  {
    m_note = note;
    m_note_opened_cid = m_note->signal_opened().connect(
      sigc::mem_fun(*this, &NoteAddin::on_note_opened_event));
    initialize();
    if(m_note->is_opened()) {
      on_note_opened();
    }
  }


  void NoteAddin::dispose(bool disposing)
  {
    if (disposing) {
      foreach (Gtk::Widget * item, m_tools_menu_items) {
        delete item;
      }

      foreach (Gtk::Widget * item, m_text_menu_items) {
        delete item;
      }
				
      foreach (const ToolItemMap::value_type & item, m_toolbar_items) {
        delete item.first;
      }

      shutdown ();
    }
    
    m_note_opened_cid.disconnect();
    m_note = Note::Ptr();
  }

  void NoteAddin::on_note_opened_event(Note & )
  {
    on_note_opened();
    NoteWindow * window = get_window();

    foreach (Gtk::Widget *item, m_tools_menu_items) {
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->plugin_menu()))
        window->plugin_menu()->add (*item);
    }

    foreach (Gtk::Widget *item, m_text_menu_items) {
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->text_menu())) {
        window->text_menu()->add (*item);
        window->text_menu()->reorder_child(*(Gtk::MenuItem*)item, 7);
      }
    }
			
    foreach (const ToolItemMap::value_type & item, m_toolbar_items) {
      if ((item.first->get_parent() == NULL) ||
          (item.first->get_parent() != window->toolbar())) {
        window->toolbar()->insert (*item.first, item.second);
      }
    }
  }


  void NoteAddin::add_plugin_menu_item (Gtk::MenuItem *item)
  {
    if (is_disposing())
      throw sharp::Exception ("Plugin is disposing already");

    m_tools_menu_items.push_back (item);

    if (m_note->is_opened()) {
      get_window()->plugin_menu()->add (*item);
    }
  }
		
  void NoteAddin::add_tool_item (Gtk::ToolItem *item, int position)
  {
    if (is_disposing())
      throw sharp::Exception ("Add-in is disposing already");
				
    m_toolbar_items [item] = position;
			
    if (m_note->is_opened()) {
      get_window()->toolbar()->insert (*item, position);
    }
  }

  void NoteAddin::add_text_menu_item (Gtk::MenuItem * item)
  {
    if (is_disposing())
      throw sharp::Exception ("Plugin is disposing already");

    m_text_menu_items.push_back(item);

    if (m_note->is_opened()) {
      get_window()->text_menu()->add (*item);
      get_window()->text_menu()->reorder_child (*item, 7);
    }
  }

  
}
