/*
 * bfVFS : vfs/Core/File/vfs_file.cpp
 *  - File with read/read-write access
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

#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Core/vfs_os_functions.h>

#include <vfs/Aspects/vfs_logging.h>
#include <vfs/Aspects/vfs_settings.h>

#ifndef WIN32
#	include <cstring>
#	include <errno.h>
#endif

#define ERROR_FILE(msg)		(_BS(msg) << L" : " << this->getPath() << _BS::wget)

static inline bool hasAttrib(vfs::UInt32 const& attrib, vfs::UInt32 Attribs)
{
	return attrib == (attrib & Attribs);
}

static inline void copyAttributes(vfs::UInt32 osFileAttributes, vfs::UInt32& vfsFileAttributes)
{
	if(vfs::OS::FileAttributes::ATTRIB_ARCHIVE == (vfs::OS::FileAttributes::ATTRIB_ARCHIVE & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_ARCHIVE;
	}
	if(vfs::OS::FileAttributes::ATTRIB_COMPRESSED == (vfs::OS::FileAttributes::ATTRIB_COMPRESSED & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_COMPRESSED;
	}
	if(vfs::OS::FileAttributes::ATTRIB_DIRECTORY == (vfs::OS::FileAttributes::ATTRIB_DIRECTORY & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_DIRECTORY;
	}
	if(vfs::OS::FileAttributes::ATTRIB_HIDDEN == (vfs::OS::FileAttributes::ATTRIB_HIDDEN & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_HIDDEN;
	}
	if(vfs::OS::FileAttributes::ATTRIB_NORMAL == (vfs::OS::FileAttributes::ATTRIB_NORMAL & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_NORMAL;
	}
	if(vfs::OS::FileAttributes::ATTRIB_OFFLINE == (vfs::OS::FileAttributes::ATTRIB_OFFLINE & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_OFFLINE;
	}
	if(vfs::OS::FileAttributes::ATTRIB_READONLY == (vfs::OS::FileAttributes::ATTRIB_READONLY & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_READONLY;
	}
	if(vfs::OS::FileAttributes::ATTRIB_SYSTEM == (vfs::OS::FileAttributes::ATTRIB_SYSTEM & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_SYSTEM;
	}
	if(vfs::OS::FileAttributes::ATTRIB_TEMPORARY == (vfs::OS::FileAttributes::ATTRIB_TEMPORARY & osFileAttributes))
	{
		vfsFileAttributes |= vfs::FileAttributes::ATTRIB_TEMPORARY;
	}
}

#ifdef WIN32
	static inline DWORD _seekDir(vfs::IBaseFile::ESeekDir seekDir)
	{
		if(seekDir == vfs::IBaseFile::SD_BEGIN)
		{
			return FILE_BEGIN;
		}
		else if(seekDir == vfs::IBaseFile::SD_CURRENT)
		{
			return FILE_CURRENT;
		}
		else if(seekDir == vfs::IBaseFile::SD_END)
		{
			return FILE_END;
		}
		VFS_THROW(_BS(L"Unknown seek direction [") << seekDir << L"]" << _BS::wget);
	}
#else
	static inline int _seekDir(vfs::IBaseFile::ESeekDir seekDir)
	{
		if(seekDir == vfs::IBaseFile::SD_BEGIN)
		{
			return SEEK_SET;
		}
		else if(seekDir == vfs::IBaseFile::SD_CURRENT)
		{
			return SEEK_CUR;
		}
		else if(seekDir == vfs::IBaseFile::SD_END)
		{
			return SEEK_END;
		}
		VFS_THROW(_BS(L"Unknown seek direction [") << seekDir << L"]" << _BS::wget);
	}
#endif

template<typename WriteType>
vfs::TFile<WriteType>::TFile(vfs::Path const& filename)
: BaseClass_t(filename), m_isOpen_read(false), m_file(0)
{
}

template<typename WriteType>
vfs::TFile<WriteType>::~TFile()
{
	//VFS_LOCK(m_mutex);
#ifndef WIN32
	if(m_file)clearerr(m_file);
#endif
	if(m_isOpen_read)
	{
		this->close();
	}
}

template<typename WriteType>
void vfs::TFile<WriteType>::close()
{
	//VFS_LOCK(m_mutex);
	if(m_file)
	{
#ifdef WIN32
		if(!CloseHandle(m_file))
		{
			DWORD err = GetLastError();
			if(err != NO_ERROR)
			{
				VFS_THROW( ERROR_FILE(_BS(L"Could not close file : ") << err << _BS::wget) );
			}
		}
#else
		//clearerr(m_file);
		fflush(m_file);
		int error = fclose(m_file);
		if(error)
		{
			String err = strerror(errno);
			VFS_THROW( ERROR_FILE(_BS(L"Could not close file : ") << err << _BS::wget) );
		}
#endif
		m_file = NULL;
	}
	m_isOpen_read = false;
}

template<typename WriteType>
vfs::FileAttributes vfs::TFile<WriteType>::getAttributes()
{
	//VFS_LOCK(m_mutex);
	Path fullpath;
	VFS_THROW_IFF(this->_getRealPath(fullpath), ERROR_FILE(L""));

	vfs::UInt32 osFileAttributes = 0;
	OS::FileAttributes fa;
	VFS_THROW_IFF( fa.getFileAttributes(fullpath, osFileAttributes), ERROR_FILE(L"Could not read file attributes") );

	vfs::UInt32 _attribs = FileAttributes::ATTRIB_INVALID;
	copyAttributes(osFileAttributes, _attribs);

	if(!this->implementsWritable())
	{
		_attribs &= ~FileAttributes::ATTRIB_NORMAL;
		_attribs |=  FileAttributes::ATTRIB_READONLY;
	}

	return FileAttributes(_attribs, FileAttributes::LT_NONE);
}

template<typename WriteType>
bool vfs::TFile<WriteType>::isOpenRead()
{
	//VFS_LOCK(m_mutex);
	return m_isOpen_read;
}

template<typename WriteType>
bool vfs::TFile<WriteType>::_internalOpenRead(vfs::Path const& path)
{
	//VFS_LOCK(m_mutex);
	if( m_isOpen_read )
		return true;
#ifdef WIN32
	m_file = Settings::getUseUnicode() ?
		CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) :
		CreateFileA(String::narrow(path.c_str(),path.length()).c_str(),GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	DWORD err = GetLastError();
	if(err != NO_ERROR && err != ERROR_ALREADY_EXISTS)
	{
		m_file = NULL;
		VFS_LOG_ERROR( ERROR_FILE(_BS(L"Error when opening file - ") << err << _BS::wget) );
		return m_isOpen_read = false;
	}
	return m_isOpen_read = true;
#else
	m_file = fopen(path.to_string().c_str(), "r");
	if(m_file == NULL)
	{
		String err = strerror(errno);
		VFS_LOG_ERROR( ERROR_FILE(_BS(L"Error when opening file - ") << err << _BS::wget) );
	}
	return m_isOpen_read = (m_file != NULL);
#endif
}

template<typename WriteType>
bool vfs::TFile<WriteType>::openRead()
{
	//VFS_LOCK(m_mutex);
	return _internalOpenRead(this->m_filename);
}

template<typename WriteType>
vfs::size_t vfs::TFile<WriteType>::read(vfs::Byte* pData, vfs::size_t bytesToRead)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	DWORD has_read = 0;
	DWORD toRead   = (DWORD)bytesToRead;
	if(!ReadFile(m_file, pData, toRead, &has_read, NULL))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR && err != ERROR_HANDLE_EOF)
		{
			VFS_THROW( ERROR_FILE(_BS(L"read error : ") << err << _BS::wget) );
		}
	}
#else
	size_t has_read = fread(pData,1,bytesToRead,m_file);
	if(has_read != bytesToRead)
	{
		int error = ferror(m_file);
		if(error)
		{
			VFS_THROW( ERROR_FILE(_BS(L"read error : ") << error << _BS::wget) );
		}
		clearerr(m_file);
	}
#endif
	return has_read;
}

template<typename WriteType>
vfs::size_t vfs::TFile<WriteType>::getReadPosition()
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	LARGE_INTEGER current_position,zero;
	zero.QuadPart = 0;
	if(!SetFilePointerEx(m_file, zero, &current_position, FILE_CURRENT))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
	return (vfs::size_t)current_position.QuadPart;
#else
	long int pos = ftell(m_file);
	if(pos == -1L)
	{
		int error = ferror(m_file);
		if(error)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << error << _BS::wget) );
		}
	}
	return (vfs::size_t)pos;
#endif
}

template<typename WriteType>
void vfs::TFile<WriteType>::setReadPosition(vfs::size_t positionInBytes)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	LARGE_INTEGER pos;
	pos.QuadPart = positionInBytes;
	if(!SetFilePointerEx(m_file, pos, NULL, FILE_BEGIN))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
#else
	int error = fseek(m_file,positionInBytes,SEEK_SET);
	if(error)
	{
		String err = strerror(errno);
		VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
	}
#endif
}

template<typename WriteType>
void vfs::TFile<WriteType>::setReadPosition(vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_read, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	DWORD ioSeekDir;
	VFS_TRYCATCH_RETHROW( ioSeekDir = _seekDir(seekDir), ERROR_FILE(L"seek error"));

	LARGE_INTEGER offset;
	offset.QuadPart = offsetInBytes;
	if(!SetFilePointerEx(m_file, offset, NULL, ioSeekDir))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
#else
	int ioSeekDir;
	VFS_TRYCATCH_RETHROW( ioSeekDir = _seekDir(seekDir), ERROR_FILE(L"seek error"));
	int error = fseek(m_file, offsetInBytes, ioSeekDir);
	if(error)
	{
		String err = strerror(errno);
		VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
	}
#endif
}

/********************************************************************************/
/********************************************************************************/


