/*
 * bfVFS : vfs/Core/vfs_profile.cpp
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

#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/Location/vfs_lib_dir.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <vfs/Tools/vfs_parser_tools.h>

#include <vfs/Aspects/vfs_logging.h>

#include <sstream>


class vfs::VirtualProfile::IterImpl : public vfs::VirtualProfile::Iterator::IImplementation
{
	friend class VirtualProfile;
	typedef      VirtualProfile::Iterator::IImplementation  BaseClass_t;

	IterImpl(VirtualProfile* profile) : BaseClass_t(), m_profile(profile)
	{
		VFS_THROW_IFF(profile, L"");
		// only unique locations
		_loc_iter = m_profile->m_setLocations.begin();
	}

public:
	VFS_SMARTPOINTER(IterImpl);

	IterImpl() : BaseClass_t()
	{};
	virtual ~IterImpl()
	{};
	//////
	virtual IBaseLocation*  value()
	{
		if(_loc_iter != m_profile->m_setLocations.end())
		{
			return *_loc_iter;
		}
		return NULL;
	}
	virtual void            next()
	{
		if(_loc_iter != m_profile->m_setLocations.end())
		{
			_loc_iter++;
		}
	}

protected:
	virtual BaseClass_t::SP clone() const
	{
		IterImpl::SP iter( VFS_NEW(IterImpl) );
		iter->m_profile = m_profile;
		iter->_loc_iter = _loc_iter;
		return iter;
	}

private:
	VirtualProfile::SP                      m_profile;
	VirtualProfile::UniqueLoc_t::iterator   _loc_iter;
};

/***************************************************************************/
/***************************************************************************/

class vfs::VirtualProfile::FileIterImpl : public vfs::VirtualProfile::FileIterator::IImplementation
{
	friend class VirtualProfile;
	typedef      VirtualProfile::FileIterator::IImplementation BaseClass_t;
	FileIterImpl(Path const& sPattern, VirtualProfile* profile);

public:
	VFS_SMARTPOINTER(FileIterImpl);

	FileIterImpl() : BaseClass_t()
	{};
	virtual ~FileIterImpl()
	{};
	/////
	virtual IBaseFile*      value()
	{
		return file;
	}
	virtual void            next();

protected:
	virtual BaseClass_t::SP clone() const
	{
		FileIterImpl::SP iter2( VFS_NEW(FileIterImpl) );
		iter2->m_pattern = m_pattern;
		iter2->m_profile = m_profile;
		iter2->iter      = iter;
		iter2->fiter     = fiter;
		iter2->file      = file;
		return iter2;
	}

private:
	Path                        m_pattern;
	VirtualProfile::SP          m_profile;
	VirtualProfile::Iterator    iter;
	IBaseLocation::Iterator     fiter;
	IBaseFile*                  file;
};

vfs::VirtualProfile::FileIterImpl::FileIterImpl(vfs::Path const& sPattern, VirtualProfile* profile)
: BaseClass_t(), m_pattern(sPattern), m_profile(profile)
{
	VFS_THROW_IFF(profile, L"");
	iter = m_profile->begin();
	while(!iter.end())
	{
		fiter = iter.value()->begin(m_profile);
		while(!fiter.end())
		{
			file = fiter.value();
			if( matchPattern(m_pattern(), file->getPath()()) )
			{
				return;
			}
			fiter.next();
		}
		iter.next();
	}
	file = NULL;
}

void vfs::VirtualProfile::FileIterImpl::next()
{
	if(!fiter.end())
	{
		fiter.next();
	}
	while(!iter.end())
	{
		while(!fiter.end())
		{
			file = fiter.value();
			if( matchPattern(m_pattern(), file->getPath()()) )
			{
				return;
			}
			fiter.next();
		}
		iter.next();
		if(!iter.end())
		{
			fiter = iter.value()->begin(m_profile);
		}
	}
	file = NULL;
}

/***************************************************************************/
/***************************************************************************/

vfs::VirtualProfile::VirtualProfile(vfs::String const& profile_name, vfs::Path profile_root, bool writable)
: cName(profile_name), cRoot(profile_root), cWritable(writable)
{
}

vfs::VirtualProfile::~VirtualProfile()
{
	m_setLocations.clear();
	m_mapLocations.clear();
};

vfs::VirtualProfile::Iterator vfs::VirtualProfile::begin()
{
	return Iterator    ( VFS_NEW1(IterImpl, this) );
}

vfs::VirtualProfile::FileIterator vfs::VirtualProfile::files(vfs::Path const& sPattern)
{
	return FileIterator( VFS_NEW2(FileIterImpl, sPattern, this) );
}

void vfs::VirtualProfile::addLocation(vfs::IBaseLocation* location)
{
	if(location != NULL)
	{
		m_setLocations.insert(location);
		std::list<Path> sub_dirs;
		location->getSubDirList(sub_dirs, this);

		std::list<Path>::const_iterator cit = sub_dirs.begin();
		for(;cit != sub_dirs.end(); ++cit)
		{
			IBaseLocation::SP old_loc = m_mapLocations[*cit].lock();
			if(old_loc.isNull())
			{
				m_mapLocations[*cit] = location;
			}
			else if(old_loc == location)
			{
				// seems to be an update. do nothing
			}
			else
			{
				VFS_LOG_WARNING(_BS(L"Another location is already mapped to '") << ((*cit)()) << L"' [keeping old location]" << _BS::wget);
				//VFS_THROW(L"Location already taken");
			}
		}
	}
}

vfs::IBaseLocation* vfs::VirtualProfile::getLocation(vfs::Path const& loc_path) const
{
	Locations_t::const_iterator it = m_mapLocations.find(loc_path);
	if(it != m_mapLocations.end())
	{
		return it->second.lock();
	}
	return NULL;
}

