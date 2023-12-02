/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013,2017,2019,2022-2023 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <glibmm/i18n.h>
#include <gtkmm/expression.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/sortlistmodel.h>
#include <gtkmm/stringsorter.h>

#include "sharp/pluginsmodel.hpp"
#include "abstractaddin.hpp"
#include "utils.hpp"


namespace {

class PluginNameFactory
  : public Gtk::SignalListItemFactory
{
public:
  static Glib::RefPtr<PluginNameFactory> create()
  {
    return Glib::make_refptr_for_instance(new PluginNameFactory);
  }
private:
  PluginNameFactory()
  {
    signal_setup().connect(sigc::mem_fun(*this, &PluginNameFactory::on_setup));
    signal_bind().connect(sigc::mem_fun(*this, &PluginNameFactory::on_bind));
  }

  void on_setup(const Glib::RefPtr<Gtk::ListItem> & list_item)
  {
    auto child = Gtk::make_managed<Gtk::Grid>();
    auto image = Gtk::make_managed<Gtk::Image>();
    image->property_icon_name() = "application-x-addon-symbolic";
    image->set_margin_end(5);
    child->attach(*image, 0, 0);
    auto label = Gtk::make_managed<Gtk::Label>();
    child->attach(*label, 1, 0);
    list_item->set_child(*child);
  }

  void on_bind(const Glib::RefPtr<Gtk::ListItem> & list_item)
  {
    auto child = dynamic_cast<Gtk::Grid*>(list_item->get_child());
    auto item = std::dynamic_pointer_cast<sharp::Plugin>(list_item->get_item());
    auto label = dynamic_cast<Gtk::Label*>(child->get_child_at(1, 0));
    auto module = item->module();
    auto color = (module && module->is_enabled()) ? "black" : "grey";
    label->set_markup(Glib::ustring::compose("<span foreground=\"%2\">%1</span>", item->info.name(), color));
  }
};

class PluginVersionFactory
  : public gnote::utils::LabelFactory
{
public:
  static Glib::RefPtr<PluginVersionFactory> create()
  {
    return Glib::make_refptr_for_instance(new PluginVersionFactory);
  }
protected:
  Glib::ustring get_text(Gtk::ListItem & item) override
  {
    auto plugin = std::dynamic_pointer_cast<sharp::Plugin>(item.get_item());
    return plugin->info.version();
  }
};

class PluginCategoryFactory
  : public gnote::utils::LabelFactory
{
public:
  static Glib::RefPtr<PluginCategoryFactory> create()
  {
    return Glib::make_refptr_for_instance(new PluginCategoryFactory);
  }
protected:
  Glib::ustring get_text(Gtk::ListItem & item) override
  {
    auto plugin = std::dynamic_pointer_cast<sharp::Plugin>(item.get_item());
    auto category = plugin->info.category();
    return sharp::AddinsModel::get_addin_category_name(category);
  }
};

}


namespace sharp {

  Glib::RefPtr<Plugin> Plugin::create(const gnote::AddinInfo & info, const sharp::DynamicModule *module)
  {
    return Glib::make_refptr_for_instance(new Plugin(info, module));
  }

  Plugin::Plugin(const gnote::AddinInfo & info, const sharp::DynamicModule *module)
    : info(info)
    , m_module(module)
  {
  }

  void Plugin::set_module(DynamicModule *mod)
  {
    m_module = mod;
  }


  AddinsModel::Ptr AddinsModel::create(Gtk::ColumnView *view)
  {
    auto self = new AddinsModel;
    auto p = Glib::make_refptr_for_instance(self);
    if(view) {
      auto expr = Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase> & obj) {
        return get_addin_category_name(std::dynamic_pointer_cast<Plugin>(obj)->info.category());
      });
      auto sorter = Gtk::StringSorter::create(expr);
      self->m_selection = Gtk::SingleSelection::create(Gtk::SortListModel::create(p, sorter));
      view->set_model(self->m_selection);
      p->set_columns(view);
      self->m_selection->signal_selection_changed().connect(sigc::mem_fun(*self, &AddinsModel::on_selection_changed));
    }
    return p;
  }

  AddinsModel::AddinsModel()
  {
  }

  Glib::RefPtr<Plugin> AddinsModel::get_selected_plugin()
  {
    auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(m_selection);
    if(selection) {
      return std::dynamic_pointer_cast<Plugin>(selection->get_selected_item());
    }

    return Glib::RefPtr<Plugin>();
  }

  void AddinsModel::on_selection_changed(guint, guint)
  {
    signal_selection_changed(get_selected_plugin());
  }

  void AddinsModel::set_columns(Gtk::ColumnView *view)
  {
    auto column = Gtk::ColumnViewColumn::create(_("Name"), PluginNameFactory::create());
    column->set_resizable(true);
    view->append_column(column);
    column = Gtk::ColumnViewColumn::create(_("Version"), PluginVersionFactory::create());
    view->append_column(column);
    column = Gtk::ColumnViewColumn::create(_("Category"), PluginCategoryFactory::create());
    view->append_column(column);
  }


  void AddinsModel::append(const gnote::AddinInfo & module_info, const sharp::DynamicModule *module)
  {
    Gio::ListStore<Plugin>::append(Plugin::create(module_info, module));
  }

  Glib::ustring AddinsModel::get_addin_category_name(gnote::AddinCategory category)
  {
    switch(category) {
      case gnote::ADDIN_CATEGORY_FORMATTING:
        /* TRANSLATORS: Addin category. */
        return _("Formatting");
      case gnote::ADDIN_CATEGORY_DESKTOP_INTEGRATION:
        /* TRANSLATORS: Addin category. */
        return _("Desktop integration");
      case gnote::ADDIN_CATEGORY_TOOLS:
        /* TRANSLATORS: Addin category. */
        return _("Tools");
      case gnote::ADDIN_CATEGORY_SYNCHRONIZATION:
        /* TRANSLATORS: Addin category. */
        return _("Synchronization");
      case gnote::ADDIN_CATEGORY_UNKNOWN:
      default:
        /* TRANSLATORS: Addin category is unknown. */
        return _("Unknown");
    }
  }

}
