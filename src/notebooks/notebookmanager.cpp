/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019,2022-2024 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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


#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>

#include "sharp/string.hpp"
#include "sharp/exception.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "debug.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"


namespace gnote {
  namespace notebooks {

    NotebookManager::NotebookManager(NoteManagerBase & manager)
      : m_active_notes(ActiveNotesNotebook::create(manager))
      , m_note_manager(manager)
    { 
    }

    void NotebookManager::init()
    {
      m_all_notebooks.push_back(AllNotesNotebook::create(m_note_manager));
      m_all_notebooks.push_back(UnfiledNotesNotebook::create(m_note_manager));
      m_all_notebooks.push_back(PinnedNotesNotebook::create(m_note_manager));
      m_all_notebooks.push_back(m_active_notes);

      load_notebooks();
    }


    Notebook::ORef NotebookManager::get_notebook(const Glib::ustring & notebookName) const
    {
      if (notebookName.empty()) {
        throw sharp::Exception ("NotebookManager::get_notebook() called with an empty name.");
      }
      Glib::ustring normalizedName = Notebook::normalize(notebookName);
      if (normalizedName.empty()) {
        throw sharp::Exception ("NotebookManager::get_notebook() called with an empty name.");
      }
      for(const auto& nb : m_all_notebooks) {
        if(nb->get_normalized_name() == normalizedName) {
          return *nb;
        }
      }

      return Notebook::ORef();
    }
    

    bool NotebookManager::notebook_exists(const Glib::ustring & notebookName) const
    {
      Glib::ustring normalizedName = Notebook::normalize(notebookName);
      for(const auto& nb : m_all_notebooks) {
        if(nb->get_normalized_name() == normalizedName) {
          return true;
        }
      }

      return false;
    }

    Notebook & NotebookManager::get_or_create_notebook(const Glib::ustring & notebookName)
    {
      if (notebookName.empty())
        throw sharp::Exception ("NotebookManager.GetNotebook () called with a null name.");
      
//      lock (locker) {
        if(auto nb = get_notebook(notebookName)) {
          return nb.value();
        }

        Notebook::Ptr notebook = Notebook::create(m_note_manager, notebookName);
        m_all_notebooks.push_back(notebook);
        
        // Create the template note so the system tag
        // that represents the notebook actually gets
        // saved to a note (and persisted after Tomboy
        // is shut down).
        auto & template_note = notebook->get_template_note();
        
        // Make sure the template note has the notebook tag.
        // Since it's possible for the template note to already
        // exist, we need to make sure it gets tagged.
        template_note.add_tag(*notebook->get_tag());
        signal_note_added_to_notebook(template_note, *notebook);
//      }

      signal_notebook_list_changed();
      return *notebook;
    }

    bool NotebookManager::add_notebook(Notebook::Ptr && notebook)
    {
      auto normalized_name = notebook->get_normalized_name();
      if(get_notebook(normalized_name)) {
        return false;
      }

      m_all_notebooks.emplace_back(std::move(notebook));
      signal_notebook_list_changed();
      return true;
    }

    void NotebookManager::delete_notebook(Notebook & notebook)
    {
      Glib::ustring normalized_name = notebook.get_normalized_name();
      auto iter = m_all_notebooks.begin();
      for(; iter != m_all_notebooks.end(); ++iter) {
        if(&**iter == &notebook) {
          break;
        }
      }
      if(iter == m_all_notebooks.end()) {
        return;
      }

      auto tag = notebook.get_tag();
      Notebook::Ptr keep_alive = *iter;
      m_all_notebooks.erase(iter);

      // Remove the notebook tag from every note that's in the notebook
      if(tag) {
        Tag &t = tag.value();
        for(NoteBase *note : t.get_notes()) {
          note->remove_tag(t);
          signal_note_removed_from_notebook(*static_cast<Note*>(note), notebook);
        }
      }

      signal_notebook_list_changed();
    }

