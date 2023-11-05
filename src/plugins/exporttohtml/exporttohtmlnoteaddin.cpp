/*
 * gnote
 *
 * Copyright (C) 2010-2013,2016-2017,2019-2023 Aurimas Cernius
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


#include <libxml/xmlmemory.h>
#include <libxml/xpathInternals.h>
#include <libxslt/extensions.h>

#include <glibmm/i18n.h>

#include "config.h"
#include "sharp/exception.hpp"
#include "sharp/files.hpp"
#include "sharp/streamwriter.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "sharp/xsltargumentlist.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "preferences.hpp"
#include "notewindow.hpp"
#include "utils.hpp"

#include "exporttohtmlnoteaddin.hpp"
#include "notenameresolver.hpp"

#define STYLESHEET_NAME "exporttohtml.xsl"


using gnote::Preferences;

namespace exporttohtml {


ExportToHtmlModule::ExportToHtmlModule()
{
  ADD_INTERFACE_IMPL(ExportToHtmlNoteAddin);
}

sharp::XslTransform *ExportToHtmlNoteAddin::s_xsl = NULL;


void ExportToHtmlNoteAddin::initialize()
{
  
}


void ExportToHtmlNoteAddin::shutdown()
{
}


void ExportToHtmlNoteAddin::on_note_opened()
{
  register_main_window_action_callback("exporttohtml-export",
    sigc::mem_fun(*this, &ExportToHtmlNoteAddin::export_button_clicked));
}


std::vector<gnote::PopoverWidget> ExportToHtmlNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto item = Gio::MenuItem::create(_("Export to HTML…"), "win.exporttohtml-export");
  widgets.push_back(gnote::PopoverWidget::create_for_note(gnote::EXPORT_TO_HTML_ORDER, item));
  return widgets;
}


void ExportToHtmlNoteAddin::export_button_clicked(const Glib::VariantBase&)
{
  auto dialog = Gtk::make_managed<ExportToHtmlDialog>(ignote(), get_note().get_title() + ".html");
  dialog->show();
  dialog->signal_response().connect([this, dialog](int response) {
    dialog->hide();
    if(response != Gtk::ResponseType::OK) {
      return;
    }

    export_dialog_response(*dialog);
  });
}


void ExportToHtmlNoteAddin::export_dialog_response(ExportToHtmlDialog & dialog)
{
  Glib::ustring output_path = dialog.get_file()->get_path();
  DBG_OUT("Exporting Note '%s' to '%s'...", get_note().get_title().c_str(), output_path.c_str());

  sharp::StreamWriter writer;
  Glib::ustring error_message;

  try {
    // FIXME: Warn about file existing.  Allow overwrite.
    sharp::file_delete(output_path);

    writer.init(output_path);
    write_html_for_note(writer, get_note(), dialog.get_export_linked(), dialog.get_export_linked_all());

    // Save the dialog preferences now that the note has
    // successfully been exported
    dialog.save_preferences ();

    try {
      sharp::Uri output_uri{Glib::ustring(output_path)};
      gnote::utils::open_url(*get_host_window(), "file://" + output_uri.get_absolute_uri());
    } 
    catch (const std::exception & ex) {
      ERR_OUT(_("Could not open exported note in a web browser: %s"), ex.what());

      Glib::ustring detail = Glib::ustring::compose(
                                 // TRANSLATORS: %1%: boost format placeholder for the path
                                 _("Your note was exported to \"%1\"."),
                                 output_path);

      // Let the user know the note was saved successfully
      // even though showing the note in a web browser failed.
      auto msg_dialog = Gtk::make_managed<gnote::utils::HIGMessageDialog>(
        get_host_window(),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        Gtk::MessageType::INFO, Gtk::ButtonsType::OK,
        _("Note exported successfully"),
        detail);
      msg_dialog->show();
      msg_dialog->signal_response().connect([msg_dialog](int) { msg_dialog->hide(); });
    }
  } 
#if 0
  catch (UnauthorizedAccessException) {
    error_message = Catalog.GetString ("Access denied.");
  } 
  catch (DirectoryNotFoundException) {
    error_message = Catalog.GetString ("Folder does not exist.");
  } 
#endif
  catch (const sharp::Exception & e) {
    ERR_OUT(_("Could not export: %s"), e.what());

    error_message = e.what();
  } 
  writer.close ();

  if (!error_message.empty())
  {
    ERR_OUT(_("Could not export: %s"), error_message.c_str());

    Glib::ustring msg = Glib::ustring::compose(
                            _("Could not save the file \"%1\""),
                            output_path.c_str());

    auto msg_dialog = Gtk::make_managed<gnote::utils::HIGMessageDialog>(get_host_window(),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              Gtk::MessageType::ERROR,
                                              Gtk::ButtonsType::OK,
                                              msg, error_message);
    msg_dialog->show();
    msg_dialog->signal_response().connect([msg_dialog](int) { msg_dialog->hide(); });
  }
}



static void to_lower(xmlXPathParserContextPtr ctxt,
                     int)
{
  const xmlChar *input = xmlXPathPopString(ctxt);
  gchar * lower = g_utf8_strdown((const gchar*)input, -1);
  xmlXPathReturnString(ctxt, xmlStrdup((const xmlChar*)lower));
  g_free(lower);
}


sharp::XslTransform & ExportToHtmlNoteAddin::get_note_xsl()
{
  if(s_xsl == NULL) {
    int result = xsltRegisterExtModuleFunction((const xmlChar *)"ToLower",
                                               (const xmlChar *)"http://beatniksoftware.com/tomboy", 
                                               &to_lower);
    DBG_OUT("xsltRegisterExtModule %d", result);
    if(result == -1) {
      DBG_OUT("xsltRegisterExtModule failed");
    }

    s_xsl = new sharp::XslTransform;
    Glib::ustring stylesheet_file = DATADIR "/gnote/" STYLESHEET_NAME;

    if (sharp::file_exists (stylesheet_file)) {
      DBG_OUT("ExportToHTML: Using user-custom %s file.", STYLESHEET_NAME);
      s_xsl->load(stylesheet_file);
    } 
#if 0
    else {
      Stream resource = asm.GetManifestResourceStream (stylesheet_name);
      if (resource != null) {
        XmlTextReader reader = new XmlTextReader (resource);
        s_xsl->load (reader, null, null);
        resource.Close ();
      } 
      else {
        DBG_OUT("Unable to find HTML export template '%s'.", STYLESHEET_NAME);
      }
    }
#endif

  }
  return *s_xsl;
}


void ExportToHtmlNoteAddin::write_html_for_note(sharp::StreamWriter & writer,
  gnote::Note & note, bool export_linked, bool export_linked_all)
{
  Glib::ustring s_writer;
  s_writer = note.manager().note_archiver().write_string(note.data());
  xmlDocPtr doc = xmlParseMemory(s_writer.c_str(), s_writer.bytes());

  sharp::XsltArgumentList args;
  args.add_param("export-linked", "", export_linked);
  args.add_param("export-linked-all", "", export_linked_all);
  args.add_param("root-note", "", gnote::utils::XmlEncoder::encode(note.get_title()));

  if(ignote().preferences().enable_custom_font()) {
    Glib::ustring font_face = ignote().preferences().custom_font_face();
    Pango::FontDescription font_desc (font_face);
    Glib::ustring font = Glib::ustring::compose("font-family:'%1';", font_desc.get_family());

    args.add_param ("font", "", font);
  }

  NoteNameResolver resolver(note.manager(), note);
  get_note_xsl().transform(doc, args, writer, resolver);

  xmlFreeDoc(doc);
}


}

