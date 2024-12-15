/*
 * bfVFS : vfs/Core/Location/vfs_lib_dir.h
 *  - class for readonly (sub)directories in archives/libraries
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

#ifndef _VFS_LIB_DIR_H_
#define _VFS_LIB_DIR_H_

#include <vfs/Core/Interface/vfs_directory_interface.h>

namespace vfs
{

	class LibDirectory : public TDirectory<IWriteType>
	{
		class IterImpl;

		typedef TDirectory<IWriteType>                      BaseClass_t;
		typedef std::map<Path, FileType_t::SP, Path::Less>  FileCatalogue_t;

	public:
		VFS_SMARTPOINTER(LibDirectory);

		typedef BaseClass_t::FileType_t                 FileType_t;

	public:
		virtual ~LibDirectory();

		/**
		 *  TDirectory interface
		 */
		virtual FileType_t*     addFile                (Path const& filename, bool deleteOldFile=false);
		virtual bool            addFile                (FileType_t* file,     bool deleteOldFile=false);
		virtual bool            deleteFileFromDirectory(Path const& filename);
		virtual bool            createSubDirectory     (Path const& sub_dir_path);
		virtual bool            deleteDirectory        (Path const& dir_path);

		/**
		 *  TLocation interface
		 */
		virtual bool            fileExists             (Path const& filename, const VirtualProfile*);
		virtual IBaseFile*      getFile                (Path const& filename, const VirtualProfile*);
		virtual FileType_t*     getFileTyped           (Path const& filename, const VirtualProfile*);
		virtual void            getSubDirList          (std::list<Path>& sub_dirs, const VirtualProfile*);

		virtual Iterator        begin                  (const VirtualProfile* profile);

	protected:
		LibDirectory(Path const& local_path, Path const& real_path);

		FileCatalogue_t         m_files;
	};

} // -end- namespace

#endif // _VFS_LIB_DIR_H_
