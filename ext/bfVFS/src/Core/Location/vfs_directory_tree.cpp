/*
 * bfVFS : vfs/Core/Location/vfs_directory_tree.cpp
 *  - class for directories in a File System, implements Directory interface
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

#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Core/File/vfs_dir_file.h>
#include <vfs/Core/vfs_os_functions.h>

#include <vfs/Aspects/vfs_logging.h>

#include <queue>
#include <list>
#include <set>

#define ERROR_FILE(msg)     (_BS(msg) << L" : " << file->getPath() << _BS::wget)

namespace vfs
{
	template<typename WriteType>
	class TSubDir : public TDirectory<typename TDirectoryTree<WriteType>::WriteType_t>
	{
	protected:
		typedef TDirectory<typename TDirectoryTree<WriteType>::WriteType_t> BaseClass_t;
		typedef typename BaseClass_t::FileType_t                            FileType_t;

		typedef std::map<Path, typename FileType_t::SP, Path::Less>         FileCatalogue_t;

	public:
		VFS_SMARTPOINTER(TSubDir);

		typedef IBaseLocation::Iterator                                     Iterator;

		/////////////////////////////////////////////////////////////////////
		class IterImpl : public IBaseLocation::Iterator::IImplementation
		{
			friend class TSubDir<WriteType>;
			typedef IBaseLocation::Iterator::IImplementation BaseClass_t;

			IterImpl(TSubDir<WriteType>* dir): _dir(dir)
			{
				VFS_THROW_IFF(_dir, L"");
				_iter = _dir->m_mapFiles.begin();
			}

		public:
			VFS_SMARTPOINTER(IterImpl);

			IterImpl() : BaseClass_t(), _dir(NULL)
			{};
			virtual ~IterImpl()
			{};

			virtual FileType_t* value()
			{
				if(_iter != _dir->m_mapFiles.end())
				{
					return _iter->second;
				}
				return NULL;
			}
			virtual void        next()
			{
				if(_iter != _dir->m_mapFiles.end())
				{
					_iter++;
				}
			}

		protected:
			BaseClass_t::SP     clone() const
			{
				IterImpl::SP iter( VFS_NEW(IterImpl) );
				iter->_dir  = _dir;
				iter->_iter = _iter;
				return iter;
			}

		private:
			TSubDir<WriteType>*                 _dir;
			typename FileCatalogue_t::iterator  _iter;
		};
		/////////////////////////////////////////////////////////////////////

	protected:
		TSubDir(Path const& sMountPoint, Path const& sRealPath)
			: BaseClass_t(sMountPoint,sRealPath)
		{};

	public:
		virtual ~TSubDir();

		virtual bool            fileExists             (Path const& filename, const VirtualProfile* profile);
		virtual IBaseFile*      getFile                (Path const& filename, const VirtualProfile* profile);
		virtual FileType_t*     getFileTyped           (Path const& filename, const VirtualProfile* profile);

		virtual FileType_t*     addFile                (Path const& filename, bool deleteOldFile=false);
		virtual bool            addFile                (FileType_t* file    , bool deleteOldFile=false);

		virtual bool            createSubDirectory     (Path const& sub_dir_path);
		virtual bool            deleteDirectory        (Path const& dir_path);
		virtual bool            deleteFileFromDirectory(Path const& filename);

		virtual void            getSubDirList          (std::list<Path>& sub_dirs, const VirtualProfile* profile);

		virtual Iterator        begin                  (const VirtualProfile* profile);

	private:
		FileCatalogue_t m_mapFiles;
	};

	template<>
	TSubDir<IWriteType>::FileType_t* TSubDir<IWriteType>::addFile(Path const& filename, bool deleteOldFile);
	template<>
	TSubDir<IWritable>::FileType_t*  TSubDir<IWritable>:: addFile(Path const& filename, bool deleteOldFile);

	template<>
	bool TSubDir<IWriteType>::deleteDirectory(Path const& dir_path);
	template<>
	bool TSubDir<IWritable>:: deleteDirectory(Path const& dir_path);

	template<>
	bool TSubDir<IWriteType>::deleteFileFromDirectory(Path const& filename);
	template<>
	bool TSubDir<IWritable>:: deleteFileFromDirectory(Path const& filename);
}

/********************************************************/

