/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019,2022-2023 Aurimas Cernius
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
#include <gtkmm/treemodelsort.h>

#include "sharp/string.hpp"
#include "sharp/exception.hpp"
#include "notebooks/createnotebookdialog.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "debug.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"


namespace gnote {
  namespace notebooks {

    NotebookManager::NotebookManager(NoteManagerBase & manager)
      : m_adding_notebook(false)
      , m_active_notes(new ActiveNotesNotebook(manager))
      , m_note_manager(manager)
    { 
    }

    void NotebookManager::init()
    {
      m_notebooks = Gtk::ListStore::create(m_column_types);

      m_sortedNotebooks = Gtk::TreeModelSort::create (m_notebooks);
      m_sortedNotebooks->set_sort_func (
        0, sigc::ptr_fun(&NotebookManager::compare_notebooks_sort_func));
      m_sortedNotebooks->set_sort_column (0, Gtk::SORT_ASCENDING);

      m_notebooks_to_display = Gtk::TreeModelFilter::create(m_sortedNotebooks);
      m_notebooks_to_display->set_visible_func(
        sigc::mem_fun(*this, &NotebookManager::filter_notebooks_to_display));

      m_filteredNotebooks = Gtk::TreeModelFilter::create (m_sortedNotebooks);
      m_filteredNotebooks->set_visible_func(
        sigc::ptr_fun(&NotebookManager::filter_notebooks));

      Notebook::Ptr allNotesNotebook(std::make_shared<AllNotesNotebook>(m_note_manager));
      Gtk::TreeIter iter = m_notebooks->append ();
      iter->set_value(0, Notebook::Ptr(allNotesNotebook));

      Notebook::Ptr unfiledNotesNotebook(std::make_shared<UnfiledNotesNotebook>(m_note_manager));
      iter = m_notebooks->append ();
      iter->set_value(0, Notebook::Ptr(unfiledNotesNotebook));

      Notebook::Ptr pinned_notes_notebook(std::make_shared<PinnedNotesNotebook>(m_note_manager));
      iter = m_notebooks->append();
      iter->set_value(0, pinned_notes_notebook);

      iter = m_notebooks->append();
      iter->set_value(0, m_active_notes);
      std::static_pointer_cast<ActiveNotesNotebook>(m_active_notes)->signal_size_changed
        .connect(sigc::mem_fun(*this, &NotebookManager::on_active_notes_size_changed));

      load_notebooks ();
    }


    Notebook::Ptr NotebookManager::get_notebook(const Glib::ustring & notebookName) const
    {
      if (notebookName.empty()) {
        throw sharp::Exception ("NotebookManager::get_notebook() called with an empty name.");
      }
      Glib::ustring normalizedName = Notebook::normalize(notebookName);
      if (normalizedName.empty()) {
        throw sharp::Exception ("NotebookManager::get_notebook() called with an empty name.");
      }
      auto map_iter = m_notebookMap.find(normalizedName);
      if (map_iter != m_notebookMap.end()) {
        Gtk::TreeIter iter = map_iter->second;
        Notebook::Ptr notebook;
        iter->get_value(0, notebook);
        return notebook;
      }
      
      return Notebook::Ptr();
    }
    

    bool NotebookManager::notebook_exists(const Glib::ustring & notebookName) const
    {
      Glib::ustring normalizedName = Notebook::normalize(notebookName);
      return m_notebookMap.find(normalizedName) != m_notebookMap.end();
    }

