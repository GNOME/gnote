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


#include <cerrno>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "debug.hpp"
#include "process.hpp"


namespace sharp {

Process::Process()
  : m_exit_code(-1)
  , m_pid(-1)
  , m_stdout(0)
  , m_stderr(0)
{
}

void Process::start()
{
  if(m_file_name == "") {
    return;
  }
  int stdo[2];
  int stde[2];
  if(m_redirect_stdout) {
    pipe(stdo);
  }
  if(m_redirect_stderr) {
    pipe(stde);
  }
  m_pid = fork();
  if(m_pid == 0) {
    redirect_output(m_redirect_stdout, 1, stdo);
    redirect_output(m_redirect_stderr, 2, stde);
    char **argv = (char**) std::malloc((m_args.size() + 2) * sizeof(char*));
    argv[0] = strdup(m_file_name.c_str());
    argv[m_args.size() + 1] = NULL;
    for(unsigned i = 0; i < m_args.size(); ++i) {
      argv[i+1] = strdup(m_args[i].c_str());
    }
    execv(m_file_name.c_str(), argv);
    LOG_OUT("execv() failed: %s", execv_error(errno));
    _exit(1);
  }
  else {
    if(m_redirect_stdout) {
      close(stdo[1]);
      m_stdout = stdo[0];
    }
    if(m_redirect_stdout) {
      close(stde[1]);
      m_stderr = stde[0];
    }
  }
}

void Process::redirect_output(bool redirect, int fileno, int *pipedes)
{
  if(redirect) {
    close(fileno);
    dup2(pipedes[1], fileno);
    close(pipedes[1]);
    fcntl(fileno, F_SETFL, O_NONBLOCK);
  }
  else {
    close(pipedes[1]);
    close(fileno);
  }
  close(pipedes[0]);
}

void Process::wait_for_exit()
{
  if(m_pid < 0) {
    return;
  }
  int status = -1;
  waitpid(m_pid, &status, 0);
  if(WIFEXITED(status)) {
    m_exit_code = WEXITSTATUS(status);
  }
}

bool Process::wait_for_exit(unsigned timeout)
{
  if(m_pid < 0) {
    return false;
  }
  unsigned secs = timeout / 1000;
  if(timeout % 1000) {
    ++secs;
  }

  while(secs--) {
    int status = -1;
    waitpid(m_pid, &status, WNOHANG);
    if(WIFEXITED(status)) {
      m_exit_code = WEXITSTATUS(status);
      return true;
    }
    if(WIFSIGNALED(status)) {
      return true;
    }
    sleep(1);
  }

  return false;
}

bool Process::standard_output_eof()
{
  return eof(m_stdout_stream, m_stdout);
}

std::string Process::standard_output_read_line()
{
  return read_line(m_stdout_stream, m_stdout);
}

bool Process::standard_error_eof()
{
  return eof(m_stderr_stream, m_stderr);
}

std::string Process::standard_error_read_line()
{
  return read_line(m_stderr_stream, m_stderr);
}

bool Process::eof(std::stringstream & stream, int & m_file)
{
  if(m_file == 0 && stream.tellg() < 0) {
    return true;
  }

  if(m_file) {
    perform_read(stream, m_file);
  }
  return !m_file && stream.tellg() < 0;
}

std::string Process::read_line(std::stringstream & stream, int & m_file)
{
  while(m_file && !line_available(stream)) {
    if(!perform_read(stream, m_file)) {
      break;
    }
  }

  std::string line;
  std::getline(stream, line);
  return line;
}

bool Process::line_available(std::stringstream & stream)
{
  if(stream.tellg() < 0) {
    return false;
  }

  std::string contents = stream.str();
  if(contents.size() <= unsigned(stream.tellg())) {
    return false;
  }

  return contents.substr(stream.tellg()).find('\n') != std::string::npos;
}

bool Process::perform_read(std::stringstream & stream, int & m_file)
{
  while(true) {
    char buf[255];
    int read_count = read(m_file, buf, sizeof(buf));
    if(read_count < 0) {
      return false;
    }
    else if(read_count == 0) {
      if(errno == EAGAIN) {
        int status;
        waitpid(m_pid, &status, WNOHANG);
        if(WIFEXITED(status) || WIFSIGNALED(status)) {
          close(m_file);
          m_file = 0;
          m_exit_code = WEXITSTATUS(status);
          return false;
        }
	continue;
      }
      else {
	close(m_file);
	m_file = 0;
        return false;
      }
    }
    else {
      stream.write(buf, read_count);
      break;
    }
  }
  return true;
}

const char *Process::execv_error(int error)
{
  switch(error) {
  case EACCES:
    return "EACCES";
  case EPERM:
    return "EPERM";
  case E2BIG:
    return "E2BIG";
  case ENOEXEC:
    return "ENOEXEC";
  case EFAULT:
    return "EFAULT";
  case ENAMETOOLONG:
    return "ENAMETOOLONG";
  case ENOENT:
    return "ENOENT";
  case ENOMEM:
    return "ENOMEM";
  case ENOTDIR:
    return "ENOTDIR";
  case ELOOP:
    return "ELOOP";
  case ETXTBSY:
    return "ETXTBSY";
  case EIO:
    return "EIO";
  case ENFILE:
    return "ENFILE";
  case EMFILE:
    return "EMFILE";
  case EINVAL:
    return "EINVAL";
  case EISDIR:
    return "EISDIR";
#ifdef ELIBBAD
  case ELIBBAD:
    return "ELIBBAD";
#endif
  default:
    return "Unknown";
  }
}


}

