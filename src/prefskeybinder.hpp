

#ifndef _PREFSKEYBINDER_HPP_
#define _PREFSKEYBINDER_HPP_


namespace gnote {

class TrayIcon;
class NoteManager;


class PrefsKeybinder
{
public:
	PrefsKeybinder(NoteManager & manager, TrayIcon & trayicon)
		: m_manager(manager)
		, m_trayicon(trayicon)
		{
		}
private:
	NoteManager & m_manager;
	TrayIcon & m_trayicon;
};

}

#endif