vfs::File::File(vfs::Path const& filename)
: BaseClass_t(filename), m_isOpen_write(false)
{
}

vfs::File::~File()
{
	//VFS_LOCK(m_mutex);
	if(m_isOpen_read || m_isOpen_write)
	{
		this->close();
	}
}

void vfs::File::close()
{
	//VFS_LOCK(m_mutex);
	BaseClass_t::close();
	m_isOpen_write = false;
}

bool vfs::File::deleteFile()
{
	//VFS_LOCK(m_mutex);
	this->close();
	return OS::deleteRealFile(m_filename);
}

bool vfs::File::isOpenWrite()
{
	//VFS_LOCK(m_mutex);
	return m_isOpen_write;
}

bool vfs::File::_internalOpenWrite(vfs::Path const& path, bool createWhenNotExist, bool truncate)
{
	//VFS_LOCK(m_mutex);
	if( m_isOpen_write )
		return true;
#ifdef WIN32
	DWORD Mode = 0;
	if(createWhenNotExist)
	{
		Mode |= OPEN_ALWAYS;
	}
	else
	{
		Mode |= OPEN_EXISTING;
	}
	if(truncate)
	{
		Mode |= TRUNCATE_EXISTING;
	}

	m_file = Settings::getUseUnicode() ?
		CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,Mode,FILE_ATTRIBUTE_NORMAL,NULL) :
		CreateFileA(String::narrow(path.c_str(),path.length()).c_str(),GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,Mode,FILE_ATTRIBUTE_NORMAL,NULL);
	DWORD err = GetLastError();
	if(truncate && err == ERROR_FILE_NOT_FOUND)
	{
		Mode = CREATE_ALWAYS;
		m_file = Settings::getUseUnicode() ?
			CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,Mode,FILE_ATTRIBUTE_NORMAL,NULL) :
			CreateFileA(String::narrow(path.c_str(),path.length()).c_str(),GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,NULL,Mode,FILE_ATTRIBUTE_NORMAL,NULL);
		err = GetLastError();
	}
	if(err != NO_ERROR && err != ERROR_ALREADY_EXISTS)
	{
		m_file = NULL;
		VFS_LOG_ERROR( ERROR_FILE(_BS(L"Error when opening file - ") << err <<  _BS::wget) );
		return m_isOpen_write = false;
	}
	return m_isOpen_write = true;