    /// <summary>
    /// Returns the Notebook associated with this note or null
    /// if no notebook exists.
    /// </summary>
    /// <param name="note">
    /// A <see cref="Note"/>
    /// </param>
    /// <returns>
    /// A <see cref="Notebook"/>
    /// </returns>
    Notebook::ORef NotebookManager::get_notebook_from_note(const NoteBase & note)
    {
      for(Tag &tag : note.get_tags()) {
        if(auto notebook = get_notebook_from_tag(tag)) {
          return notebook;
        }
      }
      
      return Notebook::ORef();
    }


        /// <summary>
    /// Returns the Notebook associated with the specified tag
    /// or null if the Tag does not represent a notebook.
    /// </summary>
    /// <param name="tag">
    /// A <see cref="Tag"/>
    /// </param>
    /// <returns>
    /// A <see cref="Notebook"/>
    /// </returns>
    Notebook::ORef NotebookManager::get_notebook_from_tag(const Tag &tag)
    {
      if (!is_notebook_tag(tag)) {
        return Notebook::ORef();
      }
      
      // Parse off the system and notebook prefix to get
      // the name of the notebook and then look it up.
      Glib::ustring systemNotebookPrefix = Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
        + Notebook::NOTEBOOK_TAG_PREFIX;
      Glib::ustring notebookName = sharp::string_substring(tag.name(), systemNotebookPrefix.size());
      
      return get_notebook(notebookName);
    }
    

    /// <summary>
    /// Evaluates the specified tag and returns <value>true</value>
    /// if it's a tag which represents a notebook.
    /// </summary>
    /// <param name="tag">
    /// A <see cref="Tag"/>
    /// </param>
    /// <returns>
    /// A <see cref="System.Boolean"/>
    /// </returns>
    bool NotebookManager::is_notebook_tag(const Tag & tag)
    {
      Glib::ustring fullTagName = tag.name();
      return Glib::str_has_prefix(fullTagName,
                                  Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
                                  + Notebook::NOTEBOOK_TAG_PREFIX);
    }


    void NotebookManager::prompt_create_new_notebook(IGnote & g, Gtk::Window & parent, std::function<void(Notebook::ORef)> on_complete)
    {
      return prompt_create_new_notebook(g, parent, std::vector<NoteBase::Ref>(), on_complete);
    }


    void NotebookManager::prompt_create_new_notebook(IGnote & g, Gtk::Window & parent, std::vector<NoteBase::Ref> && notes_to_add, std::function<void(Notebook::ORef)> on_complete)
    {
      // Prompt the user for the name of a new notebook
      auto dialog = Gtk::make_managed<CreateNotebookDialog>(&parent, (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), g);
      std::vector<Glib::ustring> notes;
      for(const NoteBase & note : notes_to_add) {
        notes.emplace_back(note.uri());
      }
      dialog->signal_response().connect([&g, dialog, notes=std::move(notes), on_complete](int response) { on_create_notebook_response(g, *dialog, response, notes, on_complete); });
      dialog->show();
    }


    void NotebookManager::on_create_notebook_response(IGnote & g, CreateNotebookDialog & dialog, int response, const std::vector<Glib::ustring> & notes_to_add,
      std::function<void(Notebook::ORef)> on_complete)
    {
      Glib::ustring notebookName = dialog.get_notebook_name();
      dialog.hide();
      if(response != Gtk::ResponseType::OK) {
        if(on_complete) {
          on_complete(Notebook::ORef());
        }
        return;
      }
      
      auto & notebook = g.notebook_manager().get_or_create_notebook (notebookName);
      DBG_OUT("Created the notebook: %s (%s)", notebook.get_name().c_str(), notebook.get_normalized_name().c_str());
        
      if(!notes_to_add.empty()) {
        // Move all the specified notesToAdd into the new notebook
        for(const auto & note : notes_to_add) {
          notebook.note_manager().find_by_uri(note, [&g, &notebook](NoteBase & note) {
            g.notebook_manager().move_note_to_notebook(static_cast<Note&>(note), notebook);
          });
        }
      }

      if(on_complete) {
        on_complete(notebook);
      }
    }
    
