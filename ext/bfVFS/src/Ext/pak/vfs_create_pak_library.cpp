/*
 * bfVFS : vfs/Ext/7z/vfs_create_pak_library.cpp
 *  - writes uncompressed PAK archive files
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

#include <cstring>

#include <vfs/Core/vfs_types.h>

#include <vfs/Ext/pak/vfs_create_pak_library.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Ext/pak/vfs_pak_header.h>

/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/

vfs::CreatePAKLibrary::CreatePAKLibrary()
: m_pLibFile(NULL)
{
}

vfs::CreatePAKLibrary::~CreatePAKLibrary()
{
	m_pLibFile = NULL;
}

bool vfs::CreatePAKLibrary::addFile(vfs::ReadableFile_t* file)
{
	if(!file)
	{
		// at least nothing bad happened
		return true;
	}
	try
	{
		OpenReadFile infile(file);
	}
	catch(std::exception &ex)
	{
		std::wstringstream wss;
		wss << L"Could not open File \"" << file->getPath()() << L"\"";
		VFS_RETHROW(wss.str().c_str(), ex);
	}

	vfs::Path dirname, filename;
	file->getPath().splitLast(dirname, filename);

	typedef std::set<vfs::ReadableFile_t*> UFiles_t;
	UFiles_t& UF = m_DFM[dirname]._unique_files;
	if(UF.find(file) != UF.end())
	{
		// don't add files that are already included
	}
	else
	{
		DirInfo::FileInfo fi;
		fi.file = file;

		m_DFM[dirname]._files.push_back(fi);
		UF.insert(file);
	}

	return true;
}

bool vfs::CreatePAKLibrary::writeLibrary(vfs::Path const& lib_name)
{
	OpenWriteFile outfile(lib_name,true);
	return writeLibrary(outfile.file());
}

bool vfs::CreatePAKLibrary::writeLibrary(vfs::WritableFile_t* file)
{
	if(!file)
	{
		return false;
	}
	if(m_DFM.empty())
	{
		return false;
	}
	m_pLibFile = file;
	if(!m_pLibFile->isOpenWrite() && !m_pLibFile->openWrite(true,true))
	{
		return false;
	}

	vfs::UInt64 header_size = sizeof(pak::HEADER); // signature and stuff

	// computer header data : directory + filename strings and lengths, file offsets
	vfs::UInt64 file_offset = 0;

	DirectoryFilesMap_t::iterator diter = m_DFM.begin();
	for(; diter != m_DFM.end(); ++diter)
	{
		std::string& str = diter->second._dir.dirname;
		
		str  = "\\";
		str += diter->first.to_string();
		str += "\\";

		header_size += sizeof(pak::DIRECTORY);
		header_size += str.length() + 1; // count trailing 0s too

		DirInfo::Files_t::iterator fiter = diter->second._files.begin();
		for(; fiter != diter->second._files.end(); ++fiter)
		{
			DirInfo::FileInfo& fi = *fiter;
			
			fi.filename  = fi.file->getName().to_string();
			fi.size      = fi.file->getSize();
			fi.offset    = file_offset; // come back later and add the header size to the offsets

			file_offset += fi.size;

			header_size += sizeof(pak::FILE);
			header_size += fi.filename.length() + 1;
		}
	}

	// write header
	vfs_pak::DB db;
	db.allocate_header(header_size);

	db.header = (pak::HEADER*)db.header_data;
	db.header->header_size = header_size;
	db.header->num_dirs = m_DFM.size();
	memcpy(db.header->signature                       , pak::SIGNATURE , sizeof(pak::SIGNATURE ));
	memcpy(db.header->signature+sizeof(pak::SIGNATURE), pak::SIGNATUREv, sizeof(pak::SIGNATUREv));

	vfs::Byte* position = db.header_data + sizeof(pak::HEADER);

	DirectoryFilesMap_t::iterator dir_it = m_DFM.begin();
	for(vfs::UInt32 dir=1; dir_it != m_DFM.end(); ++dir_it, dir++)
	{
		// write directory structure
		pak::DIRECTORY d;
		d.id            = dir;
		d.num_files     = dir_it->second._files.size();
		d.dirname_bytes = dir_it->second._dir.dirname.length() + 1;

		// check if writing directory data would go over buffer boundary
		size_t to_write_d = sizeof(pak::DIRECTORY) + d.dirname_bytes;
		if( ((position - db.header_data) + to_write_d) <= header_size)
		{
			memcpy(position, &d, sizeof(pak::DIRECTORY));
			position += sizeof(pak::DIRECTORY);

			memcpy(position, dir_it->second._dir.dirname.c_str(), d.dirname_bytes - 1);
			position += d.dirname_bytes - 1;
			*position++ = '\0';
		}
		else
		{
			VFS_THROW(L"position error!");
		}

		// write files
		DirInfo::Files_t::iterator file_it = dir_it->second._files.begin();
		for(; file_it != dir_it->second._files.end(); ++file_it)
		{
			pak::FILE pakfile;
			pak::FILE_PROPS* props = pakfile.props();

			props->size     = file_it->size;
			props->offset   = file_it->offset + header_size;
			props->checksum = 0xafbeaddedeadbeaf;

			vfs::UInt32& filename_len = *(vfs::UInt32*)&pakfile;
			filename_len = file_it->filename.length() + 1;

			// check if writing file data would go over buffer boundary
			size_t to_write_f = sizeof(pak::FILE) + filename_len;
			if( ((position - db.header_data) + to_write_f) <= header_size)
			{
				memcpy(position, &pakfile, sizeof(pak::FILE));
				position += sizeof(pak::FILE);

				memcpy(position, file_it->filename.c_str(), filename_len - 1);
				position += filename_len - 1;
				*position++ = '\0';
			}
			else
			{
				VFS_THROW(L"position error!");
			}
		}
	}

	// write header
	m_pLibFile->write(db.header_data, header_size);

	// write files in 20 MB blocks
	{
		typedef std::vector<vfs::Byte> ByteVector_t;

		vfs::size_t BUFFER_SIZE = 20*1024*1024; // 20 MB buffer
		ByteVector_t data( BUFFER_SIZE );
		size_t current = 0;


		DirectoryFilesMap_t::iterator diter = m_DFM.begin();
		for(; diter != m_DFM.end(); ++diter)
		{
			DirInfo::Files_t::iterator fiter = diter->second._files.begin();
			for(; fiter != diter->second._files.end(); ++fiter)
			{
				vfs::size_t SIZE = fiter->file->getSize();
				if(current + SIZE > BUFFER_SIZE)
				{
					// write what is currently in the buffer and then clear it
					m_pLibFile->write(&data[0], current);

					memset(&data[0], 0, BUFFER_SIZE);
					current = 0;
				}
				vfs::OpenReadFile rw(fiter->file);
				VFS_THROW_IFF(SIZE == rw.file()->read(&data[current], (vfs::size_t)SIZE), L"");
				current += SIZE;
			}
		}

		// write what is left in the buffer
		m_pLibFile->write(&data[0], current);
	}

	m_pLibFile->close();

	return true;
}

#endif // VFS_WITH_PAK
