/*
 * bfVFS : vfs/Ext/pak/vfs_pak_header.h
 *  - types that describe the PAK file structure
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

#ifndef _VFS_PAK_HEADER_H_
#define _VFS_PAK_HEADER_H_

#include <vfs/Core/vfs_types.h>

#include <vector>
#include <cstring>

namespace pak
{

	extern vfs::UByte SIGNATURE [4]; // = {0x50, 0x4B, 0x4C, 0x45}; // = P K L E
	extern vfs::UByte SIGNATUREv[4]; // = {0x01, 0x00, 0x00, 0x00}; // = 01, maybe version number?

	struct HEADER
	{
		vfs::Byte    signature[8];
		vfs::UInt64  header_size;
		vfs::UInt64  num_dirs;
	};

	struct DIRECTORY
	{
		vfs::UInt32  id;
		vfs::UInt32  dirname_bytes; // with trailing 0
		vfs::UInt64  num_files;
		// char[]    dirname;
	};

	struct FILE_PROPS
	{
		vfs::UInt64  size;
		vfs::UInt64  offset;
		vfs::UInt64  checksum;
	};

	struct FILE
	{
		// stupid alignment
		vfs::Byte    data[28];          // 4 bytes + 3 * 8 bytes (= FILE_PROPS)
		vfs::UInt32  filename_bytes() { return *(vfs::UInt32*)(data+0); }
		FILE_PROPS*  props()          { return  (FILE_PROPS* )(data+4); }
	};

}; // end namespace pak

namespace vfs_pak
{
	struct FILE
	{
		FILE()
		: file_props(NULL), filename(NULL)
		{}

		pak::FILE_PROPS* file_props;
		char*            filename;
	};

	struct DIR
	{
		DIR()
		: dir(NULL), dirname(NULL)
		{}

		pak::DIRECTORY *dir;
		char           *dirname;
		std::vector<vfs_pak::FILE> files;
	};

	class DB
	{
	public:
		DB()
		: header_data(NULL), header(NULL)
		{};

		~DB()
		{
			deallocate_header();
		}

		void allocate_header(vfs::size_t SIZE)
		{
			deallocate_header();
			header_data = new vfs::Byte[SIZE];
		}
		void deallocate_header()
		{
			if(header_data != NULL) delete[] header_data;
		}

	public:
		vfs::Byte         *header_data;
		pak::HEADER       *header;
		std::vector<DIR>   dirs;
	};
};

#endif // _VFS_PAK_HEADER_H_
