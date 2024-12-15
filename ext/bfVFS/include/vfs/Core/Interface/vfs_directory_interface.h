/*
 * bfVFS : vfs/Core/Interface/vfs_directory_interface.h
 *  - partially implements Location interface for file system directories
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

#ifndef _VFS_DIRECTORY_INTERFACE_H_
#define _VFS_DIRECTORY_INTERFACE_H_

#include <vfs/Core/Interface/vfs_location_interface.h>

namespace vfs
{
	template<class WriteType>
	class TDirectory : public TLocationTemplate<IReadable, WriteType>
	{
	public:
		VFS_SMARTPOINTER(TDirectory);

		typedef          TLocationTemplate<IReadable, WriteType> BaseClass_t;
		typedef typename BaseClass_t::FileType_t                 FileType_t;
		typedef typename BaseClass_t::WriteType_t                WriteType_t;

		virtual ~TDirectory()
		{};

		Path const&	        getRealPath            ()
		{
			return m_realPath;
		}

		virtual FileType_t* addFile                (Path const& filename, bool deleteOldFile=false) = 0;
		virtual bool        addFile                (FileType_t* file,     bool deleteOldFile=false) = 0;

		virtual bool        createSubDirectory     (Path const& sub_dir_path) = 0;
		virtual bool        deleteDirectory        (Path const& dir_path)     = 0;
		virtual bool        deleteFileFromDirectory(Path const& filename)     = 0;

	protected:
		TDirectory(Path const& mountpoint, Path const& real_path)
			: BaseClass_t(mountpoint), m_realPath(real_path)
		{};

		const Path          m_realPath;

	private:
		void                operator=              (TDirectory<WriteType> const& t);
	};
}

#endif // _VFS_DIRECTORY_INTERFACE_H_
