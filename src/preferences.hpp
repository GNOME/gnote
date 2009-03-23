


#ifndef __PREFERENCES_HPP_
#define __PREFERENCES_HPP_

#include <string>
#include <gconf/gconf-client.h>
#include <sigc++/signal.h>

namespace gnote {

	class Preferences 
	{
	public:
		typedef sigc::signal<void, Preferences*, GConfEntry*> NotifyChangeSignal;
		static const char *ENABLE_SPELLCHECKING;
		static const char *ENABLE_WIKIWORDS;
		static const char *ENABLE_CUSTOM_FONT;
		static const char *ENABLE_KEYBINDINGS;
		static const char *ENABLE_STARTUP_NOTES;
		static const char *ENABLE_AUTO_BULLETED_LISTS;
		static const char *ENABLE_ICON_PASTE;
		static const char *ENABLE_CLOSE_NOTE_ON_ESCAPE;

		static const char *START_NOTE_URI;
		static const char *CUSTOM_FONT_FACE;
		static const char *MENU_NOTE_COUNT;
		static const char *MENU_PINNED_NOTES;

		static const char *KEYBINDING_SHOW_NOTE_MENU;
		static const char *KEYBINDING_OPEN_START_HERE;
		static const char *KEYBINDING_CREATE_NEW_NOTE;
		static const char *KEYBINDING_OPEN_SEARCH;
		static const char *KEYBINDING_OPEN_RECENT_CHANGES;

		static const char *EXPORTHTML_LAST_DIRECTORY;
		static const char *EXPORTHTML_EXPORT_LINKED;
		static const char *EXPORTHTML_EXPORT_LINKED_ALL;

		static const char *STICKYNOTEIMPORTER_FIRST_RUN;

		static const char *SYNC_CLIENT_ID;
		static const char *SYNC_LOCAL_PATH;
		static const char *SYNC_SELECTED_SERVICE_ADDIN;
		static const char *SYNC_CONFIGURED_CONFLICT_BEHAVIOR;

		static const char *INSERT_TIMESTAMP_FORMAT;
		
		static const char *SEARCH_WINDOW_X_POS;
		static const char *SEARCH_WINDOW_Y_POS;
		static const char *SEARCH_WINDOW_WIDTH;
		static const char *SEARCH_WINDOW_HEIGHT;


		~Preferences();

		template<typename T>
		void set(const char * p, const T&);

		template<typename T>
		void set(const std::string & p, const T&v)
			{
				set<T>(p.c_str(),v);
			}


		template<typename T>
		T get(const char * p);

		template<typename T>
		T get(const std::string & p)
			{
				return get<T>(p.c_str());
			}

		template<typename T>
		T get_default(const char * p);

		template<typename T>
		T get_default(const std::string & p)
			{
				return get_default<T>(p.c_str());
			}
		
		static Preferences * get_preferences();

		sigc::signal<void, Preferences*, GConfEntry*> & signal_setting_changed()
			{
				return  m_signal_setting_changed;
			}

		// this is very hackish. maybe I should just use gconfmm
		guint add_notify(const char *ns, GConfClientNotifyFunc func, gpointer data);
		void remove_notify(guint);
	private:
		Preferences();
		Preferences(const Preferences &); // non implemented
		static Preferences *s_instance;
		GConfClient        *m_client;
		guint               m_cnx;

		static void gconf_notify_glue(GConfClient *client, guint cid, GConfEntry *entry,
																	Preferences * self);
		NotifyChangeSignal m_signal_setting_changed;
	};


}


#endif
