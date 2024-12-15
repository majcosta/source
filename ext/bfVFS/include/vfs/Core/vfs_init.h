/*
 * bfVFS : vfs/Core/vfs_init.h
 *  - initialization functions/classes
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

#ifndef _VFS_INIT_H_
#define _VFS_INIT_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/vfs_smartpointer.h>
#include <vfs/Tools/vfs_property_container.h>

namespace vfs_init
{
	class VFS_API Location : public vfs::ObjectBase
	{
	protected:
		Location();

	public:
		VFS_SMARTPOINTER(Location);

		virtual ~Location();

		bool            m_optional;
		vfs::String     m_type;
		vfs::Path       m_path, m_vfs_path;
		vfs::Path       m_mount_point;
	};

	class VFS_API Profile : public vfs::ObjectBase
	{
	protected:
		Profile();

	public:
		VFS_SMARTPOINTER(Profile);

		typedef std::list<Location::SP> locations_t;

		virtual ~Profile();

		void addLocation(Location::SP loc);

		////////////////////////////////////////////////
		locations_t     locations;

		vfs::String     m_name;
		vfs::Path       m_root;
		bool            m_writable;
	};

	class VFS_API VfsConfig : public vfs::ObjectBase
	{
	protected:
		VfsConfig() {};

	public:
		VFS_SMARTPOINTER(VfsConfig);

		typedef std::list<Profile::SP> profiles_t;

		virtual ~VfsConfig();

		profiles_t      profiles;

		void            addProfile  (Profile::SP   prof);
		void            appendConfig(VfsConfig::SP conf);
	};

	////////////////////////////////////////////////////////////////////////////

	VFS_API bool    initWriteProfile     (vfs::VirtualProfile::SP rProf);

	VFS_API bool    initVirtualFileSystem(vfs::Path               const& vfs_ini);
	VFS_API bool    initVirtualFileSystem(std::list<vfs::Path>    const& vfs_ini_list);
	VFS_API bool    initVirtualFileSystem(vfs::PropertyContainer       & props);
	VFS_API bool    initVirtualFileSystem(vfs_init::VfsConfig::SP const& conf);
	VFS_API bool    initVirtualFileSystem(vfs::Path const& libname, vfs_init::VfsConfig::SP const& conf);
};

#endif // _VFS_INIT_H_
