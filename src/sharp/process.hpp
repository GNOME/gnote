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


#ifndef _SHARP_PROCESS_HPP_
#define _SHARP_PROCESS_HPP_

#include <sstream>
#include <string>
#include <vector>


namespace sharp {

class Process
{
public:
  Process();
  void start();
  void wait_for_exit();
  bool wait_for_exit(unsigned timeout);
  bool standard_output_eof();
  std::string standard_output_read_line();
  bool standard_error_eof();
  std::string standard_error_read_line();
  int exit_code() const
    {
      return m_exit_code;
    }
  std::string file_name() const
    {
      return m_file_name;
    }
  void file_name(const std::string & fname)
    {
      m_file_name = fname;
    }
  std::vector<std::string> arguments() const
    {
      return m_args;
    }
  void arguments(const std::vector<std::string> & args)
    {
      m_args = args;
    }
  bool redirect_standard_output() const
    {
      return m_redirect_stdout;
    }
  void redirect_standard_output(bool redirect)
    {
      m_redirect_stdout = redirect;
    }
  bool redirect_standard_error() const
    {
      return m_redirect_stderr;
    }
  void redirect_standard_error(bool redirect)
    {
      m_redirect_stderr = redirect;
    }
private:
  static const char *execv_error(int error);
  static void redirect_output(bool redirect, int fileno, int *pipedes);

  bool eof(std::stringstream & stream, int & m_file);
  std::string read_line(std::stringstream & stream, int & m_file);
  bool line_available(std::stringstream & stream);
  bool perform_read(std::stringstream & stream, int & m_file);

  int m_exit_code;
  std::string m_file_name;
  std::vector<std::string> m_args;
  bool m_redirect_stdout;
  bool m_redirect_stderr;
  int m_pid;
  int m_stdout;
  std::stringstream m_stdout_stream;
  int m_stderr;
  std::stringstream m_stderr_stream;
};

}

#endif
