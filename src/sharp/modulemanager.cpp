/*
 * gnote
 *
 * Copyright (C) 2013,2017,2019,2022 Aurimas Cernius
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



#include <dlfcn.h>

#include <gmodule.h>
#include <glibmm/i18n.h>
#include <glibmm/module.h>

#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"
#include "sharp/files.hpp"
#include "sharp/map.hpp"
#include "sharp/modulemanager.hpp"
#include "debug.hpp"

namespace sharp {


  ModuleManager::~ModuleManager()
  {
    for(ModuleMap::const_iterator mod_iter = m_modules.begin();
        mod_iter != m_modules.end(); ++mod_iter) {
      delete mod_iter->second;
    }
  }

  DynamicModule *ModuleManager::load_module(const Glib::ustring & mod)
  {
    DynamicModule *dmod = get_module(mod);
    if(dmod) {
      return dmod;
    }

    Glib::Module module(mod, Glib::Module::Flags::LOCAL);
    DBG_OUT("load module %s", file_basename(mod).c_str());
    if(!module) {
      ERR_OUT(_("Error loading %s"), Glib::Module::get_last_error().c_str());
      return dmod;
    }

    void *func = NULL;
    bool found = module.get_symbol("dynamic_module_instanciate", func);
    if(!found) {
      DBG_OUT(_("Error getting symbol dynamic_module_instanciate: %s"),
              Glib::Module::get_last_error().c_str());
      return dmod;
    }

    instanciate_func_t real_func = (instanciate_func_t)func;
    dmod = (*real_func)();
    if(dmod) {
      m_modules[mod] = dmod;
      module.make_resident();
    }

    return dmod;
  }

  void ModuleManager::load_modules(const std::vector<Glib::ustring> & modules)
  {
    for(auto module : modules) {
      load_module(module);
    }
  }

  DynamicModule * ModuleManager::get_module(const Glib::ustring & module) const
  {
    ModuleMap::const_iterator iter = m_modules.find(module);
    if(iter != m_modules.end()) {
      return iter->second;
    }
    return 0;
  }

}
