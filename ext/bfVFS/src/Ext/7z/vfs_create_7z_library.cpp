/*
 * bfVFS : vfs/Ext/7z/vfs_create_7z_library.cpp
 *  - writes uncompressed 7z archive file
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

#include <cstring>

#include <vfs/Core/vfs_types.h>

#include <vfs/Ext/7z/vfs_create_7z_library.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs_debug.h>

#include <utf8.h>

namespace sz
{
extern "C"
{
#include <7zCrc.h>
#include <7z.h>
//#include "Archive/7z/7zIn.h"
}
};

#include <vector>
#include <sstream>

/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/
namespace szExt
{
	inline ::size_t WRITEBYTE(std::ostream& out, sz::Byte const& value)
	{
		out.write((char*)&value,sizeof(sz::Byte));
		return 1;
	}
	template<typename T>
	inline ::size_t WRITEALL(std::ostream& out, T const& value)
	{
		out.write((char*)&value,sizeof(T));
		return sizeof(T);
	}
	template<typename T>
	inline ::size_t WRITEBUFFER(std::ostream& out, T* value, ::size_t num_elements)
	{
		out.write((char*)value, num_elements*sizeof(T));
		return num_elements*sizeof(T);
	}

	/**
	 *  "compress" numbers by removing heading zero-bytes
	 *  - add additional byte that represents a bit-vector of bytes within a 64-bit/8-byte number
	 *  - if the number is smaller than 128, use the extra byte to store the value
	 */
	template<typename T>
	inline ::size_t WRITE(std::ostream& out, T const& value)
	{
		::size_t    count     = 0;
		sz::Byte    data[8];
		sz::Byte    firstByte = 0;
		sz::Byte*   b         = (sz::Byte*)&value;
		::size_t    SIZE      = sizeof(T);
		b                    += SIZE-1;

		vfs::Int32 i;
		for(i = (vfs::Int32)(SIZE-1); i>=0; --i)
		{
			if( (*b & 0xFF) != 0)
			{
				break;
			}
			b--;
		}
		if(i < 0)
		{
			count += WRITEBYTE(out,0);
			return count;
		}
		if(i == 0)
		{
			if(*b >= 0x80)
			{
				count += WRITEBYTE(out,0x80);
			}
			count += WRITEBYTE(out,*b);
			return count;
		}
		vfs::Int32 num = 0;
		for(;i >= 0; --i)
		{
			firstByte |= 1 << (8 - i - 1);
			data[i] = *b--;
			num++;
		}
		count += WRITEBYTE(out,firstByte);
		count += WRITEBUFFER(out,data,num);
		return count;
	}
}
/******************************************************************************************/
/******************************************************************************************/
/******************************************************************************************/

vfs::Create7zLibrary::Create7zLibrary()
: ICreateLibrary(), m_pLibFile(NULL), m_total_file_size(0)
{
	sz::CrcGenerateTable();
}

vfs::Create7zLibrary::~Create7zLibrary()
{
	m_pLibFile = NULL;

	m_file_list.clear();
	m_file_info.clear();
	m_dir_info.clear();
}

bool vfs::Create7zLibrary::addFile(vfs::ReadableFile_t* file)
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

	OpenReadFile infile(file);

	SFileInfo fi;
	Path filename = file->getPath();
	fi.name       = filename.c_wcs();
	fi.size       = file->getSize();
	if(m_file_info.empty())
	{
		fi.offset = 0;
	}
	else
	{
		SFileInfo const& fic = m_file_info.back();
		fi.offset = fic.offset + fic.size;
	}

	typedef std::vector<vfs::Byte> tByteVector;
	tByteVector data( (tByteVector::size_type)fi.size );
	VFS_THROW_IFF(fi.size == file->read(&data[0], (vfs::size_t)fi.size), L"");
	fi.CRC = sz::CrcCalc(&data[0],(::size_t)fi.size);
	m_file_info.push_back(fi);

	m_total_file_size += fi.size;
	m_file_list.push_back(file);

	Path path, dummy;
	filename.splitLast(path, dummy);

	if(!path.empty())
	{
		DirInfo_t::iterator it_find = m_dir_info.find(path.c_wcs());
		if(it_find == m_dir_info.end())
		{
			SFileInfo dir;
			dir.name             = path.c_wcs();
			dir.offset           = 0;
			dir.size             = 0;
			dir.time_creation    = 0;
			dir.time_last_access = 0;
			dir.time_write       = 0;
			m_dir_info.insert(std::make_pair(dir.name,dir));
		}
	}
	return true;
}

bool vfs::Create7zLibrary::writeLibrary(vfs::Path const& lib_name)
{
	OpenWriteFile outfile(lib_name,true);
	return writeLibrary(outfile.file());
}

