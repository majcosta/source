/*
 * bfVFS : vfs/Ext/7z/vfs_7z_library.cpp
 *  - implements Library interface, creates library object from uncompressed 7-zip archive files
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

#ifdef VFS_WITH_7ZIP

#include <vfs/Ext/7z/vfs_7z_library.h>
#include <vfs/Core/Location/vfs_lib_dir.h>
#include <vfs/Core/File/vfs_buffer_file.h>
#include <vfs/Core/File/vfs_lib_file.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Tools/vfs_parser_tools.h>
#include <vfs/Aspects/vfs_logging.h>

#include <utf8.h>
#include <cstring>

namespace sz
{
extern "C"
{
#include <7z.h>
#include <7zCrc.h>
#include <7zAlloc.h>
}
}

/********************************************************************************************/
/***                                   my 7z extensions                                   ***/
/********************************************************************************************/

namespace szExt
{
	typedef struct SzVfsFile
	{
		vfs::ReadableFile_t*    file;
	} SzVfsFile;

	typedef struct VfsFileInStream
	{
		sz::ISeekInStream       s;
		SzVfsFile               file;
	} VfsFileInStream;


	static sz::SRes VfsFileInStream_Read(void *pp, void *buf, ::size_t *size)
	{
		VfsFileInStream *p = (VfsFileInStream *)pp;
		::size_t to_read   = *size;
		sz::SRes res;
		try
		{
			*size = (::size_t)p->file.file->read((vfs::Byte*)buf,to_read);
			res   = SZ_OK;
		}
		catch(std::exception &ex)
		{
			VFS_LOG_ERROR(ex.what());
			res   = SZ_ERROR_READ;
		}
		return res;
	}

	static sz::SRes VfsFileInStream_Seek(void *pp, sz::Int64 *pos, sz::ESzSeek origin)
	{
		VfsFileInStream *p = (VfsFileInStream *)pp;

		vfs::IBaseFile::ESeekDir eSD;
		switch (origin)
		{
			case sz::SZ_SEEK_SET:
				eSD = vfs::IBaseFile::SD_BEGIN;
				break;
			case sz::SZ_SEEK_CUR:
				eSD = vfs::IBaseFile::SD_CURRENT;
				break;
			case sz::SZ_SEEK_END:
				eSD = vfs::IBaseFile::SD_END;
				break;
			default:
				return SZ_ERROR_PARAM;
		}
		vfs::offset_t _pos = (vfs::offset_t)(*pos);
		sz::SRes res;
		try
		{
			p->file.file->setReadPosition(_pos,eSD);
			*pos = p->file.file->getReadPosition();
			res  = SZ_OK;
		}
		catch(std::exception& ex)
		{
			VFS_LOG_ERROR(ex.what());
			res  = SZ_ERROR_READ;
		}
		return res;
	}

	void VfsFileInStream_CreateVTable(VfsFileInStream *p)
	{
		p->s.Read = VfsFileInStream_Read;
		p->s.Seek = VfsFileInStream_Seek;
	}
}; // end namespace szExt

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

vfs::Uncompressed7zLibrary::Uncompressed7zLibrary(
	ReadableFile_t* lib_file,
	vfs::Path const& mountpoint,
	vfs::ObjBlockAllocator<vfs::LibFile>* allocator)
: vfs::UncompressedLibraryBase(lib_file, mountpoint), _allocator(allocator)
{
}

vfs::Uncompressed7zLibrary::~Uncompressed7zLibrary()
{
}


#define k_Copy 0

sz::UInt64 GetSum(const sz::UInt64 *values, sz::UInt32 index)
{
	sz::UInt64 sum = 0;
	sz::UInt32 i;
	for (i = 0; i < index; i++)
	{
		sum += values[i];
	}
	return sum;
}

