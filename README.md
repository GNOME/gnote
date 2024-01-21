# Gnote

Gnote started as a C++ port of Tomboy and has moved its own path later.

Starting as Tomboy clone, Gnote continues with the same initial ideas and
mostly has the same functionality. However it moved forward for the sake
of better integration with modern GNOME desktop environment.

https://wiki.gnome.org/Apps/Gnote

## Compiling

Gnote uses Meson for its build system, so compiling it is a standard:
meson <dirname> && cd <dirname> && meson compile && ninja install

Dependencies:
- Glibmm 2.74
- Gtkmm 4.0
- libadwaita
- libsecret
- libuuid
- libxml2
- libxslt

## Importing Tomboy notes

Upon first running Gnote, it will import Tomboy and eventually 
StickyNote notes. These will be *copied*.

It is deliberate that it does not share the storage with Tomboy.
Note: due to the way Tomboy work, the "pinned" state of the notes 
will not be carried over, not yet.

## Getting the code

The source tree is currently hosted on GNOME.
To view the repository online:
   https://gitlab.gnome.org/GNOME/gnote

The URL to clone it:
   git clone https://gitlab.gnome.org/GNOME/gnote.git


## Reporting bug

Please report bugs in the GNOME issue tracker:
https://gitlab.gnome.org/GNOME/gnote/-/issues/

## Translations

If you have translated Gnote, please contact the corresponding team:

https://l10n.gnome.org/teams/

## Support Forum

There is a support forum for gnote. More info here:
https://discourse.gnome.org/tag/gnote

## License

This software in GPLv3 or later. See the file COPYING. Some files are
under a different license. Check the headers for the copyright info.

## Authors:

Aurimas Cernius <aurimasc@src.gnome.org>

Debarshi Ray <debarshir@src.gnome.org>

Hubert Figuiere <hub@figuiere.net>


Original Tomboy authors and copyright:
Copyright (C) 2004-2007 Alex Graveley <alex@beatniksoftware.com>

Alex Graveley <alex@beatniksoftware.com>

Boyd Timothy <btimothy@gmail.com>

Chris Scobell <chris@thescobells.com>

David Trowbridge <trowbrds@gmail.com>

Ryan Lortie <desrt@desrt.ca>

Sandy Armstrong <sanfordarmstrong@gmail.com>

Sebastian Rittau <srittau@jroger.in-berlin.de>

Kevin Kubasik <kevin@kubasik.net>

Stefan Schweizer <steve.schweizer@gmail.com>

Benjamin Podszun <benjamin.podszun@gmail.com>

