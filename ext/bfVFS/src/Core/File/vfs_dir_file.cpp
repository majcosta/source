/*
 * bfVFS : vfs/Core/File/vfs_dir_file.cpp
 *  - read/read-write files for usage in vfs locations (directories)
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

#include <vfs/Core/File/vfs_dir_file.h>
#include <vfs/Core/Interface/vfs_directory_interface.h>
#include <vfs/Core/vfs_os_functions.h>

#include <vfs/Aspects/vfs_settings.h>

vfs::ReadOnlyDirFile::ReadOnlyDirFile(vfs::Path const& filename, Location_t *directory)
: ReadOnlyFile(filename), _location(directory)
{
}

vfs::ReadOnlyDirFile::~ReadOnlyDirFile()
{
}

vfs::Path vfs::ReadOnlyDirFile::getPath() const
{
	Location_t::SP loc = _location.lock();
	if(!loc.isNull())
	{
		return loc->getPath() + m_filename;
	}
	else
	{
		return m_filename;
	}
}

void vfs::ReadOnlyDirFile::getPath(vfs::Path& dir, vfs::Path& file) const
{
	Location_t::SP loc = _location.lock();
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

bool vfs::ReadOnlyDirFile::_getRealPath(vfs::Path& path)
{
	Location_t::SP loc = _location.lock();
	if(!loc.isNull())
	{
		path = loc->getRealPath() + m_filename;
		return true;
	}
	return false;
}


vfs::FileAttributes vfs::ReadOnlyDirFile::getAttributes()
{
	FileAttributes _attribs = ReadOnlyFile::getAttributes();

	vfs::UInt32 attr  = _attribs.getAttrib();
	attr             |= FileAttributes::ATTRIB_READONLY;

	return FileAttributes(attr, FileAttributes::LT_READONLY_DIRECTORY);
}

bool vfs::ReadOnlyDirFile::openRead()
{
	Path filename;
	if(!_getRealPath(filename))
	{
		return false;
	}
	return _internalOpenRead(filename);
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

vfs::DirFile::DirFile(vfs::Path const& filename, Location_t *directory)
: File(filename), _location(directory)
{
}

vfs::DirFile::~DirFile()
{
}

vfs::Path vfs::DirFile::getPath() const
{
	Location_t::SP loc = _location.lock();
	if(!loc.isNull())
	{
		return loc->getPath() + m_filename;
	}
	else
	{
		return m_filename;
	}
}

void vfs::DirFile::getPath(vfs::Path& dir, vfs::Path& file) const
{
	Location_t::SP loc = _location.lock();
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


bool vfs::DirFile::deleteFile()
{
	this->close();
	Path fname;
	if(_getRealPath(fname))
	{
		return OS::deleteRealFile(fname);
	}
	return false;
}


bool vfs::DirFile::_getRealPath(vfs::Path& path)
{
	Location_t::SP loc = _location.lock();
	if(!loc.isNull())
	{
		path = loc->getRealPath() + m_filename;
		return true;
	}
	return false;
}


vfs::FileAttributes vfs::DirFile::getAttributes()
{
	FileAttributes _attribs = getAttributes();

	return FileAttributes(_attribs.getAttrib(), FileAttributes::LT_DIRECTORY);
}

bool vfs::DirFile::openRead()
{
	Path filename;
	if(!_getRealPath(filename))
	{
		return false;
	}
	return _internalOpenRead(filename);
}

bool vfs::DirFile::openWrite(bool createWhenNotExist, bool truncate)
{
	Path filename;
	if(!_getRealPath(filename))
	{
		return false;
	}
	return _internalOpenWrite(filename, createWhenNotExist, truncate);
}

