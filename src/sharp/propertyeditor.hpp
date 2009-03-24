



#ifndef __PROPERTYEDITOR_HPP_
#define __PROPERTYEDITOR_HPP_

#include <vector>

#include <gtkmm/entry.h>
#include <gtkmm/togglebutton.h>

namespace sharp {

	class PropertyEditorBase
	{
	public:
    virtual ~PropertyEditorBase();
		virtual void setup() = 0;

	protected:
		PropertyEditorBase(const char *key, Gtk::Widget &w);

		std::string m_key;
		Gtk::Widget &m_widget;
		sigc::connection m_connection;
  private:
    void static destroy_notify(gpointer data);
	};

	class PropertyEditor
			: public PropertyEditorBase
	{
	public:
		PropertyEditor(const char * key, Gtk::Entry &entry);

		virtual void setup();

	private:
		void on_changed();
	};

	class PropertyEditorBool
		: public PropertyEditorBase
	{
	public:
		PropertyEditorBool(const char * key, Gtk::ToggleButton &button);
		void add_guard(Gtk::Widget* w)
			{
				m_guarded.push_back(w);
			}

		virtual void setup();

	private:
		void guard(bool v);
		void on_changed();
		std::vector<Gtk::Widget*> m_guarded;
	};

}


#endif
