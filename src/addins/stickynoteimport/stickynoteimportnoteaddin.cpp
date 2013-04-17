/*
 * gnote
 *
 * Copyright (C) 2010-2011,2013 Aurimas Cernius
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

#include <fstream>
#include <string.h>

#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <glibmm/keyfile.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "stickynoteimportnoteaddin.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "addinmanager.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "utils.hpp"

namespace stickynote {

  using gnote::Preferences;
  using gnote::Note;


StickyNoteImportModule::StickyNoteImportModule()
{
  ADD_INTERFACE_IMPL(StickyNoteImportNoteAddin);
}

static const char * STICKY_XML_REL_PATH = "/.gnome2/stickynotes_applet";
static const char * STICKY_NOTE_QUERY = "//note";
static const char * DEBUG_NO_STICKY_FILE = "StickyNoteImporter: Sticky Notes XML file does not exist or is invalid!";
static const char * DEBUG_CREATE_ERROR_BASE = "StickyNoteImporter: Error while trying to create note \"%s\": %s";
static const char * DEBUG_FIRST_RUN_DETECTED = "StickyNoteImporter: Detecting that importer has never been run...";
//static const char * DEBUG_GCONF_SET_ERROR_BASE = "StickyNoteImporter: Error setting initial GConf first run key value: %s";

static const char * PREFS_FILE = "stickynoteimport.ini";

bool StickyNoteImportNoteAddin::s_static_inited = false;
bool StickyNoteImportNoteAddin::s_sticky_file_might_exist = true;
bool StickyNoteImportNoteAddin::s_sticky_file_existence_confirmed = false;
std::string StickyNoteImportNoteAddin::s_sticky_xml_path;


void StickyNoteImportNoteAddin::_init_static()
{
  if(!s_static_inited) {
    s_sticky_xml_path = Glib::get_home_dir() + STICKY_XML_REL_PATH;
    s_static_inited = true;
  }
}

void StickyNoteImportNoteAddin::initialize()
{
	// Don't add item to tools menu if Sticky Notes XML file does not
  // exist. Only check for the file once, since Initialize is called
  // for each note when Gnote starts up.
  if (s_sticky_file_might_exist) {
    if (s_sticky_file_existence_confirmed || sharp::file_exists (s_sticky_xml_path)) {
#if 0
      m_item = manage(new Gtk::ImageMenuItem (_("Import from Sticky Notes")));
      m_item->set_image(*manage(new Gtk::Image (Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU)));
      m_item->signal_activate().connect(
        sigc::bind(sigc::mem_fun(*this, &StickyNoteImportNoteAddin::import_button_clicked));
      m_item->show ();
      add_plugin_menu_item (m_item);
#endif
      s_sticky_file_existence_confirmed = true;
    } 
    else {
      s_sticky_file_might_exist = false;
      DBG_OUT("%s", DEBUG_NO_STICKY_FILE);
    }
  }
}



void StickyNoteImportNoteAddin::shutdown()
{
}



bool StickyNoteImportNoteAddin::want_to_run(gnote::NoteManager & manager)
{
  bool want_run = false;
  std::string prefs_file =
    Glib::build_filename(manager.get_addin_manager().get_prefs_dir(),
                         PREFS_FILE);

  try {
    Glib::KeyFile ini_file;
    ini_file.load_from_file(prefs_file);

    if(s_sticky_file_might_exist) {
      want_run = !ini_file.get_boolean("status", "first_run");
    }
  }
  catch(Glib::Error & e) {
    DBG_OUT("Failed to read key file %s: %s", prefs_file.c_str(), e.what().c_str());
    want_run = true;
  }

  return want_run;
}


bool StickyNoteImportNoteAddin::first_run(gnote::NoteManager & manager)
{
  std::string prefs_file(Glib::build_filename(
                           manager.get_addin_manager().get_prefs_dir(), 
                           PREFS_FILE));

  Glib::KeyFile ini_file;
  try {
    ini_file.load_from_file(prefs_file);
  }
  catch(Glib::Error&) {
    // ignore
  }

  bool firstRun = true;
  try {
    ini_file.get_boolean("status", "first_run");
  }
  catch(Glib::Error&) {
    // ignore
  }

  if(firstRun) {
    ini_file.set_boolean("status", "first_run", true);

    DBG_OUT("%s", DEBUG_FIRST_RUN_DETECTED);

    xmlDocPtr xml_doc = get_sticky_xml_doc();
    if(xml_doc) {
      // Don't show dialog when automatically importing
      import_notes(xml_doc, false, manager);
      xmlFreeDoc(xml_doc);
    }
    else {
      firstRun = false;
    }

    std::ofstream fout(prefs_file.c_str());
    if(fout) {
      fout << ini_file.to_data().c_str();
      fout.close();
    }
  }

  return firstRun;
}


xmlDocPtr StickyNoteImportNoteAddin::get_sticky_xml_doc()
{
  if (sharp::file_exists(s_sticky_xml_path)) {
    xmlDocPtr xml_doc = xmlReadFile(s_sticky_xml_path.c_str(), "UTF-8", 0);
    if(xml_doc == NULL) {
      DBG_OUT("%s", DEBUG_NO_STICKY_FILE);
    }
    return xml_doc;
  }
  else {
    DBG_OUT("%s", DEBUG_NO_STICKY_FILE);
    return NULL;
  }
}


void StickyNoteImportNoteAddin::import_button_clicked(gnote::NoteManager & manager)
{
  xmlDocPtr xml_doc = get_sticky_xml_doc();
  if(xml_doc) {
    import_notes (xml_doc, true, manager);
  }
  else {
    show_no_sticky_xml_dialog(s_sticky_xml_path);
  }
}


void StickyNoteImportNoteAddin::show_no_sticky_xml_dialog(const std::string & xml_path)
{
  show_message_dialog (
    _("No Sticky Notes found"),
    // %1% is a the file name
    str(boost::format(_("No suitable Sticky Notes file was found at \"%1%\"."))
        % xml_path), Gtk::MESSAGE_ERROR);
}


void StickyNoteImportNoteAddin::show_results_dialog(int numNotesImported, int numNotesTotal)
{
	show_message_dialog (
    _("Sticky Notes import completed"),
    // here %1% is the number of notes imported, %2% the total number of notes.
    str(boost::format(_("<b>%1%</b> of <b>%2%</b> Sticky Notes "
                        "were successfully imported.")) 
        % numNotesImported % numNotesTotal), Gtk::MESSAGE_INFO);
}


void StickyNoteImportNoteAddin::import_notes(xmlDocPtr xml_doc,
                                             bool showResultsDialog,
                                             gnote::NoteManager & manager)
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
      if (create_note_from_sticky ((const char*)stickyTitle, (const char*)stickyContent, manager)) {
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


bool StickyNoteImportNoteAddin::create_note_from_sticky(const char * stickyTitle,
                                                        const char* content,
                                                        gnote::NoteManager & manager)
{
  std::string preferredTitle = _("Sticky Note: ");
  preferredTitle += stickyTitle;
  std::string title = preferredTitle;

  int i = 2; // Append numbers to create unique title, starting with 2
  while (manager.find(title)){
    title = str(boost::format("%1% (#%2%)") % preferredTitle % i);
    i++;
  }

  std::string noteXml = str(boost::format("<note-content><note-title>%1%</note-title>\n\n"
                                          "%2%</note-content>")
                                          % gnote::utils::XmlEncoder::encode(title)
                                          % gnote::utils::XmlEncoder::encode(content));

  try {
    Note::Ptr newNote = manager.create(title, noteXml);
    newNote->queue_save (gnote::NO_CHANGE);
    newNote->save();
    return true;
  } 
  catch (const std::exception & e) {
    DBG_OUT(DEBUG_CREATE_ERROR_BASE, title.c_str(), e.what());
    return false;
  }
}


void StickyNoteImportNoteAddin::show_message_dialog(const std::string & title,
                                                   const std::string & message,
                                                   Gtk::MessageType messageType)
{
  gnote::utils::HIGMessageDialog dialog(NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        messageType,
                                        Gtk::BUTTONS_OK,
                                        title,
                                        message);
  dialog.run();
}

  
}
