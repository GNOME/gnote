/*
 * gnote
 *
 * Copyright (C) 2013-2014,2016-2017,2019-2020,2023 Aurimas Cernius
 * Copyright (C) 2011 Debarshi Ray
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

#ifndef __TRIE_HPP_
#define __TRIE_HPP_

#include <queue>

#include "triehit.hpp"

namespace gnote {

template<class value_t>
class TrieTree
{

private:

  class TrieState;
  typedef TrieState* TrieStatePtr;
  typedef std::deque<TrieStatePtr> TrieStateList;
  typedef std::queue<TrieStatePtr> TrieStateQueue;

  class TrieState
  {
  public:

    TrieState(gunichar v, int d, const TrieStatePtr & s)
      : m_value(v)
      , m_depth(d)
      , m_fail_state(s)
      , m_transitions()
      , m_payload()
      , m_payload_present(false)
    {
    }

    gunichar value() const
    {
      return m_value;
    }

    int depth() const
    {
      return m_depth;
    }

    TrieStatePtr fail_state()
    {
      return m_fail_state;
    }

    void fail_state(const TrieStatePtr & s)
    {
      m_fail_state = s;
    }

    TrieStateList & transitions()
    {
      return m_transitions;
    }

    value_t payload() const
    {
      return m_payload;
    }

    void payload(const value_t & p)
    {
      m_payload = p;
    }

    bool payload_present() const
    {
      return m_payload_present;
    }

    void payload_present(bool pp)
    {
      m_payload_present = pp;
    }

  private:

    gunichar m_value;
    int m_depth;
    TrieStatePtr m_fail_state;
    TrieStateList m_transitions;
    value_t m_payload;
    bool m_payload_present;
  };

  std::vector<TrieState*> m_states;
  const bool m_case_sensitive;
  const TrieStatePtr m_root;
  size_t m_max_length;

public:

  TrieTree(bool case_sensitive)
    : m_case_sensitive(case_sensitive)
    , m_root(new TrieState('\0', -1, TrieStatePtr()))
    , m_max_length(0)
  {
    m_states.push_back(m_root);
  }

  ~TrieTree()
  {
    for(auto state : m_states) {
      delete state;
    }
  }

  void add_keyword(const Glib::ustring & keyword, const value_t & pattern_id)
  {
    TrieStatePtr current_state = m_root;
    Glib::ustring::size_type i;
    Glib::ustring::const_iterator iter;
    for(i = 0, iter = keyword.begin(); iter != keyword.end(); ++i, ++iter) {
      gunichar c = *iter;
      if (!m_case_sensitive)
        c = Glib::Unicode::tolower(c);

      TrieStatePtr target_state = find_state_transition(current_state, c);
      if (0 == target_state) {
        target_state = new TrieState(c, i, m_root);
        m_states.push_back(target_state);
        current_state->transitions().push_front(target_state);
      }

      current_state = target_state;
    }

    current_state->payload(pattern_id);
    current_state->payload_present(true);
    m_max_length = std::max(m_max_length, keyword.size());
  }

  void compute_failure_graph()
  {
    // Failure state is computed breadth-first (-> Queue)
    TrieStateQueue state_queue;

    // For each direct child of the root state
    // * Set the fail state to the root state
    // * Enqueue the state for failure graph computing
    for (typename TrieStateList::iterator iter = m_root->transitions().begin();
         m_root->transitions().end() != iter; iter++) {
      TrieStatePtr & transition = *iter;
      transition->fail_state(m_root);
      state_queue.push(transition);
    }

    while (false == state_queue.empty()) {
      // Current state already has a valid fail state at this point
      TrieStatePtr current_state = state_queue.front();
      state_queue.pop();

      for (typename TrieStateList::iterator iter
             = current_state->transitions().begin();
           current_state->transitions().end() != iter; iter++) {
        TrieStatePtr & transition = *iter;
        state_queue.push(transition);

        TrieStatePtr fail_state = current_state->fail_state();
        while ((0 != fail_state)
               && 0 == find_state_transition(fail_state, transition->value())) {
          fail_state = fail_state->fail_state();
        }

        if (0 == fail_state)
          transition->fail_state(m_root);
        else
          transition->fail_state(find_state_transition(fail_state, transition->value()));
      }
    }
  }

  static TrieStatePtr find_state_transition(const TrieStatePtr & state,
                                            gunichar value)
  {
    if (true == state->transitions().empty())
      return TrieStatePtr();

    for (typename TrieStateList::const_iterator iter
           = state->transitions().begin();
         state->transitions().end() != iter; iter++) {
      const TrieStatePtr & transition = *iter;
      if (transition->value() == value)
        return transition;

    }

    return TrieStatePtr();
  }

  typename TrieHit<value_t>::List find_matches (const Glib::ustring & haystack)
  {
    TrieStatePtr current_state = m_root;
    typename TrieHit<value_t>::List matches;
    int start_index = 0;

    Glib::ustring::const_iterator haystack_iter = haystack.begin();
    for (Glib::ustring::size_type i = 0; haystack_iter != haystack.end(); ++i, ++haystack_iter ) {
      gunichar c = *haystack_iter;
      if (!m_case_sensitive)
        c = Glib::Unicode::tolower(c);

      if (current_state == m_root)
        start_index = i;

      // While there's no matching transition, follow the fail states
      // Because we're potentially changing the depths (aka length of
      // matched characters) in the tree we're updating the start_index
      // accordingly
      while ((current_state != m_root)
             && 0 == find_state_transition(current_state, c)) {
        TrieStatePtr old_state = current_state;
        current_state = current_state->fail_state();
        start_index += old_state->depth() - current_state->depth();
      }

      current_state = find_state_transition (current_state, c);
      if (0 == current_state)
        current_state = m_root;

      // If the state contains a payload: We've got a hit
      // Return a TrieHit with the start and end index, the matched
      // string and the payload object
      if (current_state->payload_present()) {
        int hit_length = i - start_index + 1;
        TrieHit<value_t> hit(start_index, start_index + hit_length, haystack.substr(start_index, hit_length), current_state->payload());
        matches.push_back(hit);
      }
    }

    return matches;
  }

  size_t max_length() const
  {
    return m_max_length;
  }

};

}

#endif
