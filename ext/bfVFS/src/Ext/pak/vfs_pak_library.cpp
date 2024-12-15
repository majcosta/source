/*
 * bfVFS : vfs/Ext/pak/vfs_pak_library.cpp
 *  - implements Library interface, creates library object from uncompressed JA-BiA PAK archive files
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

#ifdef VFS_WITH_PAK

#include <vfs/Ext/pak/vfs_pak_library.h>
#include <vfs/Core/Location/vfs_lib_dir.h>
#include <vfs/Core/File/vfs_buffer_file.h>
#include <vfs/Core/File/vfs_lib_file.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Tools/vfs_parser_tools.h>
#include <vfs/Aspects/vfs_logging.h>

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

namespace pak
{
	vfs::UByte SIGNATURE [4] = {0x50, 0x4B, 0x4C, 0x45}; // = P K L E
	vfs::UByte SIGNATUREv[4] = {0x01, 0x00, 0x00, 0x00}; // = 01, maybe version number?
}

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

vfs::PAKLibrary::PAKLibrary(
	ReadableFile_t* lib_file,
	vfs::Path const& mountpoint)
: vfs::UncompressedLibraryBase(lib_file, mountpoint)
{
}

vfs::PAKLibrary::~PAKLibrary()
{
}

bool vfs::PAKLibrary::checkSignature(ReadableFile_t* lib_file)
{
	if(!lib_file) return false;

	bool isPakArchive = false;
	bool keepOpen     = lib_file->isOpenRead();

	vfs::size_t readPosition = 0;
	if(keepOpen || lib_file->openRead())
	{
		readPosition = lib_file->getReadPosition();

		pak::HEADER header;

		VFS_THROW_IFF( lib_file->read((vfs::Byte*)&header, sizeof(header)) == sizeof(header), L"Could not read signature");

		isPakArchive = ( memcmp(pak::SIGNATURE, header.signature, sizeof(pak::SIGNATURE)) == 0 );

		int isPakV   =   memcmp(pak::SIGNATUREv, header.signature+sizeof(pak::SIGNATURE), sizeof(pak::SIGNATUREv));
		if(isPakV != 0)
		{
			VFS_LOG_WARNING(L"Last four bytes of the signature are not as expected");
		}
	}

	if(keepOpen)
	{
		lib_file->setReadPosition(readPosition);
	}
	else
	{
		lib_file->close();
	}
	return isPakArchive;
}


void vfs::PAKLibrary::openArchive(vfs::ReadableFile_t* file, vfs_pak::DB& db)
{
	file->setReadPosition(0);

	pak::HEADER header;
	VFS_THROW_IFF( file->read((vfs::Byte*)&header, sizeof(header)) == sizeof(header), L"Could not read PAK header [1]");

	if(header.num_dirs == 0)
	{
		// there is nothing in the file anyway
		return;
	}

	db.allocate_header(header.header_size);

	file->setReadPosition(0);
	
	VFS_THROW_IFF( file->read((vfs::Byte*)db.header_data, header.header_size) == header.header_size, L"Could not read PAK header [2]");

	db.header = (pak::HEADER*)db.header_data;

	vfs::UInt64 num_dirs = db.header->num_dirs;

	db.dirs.resize(num_dirs);

	vfs::Byte* data = db.header_data + sizeof(pak::HEADER);

	for(vfs::UInt64 dir_i=0; dir_i < num_dirs; ++dir_i)
	{
		pak::DIRECTORY* dir = (pak::DIRECTORY*)data;
		
		vfs_pak::DIR &dd = db.dirs[dir_i];

		dd.dir     = dir;
		dd.dirname = data + sizeof(pak::DIRECTORY);
		dd.files.resize(dir->num_files);
		
		data += sizeof(pak::DIRECTORY) + dir->dirname_bytes;

		for(int file_i=0; file_i < dir->num_files; ++file_i)
		{
			pak::FILE*       file       = (pak::FILE*)data;
			pak::FILE_PROPS* file_props = file->props();

			vfs_pak::FILE &ff = db.dirs[dir_i].files[file_i];
			ff.file_props = file_props;
			ff.filename   = data + sizeof(pak::FILE);

			data += sizeof(pak::FILE) + file->filename_bytes();
		}
	}
}

bool vfs::PAKLibrary::init()
{
	if(m_lib_file.isNull())
	{
		return false;
	}
	try
	{
		OpenReadFile rfile(m_lib_file);
		vfs_pak::DB db;

		openArchive(m_lib_file, db);

		TDirectory<ILibrary::WriteType_t>::SP lib_dir;
		Path dir, file, dir_path;

		for(size_t i = 0; i < db.dirs.size(); i++)
		{
			vfs_pak::DIR& dir = db.dirs[i];

			if(dir.files.size() == 0) continue;

			// add 1 to directory name as it has a "\" as prefix which lets it look like a 
			// root directory on linux and might disturb path concatenation
			Path dir_path = m_mountPoint + Path(dir.dirname+1);

			// get or create directory object
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


			for(size_t j=0; j < dir.files.size(); ++j)
			{
				vfs_pak::FILE& file = dir.files[j];

				Path file_path = file.filename;

				// create file
				LibFile::SP lib_file = LibFile::create(file_path,lib_dir,this);
				// add file to directory
				VFS_THROW_IFF( lib_dir->addFile(lib_file), L"" );

				// link file data struct to file object
				m_fileData.insert(std::make_pair(lib_file, SFileData(lib_file, file.file_props->size, (vfs::size_t)file.file_props->offset)));
			}
		}
		return true;
	}
	catch(std::exception& ex)
	{
		VFS_LOG_ERROR(ex.what());
	}
	return false;
}

bool vfs::PAKLibrary::copyFile(Path const& filename, vfs::BufferFile* file)
{
	if(m_lib_file.isNull())
	{
		return false;
	}
	try
	{
		OpenReadFile rfile(m_lib_file);
		vfs_pak::DB db;

		openArchive(m_lib_file, db);

		if(db.dirs.size() == 0) return false;

		Path dir_pattern, file_pattern;
		filename.splitLast(dir_pattern, file_pattern);

		for(size_t i=0; i < db.dirs.size(); ++i)
		{
			vfs_pak::DIR& dir = db.dirs[i];

			// jump over "\" in directory name
			vfs::String dirname = dir.dirname + 1;

			if( !matchPattern(dir_pattern(), dirname) ) continue;
			
			for(size_t j=0; j < dir.files.size(); ++j)
			{
				vfs_pak::FILE& _file = dir.files[j];

				vfs::String filename = _file.filename;

				if( !matchPattern(file_pattern(), filename) ) continue;

				if(_file.file_props->size > 0)
				{
					vfs::size_t SIZE = _file.file_props->size;
					std::vector<vfs::Byte> vBuffer(SIZE);

					m_lib_file->setReadPosition(_file.file_props->offset, IBaseFile::SD_BEGIN);

					m_lib_file->read(&vBuffer[0], SIZE);
					file->write(&vBuffer[0], SIZE);
				}
			}
		}
	}
	catch(std::exception& ex)
	{
		VFS_LOG_ERROR(ex.what());
	}
	return false;
}


#endif // VFS_WITH_PAK
