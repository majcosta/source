/*
 * bfVFS : vfs/Ext/7z/vfs_create_7z_library.h
 *  - writes uncompressed 7z archive file
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

#ifndef _VFS_CREATE_7Z_LIBRARY_H_
#define _VFS_CREATE_7Z_LIBRARY_H_

#ifdef VFS_WITH_7ZIP

#include <vfs/Core/Interface/vfs_create_library.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <sstream>
#include <map>
#include <list>

namespace vfs
{
	class VFS_API Create7zLibrary : public ICreateLibrary
	{
		Create7zLibrary();

	public:
		VFS_SMARTPOINTER(Create7zLibrary);

		virtual ~Create7zLibrary();

		virtual bool    addFile             (ReadableFile_t* file);

		virtual bool    writeLibrary        (Path const&     lib_name);
		virtual bool    writeLibrary        (WritableFile_t* file);

	protected:
		bool            writeSignatureHeader(std::ostream&   out);
		bool            writeNextHeader     (std::ostream&   out);
		bool            writeMainStreamsInfo(std::ostream&   out);
		bool            writePackInfo       (std::ostream&   out);
		bool            writeUnPackInfo     (std::ostream&   out);
		bool            writeSubStreamsInfo (std::ostream&   out);
		bool            writeFolder         (std::ostream&   out);
		bool            writeFilesInfo      (std::ostream&   out);

	private:
		vfs::size_t 	writeFileName       (std::ostream& out, vfs::String const& filename);

	protected:
		WritableFile_t*     m_pLibFile;

		struct SFileInfo
		{
			SFileInfo()
				: name(L""), CRC(0), offset(0), size(0), time_creation(0), time_last_access(0), time_write(0)
			{};
			//////
			String::str_t   name;
			vfs::UInt32     CRC;
			vfs::UInt64     offset;
			vfs::UInt64     size;
			vfs::UInt64     time_creation,time_last_access,time_write;
		};
		typedef std::map<String::str_t,SFileInfo>   DirInfo_t;
		typedef std::list<vfs::ReadableFile_t::SP>  FileList_t;

		std::list<SFileInfo>    m_file_info;
		DirInfo_t               m_dir_info;

		std::stringstream       m_info_stream;

		// store files as smartpointers to not write them into large buffer until the header size is determined
		FileList_t              m_file_list;
		vfs::UInt64             m_total_file_size;
	};
} // end namespace

#endif // VFS_WITH_7ZIP

#endif // _VFS_CREATE_7Z_LIBRARY_H_

