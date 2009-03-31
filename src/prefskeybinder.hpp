

#ifndef _PREFSKEYBINDER_HPP_
#define _PREFSKEYBINDER_HPP_

#include <memory>

namespace gnote {

class TrayIcon;
class NoteManager;


class PrefsKeybinder
{
public:
  typedef std::tr1::shared_ptr<PrefsKeybinder> Ptr;
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
