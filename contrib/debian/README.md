
Debian
====================
This directory contains files used to package ankerd/anker-qt
for Debian-based Linux systems. If you compile ankerd/anker-qt yourself, there are some useful files here.

## anker: URI support ##


anker-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install anker-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your ankerqt binary to `/usr/bin`
and the `../../share/pixmaps/anker128.png` to `/usr/share/pixmaps`

anker-qt.protocol (KDE)

