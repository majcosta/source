/*
 * bfVFS : vfs/Core/Location/vfs_uncompressed_lib_base.cpp
 *  - partially implements library interface for uncompressed archive files
 *  - initialization is done in format-specific sub-classes
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

#include <vfs/Core/Location/vfs_uncompressed_lib_base.h>
#include <vfs/Core/vfs.h>
#include <vfs/Tools/vfs_parser_tools.h>
#include <vfs/Aspects/vfs_logging.h>

/********************************************************************************************/
vfs::UncompressedLibraryBase::SFileData& vfs::UncompressedLibraryBase::_fileDataFromHandle(FileType_t* handle)
{
	FileData_t::iterator it = m_fileData.find(handle);
	if(it != m_fileData.end())
	{
		return it->second;
	}
	VFS_THROW(L"Invalid file handle");
}
/********************************************************************************************/
/********************************************************************************************/

class vfs::UncompressedLibraryBase::IterImpl : public vfs::IBaseLocation::Iterator::IImplementation
{
	typedef vfs::IBaseLocation::Iterator::IImplementation BaseClass_t;

public:
	VFS_SMARTPOINTER(IterImpl);

	IterImpl(UncompressedLibraryBase* library, const VirtualProfile* profile);
	virtual ~IterImpl();

	virtual FileType_t*     value();
	virtual void            next();

protected:
	virtual BaseClass_t::SP  clone() const
	{
		IterImpl::SP iter( VFS_NEW2(IterImpl, _lib, _profile) );
		iter->_iter = _iter;
		return iter;
	}

private:
	inline void             nextProfileFile(bool first = false)
	{
		if(first)
		{
			_iter = _lib->m_fileData.begin();
		}
		else if(_iter != _lib->m_fileData.end())
		{
			_iter++;
		}
		if(_lib->m_profMap.empty())
		{
			// just iterate, no test
			return;
		}

		UncompressedLibraryBase::ProfileMap_t::const_iterator it = _lib->m_profMap.find(_profile);
		if(it == _lib->m_profMap.end())
		{
			// if there is no map for this profile, then set iterator to 'end' so that following loop immediately exits
			_iter = _lib->m_fileData.end();
		}

		UncompressedLibraryBase::PathToPathMap_t const& pmap = it->second.ProfileToLibrary;

		while(_iter != _lib->m_fileData.end())
		{
			Path dir, file;
			_iter->first->getPath(dir, file);

			if(pmap.find(dir) != pmap.end())
			{
				return;
			}
			_iter++;
		}
	}

private:
	UncompressedLibraryBase::SP                     _lib;
	UncompressedLibraryBase::FileData_t::iterator   _iter;
	const VirtualProfile*                           _profile;
};


vfs::UncompressedLibraryBase::IterImpl::IterImpl(vfs::UncompressedLibraryBase* library, const VirtualProfile* profile)
: BaseClass_t(), _lib(library), _profile(profile)
{
	nextProfileFile(true);
}
vfs::UncompressedLibraryBase::IterImpl::~IterImpl()
{
}

vfs::UncompressedLibraryBase::FileType_t* vfs::UncompressedLibraryBase::IterImpl::value()
{
	if(_iter != _lib->m_fileData.end())
	{
		return _iter->first;
	}
	return NULL;
}

void vfs::UncompressedLibraryBase::IterImpl::next()
{
	nextProfileFile();
}

/************************************************************************/
/************************************************************************/

vfs::UncompressedLibraryBase::UncompressedLibraryBase(vfs::ReadableFile_t *lib_file, vfs::Path const& mountpoint)
: vfs::ILibrary(lib_file, mountpoint), m_numberOfOpenedFiles(0)
{
}

vfs::UncompressedLibraryBase::~UncompressedLibraryBase()
{
	this->closeLibrary();
	// delete sub dirs from catalogue
	DirCatalogue_t::iterator it = m_dirs.begin();
	for(; it != m_dirs.end(); ++it)
	{
		it->second.null();
	}
	// LibData is invalid
	// just clear it, since the file handles were deleted before
	m_fileData.clear();
	m_dirs.clear();
}

void vfs::UncompressedLibraryBase::closeLibrary()
{
	FileData_t::iterator it = m_fileData.begin();
	for(; it != m_fileData .end(); ++it)
	{
		// what if closing of (at least) one file fails?? continue or not??
		// in the end, these are not real files!
		VFS_IGNOREEXCEPTION(it->first->close(), true);
	}
}

