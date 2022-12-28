/*
 * gnote
 *
 * Copyright (C) 2017-2019,2022 Aurimas Cernius
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


#include <thread>
#include <glibmm/init.h>
#include <glibmm/main.h>
#include <giomm/init.h>

#include <UnitTest++/UnitTest++.h>

int main(int /*argc*/, char ** /*argv*/)
{
  // force certain timezone so that time tests work
  setenv("TZ", "Europe/London", 1);
  // also force the locale for formatting tests
  setenv("LC_ALL", "en_US", 1);
  Glib::init();
  Gio::init();

  auto main_loop = Glib::MainLoop::create();
  int ret = 0;
  std::thread thread([&main_loop, &ret]() {
    ret = UnitTest::RunAllTests();
    main_loop->quit();
  });
  main_loop->run();
  thread.join();
  return ret;
}

