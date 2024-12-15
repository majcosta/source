/*
 * bfVFS : vfs/Core/Location/vfs_directory_tree.h
 *  - class for directories in a File System, implements Directory interface
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

#ifndef _VFS_DIRECTORY_H_
#define _VFS_DIRECTORY_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/Interface/vfs_directory_interface.h>

#include <map>
#include <vector>

namespace vfs
{

	/**
	 *  TDirectory<read,write>
	 */
	template<typename WriteType>
	class VFS_API TDirectoryTree : public TDirectory<WriteType>
	{
		typedef std::map<Path, typename TDirectory<typename TDirectoryTree<WriteType>::WriteType_t>::SP, Path::Less> DirCatalogue_t;

		class IterImpl;

	public:
		VFS_SMARTPOINTER(TDirectoryTree)

		typedef TDirectory<WriteType>               BaseClass_t;
		typedef typename BaseClass_t::WriteType_t   WriteType_t;
		typedef typename BaseClass_t::FileType_t    FileType_t;

		typedef TIterator<IBaseFile>                Iterator;

		virtual ~TDirectoryTree();

		bool                    init                   ();

		/**
		 *  TDirectory interface
		 */
		virtual FileType_t*     addFile                (Path const& filename, bool deleteOldFile=false);
		virtual bool            addFile                (FileType_t* file,     bool deleteOldFile=false);

		virtual bool            createSubDirectory     (Path const& sub_dir_path);
		virtual bool            deleteDirectory        (Path const& dir_path);
		virtual bool            deleteFileFromDirectory(Path const& filename);

		/**
		 *  TLocation interface
		 */
		virtual bool            fileExists             (Path const& filename,      const VirtualProfile* profile);
		virtual IBaseFile*      getFile                (Path const& filename,      const VirtualProfile* profile);
		virtual FileType_t*     getFileTyped           (Path const& filename,      const VirtualProfile* profile);
		virtual void            getSubDirList          (std::list<Path>& sub_dirs, const VirtualProfile* profile);

		virtual Iterator        begin                  (const VirtualProfile* profile);

	protected:
		TDirectoryTree(Path const& mountpoint, Path const& real_path);

		DirCatalogue_t          m_Dirs;
	};

	typedef TDirectoryTree<IWritable>       DirectoryTree;
	typedef TDirectoryTree<IWriteType>      ReadOnlyDirectoryTree;

} // end namespace

#endif // _VFS_DIRECTORY_H_
