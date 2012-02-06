/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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


#include "debug.hpp"
#include "syncui.hpp"


namespace {

  typedef GObject GnoteSyncUI;
  typedef GObjectClass GnoteSyncUIClass;

  G_DEFINE_TYPE(GnoteSyncUI, gnote_sync_ui, G_TYPE_OBJECT)

  void gnote_sync_ui_init(GnoteSyncUI*)
  {
  }

  void gnote_sync_ui_class_init(GnoteSyncUIClass * klass)
  {
    g_signal_new("connecting", G_TYPE_FROM_CLASS(klass),
                 GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0, NULL);
    g_signal_new("idle", G_TYPE_FROM_CLASS(klass),
                 GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0, NULL);
    g_signal_new("note-synchronized", G_TYPE_FROM_CLASS(klass),
                 GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                 0, NULL, NULL, g_cclosure_marshal_generic,
                 G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT, NULL);
  }

  GnoteSyncUI * gnote_sync_ui_new()
  {
    g_type_init();
    return static_cast<GnoteSyncUI*>(g_object_new(gnote_sync_ui_get_type(), NULL));
  }

}


namespace gnote {
namespace sync {

  SyncUI::SyncUI()
    : m_obj(gnote_sync_ui_new())
  {
    g_signal_connect(m_obj, "connecting", G_CALLBACK(SyncUI::on_signal_connecting), this);
    g_signal_connect(m_obj, "idle", G_CALLBACK(SyncUI::on_signal_idle), this);
    g_signal_connect(m_obj, "note-synchronized", G_CALLBACK(SyncUI::on_signal_note_synchronized), this);
  }


  void SyncUI::on_signal_connecting(GObject*, gpointer data)
  {
    static_cast<SyncUI*>(data)->m_signal_connecting.emit();
  }


  void SyncUI::on_signal_idle(GObject*, gpointer data)
  {
    static_cast<SyncUI*>(data)->m_signal_idle.emit();
  }


  void SyncUI::on_signal_note_synchronized(GObject*, const char *noteTitle, int type, gpointer data)
  {
    try {
      static_cast<SyncUI*>(data)->note_synchronized(noteTitle, static_cast<NoteSyncType>(type));
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncUI::note_synchronized_th(const std::string & noteTitle, NoteSyncType type)
  {
    gdk_threads_enter();
    g_signal_emit_by_name(m_obj, "note-synchronized", noteTitle.c_str(), static_cast<int>(type));
    gdk_threads_leave();
  }


  sigc::connection SyncUI::signal_connecting_connect(const SlotConnecting & slot)
  {
    return m_signal_connecting.connect(slot);
  }


  void SyncUI::signal_connecting_emit()
  {
    gdk_threads_enter();
    g_signal_emit_by_name(m_obj, "connecting");
    gdk_threads_leave();
  }


  sigc::connection SyncUI::signal_idle_connect(const SlotIdle & slot)
  {
    return m_signal_idle.connect(slot);
  }


  void SyncUI::signal_idle_emit()
  {
    gdk_threads_enter();
    g_signal_emit_by_name(m_obj, "idle");
    gdk_threads_leave();
  }

}
}
