/*
 * gnote
 *
 * Copyright (C) 2010-2014,2016 Aurimas Cernius
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


#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <glibmm/i18n.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "itagmanager.hpp"
#include "notemanagerbase.hpp"
#include "utils.hpp"
#include "trie.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "sharp/uuid.hpp"


namespace gnote {

bool compare_dates(const NoteBase::Ptr & a, const NoteBase::Ptr & b)
{
  return (static_pointer_cast<Note>(a)->change_date() > static_pointer_cast<Note>(b)->change_date());
}


class TrieController
{
public:
  TrieController(NoteManagerBase &);
  ~TrieController();

  void add_note(const NoteBase::Ptr & note);
  void update();
  TrieTree<NoteBase::WeakPtr> *title_trie() const
    {
      return m_title_trie;
    }
private:
  void on_note_added(const NoteBase::Ptr & added);
  void on_note_deleted (const NoteBase::Ptr & deleted);
  void on_note_renamed(const NoteBase::Ptr & renamed, const Glib::ustring & old_title);

  NoteManagerBase & m_manager;
  TrieTree<NoteBase::WeakPtr> *m_title_trie;
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


NoteManagerBase::NoteManagerBase(const Glib::ustring & directory)
  : m_trie_controller(NULL)
  , m_notes_dir(directory)
{
}

NoteManagerBase::~NoteManagerBase()
{
  if(m_trie_controller) {
    delete m_trie_controller;
  }
}

void NoteManagerBase::_common_init(const Glib::ustring & /*directory*/, const Glib::ustring & backup_directory)
{
  m_default_note_template_title = _("New Note Template");
  m_backup_dir = backup_directory;
  bool is_first_run = first_run();

  const std::string old_note_dir = IGnote::old_note_dir();
  const bool migration_needed = is_first_run && sharp::directory_exists(old_note_dir);
  create_notes_dir();

  if(migration_needed) {
    try {
      migrate_notes(old_note_dir);
    }
    catch(Glib::Exception & e) {
      ERR_OUT("Migration failed! Exception: %s", e.what().c_str());
    }
    is_first_run = false;
  }

  m_trie_controller = create_trie_controller();
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

void NoteManagerBase::migrate_notes(const std::string & /*old_note_dir*/)
{
}

// Create the TrieController. For overriding in test methods.
TrieController *NoteManagerBase::create_trie_controller()
{
  return new TrieController(*this);
}

void NoteManagerBase::post_load()
{
  m_notes.sort(boost::bind(&compare_dates, _1, _2));

  // Update the trie so addins can access it, if they want.
  m_trie_controller->update ();
}

size_t NoteManagerBase::trie_max_length()
{
  return m_trie_controller->title_trie()->max_length();
}

TrieHit<NoteBase::WeakPtr>::ListPtr NoteManagerBase::find_trie_matches(const Glib::ustring & match)
{
  return m_trie_controller->title_trie()->find_matches(match);
}

NoteBase::List NoteManagerBase::get_notes_linking_to(const Glib::ustring & title) const
{
  Glib::ustring tag = "<link:internal>" + utils::XmlEncoder::encode(title) + "</link:internal>";
  NoteBase::List result;
  FOREACH(const NoteBase::Ptr & note, m_notes) {
    if(note->get_title() != title) {
      if(note->get_complete_note_xml().find(tag) != std::string::npos) {
        result.push_back(note);
      }
    }
  }
  return result;
}

void NoteManagerBase::add_note(const NoteBase::Ptr & note)
{
  if(note) {
    note->signal_renamed.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_rename));
    note->signal_saved.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_save));
    m_notes.push_back(note);
  }
}

void NoteManagerBase::on_note_rename(const NoteBase::Ptr & note, const Glib::ustring & old_title)
{
  signal_note_renamed(note, old_title);
  m_notes.sort(boost::bind(&compare_dates, _1, _2));
}

void NoteManagerBase::on_note_save (const NoteBase::Ptr & note)
{
  signal_note_saved(note);
  m_notes.sort(boost::bind(&compare_dates, _1, _2));
}

NoteBase::Ptr NoteManagerBase::find(const Glib::ustring & linked_title) const
{
  FOREACH(const NoteBase::Ptr & note, m_notes) {
    if(note->get_title().lowercase() == linked_title.lowercase()) {
      return note;
    }
  }
  return NoteBase::Ptr();
}

