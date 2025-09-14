/*
 * gnote
 *
 * Copyright (C) 2010-2014,2016-2017,2019-2025 Aurimas Cernius
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


#include <glib/gstdio.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "notemanagerbase.hpp"
#include "utils.hpp"
#include "trie.hpp"
#include "notebooks/notebookmanager.hpp"
#include "base/hash.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "sharp/uuid.hpp"


namespace gnote {

class TrieController
{
public:
  TrieController(NoteManagerBase &);

  void add_note(const NoteBase::Ptr & note);
  void update();
  TrieTree<Glib::ustring> & title_trie() const
    {
      return *m_title_trie;
    }
private:
  void on_note_added(NoteBase & added);
  void on_note_deleted(NoteBase & deleted);
  void on_note_renamed(const NoteBase & renamed, const Glib::ustring & old_title);

  NoteManagerBase & m_manager;
  std::unique_ptr<TrieTree<Glib::ustring>> m_title_trie;
};



Glib::ustring NoteManagerBase::sanitize_xml_content(const Glib::ustring & xml_content)
{
  Glib::ustring::size_type pos = xml_content.find('\n');
  int i = (pos == Glib::ustring::npos) ? -1 : pos;
  Glib::ustring result(xml_content);

  while(--i >= 0) {
    if(xml_content[i] == '\r') {
      continue;
    }

    if(std::isspace(result[i])) {
      result.erase(i, 1);
    }
    else {
      break;
    }
  }

  return result;
}


NoteManagerBase::NoteManagerBase(IGnote & g)
  : m_gnote(g)
  , m_trie_controller(NULL)
{
}

NoteManagerBase::~NoteManagerBase()
{
  if(m_trie_controller) {
    delete m_trie_controller;
  }
}

void NoteManagerBase::delete_old_backups(const Glib::ustring &backup, const Glib::DateTime &keep_since)
{
  auto backups = sharp::directory_get_files(backup);
  const auto keep = keep_since.to_unix();

  for(const auto &file : backups) {
    GStatBuf sb;
    g_stat(file.c_str(), &sb);
    if(sb.st_mtime < keep) {
      sharp::file_delete(file);
    }
  }
}

bool NoteManagerBase::init(const Glib::ustring & directory, const Glib::ustring & backup_directory)
{
  m_notes_dir = directory;
  m_default_note_template_title = _("New Note Template");
  m_backup_dir = backup_directory;
  bool is_first_run = first_run();

  const Glib::ustring old_note_dir = IGnote::old_note_dir();
  const bool migration_needed = is_first_run && sharp::directory_exists(old_note_dir);
  create_notes_dir();

  if(migration_needed) {
    try {
      migrate_notes(old_note_dir);
    }
    catch(std::exception & e) {
      ERR_OUT("Migration failed! Exception: %s", e.what());
    }
    is_first_run = false;
  }

  m_trie_controller = create_trie_controller();
  return is_first_run;
}

bool NoteManagerBase::first_run() const
{
  return !sharp::directory_exists(notes_dir());
}

// Create the notes directory if it doesn't exist yet.
void NoteManagerBase::create_notes_dir() const
{
  if(!sharp::directory_exists(notes_dir())) {
    // First run. Create storage directory.
    create_directory(notes_dir());
  }
  if(!sharp::directory_exists(m_backup_dir)) {
    create_directory(m_backup_dir);
  }
}

bool NoteManagerBase::create_directory(const Glib::ustring & directory) const
{
  return g_mkdir_with_parents(directory.c_str(), S_IRWXU) == 0;
}

void NoteManagerBase::migrate_notes(const Glib::ustring & /*old_note_dir*/)
{
}

// Create the TrieController. For overriding in test methods.
TrieController *NoteManagerBase::create_trie_controller()
{
  return new TrieController(*this);
}

void NoteManagerBase::post_load()
{
  // Update the trie so addins can access it, if they want.
  m_trie_controller->update ();
}

size_t NoteManagerBase::trie_max_length()
{
  return m_trie_controller->title_trie().max_length();
}

TrieHit<Glib::ustring>::List NoteManagerBase::find_trie_matches(const Glib::ustring & match)
{
  return m_trie_controller->title_trie().find_matches(match);
}

std::vector<NoteBase::Ref> NoteManagerBase::get_notes_linking_to(const Glib::ustring & title) const
{
  Glib::ustring tag = "<link:internal>" + utils::XmlEncoder::encode(title) + "</link:internal>";
  std::vector<NoteBase::Ref> result;
  for(const NoteBase::Ptr & note : m_notes) {
    if(note->get_title() != title) {
      if(note->get_complete_note_xml().find(tag) != Glib::ustring::npos) {
        result.push_back(*note);
      }
    }
  }
  return result;
}

void NoteManagerBase::add_note(NoteBase::Ptr note)
{
  if(note) {
    note->signal_renamed.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_rename));
    note->signal_saved.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_save));
    m_notes.insert(std::move(note));
  }
}

void NoteManagerBase::on_note_rename(const NoteBase & note, const Glib::ustring & old_title)
{
  signal_note_renamed(note, old_title);
}

