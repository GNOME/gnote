/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "stickynoteimportnoteaddin.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "utils.hpp"

namespace stickynote {

  using gnote::Preferences;
  using gnote::Note;


StickNoteImportModule::StickNoteImportModule()
{
  ADD_INTERFACE_IMPL(StickNoteImportNoteAddin);
}
const char * StickNoteImportModule::id() const
{
  return "StickNoteImportAddin";
}
const char * StickNoteImportModule::name() const
{
  return _("Sticky Notes Importer");
}
const char * StickNoteImportModule::description() const
{
  return _("Import your notes from the Sticky Notes applet.");
}
const char * StickNoteImportModule::authors() const
{
  return _("Hubert Figuiere and the Tomboy Project");
}
const char * StickNoteImportModule::category() const
{
  return "Tools";
}
const char * StickNoteImportModule::version() const
{
  return "0.1";
}

static const char * STICKY_XML_REL_PATH = "/.gnome2/stickynotes_applet";
static const char * STICKY_NOTE_QUERY = "//note";
static const char * DEBUG_NO_STICKY_FILE = "StickyNoteImporter: Sticky Notes XML file does not exist or is invalid!";
static const char * DEBUG_CREATE_ERROR_BASE = "StickyNoteImporter: Error while trying to create note \"%s\": %s";
static const char * DEBUG_FIRST_RUN_DETECTED = "StickyNoteImporter: Detecting that importer has never been run...";
//static const char * DEBUG_GCONF_SET_ERROR_BASE = "StickyNoteImporter: Error setting initial GConf first run key value: %s";


bool StickNoteImportNoteAddin::s_static_inited = false;
bool StickNoteImportNoteAddin::s_sticky_file_might_exist = true;
bool StickNoteImportNoteAddin::s_sticky_file_existence_confirmed = false;
std::string StickNoteImportNoteAddin::s_sticky_xml_path;


void StickNoteImportNoteAddin::_init_static()
{
  if(!s_static_inited) {
    s_sticky_xml_path = Glib::get_home_dir() + STICKY_XML_REL_PATH;
    s_static_inited = true;
  }
}

void StickNoteImportNoteAddin::initialize()
{
	// Don't add item to tools menu if Sticky Notes XML file does not
  // exist. Only check for the file once, since Initialize is called
  // for each note when Tomboy starts up.
  if (s_sticky_file_might_exist) {
    if (s_sticky_file_existence_confirmed || sharp::file_exists (s_sticky_xml_path)) {
      m_item = manage(new Gtk::ImageMenuItem (_("Import from Sticky Notes")));
      m_item->set_image(*manage(new Gtk::Image (Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU)));
      m_item->signal_activate().connect(
        sigc::mem_fun(*this, &StickNoteImportNoteAddin::import_button_clicked));
      m_item->show ();
      add_plugin_menu_item (m_item);

      s_sticky_file_existence_confirmed = true;
      check_for_first_run();
    } 
    else {
      s_sticky_file_might_exist = false;
      DBG_OUT(DEBUG_NO_STICKY_FILE);
    }
  }

}



void StickNoteImportNoteAddin::shutdown()
{
}


void StickNoteImportNoteAddin::on_note_opened()
{
}


void StickNoteImportNoteAddin::check_for_first_run()
{
  bool firstRun = Preferences::obj().get<bool> (Preferences::STICKYNOTEIMPORTER_FIRST_RUN);

  if (firstRun) {
    Preferences::obj().set<bool> (Preferences::STICKYNOTEIMPORTER_FIRST_RUN, false);

    DBG_OUT(DEBUG_FIRST_RUN_DETECTED);

    xmlDocPtr xml_doc = get_sticky_xml_doc();
    if (xml_doc) {
      // Don't show dialog when automatically importing
      import_notes (xml_doc, false);
      xmlFreeDoc(xml_doc);
    }
  }

}


xmlDocPtr StickNoteImportNoteAddin::get_sticky_xml_doc()
{
  if (sharp::file_exists(s_sticky_xml_path)) {
    xmlDocPtr xml_doc = xmlReadFile(s_sticky_xml_path.c_str(), "UTF-8", 0);
    if(xml_doc == NULL) {
      DBG_OUT(DEBUG_NO_STICKY_FILE);
    }
    return xml_doc;
  }
  else {
    DBG_OUT(DEBUG_NO_STICKY_FILE);
    return NULL;
  }
}


void StickNoteImportNoteAddin::import_button_clicked()
{
  xmlDocPtr xml_doc = get_sticky_xml_doc();
  if(xml_doc) {
    import_notes (xml_doc, true);
  }
  else {
    show_no_sticky_xml_dialog(s_sticky_xml_path);
  }
}


void StickNoteImportNoteAddin::show_no_sticky_xml_dialog(const std::string & xml_path)
{
  show_message_dialog (
    _("No Sticky Notes found"),
    // %1% is a the file name
    str(boost::format(_("No suitable Sticky Notes file was found at \"%1%\"."))
        % xml_path), Gtk::MESSAGE_ERROR);
}


void StickNoteImportNoteAddin::show_results_dialog(int numNotesImported, int numNotesTotal)
{
	show_message_dialog (
    _("Sticky Notes import completed"),
    // here %1% is the number of notes imported, %2% the total number of notes.
    str(boost::format(_("<b>%1%</b> of <b>%2%</b> Sticky Notes "
                        "were successfully imported.")) 
        % numNotesImported % numNotesTotal), Gtk::MESSAGE_INFO);
}


void StickNoteImportNoteAddin::import_notes(xmlDocPtr xml_doc, bool showResultsDialog)
{
  xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
  if(!root_node) {
    if (showResultsDialog)
      show_no_sticky_xml_dialog(s_sticky_xml_path);
    return;
  }
  sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(root_node, STICKY_NOTE_QUERY);

  int numSuccessful = 0;
  const xmlChar * defaultTitle = (const xmlChar *)_("Untitled");

  for(sharp::XmlNodeSet::const_iterator iter = nodes.begin();
      iter != nodes.end(); ++iter) {

    xmlNodePtr node = *iter;
    const xmlChar * stickyTitle;
    xmlChar * titleAttr = xmlGetProp(node, (const xmlChar*)"title");
    if(titleAttr) {
      stickyTitle = titleAttr;
    }
    else {
      stickyTitle = defaultTitle;
    }
    xmlChar * stickyContent = xmlNodeGetContent(node);

    if(stickyContent) {
      if (create_note_from_sticky ((const char*)stickyTitle, (const char*)stickyContent)) {
        numSuccessful++;
      }
      xmlFree(stickyContent);
    }

    if(titleAttr) {
      xmlFree(titleAttr);
    }
  }

  if (showResultsDialog) {
    show_results_dialog (numSuccessful, nodes.size());
  }
}


bool StickNoteImportNoteAddin::create_note_from_sticky(const char * stickyTitle, 
                                                       const char* content)
{
  // There should be no XML in the content
  // TODO: Report the error in the results dialog
  //  (this error should only happen if somebody has messed with the XML file)
  if(strchr(content, '>') || strchr(content, '<')) {
    DBG_OUT(DEBUG_CREATE_ERROR_BASE, stickyTitle,
            "Invalid characters in note XML");
    return false;
  }

  std::string preferredTitle = _("Sticky Note: ");
  preferredTitle += stickyTitle;
  std::string title = preferredTitle;

  int i = 2; // Append numbers to create unique title, starting with 2
  while (manager().find(title)){
    title = str(boost::format("%1% (#%2%)") % preferredTitle % i);
    i++;
  }
  
  std::string noteXml = str(boost::format("<note-content><note-title>%1%</note-title>\n\n"
                                          "%2%</note-content>") % title % content);

  try {
    Note::Ptr newNote = manager().create(title, noteXml);
    newNote->queue_save (Note::NO_CHANGE);
    newNote->save();
    return true;
  } 
  catch (const std::exception & e) {
    DBG_OUT(DEBUG_CREATE_ERROR_BASE, title.c_str(), e.what());
    return false;
  }
}


void StickNoteImportNoteAddin::show_message_dialog(const std::string & title, 
                                                   const std::string & message,
                                                   Gtk::MessageType messageType)
{
  gnote::utils::HIGMessageDialog dialog(get_window(),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        messageType,
                                        Gtk::BUTTONS_OK,
                                        title,
                                        message);
  dialog.run();
}

  
}