    Notebook::Ptr NotebookManager::get_or_create_notebook(const Glib::ustring & notebookName)
    {
      if (notebookName.empty())
        throw sharp::Exception ("NotebookManager.GetNotebook () called with a null name.");
      
      Notebook::Ptr notebook = get_notebook (notebookName);
      if (notebook) {
        return notebook;
      }
      
      Gtk::TreeIter iter;
//      lock (locker) {
        notebook = get_notebook (notebookName);
        if (notebook)
          return notebook;
        
        try {
          m_adding_notebook = true;
          notebook = std::make_shared<Notebook>(m_note_manager, notebookName);
        } 
        catch(...)
        {
          // set flag to fast and rethrow
          m_adding_notebook = false;
          throw;
        }
        m_adding_notebook = false;
        iter = m_notebooks->append ();
        iter->set_value(0, notebook);
        m_notebookMap [notebook->get_normalized_name()] = iter;
        
        // Create the template note so the system tag
        // that represents the notebook actually gets
        // saved to a note (and persisted after Tomboy
        // is shut down).
        Note::Ptr templateNote = notebook->get_template_note ();
        
        // Make sure the template note has the notebook tag.
        // Since it's possible for the template note to already
        // exist, we need to make sure it gets tagged.
        templateNote->add_tag (notebook->get_tag());
        m_note_added_to_notebook (*templateNote, notebook);
//      }

      signal_notebook_list_changed();
      return notebook;
    }

    bool NotebookManager::add_notebook(const Notebook::Ptr & notebook)
    {
      if(m_notebookMap.find(notebook->get_normalized_name()) != m_notebookMap.end()) {
        return false;
      }

      Gtk::TreeIter iter = m_notebooks->append();
      iter->set_value(0, notebook);
      m_notebookMap[notebook->get_normalized_name()] = iter;
      signal_notebook_list_changed();
      return true;
    }

    void NotebookManager::delete_notebook(const Notebook::Ptr & notebook)
    {
      if (!notebook)
        throw sharp::Exception ("NotebookManager::delete_notebook () called with a null argument.");
      Glib::ustring normalized_name = notebook->get_normalized_name();
      auto map_iter = m_notebookMap.find (normalized_name);
      if (map_iter == m_notebookMap.end())
        return;
      
//      lock (locker) {
        map_iter = m_notebookMap.find (normalized_name);
        if (map_iter == m_notebookMap.end()) {
          return;
        }
        
        Gtk::TreeIter iter = map_iter->second;;
        // first remove notebook from map, then from store, because the later cases a UI refresh, that can query back here
        m_notebookMap.erase(map_iter);
        m_notebooks->erase (iter);

        // Remove the notebook tag from every note that's in the notebook
        std::vector<NoteBase*> notes;
        Tag::Ptr tag = notebook->get_tag();
        if(tag) {
          notes = tag->get_notes();
        }
        for(NoteBase *note : notes) {
          note->remove_tag (notebook->get_tag());
          m_note_removed_from_notebook (*static_cast<Note*>(note), notebook);
        }
//      }
      signal_notebook_list_changed();
    }

