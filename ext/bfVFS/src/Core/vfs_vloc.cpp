/*
 * bfVFS : vfs/Core/vfs_vloc.cpp
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

#include <vfs/Core/vfs_vloc.h>
#include <vfs/Core/vfs_vfile.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/vfs.h>

/************************************************************************/

class vfs::VirtualLocation::VFileIterator : public vfs::VirtualLocation::Iterator::IImplementation
{
	friend class VirtualLocation;
	typedef      VirtualLocation::Iterator::IImplementation BaseClass_t;

	VFileIterator(VirtualLocation* pLoc) : BaseClass_t(), _vloc(pLoc)
	{
		VFS_THROW_IFF(pLoc, L"");
		_vfile_iter = _vloc->m_VFiles.begin();
	}

public:
	VFS_SMARTPOINTER(VFileIterator);

	VFileIterator() : BaseClass_t(), _vloc(NULL)
	{};
	virtual ~VFileIterator()
	{};

	virtual VirtualFile*    value()
	{
		if(_vloc && _vfile_iter != _vloc->m_VFiles.end())
		{
			return _vfile_iter->second;
		}
		return NULL;
	}

	virtual void            next()
	{
		if(_vloc && _vfile_iter != _vloc->m_VFiles.end())
		{
			_vfile_iter++;
		}
	}

protected:
	virtual BaseClass_t::SP clone() const
	{
		VFileIterator::SP iter( VFS_NEW1(VFileIterator, _vloc) );
		iter->_vfile_iter = _vfile_iter;
		return iter;
	}

private:
	VirtualLocation::SP                 _vloc;
	VirtualLocation::VFiles_t::iterator _vfile_iter;
};

/************************************************************************/

vfs::VirtualLocation::VirtualLocation(vfs::Path const& path)
: cPath(path), m_exclusive(false)
{};

vfs::VirtualLocation::~VirtualLocation()
{
	VFiles_t::iterator it = m_VFiles.begin();
	for(; it != m_VFiles.end(); ++it)
	{
		it->second.null();
	}
	m_VFiles.clear();
}

void vfs::VirtualLocation::setIsExclusive(bool exclusive)
{
	m_exclusive = exclusive;
}
bool vfs::VirtualLocation::getIsExclusive()
{
	return m_exclusive;
}

void vfs::VirtualLocation::addFile(vfs::IBaseFile* file, vfs::String const& profileName)
{
	VFS_THROW_IFF(file != NULL, L"");
	VFiles_t::iterator it = m_VFiles.find(file->getName());
	if(it == m_VFiles.end())
	{
		Path             path  = file->getPath();
		ProfileStack::SP pstack( getVFS()->getProfileStack() );
		VirtualFile::SP  vfile ( vfs::VirtualFile::create(path,pstack) );
		it                     = m_VFiles.insert(m_VFiles.end(), std::make_pair(file->getName(), vfile));
	}
	it->second->add(file,profileName,true);
}

vfs::IBaseFile* vfs::VirtualLocation::getFile(vfs::Path const& filename, vfs::String const& profileName) const
{
	VFiles_t::const_iterator cit = m_VFiles.find(filename);
	if(cit != m_VFiles.end() && cit->second)
	{
		if(profileName.empty())
		{
			if(m_exclusive)
			{
				return cit->second->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
			}
			else
			{
				return cit->second->file(VirtualFile::SF_TOP);
			}
		}
		else
		{
			// you know what you are doing
			return cit->second->file(profileName);
		}
	}
	return NULL;
}
vfs::VirtualFile* vfs::VirtualLocation::getVirtualFile(vfs::Path const& filename)
{
	VFiles_t::const_iterator cit = m_VFiles.find(filename);
	if(cit != m_VFiles.end())
	{
		return cit->second;
	}
	return NULL;
}

bool vfs::VirtualLocation::removeFile(vfs::IBaseFile* file)
{
	if(file != NULL)
	{
		Path filename;
		file->getName(filename);

		VFiles_t::iterator it = m_VFiles.find(filename);
		if(it != m_VFiles.end())
		{
			if(!it->second->remove(file))
			{
				m_VFiles.erase(it);
			}
			return true;
		}
	}
	return false;
}


vfs::VirtualLocation::Iterator vfs::VirtualLocation::iterate()
{
	return Iterator( VFS_NEW1(VFileIterator, this) );
}

