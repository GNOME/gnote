/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2021-2022,2024 Aurimas Cernius
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

  TagManager::TagManager()
  {
  }


  // <summary>
  // Return an existing tag for the specified tag name.  If no Tag exists
  // null will be returned.
  // </summary>
  Tag::ORef TagManager::get_tag(const Glib::ustring & tag_name) const
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
        return *iter->second;
      }
      return Tag::ORef();
    }

    for(auto &tag : m_tags) {
      if(tag->normalized_name() == normalized_tag_name) {
        return *tag;
      }
    }

    return Tag::ORef();
  }
  
  // <summary>
  // Same as GetTag () but will create a new tag if one doesn't already exist.
  // </summary>
  Tag &TagManager::get_or_create_tag(const Glib::ustring & tag_name)
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
        return *iter->second;
      }
      else {
        TagPtr t(new Tag(Glib::ustring(tag_name)));
        Tag& ret = *t;
        m_internal_tags[ret.normalized_name()] = std::move(t);
        return ret;
      }
    }
    auto tag = get_tag(normalized_tag_name);
    if(!tag) {
      std::lock_guard<std::mutex> lock(m_locker);

      tag = get_tag(normalized_tag_name);
      if(!tag) {
        TagPtr tg(new Tag(sharp::string_trim(tag_name)));
        tag = *tg;
        m_tags.emplace_back(std::move(tg));
      }
    }

    return *tag;
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
  Tag::ORef TagManager::get_system_tag (const Glib::ustring & tag_name) const
  {
    if(auto tag = get_tag(Tag::SYSTEM_TAG_PREFIX + tag_name)) {
      return *tag;
    }
    return Tag::ORef();
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
  Tag &TagManager::get_or_create_system_tag(const Glib::ustring & tag_name)
  {
    return get_or_create_tag(Tag::SYSTEM_TAG_PREFIX + tag_name);
  }
    

    // <summary>
    // This will remove the tag from every note that is currently tagged
    // and from the main list of tags.
    // </summary>
  void TagManager::remove_tag(Tag &tag)
  {
    auto tag_name = tag.normalized_name();
    std::lock_guard<std::mutex> lock(m_locker);
    if(tag.is_property() || tag.is_system()) {
      m_internal_tags.erase(tag_name);
    }

    auto iter = std::find_if(m_tags.begin(), m_tags.end(), [&tag](const TagPtr &t) { return t.get() == &tag; });
    if(iter != m_tags.end()) {
      m_tags.erase(iter);
      DBG_OUT("TagManager: Removed tag: %s", tag_name.c_str());
    }
    else {
      // FIXME: For some really weird reason, this block actually gets called sometimes!
      DBG_OUT("TagManager: Call to remove tag from ListStore failed: %s", tag_name.c_str());
    }

    auto notes = tag.get_notes();
    for(NoteBase *note_iter : notes) {
      note_iter->remove_tag(tag);
    }
  }
  
  std::vector<Tag::Ref> TagManager::all_tags() const
  {
    std::vector<Tag::Ref> tags;
    tags.reserve(m_internal_tags.size() + m_tags.size());
    // Add in the system tags first
    for(auto &tag : m_internal_tags) {
      tags.emplace_back(*tag.second);
    }
    for(auto &tag : m_tags) {
      tags.emplace_back(*tag);
    }
    return tags;
  }

}

