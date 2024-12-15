/*
 * bfVFS : vfs/Core/vfs.cpp
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

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/vfs.h>

#include <vfs/Aspects/vfs_logging.h>
#include <vfs/Core/Interface/vfs_directory_interface.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/File/vfs_dir_file.h>
#include <vfs/Core/File/vfs_lib_file.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs_vfile.h>

#include <vfs/Tools/vfs_property_container.h>
#include <vfs/Tools/vfs_parser_tools.h>

#include <stack>

template class vfs::TIterator<vfs::ReadableFile_t>; // explicit instantiation

/********************************************************************/
/********************************************************************/

class vfs::VirtualFileSystem::RegularIterator : public vfs::VirtualFileSystem::Iterator::IImplementation
{
	friend class VirtualFileSystem;
	typedef      VirtualFileSystem::Iterator::IImplementation BaseClass_t;

	RegularIterator(VirtualFileSystem* VFS);

public:
	VFS_SMARTPOINTER(RegularIterator);

	RegularIterator() : BaseClass_t(), m_VFS(NULL)
	{};
	virtual ~RegularIterator()
	{};

	virtual ReadableFile_t* value();
	virtual void            next();

protected:
	virtual BaseClass_t::SP    clone() const
	{
		RegularIterator::SP iter( VFS_NEW(RegularIterator) );
		iter->m_VFS       = m_VFS;
		iter->_vloc_iter  = _vloc_iter;
		iter->_vfile_iter = _vfile_iter;
		return iter;
	}

private:
	VirtualFileSystem*                  m_VFS;
	VirtualFileSystem::VFS_t::iterator  _vloc_iter;
	VirtualLocation::Iterator           _vfile_iter;
};

vfs::VirtualFileSystem::RegularIterator::RegularIterator(vfs::VirtualFileSystem* VFS)
: BaseClass_t(), m_VFS(VFS)
{
	_vloc_iter = m_VFS->m_mapFS.begin();
	if(_vloc_iter != m_VFS->m_mapFS.end())
	{
		_vfile_iter = _vloc_iter->second->iterate();
	}
}

vfs::ReadableFile_t* vfs::VirtualFileSystem::RegularIterator::value()
{
	bool exclusive_vloc = false;
	if(_vloc_iter != m_VFS->m_mapFS.end())
	{
		exclusive_vloc = _vloc_iter->second->getIsExclusive();
	}
	if(!_vfile_iter.end())
	{
		VirtualFile* vfile = _vfile_iter.value();
		if(vfile)
		{
			if(exclusive_vloc)
			{
				return ReadableFile_t::cast(vfile->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE));
			}
			else
			{
				return ReadableFile_t::cast(vfile->file(VirtualFile::SF_TOP));
			}
		}
	}
	return NULL;
}

void vfs::VirtualFileSystem::RegularIterator::next()
{
	if(!_vfile_iter.end())
	{
		_vfile_iter.next();
	}
	while(_vfile_iter.end())
	{
		if(_vloc_iter != m_VFS->m_mapFS.end())
		{
			_vloc_iter++;
			if(_vloc_iter != m_VFS->m_mapFS.end())
			{
				_vfile_iter = _vloc_iter->second->iterate();
			}
		}
		else
		{
			return;
		}
	}
}

/********************************************************************/
/********************************************************************/

class vfs::VirtualFileSystem::MatchingIterator : public vfs::VirtualFileSystem::Iterator::IImplementation
{
	friend class VirtualFileSystem;
	typedef      VirtualFileSystem::Iterator::IImplementation BaseClass_t;

	MatchingIterator(Path const& pattern, VirtualFileSystem* VFS);

public:
	VFS_SMARTPOINTER(MatchingIterator);

	MatchingIterator() : BaseClass_t(), m_VFS(NULL)
	{};
	virtual ~MatchingIterator()
	{};

	virtual ReadableFile_t* value();
	virtual void            next ();

protected:
	virtual BaseClass_t::SP  clone() const
	{
		MatchingIterator::SP iter( VFS_NEW(MatchingIterator) );
		iter->m_LocPattern   = m_LocPattern;
		iter->m_FilePattern  = m_FilePattern;
		iter->m_VFS          = m_VFS;
		iter->_vloc_iter     = _vloc_iter;
		iter->_vfile_iter    = _vfile_iter;
		return iter;
	}

private:
	bool                    nextLocationMatch();
	bool                    nextFileMatch();

private:
	Path                                m_LocPattern, m_FilePattern;
	VirtualFileSystem*                  m_VFS;
	VirtualFileSystem::VFS_t::iterator  _vloc_iter;
	VirtualLocation::Iterator           _vfile_iter;
};