template<typename WriteType>
vfs::TSubDir<WriteType>::~TSubDir()
{
	typename FileCatalogue_t::iterator it = m_mapFiles.begin();
	for(; it != m_mapFiles.end(); ++it)
	{
		if(!it->second.isNull())
		{
			FileType_t* tmp = it->second;
			VFS_LOG_DEBUG(_BS(L" deleting file : ") << tmp->getName() << _BS::wget);

			it->second->close();
			it->second.null();
		}
	}
	m_mapFiles.clear();
}

template<typename WriteType>
bool vfs::TSubDir<WriteType>::fileExists(vfs::Path const& filename, const VirtualProfile* profile)
{
	typename FileCatalogue_t::iterator it = m_mapFiles.find(filename);
	bool success = (it != m_mapFiles.end()) && (it->second != NULL);
	return success;
}

template<typename WriteType>
vfs::IBaseFile* vfs::TSubDir<WriteType>::getFile(vfs::Path const& filename, const VirtualProfile* profile)
{
	return getFileTyped(filename, profile);
}

template<typename WriteType>
typename vfs::TSubDir<WriteType>::FileType_t* vfs::TSubDir<WriteType>::getFileTyped(vfs::Path const& filename, const VirtualProfile* profile)
{
	typename FileCatalogue_t::iterator it = m_mapFiles.find(filename);
	if(it != m_mapFiles.end())
	{
		return it->second;
	}
	return NULL;
}

template<>
vfs::TSubDir<vfs::IWriteType>::FileType_t* vfs::TSubDir<vfs::IWriteType>::addFile(vfs::Path const& filename, bool deleteOldFile)
{
	FileType_t::SP file = m_mapFiles[filename];
	if(!file.isNull())
	{
		if(!deleteOldFile)
		{
			// not allowed to replace old file
			return NULL;
		}
		file.null();
	}
	file = VFS_NEW2(ReadOnlyDirFile, filename, this);
	m_mapFiles[filename] = file;

	return file;
}

template<>
vfs::TSubDir<vfs::IWritable>::FileType_t* vfs::TSubDir<vfs::IWritable>::addFile(vfs::Path const& filename, bool deleteOldFile)
{
	FileType_t::SP file = m_mapFiles[filename];
	if(!file.isNull())
	{
		if(!deleteOldFile)
		{
			// not allowed to replace old file
			return NULL;
		}
		file.null();
	}
	file = VFS_NEW2(DirFile, filename, this);
	m_mapFiles[filename] = file;

	return file;
}

template<typename WriteType>
bool vfs::TSubDir<WriteType>::addFile(FileType_t* file, bool deleteOldFile)
{
	if(!file)
	{
		return false;
	}
	typename FileType_t::SP old_file = m_mapFiles[file->getName()];
	if( !old_file.isNull() && (old_file.get() != file) )
	{
		if(deleteOldFile)
		{
			old_file->close();
			//delete old_file;
			old_file.null();
		}
	}
	m_mapFiles[file->getName()] = file;
	return true;
}

template<>
bool vfs::TSubDir<vfs::IWritable>::deleteDirectory(vfs::Path const& dir_path)
{
	if( !(this->m_mountPoint == dir_path) )
	{
		return false;
	}
	FileCatalogue_t::iterator it = m_mapFiles.begin();
	for(; it != m_mapFiles.end(); ++it)
	{
		FileType_t::SP file = it->second;
		if(!file.isNull())
		{
			file->close();
			if(!file->deleteFile())
			{
				VFS_THROW( ERROR_FILE(L"Could not delete file") );
			}
			file.null();
		}
	}
	m_mapFiles.clear();
	return true;
}

