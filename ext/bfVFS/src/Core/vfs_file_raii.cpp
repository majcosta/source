/*
 * bfVFS : vfs/Core/vfs_file_raii.cpp
 *  - RAII classes to open files for reading/writing
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

#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs.h>

#include <sstream>
/********************************************************************************************/
/********************************************************************************************/

vfs::OpenReadFile::OpenReadFile(vfs::Path const& path, vfs::VirtualFile::ESearchFile eSF)
{
	IBaseFile::SP file = getVFS()->getFile(path,eSF);
	VFS_THROW_IFF(!file.isNull(), _BS(L"file \"") << path << L"\" does not exist" << _BS::wget);

	m_file = ReadableFile_t::cast(file);

	VFS_THROW_IFF(m_file, _BS(L"File \"") << path << L"\" is not readable" << _BS::wget);

	VFS_THROW_IFF(m_file->openRead(), _BS(L"Could not open file : ") << m_file->getPath() << _BS::wget);
}

vfs::OpenReadFile::OpenReadFile(vfs::ReadableFile_t *file)
{
	try
	{
		m_file = file;
		VFS_THROW_IFF(!m_file.isNull(), L"Invalid file object");

		VFS_THROW_IFF(m_file->openRead(), _BS(L"Could not open file : ") << file->getPath() << _BS::wget);
	}
	catch(std::exception &ex)
	{
		VFS_RETHROW(L"",ex);
	}
}
vfs::OpenReadFile::~OpenReadFile()
{
	if(m_file)
	{
		m_file->close();
		m_file.null();
	}
}

vfs::ReadableFile_t* vfs::OpenReadFile::operator->()
{
	return m_file;
}

vfs::ReadableFile_t* vfs::OpenReadFile::file()
{
	return m_file;
}

void vfs::OpenReadFile::release()
{
	m_file.null();
}

/**************************************************************************/

vfs::OpenWriteFile::OpenWriteFile(vfs::Path const& path,
									bool create,
									bool truncate,
									vfs::VirtualFile::ESearchFile eSF)
{
	IBaseFile::SP file = getVFS()->getFile(path,eSF);
	if(file.isNull() && create)
	{
		if(getVFS()->createNewFile(path))
		{
			file = getVFS()->getFile(path,eSF);
		}
		else
		{
			VFS_THROW(_BS(L"Could not create VFS file \"") << path << L"\"" << _BS::wget);
		}
	}
	VFS_THROW_IFF(file, _BS(L"File \"") << path << L"\" not found" << _BS::wget);

	m_file = WritableFile_t::cast(file);

	VFS_THROW_IFF(!m_file.isNull(), _BS(L"File \"") << path << L"\" exists, but is not writable" << _BS::wget);

	VFS_THROW_IFF(m_file->openWrite(create,truncate), _BS(L"File \"") << path << L"\" could not be opened for writing" << _BS::wget);
}
vfs::OpenWriteFile::OpenWriteFile(vfs::WritableFile_t *file, bool append)
{
	try
	{
		m_file = file;
		VFS_THROW_IFF(!m_file.isNull(), L"Invalid file object");
		VFS_THROW_IFF(m_file->openWrite(true,!append), _BS(L"Could not open file : ") << m_file->getPath() << _BS::wget);
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"",ex);
	};
}
vfs::OpenWriteFile::~OpenWriteFile()
{
	if(m_file)
	{
		m_file->close();
		m_file.null();
	}
}

vfs::WritableFile_t* vfs::OpenWriteFile::operator->()
{
	return m_file;
}

vfs::WritableFile_t* vfs::OpenWriteFile::file()
{
	return m_file;
}

void vfs::OpenWriteFile::release()
{
	m_file.null();
}

