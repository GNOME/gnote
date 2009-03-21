

#include <algorithm>
#include <tr1/array>

#include <pango/pango-bidi-type.h>

#include "debug.hpp"
#include "notebuffer.hpp"
#include "notetag.hpp"
#include "note.hpp"
#include "preferences.hpp"

#include "sharp/foreach.hpp"

namespace gnote {


#define NUM_INDENT_BULLETS 3
	const gunichar NoteBuffer::s_indent_bullets[NUM_INDENT_BULLETS] = { 2022, 2218, 2023 };

	bool NoteBuffer::get_enable_auto_bulleted_lists() const
	{
		return Preferences::get_preferences()->get<bool>(Preferences::ENABLE_AUTO_BULLETED_LISTS);
	}
	

	NoteBuffer::NoteBuffer(const NoteTagTable::Ptr & tags, Note & note)
		: m_note(note)
	{
		
		signal_insert().connect(sigc::mem_fun(*this, &NoteBuffer::text_insert_event));
		signal_erase().connect(sigc::mem_fun(*this, &NoteBuffer::range_deleted_event));
		signal_mark_set().connect(sigc::mem_fun(*this, &NoteBuffer::mark_set_event));

		signal_apply_tag().connect(sigc::mem_fun(*this, &NoteBuffer::on_tag_applied));
		

		tags->signal_tag_changed().connect(sigc::mem_fun(*this, &NoteBuffer::on_tag_changed));
	}


	void NoteBuffer::toggle_active_tag(const std::string & tag_name)
	{
		DBG_OUT("ToggleTag called for '%s'", tag_name.c_str());
		
		Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
		Gtk::TextIter select_start, select_end;

		if (get_selection_bounds(select_start, select_end)) {
			// Ignore the bullet character
			if (find_depth_tag(select_start))
				select_start.set_line_offset(2);

			if (select_start.begins_tag(tag) || select_start.has_tag(tag)) {
				remove_tag(tag, select_start, select_end);
			}
			else {
				apply_tag(tag, select_start, select_end);
			}
		} else {
			std::list<Glib::RefPtr<Gtk::TextTag> >::iterator iter = std::find(m_active_tags.begin(), 
																																	 m_active_tags.end(), tag);
			if (iter != m_active_tags.end()) {
				m_active_tags.erase(iter);
			}
			else {
				m_active_tags.push_back(tag);
			}
		}
	}

	void NoteBuffer::set_active_tag (const std::string & tag_name)
	{
		DBG_OUT("SetTag called for '%s'", tag_name.c_str());

		Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
		Gtk::TextIter select_start, select_end;

		if (get_selection_bounds(select_start, select_end)) {
			apply_tag(tag, select_start, select_end);
		} else {
			m_active_tags.push_back(tag);
		}
	}

	void NoteBuffer::remove_active_tag (const std::string & tag_name)
	{
		DBG_OUT("remove_tagcalled for '%s'", tag_name.c_str());

		Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
		Gtk::TextIter select_start, select_end;

		if (get_selection_bounds(select_start, select_end)) {
			remove_tag(tag, select_start, select_end);
		} else {
			std::list<Glib::RefPtr<Gtk::TextTag> >::iterator iter = std::find(m_active_tags.begin(), 
																																	 m_active_tags.end(), tag);
			if (iter != m_active_tags.end()) {
				m_active_tags.erase(iter);
			}
		}
	}


