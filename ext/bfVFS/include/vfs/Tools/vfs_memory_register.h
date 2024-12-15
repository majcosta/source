/*
 * bfVFS : vfs/Tools/vfs_memory_register.h
 *  - store pointers to objects along with the location of its creation,
 *  - user has to provide additional information (class name, file and line where object was created)
 *  - macros to use register depending on compiler flags (VFS_DEBUG_MEMORY, VFS_BUILD_LIBRARY and VFS_DEBUG_MEMORY_INTERNAL)
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

#ifndef VFS_MEMORY_REGISTER_H_
#define VFS_MEMORY_REGISTER_H_

#include <vfs/vfs_config.h>

#include <string>
#include <map>

namespace vfs
{
	class ObjectBase;

	class VFS_API MemRegister
	{
		struct Entry
		{
			const void*		        ptr;
			int                     num;
			std::string             name;
		};

		typedef std::map<const void*, Entry> Map_t;

		Map_t m_map;

		static long unsigned int    s_counter;
		static long int             s_total;

	public:

		static MemRegister& instance();

		~MemRegister();

		void registerPointer  (const void* ptr, std::string const& s, const char* file="", int line=-1);
		void unregisterPointer(const void* ptr);
		void printMem();
	};
}

#ifdef VFS_DEBUG_MEMORY

#if defined(VFS_BUILD_LIBRARY) && !defined(VFS_DEBUG_MEMORY_INTERNAL)

#define VFS_MEM_REGISTER(Type, pointer, file, line)
#define VFS_MEM_UNREGISTER(pointer)

#else

#define VFS_MEM_REGISTER(Type, pointer, file, line)     vfs::MemRegister::instance().registerPointer(pointer, Classname::get<Type>(), file, line);
#define VFS_MEM_UNREGISTER(pointer)                     vfs::MemRegister::instance().unregisterPointer(pointer);

#endif

#else

#define VFS_MEM_REGISTER(Type, pointer, file, line)
#define VFS_MEM_UNREGISTER(pointer)

#endif // VFS_DEBUG_MEMORY

#endif /* VFS_MEMORY_REGISTER_H_ */
