/*
 * bfVFS : vfs/Core/vfs.h
 *  - primary interface for the using program, get files from the VFS internal storage
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

#ifndef _VFS_H_
#define _VFS_H_

#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/Interface/vfs_location_interface.h>
#include <vfs/Core/Interface/vfs_iterator_interface.h>
#include <vfs/Core/vfs_vloc.h>
#include <vfs/Core/vfs_vfile.h>

#include <map>

namespace vfs
{
	class ProfileStack;

	class VFS_API VirtualFileSystem
	{
		typedef std::map<Path,VirtualLocation::SP,Path::Less> VFS_t;

		class RegularIterator;
		class MatchingIterator;

	public:
		typedef TIterator<vfs::ReadableFile_t> Iterator;

		~VirtualFileSystem();

		static VirtualFileSystem*   getVFS               ();
		static void                 shutdownVFS          ();

		ProfileStack*               getProfileStack      ();

		VirtualLocation*            getVirtualLocation   (Path const& loc_path, bool bCreate = false);
		bool                        addLocation          (IBaseLocation* location,   const VirtualProfile* profile);

		bool                        fileExists           (Path const& file_path, VirtualFile::ESearchFile eSF = VirtualFile::SF_TOP );
		bool                        fileExists           (Path const& file_path, String const& profile_name);

		IBaseFile*                  getFile              (Path const& file_path, VirtualFile::ESearchFile eSF = VirtualFile::SF_TOP );
		IBaseFile*                  getFile              (Path const& file_path, String const& profile_name);

		ReadableFile_t*             getReadFile          (Path const& file_path, VirtualFile::ESearchFile eSF = VirtualFile::SF_TOP );
		ReadableFile_t*             getReadFile          (Path const& file_path, String const& profile_name);

		WritableFile_t*             getWriteFile         (Path const& file_path, VirtualFile::ESearchFile eSF = VirtualFile::SF_TOP );
		WritableFile_t*             getWriteFile         (Path const& file_path, String const& profile_name);

		bool                        removeFileFromFS     (Path const& file_path);
		bool                        removeDirectoryFromFS(Path const& dir_path);
		bool                        createNewFile        (Path const& filename);

		Iterator                    begin                ();
		Iterator                    begin                (Path const& pattern);

		void                        print(std::wostream& out);

	private:
		ProfileStack::SP            m_ProfileStack;
		VFS_t                       m_mapFS;

	private:
		VirtualFileSystem();
		static VirtualFileSystem*   m_pSingleton;
	};

	VFS_API bool canWrite();
} // end namespace

VFS_API vfs::VirtualFileSystem*  getVFS();


#endif // _VFS_H_