  /// <summary>
	/// Returns the specified DynamicNoteTag if one exists on the TextIter
	/// or null if none was found.
	/// </summary>
	DynamicNoteTag::ConstPtr NoteBuffer::get_dynamic_tag (const std::string  & tag_name, 
																												const Gtk::TextIter & iter)
	{
		// TODO: Is this variables used, or do we just need to
		// access iter.Tags to work around a bug?
		foreach (Glib::RefPtr<const Gtk::TextTag> tag, iter.get_tags()) {
			DynamicNoteTag::ConstPtr dynamic_tag =  DynamicNoteTag::ConstPtr::cast_dynamic(tag);
			if (dynamic_tag &&
					(dynamic_tag->get_element_name() == tag_name)) {
				return dynamic_tag;
			}
		}

		return DynamicNoteTag::ConstPtr();
	}

	
	void NoteBuffer::on_tag_applied(const Glib::RefPtr<Gtk::TextTag> & tag1,
																	const Gtk::TextIter & start_char, const Gtk::TextIter &end_char)
	{
		DepthNoteTag::Ptr dn_tag = DepthNoteTag::Ptr::cast_dynamic(tag1);
		if (!dn_tag) {
			// Remove the tag from any bullets in the selection
			m_undomanager.freeze_undo();
			Gtk::TextIter iter;
			for (int i = start_char.get_line(); i <= end_char.get_line(); i++) {
				iter = get_iter_at_line(i);

				if (find_depth_tag(iter)) {
					Gtk::TextIter next = iter;
					next.forward_chars(2);
					remove_tag(tag1, iter, next);
				}
			}
			m_undomanager.thaw_undo();
		} 
		else {
			// Remove any existing tags when a depth tag is applied
			m_undomanager.freeze_undo();
			foreach (const Glib::RefPtr<const Gtk::TextTag> & tag, start_char.get_tags()) {
				DepthNoteTag::ConstPtr dn_tag2 = DepthNoteTag::ConstPtr::cast_dynamic(tag);
				if (!dn_tag2) {
					// here it gets hairy. Gtkmm does not implement remove_tag() on a const.
					// given that Gtk does not have const, I assume I can work that out.
					remove_tag(Glib::RefPtr<Gtk::TextTag>::cast_const(tag), start_char, end_char);
				}
			}
			m_undomanager.thaw_undo();
		}
	}


	bool NoteBuffer::is_active_tag(const std::string & tag_name)
	{
		Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
		Gtk::TextIter iter, select_end;

		if (get_selection_bounds (iter, select_end)) {
			// Ignore the bullet character and look at the
			// first character of the list item
			if (find_depth_tag(iter)) {
				iter.forward_chars(2);
			}
			return iter.begins_tag(tag) || iter.has_tag(tag);
		} 
		else {
			return (find(m_active_tags.begin(), m_active_tags.end(), tag) != m_active_tags.end());
		}
	}

		// Returns true if the cursor is inside of a bulleted list
	bool NoteBuffer::is_bulleted_list_active()
	{
		Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
		Gtk::TextIter iter = get_iter_at_mark(insert_mark);
		iter.set_line_offset(0);

		Glib::RefPtr<Gtk::TextTag> depth = find_depth_tag(iter);

		return depth;
	}


	// Returns true if the cursor is at a position that can
		// be made into a bulleted list
	bool NoteBuffer::can_make_bulleted_list()
	{
		Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
		Gtk::TextIter iter = get_iter_at_mark(insert_mark);

		return iter.get_line();
	}

	// Apply active_tags to inserted text
	void NoteBuffer::text_insert_event(const Gtk::TextIter & pos, const Glib::ustring & text, int bytes)
	{
		// Only apply active tags when typing, not on paste.
		if (text.size() == 1) {
			Gtk::TextIter insert_start(pos);
			insert_start.backward_chars (text.size());

			m_undomanager.freeze_undo();
			foreach (const Glib::RefPtr<Gtk::TextTag> & tag, insert_start.get_tags()) {
				remove_tag(tag, insert_start, pos);
			}

			foreach (const Glib::RefPtr<Gtk::TextTag> & tag, m_active_tags) {
				apply_tag(tag, insert_start, pos);
			}
			m_undomanager.thaw_undo();
		}

		// See if we want to change the direction of the bullet
		Gtk::TextIter line_start(pos);
		line_start.set_line_offset(0);

		if (((pos.get_line_offset() - text.size()) == 2) &&
				find_depth_tag(line_start)) {
			PangoDirection direction = PANGO_DIRECTION_LTR;

			if (text.size() > 0) {
				direction = pango_unichar_direction(text[0]);
			}
			change_bullet_direction(pos, direction);
		}

		// TODO make sure these are the right params
		signal_insert_text_with_tags(text, &bytes);
	}