bool vfs::UncompressedLibraryBase::fileExists(vfs::Path const& filename, const VirtualProfile* profile)
{
	Path dir, file;
	filename.splitLast(dir, file);

	// if profile is NULL, then the user intends to check the existence of a file
	// by using its actual filename in the library (and not one mapped to a profile)
	if( this->_mapProfileDirToLibraryDir(dir, profile) || profile == NULL)
	{
		DirCatalogue_t::iterator it = m_dirs.find(dir);
		if(it != m_dirs.end())
		{
			return it->second->fileExists(file, profile);
		}
	}
	return false;
}

vfs::IBaseFile*	vfs::UncompressedLibraryBase::getFile(vfs::Path const& filename, const VirtualProfile* profile)
{
	return getFileTyped(filename, profile);
}

vfs::UncompressedLibraryBase::FileType_t* vfs::UncompressedLibraryBase::getFileTyped(vfs::Path const& filename, const VirtualProfile* profile)
{
	Path dir, file;
	filename.splitLast(dir,file);

	if( this->_mapProfileDirToLibraryDir(dir, profile) || profile == NULL)
	{
		DirCatalogue_t::iterator it = m_dirs.find(dir);
		if(it != m_dirs.end())
		{
			return it->second->getFileTyped(file, profile);
		}
	}
	return NULL;
}

