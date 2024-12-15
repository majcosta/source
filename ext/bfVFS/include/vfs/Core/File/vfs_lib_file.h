/*
 * bfVFS : vfs/Core/File/vfs_lib_file.h
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

#ifndef _VFS_LIB_FILE_H_
#define _VFS_LIB_FILE_H_

#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/Interface/vfs_location_interface.h>
#include <vfs/Core/Interface/vfs_library_interface.h>
#include <vfs/Tools/vfs_allocator.h>

namespace vfs
{
	class ILibrary;

	class LibFile : public TFileTemplate<IReadable,IWriteType>
	{
		typedef TFileTemplate<IReadable,IWriteType>         BaseClass_t;
		typedef TLocationTemplate<IReadable,IWriteType>     Location_t;

	public:
		VFS_SMARTPOINTER(LibFile);

		static LibFile::SP create(vfs::Path const& filename,
			Location_t *location,
			ILibrary   *library,
			ObjBlockAllocator<LibFile>* allocator = NULL);

		// don't delete objects that YOU have not created with 'new'
		// dtor has to remain public to be usable at all
		virtual ~LibFile();

		virtual FileAttributes  getAttributes  ();

		virtual void            close          ();

		virtual Path            getPath        () const;
		virtual void            getPath(Path& dir, Path& file) const;

		virtual bool            isOpenRead     ();
		virtual bool            openRead       ();
		virtual vfs::size_t     read           (vfs::Byte* pData, vfs::size_t bytesToRead);

		virtual vfs::size_t     getReadPosition();
		virtual void            setReadPosition(vfs::size_t positionInBytes);
		virtual void            setReadPosition(vfs::offset_t offsetInBytes, vfs::IBaseFile::ESeekDir seekDir);

		virtual vfs::size_t     getSize        ();

	protected:
		LibFile();

		bool                    m_isOpen_read;
		ILibrary::WP            m_library;
		Location_t::WP          m_location;

	private:
		static ObjBlockAllocator<LibFile>*	_lfile_pool;
	};

} // end namespace

#endif // _VFS_LIB_FILE_H_