vfs::VirtualFileSystem::MatchingIterator::MatchingIterator(vfs::Path const& pattern, vfs::VirtualFileSystem* VFS)
: BaseClass_t(), m_VFS(VFS)
{
	if(pattern() == vfs::Const::STAR())
	{
		m_LocPattern  = Path(vfs::Const::STAR());
		m_FilePattern = Path(vfs::Const::STAR());
	}
	else
	{
		pattern.splitLast(m_LocPattern,m_FilePattern);
	}

	_vloc_iter = m_VFS->m_mapFS.begin();
	while(_vloc_iter != m_VFS->m_mapFS.end())
	{
		if( matchPattern(m_LocPattern(),_vloc_iter->second->cPath()) )
		{
			bool exclusive_vloc = _vloc_iter->second->getIsExclusive();
			_vfile_iter = _vloc_iter->second->iterate();
			while(!_vfile_iter.end())
			{
				vfs::IBaseFile* file = NULL;
				if(exclusive_vloc)
				{
					file = _vfile_iter.value()->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
				}
				else
				{
					file = _vfile_iter.value()->file(VirtualFile::SF_TOP);
				}
				if(file)
				{
					Path const& filename = file->getName();
					if( matchPattern(m_FilePattern(), filename()) )
					{
						return;
					}
				}
				_vfile_iter.next();
			}
		}
		_vloc_iter++;
	}
}

vfs::ReadableFile_t* vfs::VirtualFileSystem::MatchingIterator::value()
{
	bool exclusive_vloc = false;
	if( _vloc_iter != m_VFS->m_mapFS.end() )
	{
		exclusive_vloc = _vloc_iter->second->getIsExclusive();
	}
	if(!_vfile_iter.end())
	{
		VirtualFile* vfile = _vfile_iter.value();
		if(vfile)
		{
			IBaseFile* file = NULL;
			if(exclusive_vloc)
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
			}
			else
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_TOP);
			}
			return ReadableFile_t::cast(file);
		}
	}
	return NULL;
}

bool vfs::VirtualFileSystem::MatchingIterator::nextLocationMatch()
{
	while(_vloc_iter != m_VFS->m_mapFS.end())
	{
		_vloc_iter++;
		if(_vloc_iter != m_VFS->m_mapFS.end())
		{
			if(matchPattern(m_LocPattern(),_vloc_iter->second->cPath()))
			{
				return true;
			}
		}
	}
	return false;
}
bool vfs::VirtualFileSystem::MatchingIterator::nextFileMatch()
{
	bool exclusive_vloc = false;
	if( _vloc_iter != m_VFS->m_mapFS.end() )
	{
		exclusive_vloc = _vloc_iter->second->getIsExclusive();
	}
	while(!_vfile_iter.end())
	{
		_vfile_iter.next();
		if(!_vfile_iter.end())
		{
			IBaseFile* file = NULL;
			if(exclusive_vloc)
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
			}
			else
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_TOP);
			}
			if(file)
			{
				Path const& filename = file->getName();
				if(matchPattern(m_FilePattern(),filename()))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void vfs::VirtualFileSystem::MatchingIterator::next()
{
	if(nextFileMatch())
	{
		return;
	}
	while(nextLocationMatch())
	{
		_vfile_iter = _vloc_iter->second->iterate();
		if(!_vfile_iter.end())
		{
			bool bExclusiveVLoc = _vloc_iter->second->getIsExclusive();
			IBaseFile* file    = NULL;
			if(bExclusiveVLoc)
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
			}
			else
			{
				file = _vfile_iter.value()->file(VirtualFile::SF_TOP);
			}
			if(file && matchPattern(m_FilePattern(), file->getName()()))
			{
				return;
			}
			else if(nextFileMatch())
			{
				return;
			}
		}
	}
}

/********************************************************************/
/********************************************************************/

bool vfs::canWrite()
{
	VirtualProfile::SP prof = getVFS()->getProfileStack()->topProfile();
	return !prof.isNull() && prof->cWritable;
}

vfs::VirtualFileSystem* getVFS()
{
	return vfs::VirtualFileSystem::getVFS();
}

vfs::VirtualFileSystem* vfs::VirtualFileSystem::m_pSingleton = NULL;

vfs::VirtualFileSystem* vfs::VirtualFileSystem::getVFS()
{
	if(!m_pSingleton)
	{
		m_pSingleton = new VirtualFileSystem();
	}
	return m_pSingleton;
}

void vfs::VirtualFileSystem::shutdownVFS()
{
	VFS_LOG_INFO(L"Shutting down VFS");
	if(m_pSingleton)
	{
		delete m_pSingleton;
		m_pSingleton = NULL;
	}
}


vfs::VirtualFileSystem::VirtualFileSystem()
{
	m_ProfileStack = VFS_NEW(ProfileStack);
}