void NoteManagerBase::on_note_save(NoteBase & note)
{
  signal_note_saved(note);
}

NoteBase::ORef NoteManagerBase::find(const Glib::ustring & linked_title) const
{
  for(const NoteBase::Ptr & note : m_notes) {
    if(note->get_title().lowercase() == linked_title.lowercase()) {
      return std::ref(*note);
    }
  }
  return NoteBase::ORef();
}

NoteBase::ORef NoteManagerBase::find_by_uri(const Glib::ustring & uri) const
{
  for(const NoteBase::Ptr & note : m_notes) {
    if (note->uri() == uri) {
      return std::ref(*note);
    }
  }
  return NoteBase::ORef();
}

NoteBase & NoteManagerBase::create_note_from_template(Glib::ustring && title, const NoteBase & template_note)
{
  return create_note_from_template(std::move(title), template_note, "");
}

NoteBase & NoteManagerBase::create()
{
  return create_note("", "");
}

NoteBase & NoteManagerBase::create(Glib::ustring && title)
{
  Glib::ustring body;
  auto note_title = split_title_from_content(title, body);
  return create_note(std::move(note_title), std::move(body));
}

NoteBase & NoteManagerBase::create(Glib::ustring && title, Glib::ustring && xml_content)
{
  return create_new_note(std::move(title), std::move(xml_content), "");
}

// Creates a new note with the given title and guid with body based on
// the template note.
NoteBase & NoteManagerBase::create_note_from_template(Glib::ustring && title, const NoteBase & template_note, Glib::ustring && guid)
{
  auto &template_save_title = tag_manager().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG);
  if(template_note.contains_tag(template_save_title)) {
    title = get_unique_name(template_note.get_title());
  }

  // Use the body from the template note
  Glib::ustring xml_content = sharp::string_replace_first(template_note.xml_content(),
                                                          utils::XmlEncoder::encode(template_note.get_title()),
                                                          utils::XmlEncoder::encode(title));
  xml_content = sanitize_xml_content(xml_content);

  return create_new_note(std::move(title), std::move(xml_content), std::move(guid));
}

// Find a title that does not exist using basename
Glib::ustring NoteManagerBase::get_unique_name(const Glib::ustring & basename) const
{
  int id = 1;  // starting point
  Glib::ustring title;
  while(true) {
    title = Glib::ustring::compose("%1 %2", basename, id++);
    if(!find (title)) {
      break;
    }
  }

  return title;
}

NoteBase & NoteManagerBase::create_note(Glib::ustring && title, Glib::ustring && body, Glib::ustring && guid)
{
  if(title.empty()) {
    title = get_unique_name(_("New Note"));
  }

  Glib::ustring content;
  if(body.empty()) {
    if(auto template_note = find_template_note()) {
      return create_note_from_template(std::move(title), *template_note, std::move(guid));
    }

    // Use a simple "Describe..." body and highlight
    // it so it can be easily overwritten
    content = get_note_template_content(title);
  }
  else {
    content = get_note_content(title, body);
  }

  return create_new_note(std::move(title), std::move(content), std::move(guid));
}

// Create a new note with the specified Xml content
NoteBase & NoteManagerBase::create_new_note(Glib::ustring && title, Glib::ustring && xml_content, Glib::ustring && guid)
{
  if(title.empty())
    throw sharp::Exception("Invalid title");

  if(find(title))
    throw sharp::Exception("A note with this title already exists: " + title);

  Glib::ustring filename;
  if(!guid.empty())
    filename = make_new_file_name(guid);
  else
    filename = make_new_file_name();

  NoteBase::Ptr new_note = note_create_new(std::move(title), Glib::ustring(filename));
  if(new_note == 0) {
    throw sharp::Exception("Failed to create new note");
  }
  new_note->set_xml_content(std::move(xml_content));
  new_note->signal_renamed.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_rename));
  new_note->signal_saved.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_save));

  m_notes.insert(new_note);

  signal_note_added(*new_note);

  return *new_note;
}

Glib::ustring NoteManagerBase::get_note_template_content(const Glib::ustring & title)
{
  return get_note_content(title, _("Describe your new note here."));
}

Glib::ustring NoteManagerBase::get_note_content(const Glib::ustring & title, const Glib::ustring & body)
{
  return Glib::ustring::compose("<note-content>"
                                  "<note-title>%1</note-title>\n\n"
                                  "%2"
                                "</note-content>",
             utils::XmlEncoder::encode(title),
             utils::XmlEncoder::encode(body));
}

NoteBase & NoteManagerBase::get_or_create_template_note()
{
  if(auto template_note = find_template_note()) {
    return template_note.value();
  }

  Glib::ustring title = m_default_note_template_title;
  if(find(title)) {
    title = get_unique_name(title);
  }
  auto content = get_note_template_content(title);
  auto & template_note = create(std::move(title), std::move(content));

  // Flag this as a template note
  auto &template_tag = tag_manager().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
  template_note.add_tag(template_tag);

  template_note.queue_save(CONTENT_CHANGED);
  return template_note;
}

