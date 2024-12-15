/*
 * bfVFS : vfs/Core/vfs_profile.h
 *  - Virtual Profile, container for real file system locations or archives
 *  - Profile Stack, orders profiles in a linear fashion (top-bottom)
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

#ifndef _VFS_PROFILE_H_
#define _VFS_PROFILE_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/Interface/vfs_location_interface.h>
#include <map>
#include <set>

namespace vfs
{
	class VFS_API VirtualProfile : public vfs::ObjectBase
	{
		typedef std::map<Path, IBaseLocation::WP, Path::Less> Locations_t;

		// needed for iteration over locations in a profile as 'Locations_t' can reference a location multiple times with different paths
		typedef std::set<IBaseLocation::SP>                   UniqueLoc_t;

		class IterImpl;
		class FileIterImpl;

	protected:
		VirtualProfile(String const& profile_name, Path profile_root, bool writable = false);

	public:
		VFS_SMARTPOINTER(VirtualProfile);

		typedef TIterator<IBaseLocation>    Iterator;
		typedef TIterator<IBaseFile>        FileIterator;

		~VirtualProfile();

		const String        cName;
		const Path          cRoot;
		const bool          cWritable;

		Iterator            begin();
		FileIterator        files(Path const& pattern);

		void                addLocation(IBaseLocation* location );
		IBaseLocation*      getLocation(Path const&    loc_path ) const;
		IBaseFile*          getFile    (Path const&    file_path) const;

	private:
		void                operator=(VirtualProfile const& vprof);
		Locations_t         m_mapLocations;
		UniqueLoc_t         m_setLocations;
	};

	class VFS_API ProfileStack : public vfs::ObjectBase
	{
		class IterImpl;

	protected:
		ProfileStack();

	public:
		VFS_SMARTPOINTER(ProfileStack);

		typedef TIterator<VirtualProfile> Iterator;

		~ProfileStack();

		VirtualProfile*     getWriteProfile();
		VirtualProfile*     getProfile(String const& profile_name) const;

		VirtualProfile*     topProfile() const;

		/**
		 *  All files from the top profile will be removed from the VFS and the profile object will be deleted.
		 */
		bool                popProfile();
		void                pushProfile(VirtualProfile* pProfile);

		Iterator            begin();

	private:
		typedef std::list<VirtualProfile::SP> profiles_t;
		profiles_t          m_profiles;
	};

} // end namespace

#endif