template<>
bool vfs::TSubDir<vfs::IWriteType>::deleteDirectory(vfs::Path const& dir_path)
{
	return false;
}

template<>
bool vfs::TSubDir<vfs::IWritable>::deleteFileFromDirectory(vfs::Path const& filename)
{
	FileCatalogue_t::iterator it = m_mapFiles.find(filename);
	if( it != m_mapFiles.end() )
	{
		FileType_t::SP file = it->second;
		if(!file.isNull())
		{
			file->close();
			VFS_THROW_IFF(file->deleteFile(), ERROR_FILE(L"Could not delete file"));
			file.null();
		}
		m_mapFiles.erase(it);
		return true;
	}
	return false;
}

template<>
bool vfs::TSubDir<vfs::IWriteType>::deleteFileFromDirectory(vfs::Path const& filename)
{
	return false;
}

template<typename WriteType>
bool vfs::TSubDir<WriteType>::createSubDirectory(vfs::Path const& sub_dir_path)
{
	return false;
}

template<typename WriteType>
void vfs::TSubDir<WriteType>::getSubDirList(std::list<vfs::Path>& sub_dirs, const VirtualProfile* profile)
{
}

template<typename WriteType>
typename vfs::TSubDir<WriteType>::Iterator vfs::TSubDir<WriteType>::begin(const VirtualProfile* profile)
{
	return Iterator( VFS_NEW1(IterImpl, this) );
}

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

template<typename WriteType>
class vfs::TDirectoryTree<WriteType>::IterImpl : public vfs::IBaseLocation::Iterator::IImplementation
{
	typedef IBaseLocation::Iterator::IImplementation    BaseClass_t;
	typedef TDirectoryTree<WriteType>                   DirTree_t;

public:
	VFS_SMARTPOINTER(IterImpl);

	IterImpl(DirTree_t& tree);
	virtual ~IterImpl();

	virtual typename DirTree_t::FileType_t*     value();
	virtual void                                next();

protected:
	virtual BaseClass_t::SP                     clone() const
	{
		IterImpl::SP iter( VFS_NEW1(IterImpl, _tree) );
		iter->_subdir_iter = _subdir_iter;
		iter->_file_iter   = _file_iter;
		return iter;
	}
private:
	void operator=(typename vfs::TDirectoryTree<WriteType>::IterImpl const& tree);
	DirTree_t&                                      _tree;
	typename DirTree_t::DirCatalogue_t::iterator    _subdir_iter;
	typename DirTree_t::LocationType_t::Iterator    _file_iter;
};

template<typename WriteType>
vfs::TDirectoryTree<WriteType>::IterImpl::IterImpl(vfs::TDirectoryTree<WriteType>& tree)
: BaseClass_t(), _tree(tree)
{
	_subdir_iter     = _tree.m_Dirs.begin();
	if(_subdir_iter != _tree.m_Dirs.end())
	{
		typename TSubDir<WriteType>::SP sdir( dynamic_cast<TSubDir<WriteType>*>(_subdir_iter->second.get()) );
		typename TSubDir<WriteType>::Iterator it = sdir->begin(NULL);
		_file_iter = it;
		if(_file_iter.end())
		{
			next();
		}
	}
}

template<typename WriteType>
vfs::TDirectoryTree<WriteType>::IterImpl::~IterImpl()
{
}

template<typename WriteType>
typename vfs::TDirectoryTree<WriteType>::FileType_t* vfs::TDirectoryTree<WriteType>::IterImpl::value()
{
	if(!_file_iter.end())
	{
		return static_cast<typename vfs::TDirectoryTree<WriteType>::FileType_t*>(_file_iter.value());
	}
	return NULL;
}

