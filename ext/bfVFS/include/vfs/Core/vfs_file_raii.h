/*
 * bfVFS : vfs/Core/vfs_file_raii.h
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

#ifndef _VFS_FILE_RAII_H_
#define _VFS_FILE_RAII_H_

#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/vfs_vfile.h>

namespace vfs
{
	class VFS_API OpenReadFile
	{
	public:
		OpenReadFile(Path const&     path, VirtualFile::ESearchFile eSF = VirtualFile::SF_TOP);
		OpenReadFile(ReadableFile_t* file);
		~OpenReadFile();

		ReadableFile_t*     operator->();
		ReadableFile_t*     file();
		void                release();

	private:
		ReadableFile_t::SP  m_file;
	};

	class VFS_API OpenWriteFile
	{
	public:
		OpenWriteFile(	Path const& path,
						bool create = false,
						bool truncate = false,
						VirtualFile::ESearchFile eSF = VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
		OpenWriteFile(WritableFile_t *file, bool append=false);
		~OpenWriteFile();

		WritableFile_t*     operator->();
		WritableFile_t*     file();
		void                release();

	private:
		WritableFile_t::SP  m_file;
	};

} // end namespace

#endif // _VFS_FILE_RAII_H_