	// Change the direction of a bulleted line to match the new
	// first character after the previous character is deleted.
	void NoteBuffer::range_deleted_event(const Gtk::TextIter & start,const Gtk::TextIter & end_iter)
	{
		//
		std::tr1::array<Gtk::TextIter, 2> iters;
		iters[0] = start;
		iters[1] = end_iter;

		foreach (const Gtk::TextIter & iter, iters) {
			Gtk::TextIter line_start = iter;
			line_start.set_line_offset(0);

			if (((iter.get_line_offset() == 3) || (iter.get_line_offset() == 2)) &&
					find_depth_tag(line_start)) {

				Gtk::TextIter first_char = iter;
				first_char.set_line_offset(2);

				PangoDirection direction = PANGO_DIRECTION_LTR;

				if (first_char.get_char() > 0)
					direction = pango_unichar_direction(first_char.get_char());

				change_bullet_direction(first_char, direction);
			}
		}
	}


	bool NoteBuffer::add_new_line(bool soft_break)
	{
		if (!can_make_bulleted_list() || !get_enable_auto_bulleted_lists())
			return false;

		Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
		Gtk::TextIter iter = get_iter_at_mark(insert_mark);
		iter.set_line_offset(0);

		DepthNoteTag::Ptr prev_depth = find_depth_tag(iter);
			
		Gtk::TextIter insert_iter = get_iter_at_mark(insert_mark);
 
		// Insert a LINE SEPARATOR character which allows us
		// to have multiple lines in a single bullet point
		if (prev_depth && soft_break) {
			bool at_end_of_line = insert_iter.ends_line();
			insert(insert_iter, Glib::ustring(4, (gunichar)2028));
				
			// Hack so that the user sees that what they type
			// next will appear on a new line, otherwise the
			// cursor stays at the end of the previous line.
			if (at_end_of_line) {
				insert(insert_iter, " ");
				Gtk::TextIter bound = insert_iter;
				bound.backward_char();
				move_mark(get_selection_bound(), bound);
			}
				
			return true;			

			// If the previous line has a bullet point on it we add a bullet
			// to the new line, unless the previous line was blank (apart from
			// the bullet), in which case we clear the bullet/indent from the
			// previous line.
		} 
		else if (prev_depth) {
			iter.forward_char();

			// See if the line was left contentless and remove the bullet
			// if so.
			if (iter.ends_line() || insert_iter.get_line_offset() < 3 ) {
				Gtk::TextIter start = get_iter_at_line(iter.get_line());
				Gtk::TextIter end_iter = start;
				end_iter.forward_to_line_end();

				if (end_iter.get_line_offset() < 2) {
					end_iter = start;
				} 
				else {
					end_iter = get_iter_at_line_offset(iter.get_line(), 2);
				}

				erase(start, end_iter);

				iter = get_iter_at_mark(insert_mark);
				insert(iter, "\n");
			} 
			else {
				iter = get_iter_at_mark(insert_mark);
				Gtk::TextIter prev = iter;
				prev.backward_char();
					
				// Remove soft breaks
				if (prev.get_char() == 2028) {
					erase(prev, iter);
				}
					
				m_undomanager.freeze_undo();
				int offset = iter.get_offset();
				insert(iter, "\n");

				iter = get_iter_at_mark(insert_mark);
				Gtk::TextIter start = get_iter_at_line(iter.get_line());

				// Set the direction of the bullet to be the same
				// as the first character on the new line
				PangoDirection direction = prev_depth->get_direction();
				if ((iter.get_char() != '\n') && (iter.get_char() > 0)) {
					direction = pango_unichar_direction(iter.get_char());
				}

				insert_bullet(start, prev_depth->get_depth(), direction);
				m_undomanager.thaw_undo();

				signal_new_bullet_inserted(offset, prev_depth->get_depth(), direction);
			}

			return true;
		}
		// Replace lines starting with any numbers of leading spaces 
		// followed by '*' or '-' and then by a space with bullets
		else if (line_needs_bullet(iter)) {
			Gtk::TextIter start = get_iter_at_line_offset (iter.get_line(), 0);
			Gtk::TextIter end_iter = get_iter_at_line_offset (iter.get_line(), 0);

			// Remove any leading white space
			while (end_iter.get_char() == ' ')
				end_iter.forward_char();
			// Remove the '*' or '-' character and the space after
			end_iter.forward_chars(2);
				
			// Set the direction of the bullet to be the same as
			// the first character after the '*' or '-'
			PangoDirection direction = PANGO_DIRECTION_LTR;
			if (end_iter.get_char() > 0)
				direction = pango_unichar_direction(end_iter.get_char());

			erase(start, end_iter);

			if (end_iter.ends_line()) {
				increase_depth(start);
			}
			else {
				increase_depth(start);

				iter = get_iter_at_mark(insert_mark);
				int offset = iter.get_offset();
				insert(iter, "\n");

				iter = get_iter_at_mark(insert_mark);
				iter.set_line_offset(0);

				m_undomanager.freeze_undo();
				insert_bullet (iter, 0, direction);
				m_undomanager.thaw_undo();

				signal_new_bullet_inserted(offset, 0, direction);
			}

			return true;
		}

		return false;
	}