NoteBase::Ptr NoteManagerBase::find_by_uri(const std::string & uri) const
{
  FOREACH(const NoteBase::Ptr & note, m_notes) {
    if (note->uri() == uri) {
      return note;
    }
  }
  return NoteBase::Ptr();
}

NoteBase::Ptr NoteManagerBase::create_note_from_template(const Glib::ustring & title, const NoteBase::Ptr & template_note)
{
  return create_note_from_template(title, template_note, "");
}

NoteBase::Ptr NoteManagerBase::create()
{
  return create("");
}

NoteBase::Ptr NoteManagerBase::create(const Glib::ustring & title)
{
  return create_new_note(title, "");
}

NoteBase::Ptr NoteManagerBase::create(const Glib::ustring & title, const Glib::ustring & xml_content)
{
  return create_new_note(title, xml_content, "");
}

// Creates a new note with the given title and guid with body based on
// the template note.
NoteBase::Ptr NoteManagerBase::create_note_from_template(const Glib::ustring & title,
                                                         const NoteBase::Ptr & template_note,
                                                         const std::string & guid)
{
  Glib::ustring new_title(title);
  Tag::Ptr template_save_title = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG);
  if(template_note->contains_tag(template_save_title)) {
    new_title = get_unique_name(template_note->get_title());
  }

  // Use the body from the template note
  Glib::ustring xml_content = sharp::string_replace_first(template_note->xml_content(),
                                                          utils::XmlEncoder::encode(template_note->get_title()),
                                                          utils::XmlEncoder::encode(new_title));
  xml_content = sanitize_xml_content(xml_content);

  NoteBase::Ptr new_note = create_new_note(new_title, xml_content, guid);

  // Copy template note's properties
  Tag::Ptr template_save_size = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SIZE_SYSTEM_TAG);
  if(template_note->data().has_extent() && template_note->contains_tag(template_save_size)) {
    new_note->data().height() = template_note->data().height();
    new_note->data().width() = template_note->data().width();
  }

  return new_note;
}

// Find a title that does not exist using basename
Glib::ustring NoteManagerBase::get_unique_name(const Glib::ustring & basename) const
{
  int id = 1;  // starting point
  Glib::ustring title;
  while(true) {
    title = str(boost::format("%1% %2%") % basename % id++);
    if(!find (title)) {
      break;
    }
  }

  return title;
}

// Create a new note with the specified title from the default
// template note. Optionally the body can be overridden.
NoteBase::Ptr NoteManagerBase::create_new_note(Glib::ustring title, const std::string & guid)
{
  Glib::ustring body;

  title = split_title_from_content(title, body);

  if(title.empty()) {
    title = get_unique_name(_("New Note"));
  }

  NoteBase::Ptr template_note = get_or_create_template_note();

  if(body.empty()) {
    return create_note_from_template(title, template_note, guid);
  }

  // Use a simple "Describe..." body and highlight
  // it so it can be easily overwritten
  Glib::ustring content = get_note_template_content(title);
  NoteBase::Ptr new_note = create_new_note(title, content, guid);

  // Select the inital text so typing will overwrite the body text
  static_pointer_cast<Note>(new_note)->get_buffer()->select_note_body();

  return new_note;
}

// Create a new note with the specified Xml content
NoteBase::Ptr NoteManagerBase::create_new_note(const Glib::ustring & title, const Glib::ustring & xml_content, 
                                               const std::string & guid)
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

  NoteBase::Ptr new_note = note_create_new(title, filename);
  if(new_note == 0) {
    throw sharp::Exception("Failed to create new note");
  }
  new_note->set_xml_content(xml_content);
  new_note->signal_renamed.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_rename));
  new_note->signal_saved.connect(sigc::mem_fun(*this, &NoteManagerBase::on_note_save));

  m_notes.push_back(new_note);

  signal_note_added(new_note);

  return new_note;
}

Glib::ustring NoteManagerBase::get_note_template_content(const Glib::ustring & title)
{
  return str(boost::format("<note-content>"
                           "<note-title>%1%</note-title>\n\n"
                           "%2%"
                           "</note-content>") 
             % utils::XmlEncoder::encode(title)
             % _("Describe your new note here."));
}