#else
	m_file = fopen(path.to_string().c_str(), "r+");
	if( (!m_file && createWhenNotExist) || (m_file && truncate))
	{
		// close file again or you might get a "too many open files" error if you try to open and close the same file in a fast loop
		if(m_file) fclose(m_file);

		m_file = fopen(path.to_string().c_str(), "w");
		if(m_file ==  NULL)
		{
			String err = strerror(errno);
			VFS_LOG_ERROR( ERROR_FILE(_BS(L"Error when opening file - ") << err << _BS::wget) );
		}
	}

	return m_isOpen_write = (m_file != NULL);
#endif
}

bool vfs::File::openWrite(bool createWhenNotExist, bool truncate)
{
	//VFS_LOCK(m_mutex);
	return _internalOpenWrite(m_filename, createWhenNotExist, truncate);
}

vfs::size_t vfs::File::write(const vfs::Byte* data, vfs::size_t bytesToWrite)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_write, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	DWORD has_written = 0;
	if(!WriteFile(m_file, data, bytesToWrite, &has_written, NULL))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW(_BS(L"write error : ") << err << _BS::wget);
		}
	}
#else
	size_t has_written = fwrite(data, 1, bytesToWrite, m_file);
	if(has_written != bytesToWrite)
	{
		int error = ferror(m_file);
		if(error)
		{
			VFS_THROW(_BS(L"write error : ") << error << _BS::wget);
		}
	}

