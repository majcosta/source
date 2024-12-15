/*
 * bfVFS : vfs/Core/Location/vfs_lib_dir.cpp
 *  - class for readonly (sub)directories in archives/libraries
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

#include <vfs/Core/Location/vfs_lib_dir.h>
#include <vfs/Tools/vfs_log.h>
#include <vfs/Aspects/vfs_logging.h>

class vfs::LibDirectory::IterImpl : public vfs::IBaseLocation::Iterator::IImplementation
{
	typedef IBaseLocation::Iterator::IImplementation    BaseClass_t;

public:
	VFS_SMARTPOINTER(IterImpl);

	IterImpl(LibDirectory& lib);
	virtual ~IterImpl();

	virtual LibDirectory::FileType_t*       value();
	virtual void                            next();

protected:
	virtual BaseClass_t::SP                 clone() const;

private:
	void operator=(LibDirectory::IterImpl const& iter);

	LibDirectory&                           _lib;
	LibDirectory::FileCatalogue_t::iterator _iter;
};

vfs::LibDirectory::IterImpl::IterImpl(LibDirectory& lib)
: BaseClass_t(), _lib(lib)
{
	_iter = _lib.m_files.begin();
}

vfs::LibDirectory::IterImpl::~IterImpl()
{
}

vfs::LibDirectory::FileType_t* vfs::LibDirectory::IterImpl::value()
{
	if(_iter != _lib.m_files.end())
	{
		return _iter->second;
	}
	return NULL;
}

void vfs::LibDirectory::IterImpl::next()
{
	if(_iter != _lib.m_files.end())
	{
		_iter++;
	}
}

vfs::LibDirectory::IterImpl::BaseClass_t::SP vfs::LibDirectory::IterImpl::clone() const
{
	IterImpl::SP iter( VFS_NEW1(IterImpl, _lib) );
	iter->_iter = _iter;
	return iter;
}

/***************************************************************************/
/***************************************************************************/

vfs::LibDirectory::LibDirectory(vfs::Path const& local_path, vfs::Path const& real_path)
: BaseClass_t(local_path,real_path)
{
}

vfs::LibDirectory::~LibDirectory()
{
	FileCatalogue_t::iterator it = m_files.begin();
	for(; it != m_files.end(); ++it)
	{
		it->second.null();
	}
	m_files.clear();
}

vfs::LibDirectory::FileType_t* vfs::LibDirectory::addFile(vfs::Path const& filename, bool deleteOldFile)
{
	return NULL;
}

bool vfs::LibDirectory::addFile(vfs::LibDirectory::FileType_t* file, bool deleteOldFile)
{
	if(!file)
	{
		return false;
	}
	Path const& name       = file->getName();
	FileType_t::SP oldFile = m_files[name];
	if(!oldFile.isNull() && (oldFile.get() != file) )
	{
		if(deleteOldFile)
		{
			oldFile.null();
			m_files[name] = file;
		}
		else
		{
			return false;
		}
	}
	m_files[name] = file;
	return true;
}

bool vfs::LibDirectory::deleteDirectory(vfs::Path const& dir_path)
{
	//if( !(m_mountPoint == dirPath) )
	//{
	//	return false;
	//}
	//if(implementsWritable())
	//{
	//	tFileCatalogue::iterator it = m_files.begin();
	//	for(; it != m_files.end(); ++it)
	//	{
	//		//delete it->second;
	//	}
	//	m_files.clear();
	//	return true;
	//}
	VFS_LOG_ERROR(L"called 'deleteDirectory', 'vfs::CLibDirectory' doesn't implement the IWritable interface");
	return false;
}

bool vfs::LibDirectory::deleteFileFromDirectory(vfs::Path const& filename)
{
	//if(implementsWritable())
	//{
	//	tFileCatalogue::iterator it = m_files.find(filename);
	//	if(it != m_files.end())
	//	{
	//		delete it->second;
	//		m_files.erase(it);
	//		return true;
	//	}
	//}
	VFS_LOG_ERROR(L"called 'deleteFileFromDirectory', 'vfs::CLibDirectory' doesn't implement the IWritable interface");
	return false;
}

bool vfs::LibDirectory::fileExists(vfs::Path const& filename, const VirtualProfile*)
{
	return (m_files[filename] != NULL);
}

vfs::IBaseFile*	vfs::LibDirectory::getFile(vfs::Path const& filename, const VirtualProfile* profile)
{
	return getFileTyped(filename, profile);
}

vfs::LibDirectory::FileType_t* vfs::LibDirectory::getFileTyped(vfs::Path const& filename, const VirtualProfile*)
{
	FileCatalogue_t::iterator it = m_files.find(filename);
	if(it != m_files.end())
	{
		return it->second;
	}
	return NULL;
}

bool vfs::LibDirectory::createSubDirectory(vfs::Path const& sub_dir_path)
{
	// libraries are readonly
	return false;
}

void vfs::LibDirectory::getSubDirList(std::list<vfs::Path>& sub_dirs, const VirtualProfile*)
{
	// nothing
}

vfs::LibDirectory::Iterator vfs::LibDirectory::begin(const VirtualProfile* profile)
{
	return Iterator( VFS_NEW1(IterImpl, *this) );
}

/***************************************************************************/
/***************************************************************************/