template<typename WriteType>
void vfs::TDirectoryTree<WriteType>::IterImpl::next()
{
	if(!_file_iter.end())
	{
		_file_iter.next();
	}
	// need to loop for the case when one or many sub directories are empty
	while(_file_iter.end())
	{
		if(_subdir_iter == _tree.m_Dirs.end())
		{
			break;
		}
		else
		{
			_subdir_iter++;
			if(_subdir_iter != _tree.m_Dirs.end())
			{
				_file_iter = _subdir_iter->second->begin(NULL);
			}
		}
	}
	return;
}

/*****************************************************************************/
/*****************************************************************************/

template<typename WriteType>
vfs::TDirectoryTree<WriteType>::TDirectoryTree(vfs::Path const& mountpoint, vfs::Path const& real_path)
: BaseClass_t(mountpoint,real_path)
{};

template<typename WriteType>
vfs::TDirectoryTree<WriteType>::~TDirectoryTree()
{
	typename DirCatalogue_t::iterator it = m_Dirs.begin();
	for(;it != m_Dirs.end(); ++it)
	{
		it->second.null();
	}
	m_Dirs.clear();
}

template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::init()
{
	// contains local path
	typedef TSubDir<WriteType>                          SubDir_t;
	typedef std::pair<vfs::Path, typename SubDir_t::SP> Dirs_t;
	std::queue<Dirs_t>                                  qSubDirs;

	qSubDirs.push(Dirs_t(vfs::Path(vfs::Const::EMPTY()), VFS_NEW2(SubDir_t, this->m_mountPoint, this->m_realPath)));

	m_Dirs[this->m_mountPoint] = qSubDirs.front().second;

	String      sFilename;
	SubDir_t*   current_dir;
	Path        cur_dir_path;

	while(!qSubDirs.empty())
	{
		current_dir  = qSubDirs.front().second;
		cur_dir_path = this->m_realPath;
		if( !qSubDirs.front().first.empty())
		{
			cur_dir_path += qSubDirs.front().first;
		}

		try
		{
			OS::IterateDirectory::EFileAttribute eFA;
			OS::IterateDirectory iterFS(cur_dir_path, vfs::Const::STAR());
			while ( iterFS.nextFile(sFilename, eFA) )
			{
				if (StrCmp::Equal(vfs::Const::DOT(),sFilename) || StrCmp::Equal(vfs::Const::DOTDOT(),sFilename) || StrCmp::Equal(vfs::Const::DOTSVN(),sFilename) )
				{
					continue;
				}
				if (eFA == OS::IterateDirectory::FA_DIRECTORY)
				{
					Path local_path  = qSubDirs.front().first + sFilename;
					Path temp        = this->m_mountPoint+local_path;
					typename SubDir_t::SP pNewDir = VFS_NEW2(SubDir_t, local_path, this->m_realPath+local_path);

					qSubDirs.push(Dirs_t(local_path,pNewDir));
					m_Dirs[temp] = pNewDir;
				}
				else
				{
					current_dir->addFile(vfs::Path(sFilename));
				}
			}
		}
		catch(std::exception &ex)
		{
			// probably directory doesn't exist. abort or continue???
			// -> abort AND continue
			VFS_LOG_WARNING(ex.what());
			return false;
		}
		qSubDirs.pop();
	}
	return true;
}

template<typename WriteType>
typename vfs::TDirectoryTree<WriteType>::FileType_t* vfs::TDirectoryTree<WriteType>::addFile(vfs::Path const& filename, bool deleteOldFile)
{
	Path dir, file;
	filename.splitLast(dir,file);

	typename DirCatalogue_t::iterator it = m_Dirs.find(dir);
	if(it == m_Dirs.end())
	{
		Path temp, create_dir, left, right = dir;
		while(!right.empty())
		{
			right.splitFirst(left,temp);
			right = temp;
			create_dir += left;
			if(!this->createSubDirectory(create_dir))
			{
				VFS_THROW(_BS(L"could not create directory : ") << create_dir << _BS::wget);
			}
		}
		it = m_Dirs.find(dir);
		if(it == m_Dirs.end())
		{
			return NULL;
		}
	}
	if(!it->second.isNull())
	{
		return it->second->addFile(file,deleteOldFile);
	}
	return NULL;
}

