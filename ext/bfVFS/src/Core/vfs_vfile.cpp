/*
 * bfVFS : vfs/Core/vfs_vfile.cpp
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

#include <vfs/Core/vfs_vfile.h>
#include <vfs/Core/vfs_vloc.h>
#include <vfs/Core/vfs.h>

#ifdef VFILE_BLOCK_CREATE
// static member
vfs::ObjBlockAllocator<vfs::VirtualFile>* vfs::VirtualFile::_vfile_pool = NULL;
#endif

vfs::VirtualFile::SP vfs::VirtualFile::create(vfs::Path const& filename, vfs::ProfileStack* pstack)
{
	unsigned int ID=0;
#ifdef VFILE_BLOCK_CREATE
	if(!_vfile_pool)
	{
		_vfile_pool = new ObjBlockAllocator<vfs::VirtualFile>();
		ObjectAllocator::registerAllocator(_vfile_pool);
	}
	VirtualFile::SP file(_vfile_pool->New(&ID));
#else
	VirtualFile::SP file( VFS_NEW(VirtualFile) );
#endif
	file->_filepath = filename;
	file->_pstack   = pstack;
	file->_myID     = ID;
	return file;
}

void vfs::VirtualFile::destroy()
{
#ifndef VFILE_BLOCK_CREATE
	// delete this;
#endif
}


vfs::VirtualFile::VirtualFile()
: _filepath(L""), _top_pname(L"_INVALID_"), _pstack(vfs::ProfileStack::WP()), _myID(vfs::UInt32(-1))
{
};

vfs::VirtualFile::~VirtualFile()
{
}

vfs::Path const& vfs::VirtualFile::path()
{
	return _filepath;
}

void vfs::VirtualFile::add(vfs::IBaseFile* file, vfs::String const& profile_name, bool replace)
{
	if(file != NULL)
	{
		// if there is no file then just set it
		// if bReplace is set to true then override all other settings and just set the file
		IBaseFile::SP top_file = _top_file.lock();
		if(top_file.isNull() || replace)
		{
			_top_file  = file;
			_top_pname = profile_name;
			return;
		}

		// file already set, but new file is? the same object
		if(file == top_file.get())
		{
			VFS_THROW_IFF( StrCmp::Equal(profile_name,_top_pname), L"same file, different profile name");
		}

		// OK, not the same file, but these two different files are supposed to have the same filename
		VFS_THROW_IFF( top_file->getName() == file->getName(), L"different filenames");

		// set new file only when its profile is on top of the current file's profile
		bool found_old = false, found_new = false;

		ProfileStack::SP pstack = _pstack.safe_lock();

		ProfileStack::Iterator it = pstack->begin();
		for(; !it.end(); it.next())
		{
			if(_top_pname == it.value()->cName)
			{
				found_old = true;
				break;
			}
			else if(profile_name == it.value()->cName)
			{
				found_new = true;
				break;
			}
		}
		if(found_new && !found_old)
		{
			_top_file   = file;
			_top_pname = profile_name;
		}
	}
}

/**
 * @returns : returns true if pFile is not top file or top file could be replaced with another file
 *            returns false if there is no more files with given name. in this case object should be destroyed
 */
bool vfs::VirtualFile::remove(vfs::IBaseFile* file)
{
	IBaseFile::SP top_file = _top_file.lock();
	if(file == top_file.get() && file != NULL)
	{
		if(_filepath == file->getPath())
		{
			// need to replace '_top_file'
			ProfileStack::SP pstack = _pstack.safe_lock();

			ProfileStack::Iterator prof_it = pstack->begin();
			for(; !prof_it.end(); prof_it.next())
			{
				VirtualProfile::SP vprof( prof_it.value() );
				if(!vprof.isNull())
				{
					IBaseFile::SP file_ = vprof->getFile(_filepath);
					if(!file_.isNull() && (file != file_.get()))
					{
						_top_file  = file_;
						_top_pname = vprof->cName;
						return true;
					}
				}
			}
			// no more files
			_top_file  = NULL;
			_top_pname = "";
			return false;
		}
		else
		{
			VFS_THROW(L"Same file object but different file paths? WTH?");
		}
	}
	return true;
}

vfs::IBaseFile* vfs::VirtualFile::file(ESearchFile eSearch) const
{
	if(eSearch == SF_TOP)
	{
		return _top_file.lock();
	}
	else if(eSearch == SF_FIRST_WRITABLE)
	{
		ProfileStack::SP pstack = _pstack.safe_lock();

		VirtualProfile::SP vprof = pstack->getWriteProfile();
		if(!vprof.isNull())
		{
			return vprof->getFile(_filepath);
		}
	}
	else if(eSearch == SF_STOP_ON_WRITABLE_PROFILE)
	{
		ProfileStack::SP pstack = _pstack.safe_lock();

		ProfileStack::Iterator prof_it = pstack->begin();
		for(; !prof_it.end(); prof_it.next())
		{
			VirtualProfile::SP vprof( prof_it.value() );
			if(!vprof.isNull())
			{
				if(vprof->cWritable)
				{
					// if profile is writable then return whatever there is
					return vprof->getFile(_filepath);
				}
				else
				{
					// if profile is not writable then return only when it has the desired file
					IBaseFile::SP file = vprof->getFile(_filepath);
					if(!file.isNull())
					{
						return file;
					}
				}
			}
		}
	}
	return NULL;
}

vfs::IBaseFile* vfs::VirtualFile::file(vfs::String const& profile_name) const
{
	if(profile_name == _top_pname)
	{
		return _top_file.lock();
	}
	else
	{
		ProfileStack::SP pstack = _pstack.safe_lock();

		VirtualProfile::SP vprof = pstack->getProfile(profile_name);
		if(!vprof.isNull())
		{
			VirtualProfile::Iterator loc_it = vprof->begin();
			for(; !loc_it.end(); loc_it.next())
			{
				if(loc_it.value() && loc_it.value()->fileExists(_filepath, vprof))
				{
					if(loc_it.value())
					{
						return loc_it.value()->getFile(_filepath, vprof);
					}
				}
			}
		}
	}
	return NULL;
}

