/*
 * bfVFS : vfs/Core/Interface/vfs_library_interface.h
 *  - partially implements Location interface for archive files
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

#ifndef _VFS_LIBRARY_INTERFACE_H_
#define _VFS_LIBRARY_INTERFACE_H_

#include <vfs/Core/Interface/vfs_location_interface.h>
#include <vfs/Core/vfs_init.h>

namespace vfs
{
	class BufferFile;

	class VFS_API ILibrary : public TLocationTemplate<IReadable,IWriteType>
	{
		typedef TLocationTemplate<IReadable,IWriteType> BaseClass_t;

	public:
		typedef BaseClass_t::FileType_t                 FileType_t;

		VFS_SMARTPOINTER(ILibrary);

		virtual ~ILibrary();

		virtual bool            init           () = 0;
		virtual void            closeLibrary   () = 0;

		/*! Copy file 'filename' into file object 'file'. Works without initialization.*/
		virtual bool            copyFile       (Path const& filename, BufferFile* file) = 0;

		virtual void            close          (FileType_t *file_handle) = 0;
		virtual bool            openRead       (FileType_t *file_handle) = 0;
		virtual vfs::size_t     read           (FileType_t *file_handle, vfs::Byte* data, vfs::size_t bytesToRead) = 0;

		virtual vfs::size_t     getReadPosition(FileType_t *file_handle) = 0;
		virtual void            setReadPosition(FileType_t *file_handle, vfs::size_t   positionInBytes) = 0;
		virtual void            setReadPosition(FileType_t *file_handle, vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir) = 0;

		virtual vfs::size_t     getSize        (FileType_t *file_handle) = 0;

		virtual bool            mapProfilePaths(vfs_init::VfsConfig::SP const& config) = 0;

		Path const&	            getName        ();

	protected:
		ILibrary(ReadableFile_t* libraryFile, Path const& mountPoint);

		ReadableFile_t::SP      m_lib_file;
	};

}

#endif // _VFS_LIBRARY_INTERFACE_H_