vfs::VirtualFileSystem::~VirtualFileSystem()
{
	VFS_LOG_DEBUG("  deleting VFS object ..");
	VFS_t::iterator it = m_mapFS.begin();
	for(; it != m_mapFS.end(); ++it)
	{
		VFS_LOG_DEBUG(_BS("    delete location : ") << it->second->cPath << _BS::wget);
		it->second.null();
	}
	m_mapFS.clear();

	VFS_LOG_DEBUG(L"    delete profile stack");
	m_ProfileStack.null();

#ifdef VFS_DEBUG_MEMORY
	MemRegister::instance().printMem();
#endif
}

vfs::ProfileStack* vfs::VirtualFileSystem::getProfileStack()
{
	return m_ProfileStack;
}

vfs::VirtualFileSystem::Iterator vfs::VirtualFileSystem::begin()
{
	return Iterator( VFS_NEW1(RegularIterator, this) );
}

vfs::VirtualFileSystem::Iterator vfs::VirtualFileSystem::begin(vfs::Path const& pattern)
{
	return Iterator( VFS_NEW2(MatchingIterator, pattern,this) );
}

bool vfs::VirtualFileSystem::addLocation(vfs::IBaseLocation* location, const vfs::VirtualProfile* profile)
{
	VFS_THROW_IFF(location != NULL, L"Invalid location object");
	VFS_THROW_IFF(profile  != NULL, L"Invalid profile object");

	std::list<Path> sub_dirs;
	location->getSubDirList(sub_dirs, profile);

	std::list<Path>::const_iterator sd_cit = sub_dirs.begin();
	for(;sd_cit != sub_dirs.end(); ++sd_cit)
	{
		this->getVirtualLocation(*sd_cit, true);
	}

	IBaseLocation::Iterator it = location->begin(profile);
	for(; !it.end(); it.next())
	{
		IBaseFile::SP _file = it.value();

		Path dir, file;
		_file->getPath(dir, file);

		VirtualLocation::SP location = this->getVirtualLocation(dir,true);
		location->addFile(_file, profile->cName);
	}
	return true;
}

vfs::ReadableFile_t* vfs::VirtualFileSystem::getReadFile(vfs::Path const& file_path, vfs::VirtualFile::ESearchFile eSF)
{
	return ReadableFile_t::cast(this->getFile(file_path,eSF));
}

vfs::WritableFile_t* vfs::VirtualFileSystem::getWriteFile(vfs::Path const& file_path, vfs::VirtualFile::ESearchFile eSF)
{
	return WritableFile_t::cast(this->getFile(file_path,eSF));
}