bool vfs::Uncompressed7zLibrary::checkSignature(ReadableFile_t* lib_file)
{
	if(!lib_file) return false;

	bool is7zArchive = false;
	bool keepOpen    = lib_file->isOpenRead();

	vfs::size_t readPosition = 0;
	if(keepOpen || lib_file->openRead())
	{
		readPosition = lib_file->getReadPosition();

		vfs::UByte sig7z[6] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};

		vfs::Byte  sig[6]   = {0,0,0,0,0,0};

		VFS_THROW_IFF( 6 == lib_file->read(sig, 6), L"Could not read signature");

		is7zArchive = (memcmp(sig7z, sig, sizeof(sig7z)) == 0);
	}

	if(keepOpen)
	{
		lib_file->setReadPosition(readPosition);
	}
	else
	{
		lib_file->close();
	}
	return is7zArchive;
}


static void openArchive(vfs::ReadableFile_t* file, sz::CSzArEx& db)
{
	szExt::VfsFileInStream archiveStream;
	archiveStream.file.file = file;

	szExt::VfsFileInStream_CreateVTable(&archiveStream);

	sz::CLookToRead lookStream;
	sz::LookToRead_CreateVTable(&lookStream, False);
	lookStream.realStream = &archiveStream.s;
	sz::LookToRead_Init(&lookStream);

	sz::ISzAlloc allocImp;
	allocImp.Alloc     = sz::SzAlloc;
	allocImp.Free      = sz::SzFree;

	sz::ISzAlloc allocTempImp;
	allocTempImp.Alloc = sz::SzAllocTemp;
	allocTempImp.Free  = sz::SzFreeTemp;

	sz::CrcGenerateTable();

	sz::SzArEx_Init(&db);

	sz::SRes res;
	if( SZ_OK != (res = sz::SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp)) )
	{
		VFS_THROW(_BS(L"Could not open 7z archive [") << file->getPath() << L"]" << _BS::wget);
	}
}

bool vfs::Uncompressed7zLibrary::init()
{
	if(m_lib_file.isNull())
	{
		return false;
	}
	try
	{
		OpenReadFile rfile(m_lib_file);
		sz::CSzArEx   db;

		openArchive(m_lib_file, db);

		TDirectory<ILibrary::WriteType_t>::SP lib_dir;
		Path dir, file, dir_path;

		// utf8 filename buffer
		std::string tmp_name;

		const vfs::size_t FBUFFER_SIZE = 1024;

		std::vector<vfs::UInt16> fname_buffer;
		fname_buffer.resize(FBUFFER_SIZE);

		for(vfs::UInt32 i = 0; i < db.db.NumFiles; i++)
		{
			sz::CSzFileItem *f = db.db.Files + i;
			if (f->IsDir)
			{
				continue;
			}
			vfs::size_t fsize = SzArEx_GetFileNameUtf16(&db, i, NULL);
			if(fsize >= fname_buffer.size())
			{
				fname_buffer.resize(fsize + 32);
			}
			fsize = SzArEx_GetFileNameUtf16(&db, i, &fname_buffer[0]);
			fname_buffer[fsize] = 0;

			Path path;
			if (sizeof(wchar_t) == 4)
			{
				// might need up to 4 characters
				if(fsize*4 > tmp_name.length()) { tmp_name.resize(fsize*4); }

				// convert to utf8
				utf8::utf16to8(&fname_buffer[0], &fname_buffer[fsize], tmp_name.begin());

				// and now convert utf8 to utf32 again
				path = tmp_name;
			}
			else if (sizeof(wchar_t) == 2)
			{
				// just cast as sizeof(wchar_t) = sizeof(UInt16)
				path = (wchar_t*)&fname_buffer[0];
			}
			else
			{
				VFS_THROW(_BS(L"Unsupported sized of type wchar_t : ") << sizeof(wchar_t) << _BS::wget);
			}

			// determine offset and size
			size_t     unpackSize  = 0;
			sz::UInt64 startOffset = 0;

			sz::UInt32 folderIndex = db.FileIndexToFolderIndexMap[i];
			if(folderIndex != (sz::UInt32)-1)
			{
				sz::CSzFolder *folder = db.db.Folders + folderIndex;
				unpackSize  = (size_t)sz::SzFolder_GetUnpackSize(folder);
				startOffset = sz::SzArEx_GetFolderStreamPos(&db, folderIndex, 0);
			}

			//const sz::UInt64 *packSizes = db.db.PackSizes + db.FolderStartPackStreamIndex[folderIndex];

			//CSzCoderInfo *coder = &folder->Coders[0];
			//if (coder->MethodID == k_Copy)
			//{
			//	UInt32 si = 0;
			//	UInt64 offset;
			//	UInt64 inSize;
			//	offset = GetSum(packSizes, si);
			//	inSize = packSizes[si];
			//}

			path.splitLast(dir,file);
			dir_path = m_mountPoint;
			if(!dir.empty())
			{
				dir_path += dir;
			}

			// get or create according directory object
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
			LibFile::SP lib_file = LibFile::create(file,lib_dir,this,_allocator);
			// add file to directory
			VFS_THROW_IFF( lib_dir->addFile(lib_file), L"" );

			// link file data struct to file object
			m_fileData.insert(std::make_pair(lib_file, SFileData(lib_file, unpackSize, (vfs::size_t)startOffset)));
		}
		return true;
	}
	catch(std::exception& ex)
	{
		VFS_LOG_ERROR(ex.what());
	}
	return false;
}