bool vfs::Create7zLibrary::writeLibrary(vfs::WritableFile_t* file)
{
	if(!file)
	{
		return false;
	}
	if(m_file_info.empty())
	{
		return false;
	}
	m_pLibFile = file;
	if(!m_pLibFile->isOpenWrite() && !m_pLibFile->openWrite(true,true))
	{
		return false;
	}
	//
	writeNextHeader(m_info_stream);
	//
	std::stringstream ssSigHeader;
	writeSignatureHeader(ssSigHeader);
	//
	m_pLibFile->write(ssSigHeader.str().c_str()		, (vfs::size_t)ssSigHeader.str().length());
	//
	{
		typedef std::vector<vfs::Byte> ByteVector_t;

		vfs::size_t BUFFER_SIZE = 20*1024*1024; // 20 MB buffer
		ByteVector_t data( BUFFER_SIZE );
		size_t current = 0;

		std::list<vfs::ReadableFile_t::SP>::iterator it = m_file_list.begin();
		for(; it != m_file_list.end(); ++it)
		{
			vfs::size_t SIZE = (*it)->getSize();
			if(current + SIZE > BUFFER_SIZE)
			{
				// write what is currently in the buffer and then clear it
				m_pLibFile->write(&data[0], current);

				memset(&data[0], 0, BUFFER_SIZE);
				current = 0;
			}
			vfs::OpenReadFile rw(*it);
			VFS_THROW_IFF(SIZE == rw.file()->read(&data[current], (vfs::size_t)SIZE), L"");
			current += SIZE;
		}

		// write what is left in the buffer
		m_pLibFile->write(&data[0], current);
	}
	//
	m_pLibFile->write(m_info_stream.str().c_str()	, (vfs::size_t)m_info_stream.str().length());

	return true;
}

/**************************************************************************************/

bool vfs::Create7zLibrary::writeSignatureHeader(std::ostream& out)
{
	::size_t count=0;

	// #define k7zSignatureSize -> not in namespace sz
	count += szExt::WRITEBUFFER(out, sz::k7zSignature, k7zSignatureSize);

	sz::Byte Major = 0, Minor = 2;
	count += szExt::WRITEALL(out, (sz::Byte)Major );
	count += szExt::WRITEALL(out, (sz::Byte)Minor );

	sz::UInt32 StartHeaderCRC, NextHeaderCRC;

	SFileInfo const& fi         = m_file_info.back();
	sz::UInt64 NextHeaderOffset = fi.offset + fi.size;
	sz::UInt64 NextHeaderSize   = m_info_stream.str().length()*sizeof(char);
	NextHeaderCRC               = sz::CrcCalc(m_info_stream.str().c_str(), (::size_t)NextHeaderSize);

	std::stringstream sstemp;
	count += szExt::WRITEALL(sstemp, (sz::UInt64)NextHeaderOffset);
	count += szExt::WRITEALL(sstemp, (sz::UInt64)NextHeaderSize);
	count += szExt::WRITEALL(sstemp, (sz::UInt32)NextHeaderCRC);
	StartHeaderCRC = sz::CrcCalc(sstemp.str().c_str(), sstemp.str().length()*sizeof(char));

	count += szExt::WRITEALL(out, (sz::UInt32)StartHeaderCRC );
	out << sstemp.str();

	return true;
}
bool vfs::Create7zLibrary::writeNextHeader(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdHeader);

	// this->WriteArchiveProperties(out);

	// this->WriteAdditionalStreamsInfo(out)

	//
	this->writeMainStreamsInfo(out);

	//
	this->writeFilesInfo(out);

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );

	return true;
}

bool vfs::Create7zLibrary::writeMainStreamsInfo(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdMainStreamsInfo );

	this->writePackInfo(out);

	this->writeUnPackInfo(out);

	this->writeSubStreamsInfo(out);

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );

	return true;
}


bool vfs::Create7zLibrary::writePackInfo(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdPackInfo );
	szExt::WRITE(out, (sz::UInt64)0 ); // data offset
	szExt::WRITE(out, (sz::UInt32)m_file_info.size() );

	szExt::WRITE(out, (sz::Byte)sz::k7zIdSize );
	std::list<SFileInfo>::iterator it = m_file_info.begin();
	for(;it != m_file_info.end(); ++it)
	{
		szExt::WRITE(out, (sz::UInt64)it->size );
	}

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );
	return true;
}
bool vfs::Create7zLibrary::writeUnPackInfo(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdUnpackInfo );

	szExt::WRITE(out, (sz::Byte)sz::k7zIdFolder );
	szExt::WRITE(out, (sz::UInt64)m_file_info.size() );
	szExt::WRITE(out, (sz::Byte)0 ); // External

	std::list<SFileInfo>::iterator fit = m_file_info.begin();
	for(;fit != m_file_info.end(); ++fit)
	{
		this->writeFolder(out);
	}

	szExt::WRITE(out, (sz::Byte)sz::k7zIdCodersUnpackSize );
	fit = m_file_info.begin();
	for(;fit != m_file_info.end(); ++fit)
	{
		szExt::WRITE(out, (sz::UInt64)fit->size );
	}

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );

	return true;
}
bool vfs::Create7zLibrary::writeSubStreamsInfo(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdSubStreamsInfo );
	szExt::WRITE(out, (sz::Byte)sz::k7zIdCRC );

	szExt::WRITE(out, (sz::Byte)1 ); // early out - all CRCs defined
	std::list<SFileInfo>::iterator fit = m_file_info.begin();
	for(;fit != m_file_info.end(); ++fit )
	{
		szExt::WRITEALL(out, (sz::UInt32)fit->CRC);
	}

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );
	return true;
}

