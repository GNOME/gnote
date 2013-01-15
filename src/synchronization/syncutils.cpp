/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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


#include <algorithm>
#include <fstream>
#include <vector>

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "debug.hpp"
#include "syncutils.hpp"
#include "utils.hpp"
#include "sharp/files.hpp"
#include "sharp/process.hpp"
#include "sharp/string.hpp"
#include "sharp/xmlreader.hpp"

namespace gnote {
namespace sync {

  NoteUpdate::NoteUpdate(const std::string & xml_content, const std::string & title, const std::string & uuid, int latest_revision)
  {
    m_xml_content = xml_content;
    m_title = title;
    m_uuid = uuid;
    m_latest_revision = latest_revision;

    // TODO: Clean this up (and remove title parameter?)
    if(m_xml_content.length() > 0) {
      sharp::XmlReader xml;
      xml.load_buffer(m_xml_content);
      //xml.Namespaces = false;

      while(xml.read()) {
        if(xml.get_node_type() == XML_READER_TYPE_ELEMENT) {
          if(xml.get_name() == "title") {
            m_title = xml.read_string();
          }
        }
      }
    }
  }


  bool NoteUpdate::basically_equal_to(const Note::Ptr & existing_note)
  {
    // NOTE: This would be so much easier if NoteUpdate
    //       was not just a container for a big XML string
    sharp::XmlReader xml;
    xml.load_buffer(m_xml_content);
    std::auto_ptr<NoteData> update_data(NoteArchiver::obj().read(xml, m_uuid));
    xml.close();

    // NOTE: Mostly a hack to ignore missing version attributes
    std::string existing_inner_content = get_inner_content(existing_note->data().text());
    std::string update_inner_content = get_inner_content(update_data->text());

    return existing_inner_content == update_inner_content &&
           existing_note->data().title() == update_data->title() &&
           compare_tags(existing_note->data().tags(), update_data->tags());
           // TODO: Compare open-on-startup, pinned
  }


  std::string NoteUpdate::get_inner_content(const std::string & full_content_element) const
  {
    sharp::XmlReader xml;
    xml.load_buffer(full_content_element);
    if(xml.read() && xml.get_name() == "note-content") {
      return xml.read_inner_xml();
    }
    return "";
  }


  bool NoteUpdate::compare_tags(const std::map<std::string, Tag::Ptr> set1, const std::map<std::string, Tag::Ptr> set2) const
  {
    if(set1.size() != set2.size()) {
      return false;
    }
    for(std::map<std::string, Tag::Ptr>::const_iterator iter = set1.begin(); iter != set1.end(); ++iter) {
      if(set2.find(iter->first) == set2.end()) {
        return false;
      }
    }
    return true;
  }


  const char *SyncUtils::common_paths[] = {"/sbin", "/bin", "/usr/bin"};

  //instance
  SyncUtils SyncUtils::s_obj;


  bool SyncUtils::is_fuse_enabled()
  {
    try {
      std::string fsFileName = "/proc/filesystems";
      if(sharp::file_exists(fsFileName)) {
        std::string fsOutput;
        std::ifstream file(fsFileName.c_str());
        while(file) {
          std::string line;
          std::getline(file, line);
          fsOutput += "\n" + line;
        }
        file.close();
        Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("\\s+fuse\\s+", Glib::REGEX_MULTILINE);
        return re->match(fsOutput);
      }
    }
    catch(...) {}
    return false;
  }

  bool SyncUtils::enable_fuse()
  {
    if(is_fuse_enabled()) {
      return true; // nothing to do
    }

    if(m_guisu_tool == "" || m_modprobe_tool == "") {
      DBG_OUT("Couldn't enable fuse; missing either GUI 'su' tool or modprobe");

      // Let the user know that FUSE could not be enabled
      utils::HIGMessageDialog cannotEnableDlg(NULL, GTK_DIALOG_MODAL, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
        _("Could not enable FUSE"),
        _("The FUSE module could not be loaded. Please check that it is installed properly and try again."));

      cannotEnableDlg.run();
      return false;
    }

    // Prompt the user first about enabling fuse
    utils::HIGMessageDialog dialog(NULL, GTK_DIALOG_MODAL, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO,
      _("Enable FUSE?"),
      // TODO: This message isn't entirely accurate.
      //       We should fix it.
      _("The synchronization you've chosen requires the FUSE module to be loaded.\n\n"
        "To avoid getting this prompt in the future, you should load FUSE at startup.  "
        "Add \"modprobe fuse\" to /etc/init.d/boot.local or \"fuse\" to /etc/modules."));
    int response = dialog.run();
    if(response == Gtk::RESPONSE_YES) {
      // "modprobe fuse"
      sharp::Process p;
      p.file_name(m_guisu_tool);
      std::vector<std::string> args;
      args.push_back(m_modprobe_tool);
      args.push_back("fuse");
      p.arguments(args);
      p.start();
      p.wait_for_exit();

      if(p.exit_code() != 0) {
        DBG_OUT("Couldn't enable fuse");

        // Let the user know that they don't have FUSE installed on their machine
        utils::HIGMessageDialog failedDlg(NULL, GTK_DIALOG_MODAL, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
          _("Could not enable FUSE"),
          _("The FUSE module could not be loaded. Please check that it is installed properly and try again."));
        failedDlg.run();
        return false;
      }

      // "echo fuse >> /etc/modules"
      /*
      // Punting for now.  Can't seem to get this to work.
      // When using /etc/init.d/boot.local, you should add "modprobe fuse",
      // and not what has been coded below.
      p = new Process ();
      p.StartInfo.UseShellExecute = false;
      p.StartInfo.FileName = guisuTool;
      p.StartInfo.Arguments = string.Format ("\"{0} fuse >> {1}\"",
        echoTool, bootLocalFile);
      p.StartInfo.CreateNoWindow = true;
      p.Start ();
      p.WaitForExit ();
      if (p.ExitCode != 0) {
       Logger.Warn ("Could not enable FUSE persistently.  User will have to be prompted again during their next login session.");
      }
      */
      return true;
    }

    return false;
  }

  std::string SyncUtils::find_first_executable_in_path(const std::vector<std::string> & executableNames)
  {
    for(std::vector<std::string>::const_iterator iter = executableNames.begin();
        iter != executableNames.end(); ++iter) {
      std::string pathVar = Glib::getenv("PATH");
      std::vector<std::string> paths;
      const char separator[] = {G_SEARCHPATH_SEPARATOR, 0};
      sharp::string_split(paths, pathVar, separator);

      for(unsigned i = 0; i < sizeof(common_paths) / sizeof(char*); ++i) {
        std::string commonPath = common_paths[i];
        if(std::find(paths.begin(), paths.end(), commonPath) == paths.end()) {
          paths.push_back(commonPath);
        }
      }

      for(std::vector<std::string>::iterator path = paths.begin(); path != paths.end(); ++path) {
        std::string testExecutablePath = Glib::build_filename(*path, *iter);
        if(sharp::file_exists(testExecutablePath)) {
          return testExecutablePath;
        }
      }
      DBG_OUT("Unable to locate '%s' in your PATH", iter->c_str());
    }

    return "";
  }

  std::string SyncUtils::find_first_executable_in_path(const std::string & executableName)
  {
    std::vector<std::string> executable_names;
    executable_names.push_back(executableName);
    return find_first_executable_in_path(executable_names);
  }

}
}
