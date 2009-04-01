/*
 * gnote
 *
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



#ifndef __CONTRAST_HPP_
#define __CONTRAST_HPP_

namespace gnote {

	enum ContrastPaletteColor
	{
		// TODO
		CONTRAST_PALETTE_COLOR_BLUE,
		CONTRAST_PALETTE_COLOR_GREY
	};

	class Contrast 
	{
	public:
		static Gdk::Color render_foreground_color(const Gdk::Color & /*color*/, 
																							ContrastPaletteColor /*symbol*/)
			{
				return Gdk::Color();
			}
	};

}

#endif