NoteBase::Ptr NoteManagerBase::get_or_create_template_note()
{
  NoteBase::Ptr template_note = find_template_note();
  if(!template_note) {
    Glib::ustring title = m_default_note_template_title;
    if(find(title)) {
      title = get_unique_name(title);
    }
    template_note = create(title, get_note_template_content(title));
    if(template_note == 0) {
      throw sharp::Exception("Failed to create template note");
    }

    // Flag this as a template note
    Tag::Ptr template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    template_note->add_tag(template_tag);

    template_note->queue_save(CONTENT_CHANGED);
  }
      
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

  std::vector<std::string> lines;
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

NoteBase::Ptr NoteManagerBase::find_template_note() const
{
  NoteBase::Ptr template_note;
  Tag::Ptr template_tag = ITagManager::obj().get_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
  if(!template_tag) {
    return template_note;
  }
  std::list<NoteBase*> notes;
  template_tag->get_notes(notes);
  FOREACH(NoteBase *iter, notes) {
    NoteBase::Ptr note = iter->shared_from_this();
    if(!notebooks::NotebookManager::obj().get_notebook_from_note(note)) {
      template_note = note;
      break;
    }
  }

  return template_note;
}

void NoteManagerBase::delete_note(const NoteBase::Ptr & note)
{
  if(sharp::file_exists(note->file_path())) {
    if(!m_backup_dir.empty()) {
      if(!sharp::directory_exists(m_backup_dir)) {
        sharp::directory_create(m_backup_dir);
      }
      Glib::ustring backup_path 
        = Glib::build_filename(m_backup_dir, sharp::file_filename(note->file_path()));

      if(sharp::file_exists(backup_path)) {
        sharp::file_delete(backup_path);
      }

      sharp::file_move(note->file_path(), backup_path);
    } 
    else {
      sharp::file_delete(note->file_path());
    }
  }

  m_notes.remove(note);
  note->delete_note();

  DBG_OUT("Deleting note '%s'.", note->get_title().c_str());

  signal_note_deleted(note);
}

NoteBase::Ptr NoteManagerBase::import_note(const Glib::ustring & file_path)
{
  Glib::ustring dest_file = Glib::build_filename(notes_dir(), 
                                                 sharp::file_filename(file_path));

  if(sharp::file_exists(dest_file)) {
    dest_file = make_new_file_name();
  }
  NoteBase::Ptr note;
  try {
    sharp::file_copy(file_path, dest_file);

    // TODO: make sure the title IS unique.
    note = note_load(dest_file);
    add_note(note);
  }
  catch(...)
  {
  }
  return note;
}


NoteBase::Ptr NoteManagerBase::create_with_guid(const Glib::ustring & title, const std::string & guid)
{
  return create_new_note(title, guid);
}



TrieController::TrieController(NoteManagerBase & manager)
  : m_manager(manager)
  ,  m_title_trie(NULL)
{
  m_manager.signal_note_deleted.connect(sigc::mem_fun(*this, &TrieController::on_note_deleted));
  m_manager.signal_note_added.connect(sigc::mem_fun(*this, &TrieController::on_note_added));
  m_manager.signal_note_renamed.connect(sigc::mem_fun(*this, &TrieController::on_note_renamed));

  update();
}

TrieController::~TrieController()
{
  delete m_title_trie;
}

void TrieController::on_note_added(const NoteBase::Ptr & note)
{
  add_note(note);
}

void TrieController::on_note_deleted(const NoteBase::Ptr &)
{
  update();
}

void TrieController::on_note_renamed(const NoteBase::Ptr &, const Glib::ustring &)
{
  update();
}

void TrieController::add_note(const NoteBase::Ptr & note)
{
  m_title_trie->add_keyword(note->get_title(), note);
  m_title_trie->compute_failure_graph();
}

void TrieController::update()
{
  if(m_title_trie) {
    delete m_title_trie;
  }
  m_title_trie = new TrieTree<NoteBase::WeakPtr>(false /* !case_sensitive */);

  FOREACH(const NoteBase::Ptr & note, m_manager.get_notes()) {
    m_title_trie->add_keyword(note->get_title(), note);
  }
  m_title_trie->compute_failure_graph();
}


}

