/*
 * Hedgewars, a free turn based strategy game
 * Copyright (c) 2009-2011 Andrey Korotaev <unC0Rr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "M3Panel.h"
#include "M3InstallController.h"

#include <Cocoa/Cocoa.h>

class M3Panel::Private
{
public:
        M3InstallController *install;
};

M3Panel::M3Panel(void)
{
	c = new Private;

	c->install = [[M3InstallController alloc] init];
	[c->install retain];

}

M3Panel::~M3Panel()
{
	[c->install release];
	delete c;
}

void M3Panel::showInstallController()
{
        [c->install displayInstaller];
}
