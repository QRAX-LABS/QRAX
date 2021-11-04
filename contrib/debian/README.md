
Debian
====================
This directory contains files used to package qraxd/qrax-qt
for Debian-based Linux systems. If you compile qraxd/qrax-qt yourself, there are some useful files here.

## qrax: URI support ##


qrax-qt.desktop  (Gnome / Open Desktop)
To install:

        sudo desktop-file-install qrax-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your qrax-qt binary to `/usr/bin`
and the `../../share/pixmaps/qrax128.png` to `/usr/share/pixmaps`

qrax-qt.protocol (KDE)