  // Returns true if line starts with any numbers of leading spaces
	// followed by '*' or '-' and then by a space
	bool NoteBuffer::line_needs_bullet(Gtk::TextIter & iter)
	{
		while (!iter.ends_line()) {
			switch (iter.get_char()) {
			case ' ':
				iter.forward_char();
				break;
			case '*':
			case '-':
				return (get_iter_at_line_offset(iter.get_line(), iter.get_line_offset() + 1).get_char() == ' ');
			default:
				return false;
			}
		}
		return false;
	}

	// Returns true if the depth of the line was increased 
	bool NoteBuffer::add_tab()
	{
		Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
		Gtk::TextIter iter = get_iter_at_mark(insert_mark);
		iter.set_line_offset(0);

		DepthNoteTag::Ptr depth = find_depth_tag(iter);

		// If the cursor is at a line with a depth and a tab has been
		// inserted then we increase the indent depth of that line.
		if (depth) {
			increase_depth(iter);
			return true;
		}
		return false;
	}


  // Returns true if the depth of the line was decreased
	bool NoteBuffer::remove_tab()
	{
		Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
		Gtk::TextIter iter = get_iter_at_mark(insert_mark);
		iter.set_line_offset(0);

		DepthNoteTag::Ptr depth = find_depth_tag(iter);

		// If the cursor is at a line with depth and a tab has been
		// inserted, then we decrease the depth of that line.
		if (depth) {
			decrease_depth(iter);
			return true;
		}

		return false;
	}


	// Returns true if a bullet had to be removed
		// This is for the Delete key not Backspace
	bool NoteBuffer::delete_key_handler()
	{
		// See if there is a selection
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		bool selection = get_selection_bounds(start, end_iter);

		if (selection) {
			augment_selection(start, end_iter);
			erase(start, end_iter);
			return true;
		} 
		else if (start.ends_line() && start.get_line() < get_line_count()) {
			Gtk::TextIter next = get_iter_at_line (start.get_line() + 1);
			end_iter = start;
			end_iter.forward_chars(3);

			DepthNoteTag::Ptr depth = find_depth_tag(next);

			if (depth) {
				erase(start, end_iter);
				return true;
			}
		} 
		else {
			Gtk::TextIter next = start;

			if (next.get_line_offset() != 0)
				next.forward_char();

			DepthNoteTag::Ptr depth = find_depth_tag(start);
			DepthNoteTag::Ptr nextDepth = find_depth_tag(next);
			if (depth || nextDepth) {
				decrease_depth (start);
				return true;
			}
		}

		return false;
	}


