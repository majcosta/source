/*
 * bfVFS : vfs/Ext/slf/vfs_slf_library.cpp
 *  - implements Library interface, creates library object from SLF archive files
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

#ifdef VFS_WITH_SLF

#include <vfs/vfs_config.h>

#include <vfs/Ext/slf/vfs_slf_library.h>
#include <vfs/Core/Location/vfs_lib_dir.h>
#include <vfs/Core/Interface/vfs_directory_interface.h>
#include <vfs/Core/File/vfs_lib_file.h>
#include <vfs/Core/vfs_file_raii.h>

#include <vfs/Aspects/vfs_logging.h>

#include <cstring>

namespace slf
{
	typedef vfs::UInt32 DWORD;
	// copy from WinDef.h
	typedef struct _FILETIME {
		DWORD dwLowDateTime;
		DWORD dwHighDateTime;
	} FILETIME, *PFILETIME, *LPFILETIME;

	typedef void* HANDLE;

	const vfs::UInt32 FILENAME_SIZE     = 256;
	const vfs::UInt32 PATH_SIZE         = 80;

	const vfs::UInt32 FILE_OK           = 0;
	const vfs::UInt32 FILE_DELETED      = 0xff;
	const vfs::UInt32 FILE_OLD          = 1;
	const vfs::UInt32 FILE_DOESNT_EXIST = 0xfe;

	struct  LIBHEADER
	{
		vfs::Byte       sLibName[ FILENAME_SIZE ];
		vfs::Byte       sPathToLibrary[ FILENAME_SIZE ];
		vfs::Int32      iEntries;
		vfs::Int32      iUsed;
		vfs::UInt16     iSort;
		vfs::UInt16     iVersion;
		vfs::UByte      fContainsSubDirectories;
		vfs::Int32      iReserved;
	};

	struct  DIRENTRY
	{
		vfs::Byte       sFileName[ FILENAME_SIZE ];
		vfs::UInt32     uiOffset;
		vfs::UInt32     uiLength;
		vfs::UInt8      ubState;
		vfs::UInt8      ubReserved;
		FILETIME        sFileTime;
		vfs::UInt16     usReserved2;
	};
}; // end namespace slf

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

vfs::SLFLibrary::SLFLibrary(ReadableFile_t* lib_file, vfs::Path const& mountpoint)
: vfs::UncompressedLibraryBase(lib_file, mountpoint)
{};

vfs::SLFLibrary::~SLFLibrary()
{
}

bool vfs::SLFLibrary::checkSignature(ReadableFile_t* lib_file)
{
	if(!lib_file) return false;

	bool isSLFArchive = false;
	bool keepOpen     = lib_file->isOpenRead();

	vfs::size_t readPosition = 0;
	if(keepOpen || lib_file->openRead())
	{
		readPosition = lib_file->getReadPosition();

		slf::LIBHEADER header;

		VFS_THROW_IFF( lib_file->read((vfs::Byte*)&header, sizeof(header)) == sizeof(header), L"Could not read signature");

		vfs::Path libname(header.sLibName);
	
		vfs::String ext;
		libname.extension(ext);

		isSLFArchive = StrCmp::Equal(ext, L"SLF");
	}

	if(keepOpen)
	{
		lib_file->setReadPosition(readPosition);
	}
	else
	{
		lib_file->close();
	}
	return isSLFArchive;
}


bool vfs::SLFLibrary::init()
{
	if(m_lib_file.isNull())
	{
		return false;
	}
	try
	{
		OpenReadFile rfile(m_lib_file);

		slf::LIBHEADER LibFileHeader;
		vfs::size_t    bytesRead = m_lib_file->read((vfs::Byte*)&LibFileHeader, sizeof( slf::LIBHEADER ));
		VFS_THROW_IFF(bytesRead == sizeof( slf::LIBHEADER ), L"");

		Path lib_path;
		//if the library has a path
		if( strlen( (char*)LibFileHeader.sPathToLibrary ) != 0 )
		{
			lib_path = Path( LibFileHeader.sPathToLibrary );
		}
		else
		{
			//else the library name does not contain a path ( most likely either an error or it is the default path )
			lib_path = Path( vfs::Const::EMPTY() );
		}
		if(m_mountPoint.empty())
		{
			m_mountPoint  = lib_path;
		}
		else
		{
			m_mountPoint += lib_path;
		}

		//place the file pointer at the begining of the file headers ( they are at the end of the file )
		m_lib_file->setReadPosition(-( LibFileHeader.iEntries * (vfs::Int32)sizeof(slf::DIRENTRY) ), vfs::IBaseFile::SD_END);

		//loop through the library and determine the number of files that are FILE_OK
		//ie.  so we dont load the old or deleted files
		slf::DIRENTRY dir_entry;
		Path dir, file, dir_path;

		for(vfs::UInt32 uiLoop=0; uiLoop < (vfs::UInt32)LibFileHeader.iEntries; uiLoop++ )
		{
			//read in the file header
			//memset(&DirEntry,0,sizeof(DirEntry));
			bytesRead = m_lib_file->read((Byte*)&dir_entry, sizeof( slf::DIRENTRY ));
			VFS_THROW_IFF(bytesRead == sizeof( slf::DIRENTRY ), L"");

			if( dir_entry.ubState == slf::FILE_OK )
			{
				Path sPath(dir_entry.sFileName);
				sPath.splitLast(dir,file);
				dir_path = m_mountPoint;
				if(!dir.empty())
				{
					dir_path += dir;
				}

				// get or create directory object
				TDirectory<ILibrary::WriteType_t>::SP lib_dir;
				DirCatalogue_t::iterator it = m_dirs.find(dir_path);
				if(it != m_dirs.end())
				{
					lib_dir = it->second;
				}
				else
				{
					lib_dir = VFS_NEW2(LibDirectory, dir_path, dir_path);
					m_dirs.insert(std::make_pair(dir_path,lib_dir));
				}
				// create file
				LibFile::SP lib_file = LibFile::create(file,lib_dir,this);

				// add file to directory
				VFS_THROW_IFF(lib_dir->addFile(lib_file), L"");

				// link file data struct to file object
				m_fileData.insert(std::make_pair(lib_file, SFileData(lib_file, dir_entry.uiLength, dir_entry.uiOffset)));
			} // end if
		} // end for
	}
	catch(std::exception& ex)
	{
		VFS_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

bool vfs::SLFLibrary::copyFile(vfs::Path const& filename, vfs::BufferFile* file)
{
	VFS_LOG_WARNING(L"Class SLFLibrary does not implement method copyFile");
	return false;
}

#endif // VFS_WITH_SLF