    void NotebookManager::prompt_delete_notebook(IGnote & g, Gtk::Window * parent, Notebook & notebook)
    {
      // Confirmation Dialog
      auto dialog = Gtk::make_managed<utils::HIGMessageDialog>(parent,
                                     GTK_DIALOG_MODAL,
                                     Gtk::MessageType::QUESTION,
                                     Gtk::ButtonsType::NONE,
                                     _("Really delete this notebook?"),
                                     _("The notes that belong to this notebook will not be "
                                       "deleted, but they will no longer be associated with "
                                       "this notebook.  This action cannot be undone."));

      Gtk::Button *button;
      button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
      dialog->add_action_widget(*button, Gtk::ResponseType::CANCEL);
      dialog->set_default_response(Gtk::ResponseType::CANCEL);

      button = Gtk::make_managed<Gtk::Button>(_("_Delete"), true);
      button->get_style_context()->add_class("destructive-action");
      dialog->add_action_widget(*button, Gtk::ResponseType::YES);

      dialog->signal_response().connect([&g, notebook = notebook.get_normalized_name(), dialog](int response) {
        dialog->hide();
        if(response != Gtk::ResponseType::YES) {
          return;
        }

        if(auto nb = g.notebook_manager().get_notebook(notebook)) {
          Notebook & nbook = nb.value();

          // Grab the template note before removing all the notebook tags
          auto & template_note = nbook.get_template_note();

          g.notebook_manager().delete_notebook(nbook);

          // Delete the template note
          g.notebook_manager().note_manager().delete_note(template_note);
        }
      });
      dialog->show();
    }


    /// <summary>
    /// Place the specified note into the specified notebook.  If the
    /// note already belongs to a notebook, it will be removed from that
    /// notebook first.
    /// </summary>
    /// <param name="note">
    /// A <see cref="Note"/>
    /// </param>
    /// <param name="notebook">
    /// A <see cref="Notebook"/>.  If Notebook is null, the note will
    /// be removed from its current notebook.
    /// </param>
    /// <returns>True if the note was successfully moved.</returns>
    bool NotebookManager::move_note_to_notebook(Note & note, Notebook::ORef notebook)
    {
      // NOTE: In the future we may want to allow notes
      // to exist in multiple notebooks.  For now, to
      // alleviate the confusion, only allow a note to
      // exist in one notebook at a time.

      auto currentNotebook = get_notebook_from_note(note);
      if(!currentNotebook && !notebook) {
        return true; // remove from without notebook
      }
      if(currentNotebook && notebook && &currentNotebook.value().get() == &notebook.value().get()) {
        return true; // It's already there.
      }

      if(currentNotebook) {
        Notebook & nb = currentNotebook.value();
        note.remove_tag(*nb.get_tag());
        signal_note_removed_from_notebook(note, nb);
      }

      // Only attempt to add the notebook tag when this
      // menu item is not the "No notebook" menu item.
      if(notebook) {
        Notebook &nb = notebook.value();
        note.add_tag(*nb.get_tag());
        signal_note_added_to_notebook(note, nb);
      }

      return true;
    }
 
    /// <summary>
    /// Loop through the system tags looking for notebooks
    /// </summary>
    void NotebookManager::load_notebooks()
    {
      Gtk::TreeIter<Gtk::TreeRow> iter;
      auto tags = m_note_manager.tag_manager().all_tags();
      auto prefix = Glib::ustring(Tag::SYSTEM_TAG_PREFIX) + Notebook::NOTEBOOK_TAG_PREFIX;
      for(const Tag &tag : tags) {
        // Skip over tags that aren't notebooks
        if(!tag.is_system() || !Glib::str_has_prefix(tag.name(), prefix)) {
          continue;
        }
        Notebook::Ptr notebook = Notebook::create(m_note_manager, tag);
        m_all_notebooks.push_back(notebook);
      }
    }

  }
}
