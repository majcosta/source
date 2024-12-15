/*
 * bfVFS : vfs/Core/vfs_vloc.h
 *  - Virtual Location, stores Virtual Files
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

#ifndef _VFS_VLOC_H_
#define _VFS_VLOC_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/vfs_vfile.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/Interface/vfs_iterator_interface.h>

#include <map>

namespace vfs
{
	class VirtualFile;

	class VFS_API VirtualLocation : public vfs::ObjectBase
	{
		class VFileIterator;
		typedef std::map<Path, VirtualFile::SP, Path::Less> VFiles_t;

	protected:
		VirtualLocation(Path const& sPath);

	public:
		VFS_SMARTPOINTER(VirtualLocation);

		typedef vfs::TIterator<VirtualFile> Iterator;

		~VirtualLocation();

		const Path      cPath;

		void            setIsExclusive(bool exclusive);
		bool            getIsExclusive();

		void            addFile       (IBaseFile*  file,     String const& profileName);
		IBaseFile*      getFile       (Path const& filename, String const& profileName = "") const;
		VirtualFile*    getVirtualFile(Path const& filename);

		bool            removeFile    (IBaseFile*  file);

		Iterator        iterate       ();

	private:
		void            operator=     (VirtualLocation const& vloc);

		bool            m_exclusive;
		VFiles_t        m_VFiles;
	};
} // end namespace

#endif // _VFS_VLOC_H_
