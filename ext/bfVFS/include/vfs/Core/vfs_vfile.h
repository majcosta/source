/*
 * bfVFS : vfs/Core/vfs_vfile.h
 *  - Virtual File, handles access to files from different VFS profiles
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

#ifndef _VFS_VFILE_H_
#define _VFS_VFILE_H_

#include <vfs/Core/vfs_profile.h>
#include <vfs/Tools/vfs_allocator.h>
#include <vector>

//#define VFILE_BLOCK_CREATE

namespace vfs
{
	class VFS_API VirtualFile : public vfs::ObjectBase
	{
	public:
		enum ESearchFile
		{
			SF_TOP,
			SF_FIRST_WRITABLE,
			SF_STOP_ON_WRITABLE_PROFILE,
		};

	protected:
		VirtualFile();

	public:
		VFS_SMARTPOINTER(VirtualFile);

		~VirtualFile();
		void destroy();
		static VirtualFile::SP  create(Path const& filename, ProfileStack* pstack);

		vfs::Path const&        path  ();
		void                    add   (IBaseFile* file, String const& profile_name, bool replace = false);
		bool                    remove(IBaseFile* file);

		//////////////////////////////////////////////////
		IBaseFile*              file  (ESearchFile   eSearch) const;
		IBaseFile*              file  (String const& profile_name) const;
		//////////////////////////////////////////////////

	private:
		Path                    _filepath;
		String                  _top_pname;
		IBaseFile::WP           _top_file;
		ProfileStack::WP        _pstack;

	private:
		vfs::UInt32             _myID;
#ifdef VFILE_BLOCK_CREATE
		static ObjBlockAllocator<VirtualFile>* _vfile_pool;
#endif
	};
} // end namspace

#endif // _VFS_VFILE_H_
