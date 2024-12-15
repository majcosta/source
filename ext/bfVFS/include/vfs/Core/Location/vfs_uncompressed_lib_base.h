/*
 * bfVFS : vfs/Core/Location/vfs_uncompressed_lib_base.h
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

#ifndef _VFS_UNCOMPRESSED_LIB_BASE_H_
#define _VFS_UNCOMPRESSED_LIB_BASE_H_

#include <vfs/Core/Interface/vfs_library_interface.h>
#include <vfs/Core/Interface/vfs_directory_interface.h>
#include <vfs/Core/vfs_init.h>

namespace vfs
{
	class VirtualProfile;

	class VFS_API UncompressedLibraryBase : public ILibrary
	{
	protected:
		typedef TDirectory<ILibrary::WriteType_t>                   LibDir_t;
		typedef std::map<Path, LibDir_t::SP, Path::Less>            DirCatalogue_t;

		typedef std::map<Path, Path, Path::Less>                    PathToPathMap_t;
		struct ProfileMaps
		{
			PathToPathMap_t ProfileToLibrary, LibraryToProfile;
		};

		typedef std::map<const VirtualProfile*, ProfileMaps>        ProfileMap_t;

		struct SFileData
		{
			SFileData(FileType_t* file, vfs::size_t const& fileSize, vfs::size_t const& fileOffset)
				: _fileSize(fileSize), _fileOffset(fileOffset), _currentReadPosition(0), _file(file)
			{};
			vfs::size_t     _fileSize, _fileOffset, _currentReadPosition;
			FileType_t::SP  _file;
		};
		typedef std::map<FileType_t*, SFileData>                                    FileData_t;

		class IterImpl;

		UncompressedLibraryBase(ReadableFile_t* lib_file, Path const& mountpoint);

	public:
		VFS_SMARTPOINTER(UncompressedLibraryBase);

		virtual ~UncompressedLibraryBase();

		/**
		 *  TLocation interface
		 */
		virtual bool            fileExists   (Path const& filename,       const VirtualProfile* profile);
		virtual IBaseFile*      getFile      (Path const& filename,       const VirtualProfile* profile);
		virtual FileType_t*     getFileTyped (Path const& filename,       const VirtualProfile* profile);
		virtual void            getSubDirList(std::list<Path>& sub_dirs,  const VirtualProfile* profile);
		/* *** */

		/**
		 *  ILibrary interface
		 */
		virtual bool            init           () = 0;
		virtual void            closeLibrary   ();

		virtual bool            copyFile       (Path const& filename, BufferFile* file) = 0;

		virtual void            close          (FileType_t *file_handle);
		virtual bool            openRead       (FileType_t *file_handle);
		virtual vfs::size_t     read           (FileType_t *file_handle, vfs::Byte* data, vfs::size_t bytesToRead);

		virtual vfs::size_t     getReadPosition(FileType_t *file_handle);
		virtual void            setReadPosition(FileType_t *file_handle, vfs::size_t   positionInBytes);
		virtual void            setReadPosition(FileType_t *file_handle, vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir);

		virtual vfs::size_t     getSize        (FileType_t *file_handle);

		virtual Iterator        begin          (const VirtualProfile* profile);

		virtual bool            mapProfilePaths(vfs_init::VfsConfig::SP const& config);
		/* *** */

	protected:
		ProfileMap_t            m_profMap;
		DirCatalogue_t          m_dirs;
		FileData_t              m_fileData;
		vfs::UInt32             m_numberOfOpenedFiles;

	private:
		SFileData&              _fileDataFromHandle       (FileType_t* file_handle);
		bool                    _mapProfileDirToLibraryDir(Path& dir, const VirtualProfile* profile) const;
	};
} // end namespace

#endif // _VFS_UNCOMPRESSED_LIB_BASE_H_