void vfs::UncompressedLibraryBase::close(FileType_t *file_handle)
{
	try
	{
		SFileData& file = _fileDataFromHandle(file_handle);

		// reset read position
		// can't do this when opening file,
		// because you could try to open a file when it is already open and would so reset the read position
		file._currentReadPosition = 0;
		if(m_numberOfOpenedFiles > 0)
		{
			m_numberOfOpenedFiles--;
			if(!m_lib_file.isNull() && m_numberOfOpenedFiles == 0)
			{
				m_lib_file->close();
			}
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
}
bool vfs::UncompressedLibraryBase::openRead(FileType_t *file_handle)
{
	try
	{
		_fileDataFromHandle(file_handle);
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}

	m_numberOfOpenedFiles++;
	if(m_numberOfOpenedFiles == 1)
	{
		if(!m_lib_file.isNull() && !m_lib_file->isOpenRead() && !m_lib_file->openRead())
		{
			return false;
		}
	}
	// already open
	return true;
}

vfs::size_t vfs::UncompressedLibraryBase::read(FileType_t *file_handle, vfs::Byte* data, vfs::size_t bytesToRead)
{
	try
	{
		SFileData& file = _fileDataFromHandle(file_handle);

		if( (file._currentReadPosition + bytesToRead) > file._fileSize )
		{
			bytesToRead = file._fileSize - file._currentReadPosition;
		}
		if(bytesToRead == 0)
		{
			// eof
			return 0;
		}

		if(!m_lib_file.isNull())
		{
			// set lib-file's read-location to match location of lib-file
			m_lib_file->setReadPosition(file._fileOffset + file._currentReadPosition, IBaseFile::SD_BEGIN);

			vfs::size_t bytesRead = m_lib_file->read(data, bytesToRead);
			VFS_THROW_IFF( bytesToRead == bytesRead, L"Number of bytes doesn't match" );
			file._currentReadPosition  += bytesRead;
			return bytesRead;
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
	return 0;
}

vfs::size_t vfs::UncompressedLibraryBase::getReadPosition(FileType_t *file_handle)
{
	try
	{
		return _fileDataFromHandle(file_handle)._currentReadPosition;
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
	return 0;
}

void vfs::UncompressedLibraryBase::setReadPosition(FileType_t *file_handle, vfs::size_t positionInBytes)
{
	try
	{
		SFileData& file = _fileDataFromHandle(file_handle);
		if( positionInBytes > file._fileSize )
		{
			positionInBytes = file._fileSize;
		}

		// positionInBytes is offset to file-offset
		file._currentReadPosition = positionInBytes;
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
}

static inline vfs::offset_t clampReadPosition(vfs::offset_t const& off, vfs::size_t const& size)
{
	return ( off < 0 ) ? ( 0 ) : ( (vfs::size_t)off > size ? size : off );
}

void vfs::UncompressedLibraryBase::setReadPosition(FileType_t *file_handle, vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir)
{
	try
	{
		SFileData& file = _fileDataFromHandle(file_handle);

		if(seekDir == IBaseFile::SD_BEGIN)
		{
			file._currentReadPosition = clampReadPosition(offsetInBytes, file._fileSize);
		}
		else if(seekDir == IBaseFile::SD_CURRENT)
		{
			vfs::offset_t pos = file._currentReadPosition + offsetInBytes;
			file._currentReadPosition = clampReadPosition(pos, file._fileSize);
		}
		else if(seekDir == IBaseFile::SD_END)
		{
			vfs::offset_t pos = file._fileSize + offsetInBytes;
			file._currentReadPosition = clampReadPosition(pos, file._fileSize);
		}
		else
		{
			VFS_THROW(L"Unknown seek direction");
		}
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
}

vfs::size_t vfs::UncompressedLibraryBase::getSize(FileType_t *file_handle)
{
	try
	{
		return _fileDataFromHandle(file_handle)._fileSize;
	}
	catch(std::exception& ex)
	{
		VFS_RETHROW(L"", ex);
	}
	return 0;
}

void vfs::UncompressedLibraryBase::getSubDirList(std::list<vfs::Path>& sub_dirs, const VirtualProfile* profile)
{
	/*
	 * If the map from profile to path-map is empty, then the library is not associated with a profile at all.
	 * In that case return all sub-directories.
	 */
	if(!m_profMap.empty())
	{
		ProfileMap_t::iterator it = m_profMap.find(profile);
		if(it != m_profMap.end())
		{
			PathToPathMap_t::iterator iter = it->second.ProfileToLibrary.begin();
			for(; iter != it->second.ProfileToLibrary.end(); ++iter)
			{
				sub_dirs.push_back(iter->first);
			}
		}
		return;
	}

	DirCatalogue_t::iterator it = m_dirs.begin();
	for(;it != m_dirs.end(); ++it)
	{
		sub_dirs.push_back(it->first);
	}
}

vfs::UncompressedLibraryBase::Iterator vfs::UncompressedLibraryBase::begin(const VirtualProfile* profile)
{
	return Iterator( VFS_NEW2(IterImpl, UncompressedLibraryBase::SP(this), profile) );
}

bool vfs::UncompressedLibraryBase::_mapProfileDirToLibraryDir(Path& dir, const VirtualProfile* profile) const
{
	ProfileMap_t::const_iterator it = m_profMap.find(profile);
	if(it != m_profMap.end())
	{
		PathToPathMap_t::const_iterator iter = it->second.ProfileToLibrary.find(dir);
		if(iter != it->second.ProfileToLibrary.end())
		{
			dir = iter->second;
			return true;
		}
	}
	return false;
}

bool vfs::UncompressedLibraryBase::mapProfilePaths(vfs_init::VfsConfig::SP const& config)
{
	m_profMap.clear();

	VirtualFileSystem* vfs    = getVFS();
	ProfileStack::SP   pstack = vfs->getProfileStack();

	vfs_init::VfsConfig::profiles_t::const_iterator iter = config->profiles.begin();
	for(; iter != config->profiles.end(); ++iter)
	{
		VirtualProfile::SP profile = pstack->getProfile( (*iter)->m_name );
		if(!profile.isNull())
		{
			vfs_init::Profile::locations_t::const_iterator it = (*iter)->locations.begin();
			for(; it != (*iter)->locations.end(); ++it)
			{
				Path path    = (*iter)->m_root + (*it)->m_path;
				Path pattern = path + "*";

				// go over all directories and see if they match
				DirCatalogue_t::iterator diter = m_dirs.begin();
				for(; diter != m_dirs.end(); ++diter)
				{
					if(StrCmp::Equal(path(), diter->first()) || matchPattern(pattern(), diter->first()))
					{
						std::wstring ws = diter->first();

						size_t len      = (*it)->m_mount_point.length() > 0 ? path.length() : path.length() + 1;

						ws.replace(0, len, (*it)->m_mount_point());

						ProfileMaps& pmaps = m_profMap[profile];
						pmaps.ProfileToLibrary[Path(ws)] = diter->first;
						pmaps.LibraryToProfile[diter->first] = Path(ws);

						diter->second->m_mountPoint = Path(ws);
					}
				}
			}
		}
	}

	return true;
}
