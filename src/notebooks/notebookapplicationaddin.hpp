/*
 * gnote
 *
 * Copyright (C) 2012-2015,2017,2019,2023-2024 Aurimas Cernius
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



#ifndef __NOTEBOOK_APPLICATION_ADDIN_HPP__
#define __NOTEBOOK_APPLICATION_ADDIN_HPP__

#include "applicationaddin.hpp"
#include "note.hpp"

namespace gnote {
  namespace notebooks {


    class NotebookApplicationAddin
      : public ApplicationAddin
    {
    public:
      static ApplicationAddin * create();
      virtual void initialize() override;
      virtual void shutdown() override;
      virtual bool initialized() override;

    protected:
      NotebookApplicationAddin();
    private:
      void on_tag_added(const NoteBase&, const Tag&);
      void on_tag_removed(const NoteBase&, const Glib::ustring&);
      void on_note_added(NoteBase &);
      void on_new_notebook_action(const Glib::VariantBase&);

      bool m_initialized;
    };


  }
}


#endif

