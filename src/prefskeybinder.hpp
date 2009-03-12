

#ifndef _PREFSKEYBINDER_HPP_
#define _PREFSKEYBINDER_HPP_

#include "notemanager.hpp"

namespace gnote {

class TrayIcon;


class PrefsKeybinder
{
public:
	PrefsKeybinder(const boost::shared_ptr<NoteManager> & manager, TrayIcon & trayicon)
		: m_manager(manager)
		, m_trayicon(trayicon)
		{
		}
private:
	boost::shared_ptr<NoteManager> m_manager;
	TrayIcon & m_trayicon;
};

}

#endif