	bool NoteBuffer::backspace_key_handler()
	{
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		bool selection = get_selection_bounds(start, end_iter);

		DepthNoteTag::Ptr depth = find_depth_tag(start);

		if (selection) {
			augment_selection(start, end_iter);
			erase(start, end_iter);
			return true;
		} 
		else {
			// See if the cursor is inside or just after a bullet region
			// ie.
			// |* lorum ipsum
			//  ^^^
			// and decrease the depth if it is.

			Gtk::TextIter prev = start;

			if (prev.get_line_offset())
				prev.backward_chars (1);

			DepthNoteTag::Ptr prev_depth = find_depth_tag(prev);
			if (depth || prev_depth) {
				decrease_depth(start);
				return true;
			} 
			else {
				// See if the cursor is before a soft line break
				// and remove it if it is. Otherwise you have to
				// press backspace twice before  it will delete
				// the previous visible character.
				prev = start;
				prev.backward_chars (2);
				if (prev.get_char() == 2028) {
					Gtk::TextIter end_break = prev;
					end_break.forward_char();
					erase(prev, end_break);
				}
			}
		}

		return false;
	}


	// On an InsertEvent we change the selection (if there is one)
		// so that it doesn't slice through bullets.
	void NoteBuffer::check_selection()
	{
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		bool selection = get_selection_bounds(start, end_iter);

		if (selection) {
			augment_selection(start, end_iter);
		} 
		else {
			// If the cursor is at the start of a bulleted line
			// move it so it is after the bullet.
			if ((start.get_line_offset() == 0 || start.get_line_offset() == 1) &&
					find_depth_tag(start))
			{
				start.set_line_offset(2);
				select_range (start, start);
			}
		}
	}


	// Change the selection on the buffer taking into account any
	// bullets that are in or near the seletion
	void NoteBuffer::augment_selection(Gtk::TextIter & start, Gtk::TextIter & end_iter)
	{
		DepthNoteTag::Ptr start_depth = find_depth_tag(start);
		DepthNoteTag::Ptr end_depth = find_depth_tag(end_iter);

		Gtk::TextIter inside_end = end_iter;
		inside_end.backward_char();

		DepthNoteTag::Ptr inside_end_depth = find_depth_tag(inside_end);

		// Start inside bullet region
		if (start_depth) {
			start.set_line_offset(2);
			select_range(start, end_iter);
		}

		// End inside another bullet
		if (inside_end_depth) {
			end_iter.set_line_offset(2);
			select_range (start, end_iter);
		}

		// Check if the End is right before start of bullet
		if (end_depth) {
			end_iter.set_line_offset(2);
			select_range(start, end_iter);
		}
	}

	// Clear active tags, and add any tags which should be applied:
	// - Avoid the having tags grow frontwords by not adding tags
	//   which start on the next character.
	// - Add tags ending on the prior character, to avoid needing to
	//   constantly toggle tags.
	void NoteBuffer::mark_set_event(const Gtk::TextIter &,const Glib::RefPtr<Gtk::TextBuffer::Mark> & mark)
	{
		if (mark != get_insert()) {
			return;
		}

		m_active_tags.clear();

		Gtk::TextIter iter = get_iter_at_mark(mark);

		// Add any growable tags not starting on the next character...
		foreach (const Glib::RefPtr<Gtk::TextTag> & tag, iter.get_tags()) {
			if (!iter.begins_tag(tag) && NoteTagTable::tag_is_growable(tag)) {
				m_active_tags.push_back(tag);
			}
		}

		// Add any growable tags not ending on the prior character...
		foreach (const Glib::RefPtr<Gtk::TextTag> & tag, iter.get_toggled_tags(false)) {
			if (!iter.ends_tag(tag) && NoteTagTable::tag_is_growable(tag)) {
				m_active_tags.push_back(tag);
			}
		}
	}