bool vfs::Uncompressed7zLibrary::copyFile(Path const& filename, vfs::BufferFile* file)
{
	if(m_lib_file.isNull())
	{
		return false;
	}
	try
	{
		OpenReadFile rfile(m_lib_file);
		sz::CSzArEx  db;

		openArchive(m_lib_file, db);

		// utf8 filename buffer
		std::string tmp_name;

		const vfs::size_t FBUFFER_SIZE = 1024;

		std::vector<vfs::UInt16> fname_buffer;
		fname_buffer.resize(FBUFFER_SIZE);

		for(vfs::UInt32 i = 0; i < db.db.NumFiles; i++)
		{
			sz::CSzFileItem *f = db.db.Files + i;
			if (f->IsDir)
			{
				continue;
			}
			vfs::size_t fsize = SzArEx_GetFileNameUtf16(&db, i, NULL);
			if(fsize >= fname_buffer.size())
			{
				fname_buffer.resize(fsize + 32);
			}
			fsize = SzArEx_GetFileNameUtf16(&db, i, &fname_buffer[0]);
			fname_buffer[fsize] = 0;

			Path sPath;
			if (sizeof(wchar_t) == 4)
			{
				// might need up to 4 characters
				if(fsize*4 > tmp_name.length()) { tmp_name.resize(fsize*4); }

				// convert to utf8
				utf8::utf16to8(&fname_buffer[0], &fname_buffer[fsize], tmp_name.begin());

				// and now convert utf8 to utf32 again
				sPath = tmp_name;
			}
			else if (sizeof(wchar_t) == 2)
			{
				// just cast as sizeof(wchar_t) = sizeof(UInt16)
				sPath = (wchar_t*)&fname_buffer[0];
			}
			else
			{
				VFS_THROW(_BS(L"Unsupported sized of type wchar_t : ") << sizeof(wchar_t) << _BS::wget);
			}

			// Check if current filename matches the one provided by the user.
			// If it does, then copy the file into the provided buffer and leave.
			// Otherwise, iterate until the end and return false.

			if( !matchPattern(filename(), sPath()) ) continue;

			// determine offset and size
			vfs::size_t unpackSize  = 0;
			sz::UInt64  startOffset = 0;

			sz::UInt32 folderIndex = db.FileIndexToFolderIndexMap[i];
			if(folderIndex != (sz::UInt32)-1)
			{
				sz::CSzFolder *folder = db.db.Folders + folderIndex;
				unpackSize  = (size_t)sz::SzFolder_GetUnpackSize(folder);
				startOffset = sz::SzArEx_GetFolderStreamPos(&db, folderIndex, 0);
			}

			if(unpackSize > 0)
			{
				std::vector<vfs::Byte> vBuffer(unpackSize);

				m_lib_file->setReadPosition(startOffset, IBaseFile::SD_BEGIN);

				m_lib_file->read(&vBuffer[0], unpackSize);
				file->write(&vBuffer[0], unpackSize);
			}

			return true;
		}
	}
	catch(std::exception& ex)
	{
		VFS_LOG_ERROR(ex.what());
	}
	return false;
}


#endif // VFS_WITH_7ZIP
