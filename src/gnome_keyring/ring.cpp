/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017 Aurimas Cernius
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



#include <cstring>

#include "keyringexception.hpp"
#include "ring.hpp"


namespace gnome {
namespace keyring {

// disable compilerdiagnostic to avoid warnings
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"


SecretSchema Ring::s_schema = {
  "org.gnome.Gnote.Password", SECRET_SCHEMA_NONE,
  {
    {"name", SECRET_SCHEMA_ATTRIBUTE_STRING},
  }
};


// restore original diagnostic options
#pragma GCC diagnostic pop


Glib::ustring Ring::find_password(const std::map<Glib::ustring, Glib::ustring> & atts)
{
  GHashTable *attributes = keyring_attributes(atts);
  GError *error = NULL;
  gchar *result = secret_password_lookupv_sync(&s_schema, attributes, NULL, &error);
  g_hash_table_unref(attributes);
  if(error) {
    KeyringException e(error->message);
    g_error_free(error);
    throw e;
  }

  Glib::ustring res;
  if(result) {
    res = result;
    secret_password_free(result);
  }
  return res;
}

Glib::ustring Ring::default_keyring()
{
  return SECRET_COLLECTION_DEFAULT;
}

void Ring::create_password(const Glib::ustring & keyring, const Glib::ustring & displayName,
                          const std::map<Glib::ustring, Glib::ustring> & attributes,
                          const Glib::ustring & secret)
{
  GHashTable *atts = keyring_attributes(attributes);
  GError *error = NULL;
  secret_password_storev_sync(&s_schema, atts, keyring.c_str(), displayName.c_str(),
                              secret.c_str(), NULL, &error);
  g_hash_table_unref(atts);
  if(error) {
    KeyringException e(error->message);
    g_error_free(error);
    throw e;
  }
}

void Ring::clear_password(const std::map<Glib::ustring, Glib::ustring> & attributes)
{
  GHashTable *atts = keyring_attributes(attributes);
  GError *error = NULL;
  secret_password_clearv_sync(&s_schema, atts, NULL, &error);
  g_hash_table_unref(atts);
  if(error) {
    KeyringException e(error->message);
    g_error_free(error);
    throw e;
  }
}

GHashTable *Ring::keyring_attributes(const std::map<Glib::ustring, Glib::ustring> & atts)
{
  GHashTable *result = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
  for(auto attribute : atts) {
    g_hash_table_insert(result, strdup(attribute.first.c_str()), strdup(attribute.second.c_str()));
  }
  return result;
}

}
}