template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::addFile(FileType_t* file, bool deleteOldFile)
{
	// no files from outside
	// these files are not connected with the correct directory object
	return false;
}

template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::deleteDirectory(vfs::Path const& dir_path)
{
	typename DirCatalogue_t::iterator it = m_Dirs.find(dir_path);
	if(it == m_Dirs.end())
	{
		// no such directory
		return false;
	}
	if(it->second)
	{
		return it->second->deleteDirectory(dir_path);
	}
	return false;
}

template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::deleteFileFromDirectory(vfs::Path const& filename)
{
	Path dir, file;
	filename.splitLast(dir,file);

	typename DirCatalogue_t::iterator it = m_Dirs.find(dir);
	if(it == m_Dirs.end())
	{
		// no such directory
		return false;
	}
	if(it->second)
	{
		return it->second->deleteFileFromDirectory(file);
	}
	return false;
}

/**
 *  IVFSLocation interface
 */
template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::fileExists(vfs::Path const& filename, const VirtualProfile* profile /* not used */)
{
	Path dir, file;
	filename.splitLast(dir, file);
	typename DirCatalogue_t::iterator it = m_Dirs.find(dir);
	if(it == m_Dirs.end())
	{
		// no such directory
		return false;
	}
	if(it->second)
	{
		return it->second->fileExists(file, profile);
	}
	return false;
}

template<typename WriteType>
vfs::IBaseFile* vfs::TDirectoryTree<WriteType>::getFile(vfs::Path const& filename, const VirtualProfile* profile /* not used */)
{
	return getFileTyped(filename, profile);
}

template<typename WriteType>
typename vfs::TDirectoryTree<WriteType>::FileType_t* vfs::TDirectoryTree<WriteType>::getFileTyped(vfs::Path const& filename, const VirtualProfile* profile /* not used */)
{
	Path dir, file;
	filename.splitLast(dir, file);

	typename DirCatalogue_t::iterator it = m_Dirs.find(dir);
	if(it == m_Dirs.end())
	{
		// no such directory
		return NULL;
	}
	if(it->second)
	{
		return it->second->getFileTyped(file, profile);
	}
	return NULL;
}

template<typename WriteType>
bool vfs::TDirectoryTree<WriteType>::createSubDirectory(vfs::Path const& sub_dir_path)
{
	if(OS::createRealDirectory( this->m_realPath + sub_dir_path ))
	{
		if( m_Dirs[sub_dir_path] == NULL)
		{
			m_Dirs[sub_dir_path] = VFS_NEW2(TSubDir<WriteType>, sub_dir_path, this->m_realPath + sub_dir_path);
		}
		return true;
	}
	return false;
}

template<typename WriteType>
void vfs::TDirectoryTree<WriteType>::getSubDirList(std::list<vfs::Path>& sub_dirs, const VirtualProfile* profile)
{
	typename DirCatalogue_t::iterator it = m_Dirs.begin();
	for(;it != m_Dirs.end(); ++it)
	{
		sub_dirs.push_back(it->first);
	}
}

template<typename WriteType>
typename vfs::TDirectoryTree<WriteType>::Iterator vfs::TDirectoryTree<WriteType>::begin(const VirtualProfile* profile)
{
	return Iterator( VFS_NEW1(IterImpl, *this) );
}

/*****************************************************************************/
/*****************************************************************************/

template class vfs::TDirectoryTree<vfs::IWritable>;		// explicit template class instantiation
template class vfs::TDirectoryTree<vfs::IWriteType>;	// explicit template class instantiation

