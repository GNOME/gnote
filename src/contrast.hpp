

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