vfs::IBaseFile* vfs::VirtualFileSystem::getFile(vfs::Path const& file_path, vfs::VirtualFile::ESearchFile eSF)
{
	VFS_LOG_DEBUG( (L"Get VFS file : " + file_path()).c_str() );
	Path dir, file;
	file_path.splitLast(dir,file);

	VirtualLocation::SP vloc  = this->getVirtualLocation(dir);
	if(!vloc.isNull())
	{
		VirtualFile::SP vfile = vloc->getVirtualFile(file);
		if(!vfile.isNull())
		{
			if(vloc->getIsExclusive())
			{
				return vfile->file(VirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
			}
			return vfile->file(eSF);
		}
	}
	VFS_LOG_DEBUG( _BS(L"Could not find VFS file : ") << file_path << _BS::wget );
	return NULL;
}

vfs::ReadableFile_t* vfs::VirtualFileSystem::getReadFile(vfs::Path const& file_path, vfs::String const& profile_name)
{
	return ReadableFile_t::cast(this->getFile(file_path, profile_name));
}

vfs::WritableFile_t* vfs::VirtualFileSystem::getWriteFile(vfs::Path const& file_path, vfs::String const& profile_name)
{
	return WritableFile_t::cast(this->getFile(file_path, profile_name));
}

vfs::IBaseFile* vfs::VirtualFileSystem::getFile(vfs::Path const& file_path, vfs::String const& profile_name)
{
	Path dir, file;
	file_path.splitLast(dir,file);

	VFS_t::iterator it_loc = m_mapFS.find(dir);
	if(it_loc == m_mapFS.end())
	{
		return NULL;
	}

	VirtualLocation::SP vloc = it_loc->second;
	if(!vloc.isNull())
	{
		return vloc->getFile(file,profile_name);
	}

	return NULL;
}

bool vfs::VirtualFileSystem::fileExists(vfs::Path const& file_path, vfs::String const& profile_name)
{
	return this->getFile(file_path, profile_name) != NULL;
}

bool vfs::VirtualFileSystem::fileExists(vfs::Path const& file_path, vfs::VirtualFile::ESearchFile eSF)
{
	return this->getFile(file_path, eSF) != NULL;
}

vfs::VirtualLocation* vfs::VirtualFileSystem::getVirtualLocation(vfs::Path const& loc_path, bool create)
{
	VFS_t::iterator it = m_mapFS.find(loc_path);
	if(it == m_mapFS.end())
	{
		if(create)
		{
			VirtualLocation::SP vLoc = VFS_NEW1(VirtualLocation, loc_path);
			m_mapFS.insert(std::make_pair(loc_path, vLoc));
			return vLoc;
		}
		return NULL;
	}
	return it->second;
}

bool vfs::VirtualFileSystem::removeDirectoryFromFS(vfs::Path const& dir_path)
{
	vfs::Path pattern = dir_path + "*";
	std::list<vfs::Path> files;

	Iterator it = this->begin(pattern);
	for(; !it.end(); it.next())
	{
		if(it.value()->implementsWritable())
		{
			files.push_back(it.value()->getPath());
		}
	}
	bool success = true;
	std::list<vfs::Path>::iterator fit = files.begin();
	for(; fit != files.end(); ++fit)
	{
		success &= this->removeFileFromFS(*fit);
	}
	return success;
}

bool vfs::VirtualFileSystem::removeFileFromFS(vfs::Path const& file_path)
{
	VFS_LOG_DEBUG(_BS(L" DELETE file from FS : ") << file_path << _BS::wget);
	Path dir, file;
	file_path.splitLast(dir,file);

	VirtualProfile::SP profile = m_ProfileStack->getWriteProfile();
	if(!profile.isNull())
	{
		IBaseLocation::SP location = profile->getLocation(dir);
		if(!location.isNull() && location->implementsWritable())
		{
			TDirectory<IWritable>::SP directory = dynamic_cast<TDirectory<IWritable>*>(location.get());
			if(!directory.isNull())
			{
				bool bSuccess = false;
				// remove file from virtual structures first
				IBaseFile::SP file = directory->getFile(file_path, NULL);
				if(!file.isNull())
				{
					Path _dir, _file;
					file_path.splitLast(_dir,_file);

					VirtualLocation::SP vloc = this->getVirtualLocation(_dir);
					if(!vloc.isNull())
					{
						bSuccess = vloc->removeFile(file);
					}
				}
				// delete actual file
				return bSuccess && directory->deleteFileFromDirectory(file_path);
			}
		}
	}
	return false;
}

bool vfs::VirtualFileSystem::createNewFile(vfs::Path const& filename)
{
	Path dir, file;
	filename.splitLast(dir,file);

	VirtualProfile::SP profile = m_ProfileStack->getWriteProfile();
	if(!profile.isNull())
	{
		bool is_exclusive          = false;
		bool new_location          = false;
		IBaseLocation::SP location = profile->getLocation(dir);
		if(location.isNull())
		{
			// try to find closest match
			Path temp, create_dir, right, left = dir;
			while(location.isNull() && !left.empty())
			{
				left.splitLast(temp, right);
				left       = temp;
				create_dir = right + create_dir;
				location   = profile->getLocation(left);
			}
			// see if the closest match is exclusive
			// if yes, then the the new path is a subdirectory and has to be exclusive too
			VirtualLocation::SP vloc = this->getVirtualLocation(left,true);
			if(!vloc.isNull())
			{
				is_exclusive = vloc->getIsExclusive();
			}
			//else
			//{
			//	VFS_THROW(L"location (closest match) should exist");
			//}
			new_location = true;
		}
		if(!location.isNull() && location->implementsWritable())
		{
			// create file and add to location
			TDirectory<IWritable>::SP pDir( dynamic_cast<TDirectory<IWritable>*>(location.get()) );
			IBaseFile* file = pDir->addFile(filename);
			if(new_location)
			{
				profile->addLocation(location);
			}
			if(file)
			{
				VirtualLocation::SP vloc = this->getVirtualLocation(dir,true);
				if(is_exclusive)
				{
					vloc->setIsExclusive(is_exclusive);
				}
				vloc->addFile(file, profile->cName);
				return true;
			}
		}
	}
	// throw ?
	return false;
}

/************************************************************************************************/

void vfs::VirtualFileSystem::print(std::wostream& out)
{
	ProfileStack::Iterator it_prof = m_ProfileStack->begin();
	for(; !it_prof.end(); it_prof.next())
	{
		VirtualProfile::SP profile = it_prof.value();
		out << L"Profile : " << profile->cName << L" : " << profile->cRoot() << L"\n";

		VirtualProfile::Iterator it_loc = profile->begin();
		for(; !it_loc.end(); it_loc.next())
		{
			IBaseLocation::SP location = it_loc.value();
			out << L"  Location : " << location->getPath()() << L"\n";

			IBaseLocation::Iterator it_file = location->begin(NULL);
			for(; !it_file.end(); it_file.next())
			{
				out << L"    File : " << it_file.value()->getName()() << L"\n";
			}
			out << std::endl;
		}
	}
}

