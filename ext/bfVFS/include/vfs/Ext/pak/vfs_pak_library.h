/*
 * bfVFS : vfs/Ext/pak/vfs_pak_library.h
 *  - implements Library interface, creates library object from uncompressed JA-BiA PAK archive files
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

#ifndef _VFS_PAK_LIBRARY_H_
#define _VFS_PAK_LIBRARY_H_

#ifdef VFS_WITH_PAK

#include <vfs/Core/Location/vfs_uncompressed_lib_base.h>
#include <vfs/Ext/pak/vfs_pak_header.h>

namespace vfs
{
	class VFS_API PAKLibrary : public UncompressedLibraryBase
	{
	protected:
		PAKLibrary(ReadableFile_t* lib_file, Path const& mountpoint);

	public:
		VFS_SMARTPOINTER(PAKLibrary);

		virtual ~PAKLibrary();

		virtual bool    init    ();
		virtual bool    copyFile(Path const& filename, BufferFile* file);

		static  bool    checkSignature(ReadableFile_t* lib_file);

		static  void    openArchive(vfs::ReadableFile_t* file, vfs_pak::DB& db);
	};

} // end namespace

#endif // VFS_WITH_PAK

#endif // _VFS_SLF_LIBRARY_H_

