/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
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


#ifndef _NOTEBOOKS_NOTEBOOK_SERIALIZER_HPP_
#define _NOTEBOOKS_NOTEBOOK_SERIALIZER_HPP_

#include <functional>
#include <vector>

#include <glibmm/datetime.h>
#include <glibmm/ustring.h>


namespace gnote {
namespace notebooks {

class INotebook
{
public:
  typedef std::reference_wrapper<const INotebook> Ref;

  [[nodiscard]]
  virtual Glib::ustring get_name() const = 0;
  virtual void set_name(const Glib::ustring &name) = 0;
  [[nodiscard]]
  virtual Glib::ustring get_normalized_name() const = 0;
  [[nodiscard]]
  virtual Glib::DateTime created() const = 0;
};


class NotebookData
  : public INotebook
{
public:
  NotebookData(Glib::ustring &&id, const Glib::DateTime &created)
    : m_id(std::move(id))
    , m_created(created)
  {}

  Glib::ustring get_name() const override
    {
      return m_name;
    }
  void set_name(const Glib::ustring &name) override
    {
      m_name = name;
    }
  Glib::ustring get_normalized_name() const override
    {
      return m_id;
    }
  Glib::DateTime created() const override
    {
      return m_created;
    }
private:
  Glib::ustring m_id;
  Glib::ustring m_name;
  Glib::DateTime m_created;
};

class NotebookSerializer
{
public:
  [[nodiscard]]
  static Glib::ustring serialize(const std::vector<INotebook::Ref> &notebooks,
    const Glib::DateTime &timestamp = Glib::DateTime::create_now_utc());

  [[nodiscard]]
  static Glib::ustring serialize(const std::vector<NotebookData> &notebooks,
    const Glib::DateTime &timestamp = Glib::DateTime::create_now_utc());

  struct Notebooks
  {
    Notebooks()
      : valid(false)
    {}

    bool valid;
    Glib::DateTime timestamp;
    std::vector<NotebookData> notebooks;
  };

  [[nodiscard]]
  static Notebooks deserialize(const Glib::ustring &xml);

  struct MergedNotebooks
  {
    MergedNotebooks()
      : all_has_changes(false)
    {}

    std::vector<NotebookData> all;
    std::vector<NotebookData> updates;
    bool all_has_changes;
  };

  [[nodiscard]]
  static MergedNotebooks merge(const Notebooks &loaded, const std::vector<INotebook::Ref> &current);

private:
  template <typename NotebookT>
  [[nodiscard]]
  static Glib::ustring serialize(const std::vector<NotebookT> &notebooks, const Glib::DateTime &timestamp);
};

}
}

#endif

