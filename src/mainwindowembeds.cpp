/*
 * gnote
 *
 * Copyright (C) 2013,2021 Aurimas Cernius
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


#include "mainwindowembeds.hpp"


namespace gnote {

void EmbeddableWidget::embed(EmbeddableWidgetHost *h)
{
  //remove from previous host, if any
  if(m_host) {
    m_host->unembed_widget(*this);
  }
  m_host = h;
  signal_embedded();
}

void EmbeddableWidget::unembed()
{
  m_host = NULL;
  signal_unembedded();
}

void EmbeddableWidget::foreground()
{
  signal_foregrounded();
}

void EmbeddableWidget::background()
{
  signal_backgrounded();
}

void EmbeddableWidget::size_internals()
{
}


bool SearchableItem::supports_goto_result()
{
  return false;
}

bool SearchableItem::goto_next_result()
{
  return false;
}

bool SearchableItem::goto_previous_result()
{
  return false;
}
 
}