	void NoteBuffer::widget_swap (const NoteTag::Ptr & tag, const Gtk::TextIter & start,
																const Gtk::TextIter & /*end*/, bool adding)
	{
		if (tag->get_widget() == NULL)
			return;

		Gtk::TextIter prev = start;
		prev.backward_char();

		WidgetInsertData data;
		data.buffer = start.get_buffer();
		data.tag = tag;
		data.widget = tag->get_widget();
		data.adding = adding;

		if (adding) {
			data.position = start.get_buffer()->create_mark (start, true);
		} 
		else {
			data.position = tag->get_widget_location();
		}

		m_widget_queue.push(data);

		if (!m_widget_queue_timeout) {
			m_widget_queue_timeout = Glib::signal_idle()
				.connect(sigc::mem_fun(*this, &NoteBuffer::run_widget_queue));
		}
	}


	bool NoteBuffer::run_widget_queue()
	{
		while(!m_widget_queue.empty()) {
			const WidgetInsertData & data(m_widget_queue.front());
			// HACK: This is a quick fix for bug #486551
			if (data.position) {
				NoteBuffer::Ptr buffer = NoteBuffer::Ptr::cast_static(data.buffer);
				Gtk::TextIter iter = buffer->get_iter_at_mark(data.position);
				Glib::RefPtr<Gtk::TextMark> location = data.position;

				// Prevent the widget from being inserted before a bullet
				if (find_depth_tag(iter)) {
					iter.set_line_offset(2);
					location = create_mark(data.position->get_name(), iter, data.position->get_left_gravity());
				}

				buffer->undoer().freeze_undo();

				if (data.adding && !data.tag->get_widget_location()) {
					Glib::RefPtr<Gtk::TextChildAnchor> childAnchor = buffer->create_child_anchor(iter);
					data.tag->set_widget_location(location);
					m_note.add_child_widget(childAnchor, data.widget);
				}
				else if (!data.adding && data.tag->get_widget_location()) {
					Gtk::TextIter end_iter = iter;
					end_iter.forward_char();
					buffer->erase(iter, end_iter);
					buffer->delete_mark(location);
					data.tag->set_widget_location(Glib::RefPtr<Gtk::TextMark>());
				}
				buffer->undoer().thaw_undo();
			}
			m_widget_queue.pop();
		}

//			m_widget_queue_timeout = 0;
		return false;
	}

	void NoteBuffer::on_tag_changed(const Glib::RefPtr<Gtk::TextTag> & tag, bool)
	{
		NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
		if (note_tag) {
			utils::TextTagEnumerator enumerator(Glib::RefPtr<Gtk::TextBuffer>(this), note_tag);
			while(enumerator.move_next()) {
				const utils::TextRange & range(enumerator.current());
				widget_swap(note_tag, range.start(), range.end(), true);
			}
		}
	}