bool vfs::Create7zLibrary::writeFolder(std::ostream& out)
{
	szExt::WRITE(out, (sz::UInt32)1 ); // NumCoders

	szExt::WRITE(out, (sz::Byte)1 ); // MainByte

	szExt::WRITE(out, (sz::Byte)0 ); // Methods

	return true;
}
bool vfs::Create7zLibrary::writeFilesInfo(std::ostream& out)
{
	szExt::WRITE(out, (sz::Byte)sz::k7zIdFilesInfo );

	vfs::UInt64 num_files = (m_file_info.size()+m_dir_info.size());
	szExt::WRITE(out, (sz::UInt64)num_files );

	// empty stream -> pack info in bit-vector
	szExt::WRITE(out, (sz::Byte)sz::k7zIdEmptyStream );
	sz::UInt64 num_empty64   = num_files/8 + (num_files%8 == 0 ? 0 : 1);
	::size_t num_empty       = (::size_t)num_empty64;
	VFS_THROW_IFF(num_empty == num_empty64, L"WTF");

	sz::Byte *empty_vector   = new sz::Byte[num_empty];
	memset(empty_vector,0,(::size_t)num_empty);
	for(::size_t e=m_file_info.size(); e < num_files; ++e)
	{
		::size_t index = e / 8;
		empty_vector[index] |= 1 << (7 - e%8);
	}
	szExt::WRITE(out, (sz::UInt64)num_empty ); // size
	szExt::WRITEBUFFER(out, empty_vector, (::size_t)num_empty);
	delete[] empty_vector;

	// names
	szExt::WRITE(out, (sz::Byte)sz::k7zIdName );
	std::stringstream name_stream;
	size_t count = 0;
	count       += szExt::WRITE(name_stream, (sz::Byte)0 ); // switch
	std::list<SFileInfo>::iterator fit = m_file_info.begin();
	for(;fit != m_file_info.end(); ++fit)
	{
		count += (::size_t)this->writeFileName(name_stream, fit->name);
	}
	std::map<vfs::String::str_t,SFileInfo>::iterator dit = m_dir_info.begin();
	for(;dit != m_dir_info.end(); ++dit)
	{
		count += (::size_t)this->writeFileName(name_stream, dit->second.name);
	}
	szExt::WRITE(out, (sz::UInt64)count ); // size
	szExt::WRITEBUFFER(out, name_stream.str().c_str(), name_stream.str().length() );

	//szExt::WRITE(out, (sz::Byte)sz::k7zIdEmptyFile );
	//szExt::WRITE(out, (sz::Byte)sz::k7zIdCTime ); // create
	//szExt::WRITE(out, (sz::Byte)sz::k7zIdATime ); // last access
	//szExt::WRITE(out, (sz::Byte)sz::k7zIdMTime ); // write

	//szExt::WRITE(out, (sz::Byte)sz::k7zIdWinAttributes );

	szExt::WRITE(out, (sz::Byte)sz::k7zIdEnd );

	return true;
}

vfs::size_t vfs::Create7zLibrary::writeFileName(std::ostream& out, vfs::String const& filename)
{
	::size_t count = 0;

	vfs::UInt32 fname_size = 0;
	std::vector<vfs::UInt16> fname(filename.length()*4);
	if(sizeof(wchar_t) == 4)
	{
		std::string fname_utf8 = filename.utf8();

		vfs::UInt16* end = utf8::utf8to16(&fname_utf8[0], &fname_utf8[fname_utf8.length()], &fname[0]);
		fname_size = end - &fname[0];
	}
	else if(sizeof(wchar_t) == 2)
	{
		memcpy(&fname[0], filename.c_str(), filename.length()*sizeof(wchar_t));
		fname_size = filename.length();
	}
	else
	{
		VFS_THROW(L"Unsupported size of type wchar_t");
	}

	VFS_THROW_IFF(fname_size != 0, L"zero length name");
	count += szExt::WRITEBUFFER(out, &fname[0], fname_size);
	count += szExt::WRITE(out, (sz::Byte)0);
	count += szExt::WRITE(out, (sz::Byte)0);
	return (vfs::size_t)count;
}

#endif // VFS_WITH_7ZIP
