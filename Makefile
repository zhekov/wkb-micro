#
# Copyright (C) 2016-2022 Dimitar Toshkov Zhekov <dimitar.zhekov@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

all: out\wkb-micro.exe

STAMP = out\__stamp

$(STAMP): Makefile
	mkdir -p out
	touch -r Makefile $(STAMP)

CC = x86_64-w64-mingw32-gcc
CRTDEFS = -D__USE_MINGW_ANSI_STDIO=0
CFLAGS = -Wall -Wextra -O2
LD = $(CC)
LDFLAGS = -s
LDSUBSYS = -municode -mwindows
CRTLIBS =
HOOKFLAGS = -shared -nostdlib -nostartfiles
RC = windres -F pe-x86-64

out\wkb-hook.o: wkb-hook.h
out\utility.o: utility.h
out\registry.o: registry.h utility.h
out\layouts.o: layouts.h registry.h utility.h
out\settings.o: settings.h layouts.h registry.h utility.h charmap.rh dialogs.rh
out\wkb-micro.o: settings.h layouts.h registry.h utility.h wkb-hook.h

OBJS = out\wkb-micro.o out\settings.o out\layouts.o out\registry.o out\utility.o out\wkb-hook.o

$(OBJS): out\\%.o : %.c $(STAMP)
	$(CC) $(CRTDEFS) $(CFLAGS) -c -o $@ $<

out\wkb-hook.dll: out\wkb-hook.o Makefile
	$(LD) $(LDFLAGS) $(HOOKFLAGS) -o $@ out\wkb-hook.o -luser32 $(LDSUBSYS)

out\wkb-version.o: wkb-version.rc gucharmap1.ico manifest.xml charmap.rh $(STAMP)
	$(RC) -o $@ wkb-version.rc

out\dialogs.o: dialogs.rc dialogs.rh charmap.rh $(STAMP)
	$(RC) -o $@ dialogs.rc

MICRO_DEPS = out\wkb-micro.o out\settings.o out\layouts.o out\registry.o \
	out\utility.o out\dialogs.o out\wkb-version.o out\wkb-hook.dll

out\wkb-micro.exe: $(MICRO_DEPS) Makefile
	$(LD) $(LDFLAGS) -o $@ $(MICRO_DEPS) -limm32 $(LDSUBSYS) $(CRTLIBS)

deps:
	$(CC) -MM *.c

clean:
	rm -rfv out

.PHONY: all deps clean
