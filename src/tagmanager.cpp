/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2021-2022 Aurimas Cernius
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


#include <glibmm/stringutils.h>

#include "tagmanager.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "sharp/map.hpp"
#include "sharp/string.hpp"
#include "sharp/exception.hpp"

namespace gnote {

  namespace {
    int compare_tags_sort_func(const Gtk::TreeIter<Gtk::TreeConstRow> & a, const Gtk::TreeIter<Gtk::TreeConstRow> & b)
    {
      Tag::Ptr tag_a;
      a->get_value(0, tag_a);
      Tag::Ptr tag_b;
      b->get_value(0, tag_b);

      if (!tag_a || !tag_b)
        return 0;

      return strcmp(tag_a->normalized_name().c_str(), 
                    tag_b->normalized_name().c_str());
    }
  }

  TagManager::TagManager()
    :  m_tags(Gtk::ListStore::create(m_columns))
    ,  m_sorted_tags(Gtk::TreeModelSort::create(m_tags))
  {
    m_sorted_tags->set_sort_func (0, sigc::ptr_fun(&compare_tags_sort_func));
    m_sorted_tags->set_sort_column(0, Gtk::SortType::ASCENDING);
    
  }


  // <summary>
  // Return an existing tag for the specified tag name.  If no Tag exists
  // null will be returned.
  // </summary>
  Tag::Ptr TagManager::get_tag (const Glib::ustring & tag_name) const
  {
    if (tag_name.empty())
      throw sharp::Exception("TagManager.GetTag () called with a null tag name.");

    Glib::ustring normalized_tag_name = sharp::string_trim(tag_name).lowercase();
    if (normalized_tag_name.empty())
      throw sharp::Exception ("TagManager.GetTag () called with an empty tag name.");

    std::vector<Glib::ustring> splits;
    sharp::string_split(splits, normalized_tag_name, ":");
    if ((splits.size() > 2) || Glib::str_has_prefix(normalized_tag_name, Tag::SYSTEM_TAG_PREFIX)) {
      std::lock_guard<std::mutex> lock(m_locker);
      auto iter = m_internal_tags.find(normalized_tag_name);
      if(iter != m_internal_tags.end()) {
        return iter->second;
      }
      return Tag::Ptr();
    }
    auto iter = m_tag_map.find(normalized_tag_name);
    if (iter != m_tag_map.end()) {
      Gtk::TreeIter tree_iter = iter->second;
      return (*tree_iter)[m_columns.m_tag];
    }

    return Tag::Ptr();
  }
  
  // <summary>
  // Same as GetTag () but will create a new tag if one doesn't already exist.
  // </summary>
  Tag::Ptr TagManager::get_or_create_tag(const Glib::ustring & tag_name)
  {
    if (tag_name.empty())
      throw sharp::Exception ("TagManager.GetOrCreateTag () called with a null tag name.");

    Glib::ustring normalized_tag_name = sharp::string_trim(tag_name).lowercase();
    if (normalized_tag_name.empty())
      throw sharp::Exception ("TagManager.GetOrCreateTag () called with an empty tag name.");

    std::vector<Glib::ustring> splits;
    sharp::string_split(splits, normalized_tag_name, ":");
    if ((splits.size() > 2) || Glib::str_has_prefix(normalized_tag_name, Tag::SYSTEM_TAG_PREFIX)){
      std::lock_guard<std::mutex> lock(m_locker);
      auto iter = m_internal_tags.find(normalized_tag_name);
      if(iter != m_internal_tags.end()) {
        return iter->second;
      }
      else {
        Tag::Ptr t(std::make_shared<Tag>(Glib::ustring(tag_name)));
        m_internal_tags [ t->normalized_name() ] = t;
        return t;
      }
    }
    Gtk::TreeIter<Gtk::TreeRow> iter;
    Tag::Ptr tag = get_tag (normalized_tag_name);
    if (!tag) {
      std::lock_guard<std::mutex> lock(m_locker);

      tag = get_tag (normalized_tag_name);
      if (!tag) {
        tag = std::make_shared<Tag>(sharp::string_trim(tag_name));
        iter = m_tags->append ();
        (*iter)[m_columns.m_tag] = tag;
        m_tag_map [tag->normalized_name()] = iter;
      }
    }

    return tag;
  }
    
  /// <summary>
  /// Same as GetTag(), but for a system tag.
  /// </summary>
  /// <param name="tag_name">
  /// A <see cref="System.String"/>.  This method will handle adding
  /// any needed "system:" or identifier needed.
  /// </param>
  /// <returns>
  /// A <see cref="Tag"/>
  /// </returns>
  Tag::Ptr TagManager::get_system_tag (const Glib::ustring & tag_name) const
  {
    return get_tag(Tag::SYSTEM_TAG_PREFIX + tag_name);
  }
    
  /// <summary>
  /// Same as <see cref="Tomboy.TagManager.GetSystemTag"/> except that
  /// a new tag will be created if the specified one doesn't exist.
  /// </summary>
  /// <param name="tag_name">
  /// A <see cref="System.String"/>
  /// </param>
  /// <returns>
  /// A <see cref="Tag"/>
  /// </returns>
  Tag::Ptr TagManager::get_or_create_system_tag (const Glib::ustring & tag_name)
  {
    return get_or_create_tag(Tag::SYSTEM_TAG_PREFIX + tag_name);
  }
    

    // <summary>
    // This will remove the tag from every note that is currently tagged
    // and from the main list of tags.
    // </summary>
  void TagManager::remove_tag (const Tag::Ptr & tag)
  {
    if (!tag)
      throw sharp::Exception ("TagManager.RemoveTag () called with a null tag");

    if(tag->is_property() || tag->is_system()){
      std::lock_guard<std::mutex> lock(m_locker);

      m_internal_tags.erase(tag->normalized_name());
    }
    auto map_iter = m_tag_map.find(tag->normalized_name());
    if (map_iter != m_tag_map.end()) {
      std::lock_guard<std::mutex> lock(m_locker);

      map_iter = m_tag_map.find(tag->normalized_name());
      if (map_iter != m_tag_map.end()) {
        Gtk::TreeIter iter = map_iter->second;
        if (!m_tags->erase(iter)) {
          DBG_OUT("TagManager: Removed tag: %s", tag->normalized_name().c_str());
        } 
        else { 
          // FIXME: For some really weird reason, this block actually gets called sometimes!
          DBG_OUT("TagManager: Call to remove tag from ListStore failed: %s", tag->normalized_name().c_str());
        }

        m_tag_map.erase(map_iter);
        DBG_OUT("Removed TreeIter from tag_map: %s", tag->normalized_name().c_str());

        auto notes = tag->get_notes();
        for(NoteBase *note_iter : notes) {
          note_iter->remove_tag(tag);
        }
      }
    }
  }
  
  std::vector<Tag::Ptr> TagManager::all_tags() const
  {
    std::vector<Tag::Ptr> tags;

    // Add in the system tags first
    tags = sharp::map_get_values(m_internal_tags);

    // Now all the other tags
    for(TagMap::const_iterator iter = m_tag_map.begin();
        iter != m_tag_map.end(); ++iter) {
      Tag::Ptr tag;
      iter->second->get_value(0, tag);      
      tags.push_back(tag);
    }

    return tags;
  }

}

