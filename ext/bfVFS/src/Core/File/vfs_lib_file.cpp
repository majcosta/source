/*
 * bfVFS : vfs/Core/File/vfs_lib_file.cpp
 *  - read/read-write files for usage in vfs locations (libraries)
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

#include <vfs/Core/File/vfs_lib_file.h>
#include <vfs/Core/vfs.h>

#include <vfs/Aspects/vfs_logging.h>

#define ERROR_FILE(msg)     (_BS(L"[") << this->getPath()() << L"] - " << msg << _BS::wget)

vfs::ObjBlockAllocator<vfs::LibFile>* vfs::LibFile::_lfile_pool = NULL;

vfs::LibFile::SP vfs::LibFile::create(vfs::Path const& filename,
									 Location_t *location,
									 ILibrary   *library,
									 vfs::ObjBlockAllocator<vfs::LibFile>* allocator)
{
#if 1
	LibFile::SP file( VFS_NEW(LibFile) );
#else
	LibFile* file;
	if(allocator)
	{
		file = allocator->New();
	}
	else
	{
		if(!_lfile_pool)
		{
			_lfile_pool = new ObjBlockAllocator<LibFile>();
			ObjectAllocator::registerAllocator(_lfile_pool);
		}
		file = _lfile_pool->New();
	}
#endif
	file->m_filename = filename;
	file->m_location = location;
	file->m_library  = library;
	return file;
}

vfs::LibFile::LibFile()
: BaseClass_t(L""),
	m_isOpen_read(false)
//	m_library(NULL),
//	m_location(NULL)
{
};

vfs::LibFile::~LibFile()
{
	// VFS_LOG_DEBUG(L" delete LibFile");
}

void vfs::LibFile::close()
{
	if(m_isOpen_read)
	{
		ILibrary::SP lib = m_library.lock();
		if(!lib.isNull())
		{
			lib->close(this);
			m_isOpen_read = false;
		}
	}
}

vfs::FileAttributes vfs::LibFile::getAttributes()
{
	return FileAttributes(FileAttributes::ATTRIB_NORMAL | FileAttributes::ATTRIB_READONLY, FileAttributes::LT_LIBRARY);
}

vfs::Path vfs::LibFile::getPath() const
{
	Location_t::SP loc = m_location.lock();
	if(!loc.isNull())
	{
		return loc->getPath() + m_filename;
	}
	else
	{
		return m_filename;
	}
}

void vfs::LibFile::getPath(vfs::Path& dir, vfs::Path& file) const
{
	Location_t::SP loc = m_location.lock();
	if(!loc.isNull())
	{
		dir  = loc->getPath();
		file = m_filename;
	}
	else
	{
		file = m_filename;
	}
}

bool vfs::LibFile::isOpenRead()
{
	return m_isOpen_read;
}

bool vfs::LibFile::openRead()
{
	if(!m_isOpen_read)
	{
		ILibrary::SP lib = m_library.lock();
		if(!lib.isNull())
		{
			VFS_TRYCATCH_RETHROW(m_isOpen_read = lib->openRead(this), ERROR_FILE(L"read open error"));
		}
	}
	return m_isOpen_read;
}

vfs::size_t vfs::LibFile::read(vfs::Byte* data, vfs::size_t bytesToRead)
{
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
	try
	{
		ILibrary::SP lib = m_library.lock();
		if(!lib.isNull())
		{
			return lib->read(this, data, bytesToRead);
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(ERROR_FILE(L"read error"), ex);
	}
	return 0;
}

vfs::size_t vfs::LibFile::getReadPosition()
{
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
	try
	{
		ILibrary::SP lib = m_library.lock();
		if(!lib.isNull())
		{
			return lib->getReadPosition(this);
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(ERROR_FILE(L"library error"), ex);
	}
	return 0;
}


void vfs::LibFile::setReadPosition(vfs::size_t uiPositionInBytes)
{
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
	ILibrary::SP lib = m_library.lock();
	if(!lib.isNull())
	{
		VFS_TRYCATCH_RETHROW(lib->setReadPosition(this,uiPositionInBytes), ERROR_FILE(L"library error") );
	}
}

void vfs::LibFile::setReadPosition(vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir)
{
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
	ILibrary::SP lib = m_library.lock();
	if(!lib.isNull())
	{
		VFS_TRYCATCH_RETHROW(lib->setReadPosition(this, offsetInBytes, seekDir), ERROR_FILE(L"library error"));
	}
}

vfs::size_t vfs::LibFile::getSize()
{
	VFS_THROW_IFF( m_isOpen_read || this->openRead(), ERROR_FILE(L"could not open file") );
	try
	{
		ILibrary::SP lib = m_library.lock();
		if(!lib.isNull())
		{
			return lib->getSize(this);
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(ERROR_FILE(L"library error"), ex);
	}
	return 0;
}