vfs::IBaseFile* vfs::VirtualProfile::getFile(vfs::Path const& file_path) const
{
	Path dir, file;
	file_path.splitLast(dir,file);

	Locations_t::const_iterator it = m_mapLocations.find(dir);
	if(it != m_mapLocations.end())
	{
		IBaseLocation::SP loc = it->second.lock();
		if(!loc.isNull())
		{
			return loc->getFile(file_path, this);
		}
	}
	return NULL;
}


/***************************************************************************/
/***************************************************************************/

class vfs::ProfileStack::IterImpl : public vfs::ProfileStack::Iterator::IImplementation
{
	friend class ProfileStack;
	typedef      ProfileStack::Iterator::IImplementation BaseClass_t;

	IterImpl(ProfileStack* pPStack) : BaseClass_t(), m_PStack(pPStack)
	{
		VFS_THROW_IFF(m_PStack, L"");
		_prof_iter = m_PStack->m_profiles.begin();
	}

public:
	VFS_SMARTPOINTER(IterImpl);

	IterImpl() : BaseClass_t(), m_PStack(NULL)
	{};
	virtual ~IterImpl()
	{};

	//////
	virtual VirtualProfile*     value()
	{
		if(_prof_iter != m_PStack->m_profiles.end())
		{
			return *_prof_iter;
		}
		return NULL;
	}

	virtual void                next()
	{
		if(_prof_iter != m_PStack->m_profiles.end())
		{
			_prof_iter++;
		}
	}

protected:
	virtual BaseClass_t::SP     clone() const
	{
		IterImpl::SP iter = VFS_NEW(IterImpl);
		iter->m_PStack    = m_PStack;
		iter->_prof_iter  = _prof_iter;
		return iter;
	}

private:
	ProfileStack::SP        m_PStack;
	profiles_t::iterator    _prof_iter;
};

/***************************************************************************/
/***************************************************************************/
vfs::ProfileStack::ProfileStack()
{
}

vfs::ProfileStack::~ProfileStack()
{
	profiles_t::iterator it = m_profiles.begin();
	for(; it != m_profiles.end(); ++it)
	{
		VFS_LOG_DEBUG(_BS(L"    delete profile : ") << (*it)->cName << _BS::wget);
		it->null();
	}
	m_profiles.clear();
}

vfs::VirtualProfile* vfs::ProfileStack::getProfile(vfs::String const& profile_name) const
{
	profiles_t::const_iterator it = m_profiles.begin();
	for(;it != m_profiles.end(); ++it)
	{
		if( StrCmp::EqualCase((*it)->cName, profile_name) )
		{
			return *it;
		}
	}
	return NULL;
}

vfs::VirtualProfile* vfs::ProfileStack::getWriteProfile()
{
	profiles_t::const_iterator it = m_profiles.begin();
	for(;it != m_profiles.end(); ++it)
	{
		if((*it)->cWritable)
		{
			return *it;
		}
	}
	return NULL;
}

vfs::VirtualProfile* vfs::ProfileStack::topProfile() const
{
	if(!m_profiles.empty())
	{
		return m_profiles.front();
	}
	return NULL;
}

bool vfs::ProfileStack::popProfile()
{
	// there might be some files in this profile that are referenced in a Log object
	// we need to it to release the file
	Log::flushReleaseAll();
	// an observer pattern would probably be the better solution,
	// but for now lets do it this way

	VirtualProfile::SP prof = this->topProfile();
	if(prof)
	{
		VirtualProfile::Iterator loc_it  = prof->begin();
		for(; !loc_it.end(); loc_it.next())
		{
			IBaseLocation::SP loc( loc_it.value() );
			IBaseLocation::Iterator f_it = loc->begin(prof);
			for(; !f_it.end(); f_it.next())
			{
				IBaseFile* file = f_it.value();
				Path dir, file_name;
				if(file)
				{
					file->getPath().splitLast(dir,file_name);
					VirtualLocation::SP vloc = getVFS()->getVirtualLocation(dir);
					if(vloc)
					{
						if( !vloc->removeFile(file) )
						{
							VFS_THROW(_BS(L"Could not remove file [") << file->getPath() << L"] in Profile [" << prof->cName << L"]" << _BS::wget);
						}
					}
					else
					{
						VFS_THROW(_BS(L"Virtual location [") << dir << L"] doesn't exist. Maybe the VFS was not properly setup." << _BS::wget);
					}
				}
				else
				{
					VFS_THROW(_BS(L"File is NULL during iteration over files in location [") << loc->getPath() << L"]" << _BS::wget);
				}
			}
		}
		// delete only when nothing went wrong
		this->m_profiles.pop_front();
		prof.null();
	}
	return true;
}

void vfs::ProfileStack::pushProfile(VirtualProfile* profile)
{
	if(!profile)
	{
		return;
	}

	if(!getProfile(profile->cName))
	{
		if(profile->cWritable)
		{
			m_profiles.push_front(profile);
		}
		else
		{
			profiles_t::iterator pit = m_profiles.begin();
			while(pit != m_profiles.end() && (*pit)->cWritable)
			{
				pit++;
			}
			//if(pit != m_profiles.end())
			{
				m_profiles.insert(pit, profile);
			}
			//else
			//{
			//	m_profiles.push_front(pProfile);
			//}
		}
		return;
	}
	VFS_THROW(L"A profile with this name already exists");
}

vfs::ProfileStack::Iterator vfs::ProfileStack::begin()
{
	return Iterator( VFS_NEW1(IterImpl, this) );
}