	void NoteBuffer::on_apply_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
											 const Gtk::TextIter & start,  const Gtk::TextIter &end_iter)
	{
		Gtk::TextBuffer::on_apply_tag(tag, start, end_iter);

		NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
		if (note_tag) {
			widget_swap(note_tag, start, end_iter, true);
		}
	}

	void NoteBuffer::on_remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
																 const Gtk::TextIter & start,  const Gtk::TextIter & end_iter)
	{
		NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
		if (note_tag) {
			widget_swap(note_tag, start, end_iter, false);
		}

		Gtk::TextBuffer::on_remove_tag(tag, start, end_iter);
	}

	std::string NoteBuffer::get_selection() const
	{
		Gtk::TextIter select_start, select_end;
		std::string text;
		
		if (get_selection_bounds(select_start, select_end)) {
			text = get_text(select_start, select_end, false);
		}

		return text;
	}


	void NoteBuffer::get_block_extents(Gtk::TextIter & start, Gtk::TextIter & end_iter,
																		 int threshold, const Glib::RefPtr<Gtk::TextTag> & avoid_tag)
	{
		// Move start and end to the beginning or end of their
		// respective paragraphs, bounded by some threshold.

		start.set_line_offset(std::max(0, start.get_line_offset() - threshold));

		// FIXME: Sometimes I need to access this before it
		// returns real values.
		(void)end_iter.get_chars_in_line();

		if (end_iter.get_chars_in_line() - end_iter.get_line_offset() > (threshold + 1 /* newline */)) {
			end_iter.set_line_offset(end_iter.get_line_offset() + threshold);
		}
		else {
			end_iter.forward_to_line_end();
		}

		if (avoid_tag) {
			if (start.has_tag(avoid_tag)) {
				start.backward_to_tag_toggle(avoid_tag);
			}

			if (end_iter.has_tag(avoid_tag)) {
				end_iter.forward_to_tag_toggle(avoid_tag);
			}
		}
	}

	void NoteBuffer::toggle_selection_bullets()
	{
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		get_selection_bounds (start, end_iter);

		start = get_iter_at_line_offset(start.get_line(), 0);

		bool toggle_on = true;
		if (find_depth_tag(start)) {
			toggle_on = false;
		}

		int start_line = start.get_line();
		int end_line = end_iter.get_line();

		for (int i = start_line; i <= end_line; i++) {
			Gtk::TextIter curr_line = get_iter_at_line(i);
			if (toggle_on && !find_depth_tag(curr_line)) {
				increase_depth(curr_line);
			} 
			else if (!toggle_on && !find_depth_tag(curr_line)) {
				Gtk::TextIter bullet_end = get_iter_at_line_offset(curr_line.get_line(), 2);
				erase(curr_line, bullet_end);
			}
		}
	}


	void NoteBuffer::change_cursor_depth_directional(bool right)
	{
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		get_selection_bounds (start, end_iter);

		// If we are moving right then:
		//   RTL => decrease depth
		//   LTR => increase depth
		// We choose to increase or decrease the depth
		// based on the fist line in the selection.
		bool increase = right;
		start.set_line_offset(0);
		DepthNoteTag::Ptr start_depth = find_depth_tag (start);

		bool rtl_depth = start_depth && (start_depth->get_direction() == PANGO_DIRECTION_RTL);
		bool first_char_rtl = (start.get_char() > 0) &&
			(pango_unichar_direction(start.get_char()) == PANGO_DIRECTION_RTL);
		Gtk::TextIter next = start;

		if (start_depth) {
			next.forward_chars (2);
		} 
		else {
			// Look for the first non-space character on the line
			// and use that to determine what direction we should go
			next.forward_sentence_end ();
			next.backward_sentence_start ();
			first_char_rtl = ((next.get_char() > 0) &&
												(pango_unichar_direction(next.get_char() == PANGO_DIRECTION_RTL)));
		}

		if ((rtl_depth || first_char_rtl) &&
				((next.get_line() == start.get_line()) && !next.ends_line ())) {
			increase = !right;
		}
				
		change_cursor_depth(increase);
	}

	void NoteBuffer::change_cursor_depth(bool increase)
	{
		Gtk::TextIter start;
		Gtk::TextIter end_iter;

		get_selection_bounds (start, end_iter);

		Gtk::TextIter curr_line;

		int start_line = start.get_line();
		int end_line = end_iter.get_line();

		for (int i = start_line; i <= end_line; i++) {
			curr_line = get_iter_at_line(i);
			if (increase)
				increase_depth (curr_line);
			else
				decrease_depth (curr_line);
		}
	}


	// Change the writing direction (ie. RTL or LTR) of a bullet.
	// This makes the bulleted line use the correct indent
	void NoteBuffer::change_bullet_direction(Gtk::TextIter iter, PangoDirection direction)
	{
		iter.set_line_offset(0);

		DepthNoteTag::Ptr tag = find_depth_tag (iter);
		if (tag) {
			if ((tag->get_direction() != direction) &&
					(direction != PANGO_DIRECTION_NEUTRAL)) {
				NoteTagTable::Ptr note_table = NoteTagTable::Ptr::cast_dynamic(get_tag_table());

				// Get the depth tag for the given direction
				Glib::RefPtr<Gtk::TextTag> new_tag = note_table->get_depth_tag (tag->get_depth(), direction);

				Gtk::TextIter next = iter;
				next.forward_char ();

				// Replace the old depth tag with the new one
				remove_all_tags (iter, next);
				apply_tag (new_tag, iter, next);
			}
		}
	}


	void NoteBuffer::insert_bullet(Gtk::TextIter & iter, int depth, PangoDirection direction)
	{
		NoteTagTable::Ptr note_table = NoteTagTable::Ptr::cast_dynamic(get_tag_table());

		DepthNoteTag::Ptr tag = note_table->get_depth_tag (depth, direction);

		std::string bullet =
			s_indent_bullets [depth % NUM_INDENT_BULLETS] + " ";

		insert_with_tag (iter, bullet, tag);
	}

	void NoteBuffer::remove_bullet(Gtk::TextIter & iter)
	{
		Gtk::TextIter end_iter;
		Gtk::TextIter line_end = iter;

		line_end.forward_to_line_end ();

		if (line_end.get_line_offset() < 2) {
			end_iter = get_iter_at_line_offset (iter.get_line(), 1);
		} 
		else {
			end_iter = get_iter_at_line_offset (iter.get_line(), 2);
		}

		// Go back one more character to delete the \n as well
		iter = get_iter_at_line (iter.get_line() - 1);
		iter.forward_to_line_end ();

		erase(iter, end_iter);
	}


	void NoteBuffer::increase_depth(Gtk::TextIter & start)
	{
		if (!can_make_bulleted_list())
			return;

		Gtk::TextIter end_iter;

		start = get_iter_at_line_offset (start.get_line(), 0);

		Gtk::TextIter line_end = get_iter_at_line (start.get_line());
		line_end.forward_to_line_end ();

		end_iter = start;
		end_iter.forward_chars(2);

		DepthNoteTag::Ptr curr_depth = find_depth_tag (start);

		undoer().freeze_undo ();
		if (!curr_depth) {
			// Insert a brand new bullet
			Gtk::TextIter next = start;
			next.forward_sentence_end ();
			next.backward_sentence_start ();

			// Insert the bullet using the same direction
			// as the text on the line
			PangoDirection direction = PANGO_DIRECTION_LTR;
			if ((next.get_char() > 0) && (next.get_line() == start.get_line()))
				direction = pango_unichar_direction(next.get_char());

			insert_bullet (start, 0, direction);
		} 
		else {
			// Remove the previous indent
			erase (start, end_iter);

			// Insert the indent at the new depth
			int nextDepth = curr_depth->get_depth() + 1;
			insert_bullet (start, nextDepth, curr_depth->get_direction());
		}
		undoer().thaw_undo ();

		signal_change_text_depth (start.get_line(), true);
	}

	void NoteBuffer::decrease_depth(Gtk::TextIter & start)
	{
		if (!can_make_bulleted_list())
			return;

		Gtk::TextIter end_iter;

		start = get_iter_at_line_offset (start.get_line(), 0);

		Gtk::TextIter line_end = start;
		line_end.forward_to_line_end ();

		if ((line_end.get_line_offset() < 2) || start.ends_line()) {
			end_iter = start;
		} 
		else {
			end_iter = get_iter_at_line_offset (start.get_line(), 2);
		}

		DepthNoteTag::Ptr curr_depth = find_depth_tag (start);

		undoer().freeze_undo ();
		if (curr_depth) {
			// Remove the previous indent
			erase(start, end_iter);

			// Insert the indent at the new depth
			int nextDepth = curr_depth->get_depth() - 1;

			if (nextDepth != -1) {
				insert_bullet (start, nextDepth, curr_depth->get_direction());
			}
		}
		undoer().thaw_undo ();

		signal_change_text_depth (start.get_line(), false);
	}


	DepthNoteTag::Ptr NoteBuffer::find_depth_tag(Gtk::TextIter & iter)
	{
		DepthNoteTag::Ptr depth_tag;

		foreach (const Glib::RefPtr<Gtk::TextTag> & tag, iter.get_tags()) {
			if (NoteTagTable::tag_has_depth (tag)) {
				depth_tag = DepthNoteTag::Ptr::cast_dynamic(tag);
				break;
			}
		}

		return depth_tag;
	}

}
