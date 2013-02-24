/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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



/* ported from */
/***************************************************************************
 *  ActionManager.cs
 *
 *  Copyright (C) 2005-2006 Novell, Inc.
 *  Written by Aaron Bockover <aaron@abock.org>
 ****************************************************************************/

/*  THIS FILE IS LICENSED UNDER THE MIT LICENSE AS OUTLINED IMMEDIATELY BELOW:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/window.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include <libxml/tree.h>

#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "iconmanager.hpp"

namespace gnote {

  ActionManager::ActionManager()
    : m_ui(Gtk::UIManager::create())
    , m_main_window_actions(Gtk::ActionGroup::create("MainWindow"))
  {
    populate_action_groups();
    make_app_actions();
    make_app_menu_items();
  }


  void ActionManager::load_interface()
  {
    Gtk::UIManager::ui_merge_id id = m_ui->add_ui_from_file(DATADIR"/gnote/UIManagerLayout.xml");
    DBG_ASSERT(id, "merge failed");
    Gtk::Window::set_default_icon_name("gnote");

    Gtk::ImageMenuItem *imageitem = (Gtk::ImageMenuItem*)m_ui->get_widget (
      "/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");
    DBG_ASSERT(imageitem, "Item not found");
    if (imageitem) {
      imageitem->set_image(*manage(new Gtk::Image(IconManager::obj().get_icon(IconManager::NOTE_NEW, 16))));
    }
  }


  /// <summary>
  /// Get all widgets represents by XML elements that are children
  /// of the placeholder element specified by path.
  /// </summary>
  /// <param name="path">
  /// A <see cref="System.String"/> representing the path to
  /// the placeholder of interest.
  /// </param>
  /// <returns>
  /// A <see cref="IList`1"/> of Gtk.Widget objects corresponding
  /// to the XML child elements of the placeholder element.
  /// </returns>
  void ActionManager::get_placeholder_children(const std::string & path, 
                                               std::list<Gtk::Widget*> & children) const
  {
    // Wrap the UIManager XML in a root element
    // so that it's real parseable XML.
    std::string xml = "<root>" + m_ui->get_ui() + "</root>";

    xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml.c_str());
    if(doc == NULL) {
      return;
    }
        
    // Get the element name
    std::string placeholderName = sharp::string_substring(path, sharp::string_last_index_of(
                                                            path, "/") + 1);
    DBG_OUT("path = %s placeholdername = %s", path.c_str(), placeholderName.c_str());

    sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(xmlDocGetRootElement(doc), 
                                                         "//placeholder");
    // Find the placeholder specified in the path
    for(sharp::XmlNodeSet::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {
      xmlNodePtr placeholderNode = *iter;

      if (placeholderNode->type == XML_ELEMENT_NODE) {

        xmlChar * prop = xmlGetProp(placeholderNode, (const xmlChar*)"name");
        if(!prop) {
          continue;
        }
        if(xmlStrEqual(prop, (const xmlChar*)placeholderName.c_str())) {

          // Return each child element's widget
          for(xmlNodePtr widgetNode = placeholderNode->children;
              widgetNode; widgetNode = widgetNode->next) {

            if(widgetNode->type == XML_ELEMENT_NODE) {

              xmlChar * widgetName = xmlGetProp(widgetNode, (const xmlChar*)"name");
              if(widgetName) {
                children.push_back(get_widget(path + "/"
                                              + (const char*)widgetName));
                xmlFree(widgetName);
              }
            }
          }
        }
        xmlFree(prop);
      }
    }
    xmlFreeDoc(doc);
  }


  void ActionManager::populate_action_groups()
  {
    Glib::RefPtr<Gtk::Action> action;

    action = Gtk::Action::create(
      "QuitGNoteAction", Gtk::Stock::QUIT,
      _("_Quit"), _("Quit Gnote"));
    m_main_window_actions->add(action, Gtk::AccelKey("<Control>Q"));

    action = Gtk::Action::create(
      "ShowPreferencesAction", Gtk::Stock::PREFERENCES,
      _("_Preferences"), _("Gnote Preferences"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create("ShowHelpAction", Gtk::Stock::HELP,
      _("_Contents"), _("Gnote Help"));
    m_main_window_actions->add(action, Gtk::AccelKey("F1"));

    action = Gtk::Action::create(
      "ShowAboutAction", Gtk::Stock::ABOUT,
      _("_About"), _("About Gnote"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "TrayIconMenuAction",  _("TrayIcon"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "TrayNewNoteAction", Gtk::Stock::NEW,
      _("Create _New Note"), _("Create a new note"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "ShowSearchAllNotesAction", Gtk::Stock::FIND,
      _("_Search All Notes"),  _("Open the Search All Notes window"));
    m_main_window_actions->add(action);

    m_ui->insert_action_group(m_main_window_actions);
  }

  Glib::RefPtr<Gtk::Action> ActionManager::find_action_by_name(const std::string & n) const
  {
    Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> > actiongroups = m_ui->get_action_groups();
    for(Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> >::const_iterator iter(actiongroups.begin()); 
        iter != actiongroups.end(); ++iter) {
      Glib::ListHandle<Glib::RefPtr<Gtk::Action> > actions = (*iter)->get_actions();
      for(Glib::ListHandle<Glib::RefPtr<Gtk::Action> >::const_iterator iter2(actions.begin()); 
          iter2 != actions.end(); ++iter2) {
        if((*iter2)->get_name() == n) {
          return *iter2;
        }
      }
    }
    DBG_OUT("%s not found", n.c_str());
    return Glib::RefPtr<Gtk::Action>();      
  }

  void ActionManager::make_app_actions()
  {
    add_app_action("new-note");
    add_app_action("new-window");
    add_app_action("show-preferences");
    add_app_action("about");
    add_app_action("help-contents");
    add_app_action("quit");
  }

  Glib::RefPtr<Gio::SimpleAction> ActionManager::get_app_action(const std::string & name) const
  {
    for(std::vector<Glib::RefPtr<Gio::SimpleAction> >::const_iterator iter = m_app_actions.begin();
        iter != m_app_actions.end(); ++iter) {
      if((*iter)->get_name() == name) {
        return *iter;
      }
    }
    
    return Glib::RefPtr<Gio::SimpleAction>();
  }

  Glib::RefPtr<Gio::SimpleAction> ActionManager::add_app_action(const std::string & name)
  {
    Glib::RefPtr<Gio::SimpleAction> action = Gio::SimpleAction::create(name);
    m_app_actions.push_back(action);
    return action;
  }

  void ActionManager::add_app_menu_item(int section, int order, const std::string & label,
                                        const std::string & action_def)
  {
    m_app_menu_items.insert(std::make_pair(section, AppMenuItem(order, label, action_def)));
  }

  void ActionManager::make_app_menu_items()
  {
    add_app_menu_item(APP_ACTION_NEW, 100, _("_New Note"), "app.new-note");
    add_app_menu_item(APP_ACTION_NEW, 200, _("New _Window"), "app.new-window");
    add_app_menu_item(APP_ACTION_MANAGE, 100, _("_Preferences"), "app.show-preferences");
    add_app_menu_item(APP_ACTION_HELP, 100, _("Help _Contents"), "app.help-contents");
    add_app_menu_item(APP_ACTION_HELP, 200, _("_About"), "app.about");
    add_app_menu_item(APP_ACTION_LAST, 100, _("_Quit"), "app.quit");
  }

  Glib::RefPtr<Gio::Menu> ActionManager::get_app_menu() const
  {
    Glib::RefPtr<Gio::Menu> menu = Gio::Menu::create();

    int pos = 0;
    Glib::RefPtr<Gio::Menu> section = make_app_menu_section(APP_ACTION_NEW);
    if(section != 0) {
      menu->insert_section(pos++, "", section);
    }

    section = make_app_menu_section(APP_ACTION_MANAGE);
    if(section != 0) {
      menu->insert_section(pos++, "", section);
    }

    section = make_app_menu_section(APP_ACTION_HELP);
    if(section != 0) {
      menu->insert_section(pos++, "", section);
    }

    section = make_app_menu_section(APP_ACTION_LAST);
    if(section != 0) {
      menu->insert_section(pos++, "", section);
    }

    return menu;
  }

  Glib::RefPtr<Gio::Menu> ActionManager::make_app_menu_section(int sec) const
  {
    std::pair<AppMenuItemMultiMap::const_iterator, AppMenuItemMultiMap::const_iterator>
    range = m_app_menu_items.equal_range(sec);

    Glib::RefPtr<Gio::Menu> section;
    if(range.first != m_app_menu_items.end()) {
      std::vector<const AppMenuItem*> menu_items;
      for(AppMenuItemMultiMap::const_iterator iter = range.first; iter != range.second; ++iter) {
        menu_items.push_back(&iter->second);
      }
      std::sort(menu_items.begin(), menu_items.end(), AppMenuItem::ptr_comparator());

      section = Gio::Menu::create();
      for(std::vector<const AppMenuItem*>::iterator iter = menu_items.begin(); iter != menu_items.end(); ++iter) {
        section->append((*iter)->label, (*iter)->action_def);
      }
    }

    return section;
  }

  void ActionManager::add_main_window_search_action(const Glib::RefPtr<Gtk::Action> & action, int order)
  {
    add_main_window_action(m_main_window_search_actions, action, order);
    signal_main_window_search_actions_changed();
  }

  void ActionManager::remove_main_window_search_action(const std::string & name)
  {
    remove_main_window_action(m_main_window_search_actions, name);
    signal_main_window_search_actions_changed();
  }

  std::vector<Glib::RefPtr<Gtk::Action> > ActionManager::get_main_window_search_actions()
  {
    return get_main_window_actions(m_main_window_search_actions);
  }

  void ActionManager::add_main_window_note_action(const Glib::RefPtr<Gtk::Action> & action, int order)
  {
    add_main_window_action(m_main_window_note_actions, action, order);
    signal_main_window_note_actions_changed();
  }

  void ActionManager::remove_main_window_note_action(const std::string & name)
  {
    remove_main_window_action(m_main_window_note_actions, name);
    signal_main_window_note_actions_changed();
  }

  std::vector<Glib::RefPtr<Gtk::Action> > ActionManager::get_main_window_note_actions()
  {
    return get_main_window_actions(m_main_window_note_actions);
  }

  void ActionManager::add_main_window_action(std::map<int, Glib::RefPtr<Gtk::Action> > & actions,
                                             const Glib::RefPtr<Gtk::Action> & action, int order)
  {
    std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = actions.find(order);
    while(iter != actions.end()) {
      iter = actions.find(++order);
    }
    actions[order] = action;
  }

  void ActionManager::remove_main_window_action(std::map<int, Glib::RefPtr<Gtk::Action> > & actions,
    const std::string & name)
  {
    for(std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = actions.begin();
        iter != actions.end(); ++iter) {
      if(iter->second->get_name() == name) {
        actions.erase(iter);
        break;
      }
    }
  }

  std::vector<Glib::RefPtr<Gtk::Action> > ActionManager::get_main_window_actions(
    std::map<int, Glib::RefPtr<Gtk::Action> > & actions)
  {
    std::vector<Glib::RefPtr<Gtk::Action> > res;
    for(std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = actions.begin();
        iter != actions.end(); ++iter) {
      res.push_back(iter->second);
    }
    return res;
  }

}