Glib::ustring NoteManagerBase::split_title_from_content(Glib::ustring title, Glib::ustring & body)
{
  body = "";

  if(title.empty())
    return "";

  title = sharp::string_trim(title);
  if(title.empty())
    return "";

  std::vector<Glib::ustring> lines;
  sharp::string_split(lines, title, "\n\r");
  if(lines.size() > 0) {
    title = lines [0];
    title = sharp::string_trim(title);
    title = sharp::string_trim(title, ".,;");
    if(title.empty())
      return "";
  }

  if(lines.size() > 1)
    body = lines [1];

  return title;
}

Glib::ustring NoteManagerBase::make_new_file_name() const
{
  return make_new_file_name(sharp::uuid().string());
}

Glib::ustring NoteManagerBase::make_new_file_name(const Glib::ustring & guid) const
{
  return Glib::build_filename(notes_dir(), guid + ".note");
}

NoteBase::ORef NoteManagerBase::find_template_note() const
{
  if(auto template_tag = tag_manager().get_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG)) {
    for(NoteBase *iter : template_tag.value().get().get_notes()) {
      if(!m_gnote.notebook_manager().get_notebook_from_note(*iter)) {
        return std::ref(*iter);
      }
    }
  }

  return NoteBase::ORef();
}

void NoteManagerBase::delete_note(NoteBase & note)
{
  DBG_OUT("Deleting note '%s'.", note.get_title().c_str());
  NoteBase::Ptr cached_ref;  // prevent note from being destroyed

  for(auto iter = m_notes.begin(); iter != m_notes.end(); ++iter) {
    if(iter->get() == &note) {
      cached_ref = *iter;
      m_notes.erase(iter);
      break;
    }
  }
  DBG_ASSERT(cached_ref != nullptr, "Deleting note that is not present");
  note.delete_note();
  signal_note_deleted(note);

  auto file_path = note.file_path();
  if(sharp::file_exists(file_path)) {
    if(!m_backup_dir.empty()) {
      if(!sharp::directory_exists(m_backup_dir)) {
        sharp::directory_create(m_backup_dir);
      }
      Glib::ustring backup_path = Glib::build_filename(m_backup_dir, sharp::file_filename(file_path));

      if(sharp::file_exists(backup_path)) {
        sharp::file_delete(backup_path);
      }

      sharp::file_move(file_path, backup_path);
    } 
    else {
      sharp::file_delete(file_path);
    }
  }
}

NoteBase::ORef NoteManagerBase::import_note(const Glib::ustring & file_path)
{
  Glib::ustring dest_file = Glib::build_filename(notes_dir(), 
                                                 sharp::file_filename(file_path));

  if(sharp::file_exists(dest_file)) {
    dest_file = make_new_file_name();
  }
  try {
    sharp::file_copy(file_path, dest_file);

    NoteBase::Ptr note = note_load(std::move(dest_file));
    if(!note) {
      return NoteBase::ORef();
    }

    if(find(note->get_title())) {
      for(int i = 1;; ++i) {
        auto new_title = note->get_title() + " " + TO_STRING(i);
        if(!find(new_title)) {
          note->set_title(std::move(new_title));
          break;
        }
      }
    }
    add_note(note);
    return *note;
  }
  catch(...)
  {
  }

  return NoteBase::ORef();
}


NoteBase & NoteManagerBase::create_with_guid(Glib::ustring && title, Glib::ustring && guid)
{
  Glib::ustring body;
  auto note_title = split_title_from_content(title, body);
  return create_note(std::move(note_title), std::move(body), std::move(guid));
}


std::size_t NoteManagerBase::NoteHash::operator()(const NoteBase::Ptr & note) const noexcept
{
  Hash<Glib::ustring> hash;
  return hash(note->uri());
}



TrieController::TrieController(NoteManagerBase & manager)
  : m_manager(manager)
{
  m_manager.signal_note_deleted.connect(sigc::mem_fun(*this, &TrieController::on_note_deleted));
  m_manager.signal_note_added.connect(sigc::mem_fun(*this, &TrieController::on_note_added));
  m_manager.signal_note_renamed.connect(sigc::mem_fun(*this, &TrieController::on_note_renamed));

  update();
}

void TrieController::on_note_added(NoteBase & note)
{
  add_note(note.shared_from_this());
}

void TrieController::on_note_deleted(NoteBase &)
{
  update();
}

void TrieController::on_note_renamed(const NoteBase &, const Glib::ustring &)
{
  update();
}

void TrieController::add_note(const NoteBase::Ptr & note)
{
  m_title_trie->add_keyword(note->get_title(), note->uri());
  m_title_trie->compute_failure_graph();
}

void TrieController::update()
{
  m_title_trie = std::make_unique<TrieTree<Glib::ustring>>(false /* !case_sensitive */);

  m_manager.for_each([this](NoteBase & note) {
    m_title_trie->add_keyword(note.get_title(), note.uri());
  });
  m_title_trie->compute_failure_graph();
}


}

