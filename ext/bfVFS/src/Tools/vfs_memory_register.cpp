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

#include <vfs/Tools/vfs_memory_register.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Aspects/vfs_logging.h>

#include <iomanip>


long unsigned int vfs::MemRegister::s_counter = 0;
long          int vfs::MemRegister::s_total   = 0;

vfs::MemRegister& vfs::MemRegister::instance()
{
	// the register is supposed to live until the very end of the program, therefore it will not be deleted manually
	static MemRegister* memreg = new MemRegister();
	return *memreg;
}

vfs::MemRegister::~MemRegister()
{
	printMem();
	m_map.clear();
}

void vfs::MemRegister::registerPointer(const void* ptr, std::string const& s, const char* file, int line)
{
	Map_t::iterator iter = m_map.find(ptr);
	if(iter == m_map.end())
	{
		Entry& e = m_map[ptr];
		e.ptr    = ptr;
		e.num    = ++s_counter;
		e.name   = s;

		std::stringstream ss;
		ss  << " +++ "       << (++s_total) << ":t"   << e.num  << "  [" << ptr << "] "
		    << std::setw(40) << std::left   << e.name << " : l."
		    << std::setw(5)  << line        << ", "   << file;

		VFS_LOG_DEBUG( String(ss.str()) );

		return;
	}
	VFS_THROW(_BS(L"Pointer [") << ptr << "] already registered" << _BS::wget);
}

void vfs::MemRegister::unregisterPointer(const void* ptr)
{
	Map_t::iterator iter = m_map.find(ptr);
	if(iter != m_map.end())
	{
		std::stringstream ss;
		ss  << " --- " << (--s_total)  << ":t" << iter->second.num
		    << "  ["   << ptr          << "] " << iter->second.name;

		VFS_LOG_DEBUG(ss.str());

		m_map.erase(iter);
	}
}

void vfs::MemRegister::printMem()
{
	std::stringstream ss;

	Map_t::iterator iter = m_map.begin();
	for(; iter != m_map.end(); ++iter)
	{
		ss.str("");
		ss  << " ??? " << (--s_total)      << ":t" << iter->second.num
			<< "  ["   << iter->second.ptr << "] " << iter->second.name;

		VFS_LOG_DEBUG(ss.str());
	}
}

