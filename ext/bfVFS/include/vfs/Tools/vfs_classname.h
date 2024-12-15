/*
 * bfVFS : vfs/Tools/vfs_classname.h
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

#ifndef VFS_CLASSNAME_H_
#define VFS_CLASSNAME_H_

#include <vfs/vfs_config.h>

#include <typeinfo>
#include <string>

namespace vfs
{
	class VFS_API Classname
	{
		static std::string name(const char* const mangled_name);

	public:

		template<typename T>
		static std::string get()
		{
			return name( typeid(T).name() );
		}
	};
}

#endif /* VFS_CLASSNAME_H_ */
