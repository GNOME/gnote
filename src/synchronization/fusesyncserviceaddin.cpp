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

#include <fstream>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <glibmm/i18n.h>
#include <sigc++/signal.h>

#include "debug.hpp"
#include "filesystemsyncserver.hpp"
#include "fusesyncserviceaddin.hpp"
#include "preferences.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "sharp/process.hpp"


namespace gnote {
namespace sync {

const int FuseSyncServiceAddin::DEFAULT_MOUNT_TIMEOUT_MS = 10000;


FuseSyncServiceAddin::FuseSyncServiceAddin()
  : m_initialized(false)
  , m_enabled(false)
{}

void FuseSyncServiceAddin::shutdown()
{
  m_enabled = false;
  // TODO: Consider replacing GnoteExitHandler with this!
}

bool FuseSyncServiceAddin::initialized()
{
  return m_initialized && m_enabled;
}

void FuseSyncServiceAddin::initialize()
{
  // TODO: When/how best to handle this?  Okay to install wdfs while Tomboy is running?  When set up mount path, timer, etc, then?
  if(is_supported()) {
    // Determine mount path, etc
    set_up_mount_path();

    if(!m_initialized) {
      m_unmount_timeout.signal_timeout
	.connect(sigc::mem_fun(*this, &FuseSyncServiceAddin::unmount_timeout));
    }
  }
  m_initialized = true;
  m_enabled = true;
}

SyncServer::Ptr FuseSyncServiceAddin::create_sync_server()
{
  SyncServer::Ptr server;

  // Cancel timer
  m_unmount_timeout.cancel();

  // Mount if necessary
  if(is_configured()) {
    if(!is_mounted() && !mount_fuse(true)) // mount_fuse may throw GnoteSyncException!
      throw std::runtime_error(("Could not mount " + m_mount_path).c_str());
    server = FileSystemSyncServer::create(m_mount_path);
  }
  else {
    throw new std::logic_error("create_sync_server called without being configured");
  }

  // Return FileSystemSyncServer
  return server;
}

void FuseSyncServiceAddin::post_sync_cleanup()
{
  // Set unmount timeout to 5 minutes or something
  m_unmount_timeout.reset(1000 * 60 * 5);
}

bool FuseSyncServiceAddin::is_supported()
{
  // Check for fusermount and child-specific executable
  m_fuse_mount_exe_path = SyncUtils::obj().find_first_executable_in_path(fuse_mount_exe_name());
  m_fuse_unmount_exe_path = SyncUtils::obj().find_first_executable_in_path("fusermount");
  m_mount_exe_path = SyncUtils::obj().find_first_executable_in_path("mount");

  return m_fuse_mount_exe_path != "" && m_fuse_unmount_exe_path != "" && m_mount_exe_path != "";
}

bool FuseSyncServiceAddin::save_configuration()
{
  // TODO: When/how best to handle this?
  if(!is_supported()) {
    throw GnoteSyncException(str(boost::format(_(
      "This synchronization addin is not supported on your computer. "
      "Please make sure you have FUSE and %1% correctly installed and configured"))
      % fuse_mount_exe_name()).c_str());
  }

  if(!verify_configuration()) {
    return false;
  }

  // TODO: Check to see if the mount is already mounted
  bool mounted = mount_fuse(false);

  if(mounted) {
    try {
      // Test creating/writing/deleting a file
      std::string testPathBase = Glib::build_filename(m_mount_path, "test");
      std::string testPath = testPathBase;
      int count = 0;

      // Get unique new file name
      while(sharp::file_exists(testPath)) {
        testPath = testPathBase + boost::lexical_cast<std::string>(++count);
      }

      // Test ability to create and write
      std::string testLine = "Testing write capabilities.";
      std::ofstream writer;
      writer.exceptions(std::ios_base::badbit|std::ios_base::failbit|std::ios_base::eofbit);
      writer.open(testPath.c_str());
      writer << testLine;
      writer.close();

      // Test ability to read
      bool testFileFound = false;
      std::list<std::string> files;
      sharp::directory_get_files(m_mount_path, files);
      for(std::list<std::string>::iterator iter = files.begin(); iter != files.end(); ++iter) {
        if(*iter == testPath) {
          testFileFound = true;
          break;
        }
      }
      if(!testFileFound) {
        throw GnoteSyncException(_("Could not read testfile."));
      }
      std::ifstream reader;
      reader.exceptions(std::ios_base::badbit|std::ios_base::failbit);
      reader.open(testPath.c_str());
      std::string read_line;
      std::getline(reader, read_line);
      reader.close();
      if(read_line != testLine) {
        throw GnoteSyncException(_("Write test failed."));
      }

      // Test ability to delete
      sharp::file_delete(testPath);
    }
    catch(...) {
      // Clean up
      post_sync_cleanup();
      throw;
    }
    post_sync_cleanup();

    // Finish save process
    save_configuration_values();
  }

  return mounted;
}

void FuseSyncServiceAddin::reset_configuration()
{
  // Unmount immediately, then reset configuration
  unmount_timeout();
  reset_configuration_values();
}

std::string FuseSyncServiceAddin::fuse_mount_timeout_error()
{
  char *error = _("Timeout connecting to server.");
  return error;
}

std::string FuseSyncServiceAddin::fuse_mount_directory_error()
{
  char *error = _("Error connecting to server.");
  return error;
}

bool FuseSyncServiceAddin::mount_fuse(bool useStoredValues)
{
  if(m_mount_path == "") {
    return false;
  }

  if(SyncUtils::obj().is_fuse_enabled() == false) {
    if(SyncUtils::obj().enable_fuse() == false) {
      DBG_OUT("User canceled or something went wrong enabling FUSE");
      throw GnoteSyncException(_("FUSE could not be enabled."));
    }
  }

  prepare_mount_path();

  sharp::Process p;

  // Need to redirect stderr for displaying errors to user,
  // but we can't use stdout and by not redirecting it, it
  // should appear in the console Tomboy is started from.
  p.redirect_standard_output(false);
  p.redirect_standard_error(true);

  p.file_name(m_fuse_mount_exe_path);
  p.arguments(get_fuse_mount_exe_args(m_mount_path, useStoredValues));
  DBG_OUT("Mounting sync path with this command: %s %s", m_fuse_mount_exe_path.c_str(),
                               // Args could include password, so may need to mask
                               get_fuse_mount_exe_args_for_display(m_mount_path, useStoredValues).c_str());
  p.start();
  int timeoutMs = get_timeout_ms();
  bool exited = p.wait_for_exit(timeoutMs);

  if(!exited) {
    unmount_timeout(); // TODO: This is awfully ugly
    DBG_OUT("Error calling %s}: timed out after %d seconds", m_fuse_mount_exe_path.c_str(), timeoutMs / 1000);
    throw GnoteSyncException(fuse_mount_timeout_error().c_str());
  }
  else if(p.exit_code() != 0) {
    unmount_timeout(); // TODO: This is awfully ugly
    DBG_OUT("Error calling %s", m_fuse_mount_exe_path.c_str());
    throw GnoteSyncException(_("An error ocurred while connecting to the specified server"));
    //TODO: provide stderr output of child
  }

  // For wdfs, incorrect user credentials will cause the mountPath to
  // be messed up, and not recognized as a directory.  This is the only
  // way I can find to report that the username/password may be incorrect (for wdfs).
  if(!sharp::directory_exists(m_mount_path)) {
    DBG_OUT("FUSE mount call succeeded, but mount path does not exist. "
            "This may be an indication that incorrect user credentials were "
            "provided, but it may also represent any number of error states "
            "not properly handled by the FUSE filesystem.");
    // Even though the mountPath is screwed up, it is still (apparently)
    // a valid FUSE mount and must be unmounted.
    unmount_timeout(); // TODO: This is awfully ugly
    throw GnoteSyncException(fuse_mount_directory_error().c_str());
  }

  return true;
}

int FuseSyncServiceAddin::get_timeout_ms()
{
  Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
  try {
    return settings->get_int(Preferences::SYNC_FUSE_MOUNT_TIMEOUT);
  }
  catch(...) {
    try {
      settings->set_int(Preferences::SYNC_FUSE_MOUNT_TIMEOUT, DEFAULT_MOUNT_TIMEOUT_MS);
    }
    catch(...) {}
    return DEFAULT_MOUNT_TIMEOUT_MS;
  }
}

void FuseSyncServiceAddin::set_up_mount_path()
{
  std::string notesPath = Glib::get_tmp_dir();
  m_mount_path = Glib::build_filename(notesPath, Glib::get_user_name(), "gnote", "sync-" + id()); // TODO: Best mount path name?
}

void FuseSyncServiceAddin::prepare_mount_path()
{
  if(sharp::directory_exists(m_mount_path) == false) {
    try {
      sharp::directory_create(m_mount_path);
    } catch(...) {
      throw new std::runtime_error(str(boost::format("Couldn't create \"%1%\" directory.") % m_mount_path));
    }
  } else
    // Just in case, make sure there is no
    // existing FUSE mount at mountPath.
    unmount_timeout();
}

// Perform clean up when Gnote exits.
void FuseSyncServiceAddin::gnote_exit_handler()
{
  unmount_timeout();
}

void FuseSyncServiceAddin::unmount_timeout()
{
  if(is_mounted()) {
    sharp::Process p;
    p.redirect_standard_output(false);
    p.file_name(m_fuse_unmount_exe_path);
    std::vector<std::string> args;
    args.push_back("-u");
    args.push_back(m_mount_path);
    p.arguments(args);
    p.start();
    p.wait_for_exit();

    // TODO: What does this return if it was not mounted?
    if(p.exit_code() != 0) {
      DBG_OUT("Error unmounting %s", id().c_str());
      m_unmount_timeout.reset(1000 * 60 * 5); // Try again in five minutes
    }
    else {
      DBG_OUT("Successfully unmounted %s", id().c_str());
      m_unmount_timeout.cancel();
    }
  }
}

// Checks to see if the mount is actually mounted and alive
bool FuseSyncServiceAddin::is_mounted()
{
  sharp::Process p;
  p.redirect_standard_output(true);
  p.file_name(m_mount_exe_path);
  p.start();
  std::list<std::string> outputLines;
  while(!p.standard_output_eof()) {
    std::string line = p.standard_output_read_line();
    outputLines.push_back(line);
  }
  p.wait_for_exit();

  if(p.exit_code() == 1) {
    DBG_OUT("Error calling %s", m_mount_exe_path.c_str());
    return false;
  }

  // TODO: Review this methodology...is it really the exe name, for example?
  for(std::list<std::string>::iterator iter = outputLines.begin(); iter != outputLines.end(); ++iter) {
    if((iter->find(fuse_mount_exe_name()) == 0 || iter->find(" type fuse." + fuse_mount_exe_name()) != std::string::npos)
       && iter->find(str(boost::format("on %1% ") % m_mount_path)) != std::string::npos) {
      return true;
    }
  }

  return false;
}

}
}