    /// <summary>
    /// Returns the Gtk.TreeIter that points to the specified Notebook.
    /// </summary>
    /// <param name="notebook">
    /// A <see cref="Notebook"/>
    /// </param>
    /// <param name="iter">
    /// A <see cref="Gtk.TreeIter"/>.  Will be set to a valid iter if
    /// the specified notebook is found.
    /// </param>
    /// <returns>
    /// A <see cref="System.Boolean"/>.  True if the specified notebook
    /// was found, false otherwise.
    /// </returns>
    bool NotebookManager::get_notebook_iter(const Notebook::Ptr & notebook, 
                                            Gtk::TreeIter & iter)
    {
      Gtk::TreeNodeChildren notebooks = m_notebooks_to_display->children();
      for (Gtk::TreeIter notebooks_iter = notebooks.begin();
           notebooks_iter != notebooks.end(); ++notebooks_iter) {
        Notebook::Ptr current_notebook;
        notebooks_iter->get_value(0, current_notebook);
        if (current_notebook == notebook) {
          iter = notebooks_iter;
          return true;
        }
      }
      
      iter = Gtk::TreeIter();
      return false;
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
    Notebook::Ptr NotebookManager::get_notebook_from_note(const NoteBase::Ptr & note)
    {
      std::vector<Tag::Ptr> tags = note->get_tags();
      for(auto & tag : tags) {
        Notebook::Ptr notebook = get_notebook_from_tag(tag);
        if (notebook)
          return notebook;
      }
      
      return Notebook::Ptr();
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
    Notebook::Ptr NotebookManager::get_notebook_from_tag(const Tag::Ptr &tag)
    {
      if (!is_notebook_tag (tag)) {
        return Notebook::Ptr();
      }
      
      // Parse off the system and notebook prefix to get
      // the name of the notebook and then look it up.
      Glib::ustring systemNotebookPrefix = Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
        + Notebook::NOTEBOOK_TAG_PREFIX;
      Glib::ustring notebookName = sharp::string_substring(tag->name(),
                                                         systemNotebookPrefix.size());
      
      return get_notebook (notebookName);
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
    bool NotebookManager::is_notebook_tag(const Tag::Ptr & tag)
    {
      Glib::ustring fullTagName = tag->name();
      return Glib::str_has_prefix(fullTagName,
                                  Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
                                  + Notebook::NOTEBOOK_TAG_PREFIX);
    }


    Notebook::Ptr NotebookManager::prompt_create_new_notebook(IGnote & g, Gtk::Window *parent)
    {
      return prompt_create_new_notebook(g, parent, Note::List());
    }


    Notebook::Ptr NotebookManager::prompt_create_new_notebook(IGnote & g, Gtk::Window *parent, const Note::List & notesToAdd)
    {
      // Prompt the user for the name of a new notebook
      CreateNotebookDialog dialog(parent, (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), g);
      
      
      int response = dialog.run ();
      Glib::ustring notebookName = dialog.get_notebook_name();
      if (response != Gtk::RESPONSE_OK)
        return Notebook::Ptr();
      
      Notebook::Ptr notebook = g.notebook_manager().get_or_create_notebook (notebookName);
      if (!notebook) {
        DBG_OUT ("Could not create notebook: %s", notebookName.c_str());
      } 
      else {
        DBG_OUT ("Created the notebook: %s (%s)", notebook->get_name().c_str(),
                 notebook->get_normalized_name().c_str());
        
        if (!notesToAdd.empty()) {
          // Move all the specified notesToAdd into the new notebook
          for(Note::List::const_iterator iter = notesToAdd.begin();
              iter != notesToAdd.end(); ++iter) {
            g.notebook_manager().move_note_to_notebook (*iter, notebook);
          }
        }
      }
      
      return notebook;
    }
    
    void NotebookManager::prompt_delete_notebook(IGnote & g, Gtk::Window * parent, const Notebook::Ptr & notebook)
    {
      // Confirmation Dialog
      utils::HIGMessageDialog dialog(parent,
                                     GTK_DIALOG_MODAL,
                                     Gtk::MESSAGE_QUESTION,
                                     Gtk::BUTTONS_NONE,
                                     _("Really delete this notebook?"),
                                     _("The notes that belong to this notebook will not be "
                                       "deleted, but they will no longer be associated with "
                                       "this notebook.  This action cannot be undone."));

      Gtk::Button *button;
      button = manage(new Gtk::Button(_("_Cancel"), true));
      button->property_can_default().set_value(true);
      button->show();
      dialog.add_action_widget(*button, Gtk::RESPONSE_CANCEL);
      dialog.set_default_response(Gtk::RESPONSE_CANCEL);

      button = manage(new Gtk::Button(_("_Delete"), true));
      button->property_can_default().set_value(true);
      button->get_style_context()->add_class("destructive-action");
      button->show();
      dialog.add_action_widget(*button, Gtk::RESPONSE_YES);

      int response = dialog.run();
      if(response != Gtk::RESPONSE_YES) {
        return;
      }

      // Grab the template note before removing all the notebook tags
      Note::Ptr templateNote = notebook->get_template_note ();
      
      g.notebook_manager().delete_notebook(notebook);

      // Delete the template note
      if (templateNote) {
        g.notebook_manager().note_manager().delete_note(templateNote);
      }
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
    bool NotebookManager::move_note_to_notebook (const Note::Ptr & note, 
                                                 const Notebook::Ptr & notebook)
    {
      if (!note) {
        return false;
      }

      // NOTE: In the future we may want to allow notes
      // to exist in multiple notebooks.  For now, to
      // alleviate the confusion, only allow a note to
      // exist in one notebook at a time.

      Notebook::Ptr currentNotebook = get_notebook_from_note (note);
      if (currentNotebook == notebook)
        return true; // It's already there.

      if(currentNotebook) {
        note->remove_tag (currentNotebook->get_tag());
        m_note_removed_from_notebook(*note, currentNotebook);
      }

      // Only attempt to add the notebook tag when this
      // menu item is not the "No notebook" menu item.
      if(notebook) {
        note->add_tag(notebook->get_tag());
        m_note_added_to_notebook(*note, notebook);
      }

      return true;
    }


    int NotebookManager::compare_notebooks_sort_func(const Gtk::TreeIter &a, 
                                                     const Gtk::TreeIter &b)
    {
      Notebook::Ptr notebook_a;
      a->get_value (0, notebook_a);
      Notebook::Ptr notebook_b;
      b->get_value (0, notebook_b);

      if (!notebook_a || !notebook_b)
        return 0;

      SpecialNotebook::Ptr spec_a = std::dynamic_pointer_cast<SpecialNotebook>(notebook_a);
      SpecialNotebook::Ptr spec_b = std::dynamic_pointer_cast<SpecialNotebook>(notebook_b);
      if(spec_a != 0 && spec_b != 0) {
        return strcmp(spec_a->get_normalized_name().c_str(), spec_b->get_normalized_name().c_str());
      }
      else if(spec_a != 0) {
        return -1;
      }
      else if(spec_b != 0) {
        return 1;
      }

      Glib::ustring a_name(notebook_a->get_name());
      a_name = a_name.lowercase();
      Glib::ustring b_name(notebook_b->get_name());
      b_name = b_name.lowercase();
      return a_name.compare(b_name);
    }
    
    /// <summary>
    /// Loop through the system tags looking for notebooks
    /// </summary>
    void NotebookManager::load_notebooks()
    {
      Gtk::TreeIter iter;
      auto tags = m_note_manager.tag_manager().all_tags();
      for(const auto & tag : tags) {
        // Skip over tags that aren't notebooks
        if (!tag->is_system()
            || !Glib::str_has_prefix(tag->name(),
                                     Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
                                     + Notebook::NOTEBOOK_TAG_PREFIX)) {
          continue;
        }
        Notebook::Ptr notebook = std::make_shared<Notebook>(m_note_manager, tag);
        iter = m_notebooks->append ();
        iter->set_value(0, notebook);
        m_notebookMap [notebook->get_normalized_name()] = iter;
      }
    }

    /// <summary>
    /// Filter out SpecialNotebooks from the model
    /// </summary>
    bool NotebookManager::filter_notebooks(const Gtk::TreeIter & iter)
    {
      Notebook::Ptr notebook;
      iter->get_value(0, notebook);
      if (!notebook || std::dynamic_pointer_cast<SpecialNotebook>(notebook)) {
        return false;
      }
      return true;
    }

    bool NotebookManager::filter_notebooks_to_display(const Gtk::TreeIter & iter)
    {
      Notebook::Ptr notebook;
      iter->get_value(0, notebook);
      if(notebook == m_active_notes) {
        return !std::static_pointer_cast<ActiveNotesNotebook>(m_active_notes)->empty();
      }

      return true;
    }

    void NotebookManager::on_active_notes_size_changed()
    {
      m_notebooks_to_display->refilter();
    }


  }
}
