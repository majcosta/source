/*
 * bfVFS : vfs/Tools/vfs_classname.cpp
 *  - creates undecorated (unmangled) class name from what typeid(Type).name() returns
 *
 * Copyright (C) 2008 - 2012 (BF) john.bf.smith@googlemail.com
 *
 * This file is part of the bfVFS library
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <vfs/Tools/vfs_classname.h>

#include <cstdlib>

#if defined __GNUC__
#	include <cxxabi.h>
#endif

std::string vfs::Classname::name(const char* const mangled_name)
{
	std::string s;
#ifdef __GNUC__
	int status;
	char* name = abi::__cxa_demangle(mangled_name,0,0,&status);
	s.assign(name);
	free(name);
#elif defined _MSC_VER
	s.assign(mangled_name);
#endif
	return s;
}

