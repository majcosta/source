/*
 * bfVFS : vfs/Ext/7z/vfs_create_pak_library.h
 *  - writes uncompressed PAK archive files
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

#ifndef _VFS_CREATE_PAK_LIBRARY_H_
#define _VFS_CREATE_PAK_LIBRARY_H_

#ifdef VFS_WITH_PAK

#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_smartpointer.h>
#include <vfs/Core/Interface/vfs_create_library.h>
#include <vfs/Core/Interface/vfs_file_interface.h>

#include <map>
#include <list>
#include <set>

namespace vfs
{
	class VFS_API CreatePAKLibrary : public ICreateLibrary
	{
	protected:
		CreatePAKLibrary();

	public:
		VFS_SMARTPOINTER(CreatePAKLibrary);

		virtual ~CreatePAKLibrary();

		virtual bool    addFile             (ReadableFile_t* file);

		virtual bool    writeLibrary        (Path const&     lib_name);
		virtual bool    writeLibrary        (WritableFile_t* file);

	protected:
		WritableFile_t*     m_pLibFile;

		struct DirInfo
		{
			struct Dir
			{
				std::string dirname;
			} _dir;
			struct FileInfo
			{
				vfs::ReadableFile_t::SP file;
				vfs::UInt64 size;
				vfs::UInt64 offset;
				std::string filename;
			};
			typedef std::list<FileInfo> Files_t;
			Files_t _files;
			std::set<vfs::ReadableFile_t*> _unique_files;
		};

		typedef std::map<vfs::Path, DirInfo, vfs::Path::Less> DirectoryFilesMap_t;
		DirectoryFilesMap_t m_DFM;
	};
} // end namespace

#endif // VFS_WITH_PAK

#endif // _VFS_CREATE_7Z_LIBRARY_H_