#endif
	return (vfs::size_t)has_written;
}

vfs::size_t vfs::File::getWritePosition()
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_write, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	LARGE_INTEGER current_position, zero;
	zero.QuadPart = 0;
	if(!SetFilePointerEx(m_file, zero, &current_position, FILE_CURRENT))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
	return (vfs::size_t)current_position.QuadPart;
#else
	long int pos = ftell(m_file);
	if(pos == -1L)
	{
		int error = ferror(m_file);
		if(error)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << error << _BS::wget) );
		}
	}
	return (vfs::size_t)pos;
#endif
}

void vfs::File::setWritePosition(vfs::size_t positionInBytes)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_write, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	LARGE_INTEGER pos;
	pos.QuadPart = positionInBytes;
	if(!SetFilePointerEx(m_file, pos, NULL, FILE_CURRENT))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
#else
	int error = fseek(m_file,positionInBytes,SEEK_SET);
	if(error)
	{
		String err = strerror(errno);
		VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
	}
#endif
}

void vfs::File::setWritePosition(vfs::offset_t offsetInBytes, vfs::IBaseFile::ESeekDir seekDir)
{
	//VFS_LOCK(m_mutex);
	VFS_THROW_IFF( m_isOpen_write, ERROR_FILE(L"file not opened") );
#ifdef WIN32
	DWORD ioSeekDir;
	VFS_TRYCATCH_RETHROW( ioSeekDir = _seekDir(seekDir), ERROR_FILE(L"seek error"));

	LARGE_INTEGER offset;
	offset.QuadPart = offsetInBytes;
	if(!SetFilePointerEx(m_file, offset, NULL, ioSeekDir))
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
		}
	}
#else
	int ioSeekDir;
	VFS_TRYCATCH_RETHROW( ioSeekDir = _seekDir(seekDir), ERROR_FILE(L"seek error"));
	int error = fseek(m_file, offsetInBytes, ioSeekDir);
	if(error)
	{
		String err = strerror(errno);
		VFS_THROW( ERROR_FILE(_BS(L"set position error : ") << err << _BS::wget) );
	}
#endif
}

template<typename T>
vfs::size_t vfs::TFile<T>::getSize()
{
	//VFS_LOCK(m_mutex);
#ifdef WIN32
	bool was_open = false;
	if(m_file)
	{
		was_open = true;
	}
	else
	{
		VFS_THROW_IFF(this->openRead(),ERROR_FILE(L"could not open file"));
	}
	vfs::size_t size;
#	ifdef _MSC_VER
		LARGE_INTEGER li_size;
		if(!GetFileSizeEx(m_file, &li_size))
		{
			DWORD err = GetLastError();
			if(err != NO_ERROR)
			{
				VFS_THROW( ERROR_FILE(_BS(L"get size error : ") << err << _BS::wget) );
			}
		}
		size = (vfs::size_t)li_size.QuadPart;
#	else
		DWORD low_part, high_part;
		low_part = GetFileSize(m_file, &high_part);
		if(low_part == INVALID_FILE_SIZE)
		{
			DWORD err = GetLastError();
			if(err != NO_ERROR)
			{
				VFS_THROW( ERROR_FILE(_BS(L"get size error : ") << err << _BS::wget) );
			}
		}
		size = low_part;
#	endif
	if(!was_open)
	{
		this->close();
	}
	return size;
#else
	// if file was already opened, keep it open, otherwise close it
	bool wasOpen = (m_file != NULL);

	VFS_THROW_IFF( wasOpen || this->openRead(), ERROR_FILE(L"could not open file") )

	// save current position
	long int current_position = ftell(m_file);

	// move to end of the file
	fseek(m_file, 0, SEEK_END);
	long int file_size = ftell(m_file);

	// move to old position
	fseek(m_file, current_position, SEEK_SET);
	VFS_THROW_IFF(current_position == ftell(m_file), ERROR_FILE(L"could not restore seek position"));

	if(!wasOpen)
	{
		this->close();
	}
	return (vfs::size_t)file_size;
#endif
}

/******************************************************************/
/******************************************************************/

template class vfs::TFile<vfs::IWriteType>;
template class vfs::TFile<vfs::IWritable>;

