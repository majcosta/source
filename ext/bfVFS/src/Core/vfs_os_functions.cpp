/*
 * bfVFS : vfs/Core/os_functions.cpp
 *  - abstractions for OS dependant code
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

#include <vfs/Core/vfs_os_functions.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Core/vfs_string.h>
#include <vfs/Tools/vfs_log.h>

#include <vfs/Aspects/vfs_logging.h>
#include <vfs/Aspects/vfs_settings.h>
#include <vfs/Aspects/vfs_synchronization.h>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <queue>

#ifndef WIN32
#	include "errno.h"
#	include "sys/stat.h"
#endif

#ifdef __FreeBSD__
#	include <sys/sysctl.h>
#endif

class PathIteratorCache
{
	static std::queue<PathIteratorCache*> s_cache;
	static vfs::Aspects::Mutex            s_mutex;
public:

	static PathIteratorCache* Get()
	{
		VFS_LOCK(s_mutex);
		if(s_cache.empty())
		{
			PathIteratorCache* _pic = new PathIteratorCache();
			s_cache.push(_pic);
		}

		PathIteratorCache* pic = s_cache.front();
		s_cache.pop();

		// give cache object to user; he better not forget to return it
		return pic;
	}

	static void Release(PathIteratorCache* pic)
	{
		VFS_LOCK(s_mutex);
		if(pic != NULL)
		{
			(*pic).file_position = 0;
			(*pic).path_cache[0] = 0;
			s_cache.push(pic);
		}
	}

	void init(const char* base_path)
	{
		int i=0;
		char* tmp = path_cache;

		while(*base_path)
		{
			*tmp++ = *base_path++;
			i++;
		}
		file_position = i;
		*tmp = 0;

	}

	void cache_file(const char* file)
	{
		char* tmp = &path_cache[file_position];
		*tmp++ = '/';

		while(*file)
		{
			*tmp++ = *file++;
		}
		*tmp = 0;
	}

	const char* cache_value()
	{
		return path_cache;
	}

private:
	PathIteratorCache() : file_position(0)
	{};

	char path_cache[1024];
	int  file_position;
};

std::queue<PathIteratorCache*> PathIteratorCache::s_cache;
vfs::Aspects::Mutex            PathIteratorCache::s_mutex;

vfs::OS::IterateDirectory::IterateDirectory(vfs::Path const& path, vfs::String const& search_pattern)
{
#ifdef WIN32
	if(!Settings::getUseUnicode())
	{
		std::string s;
		String::narrow((path+search_pattern).c_wcs(), s);
		fSearchHandle = FindFirstFileA(s.c_str(), &fFileInfoA);
	}
	else
	{
		fSearchHandle = FindFirstFileW((path+search_pattern).c_wcs().c_str(), &fFileInfoW);
	}
	if (fSearchHandle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		VFS_THROW(_BS(L"Error accessing path - ") << error << L" : " << (path+search_pattern) << _BS::wget);
	}
#else
	std::string s = path.to_string();
	files         = opendir(s.c_str());
	if(files == NULL)
	{
		String err = strerror(errno);
		VFS_THROW(_BS(L"Error accessing path - ") << err << L" : " << path << _BS::wget);
	}

	path_cache = PathIteratorCache::Get();
	(*(PathIteratorCache*)path_cache).init( s.c_str() );
#endif
	fFirstRequest = true;
}

vfs::OS::IterateDirectory::~IterateDirectory()
{
#ifdef WIN32
	FindClose(fSearchHandle);
#else
	if(files)
	{
		if(closedir(files) == -1)
		{
			String err = strerror(errno);
			VFS_THROW(err);
		}
		PathIteratorCache::Release( (PathIteratorCache*)path_cache );
	}
#endif
}

bool vfs::OS::IterateDirectory::nextFile(vfs::String &filename, IterateDirectory::EFileAttribute &attrib)
{
#ifdef WIN32
	VFS_THROW_IFF(fSearchHandle != INVALID_HANDLE_VALUE, L"Invalid Handle Value");
	if (fFirstRequest)
	{
		fFirstRequest = false;
	}
	//else
	{
		if(!Settings::getUseUnicode())
		{
			if( !FindNextFileA(fSearchHandle, &fFileInfoA) )
			{
				return false;
			}
			filename.r_wcs().assign( String::widen( fFileInfoA.cFileName, strlen(fFileInfoA.cFileName) ) );
			attrib = (fFileInfoA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? IterateDirectory::FA_DIRECTORY : IterateDirectory::FA_FILE;
		}
		else
		{
			if ( !FindNextFileW(fSearchHandle, &fFileInfoW) )
			{
				return false;
			}
			filename.r_wcs().assign(fFileInfoW.cFileName);
			attrib = (fFileInfoW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? IterateDirectory::FA_DIRECTORY : IterateDirectory::FA_FILE;
		}
	}
	return true;
#else
	struct dirent *entry = readdir(files);
	if(entry)
	{
		if(entry->d_type == DT_UNKNOWN)
		{
			// apparently we are on a file system where 'readdir' cannot determine the type of the entry
			// we will have to use 'stat' instead

			(*(PathIteratorCache*)path_cache).cache_file(entry->d_name);

			struct stat STAT;
			int stat_test = stat( (*(PathIteratorCache*)path_cache).cache_value() , &STAT );
			if(stat_test == -1)
			{
				String err = strerror(errno);
				VFS_THROW(_BS(filename) << L" : " << err << _BS::wget);
			}

			attrib = (STAT.st_mode & S_IFREG) ? IterateDirectory::FA_FILE      :
					 (STAT.st_mode & S_IFDIR) ? IterateDirectory::FA_DIRECTORY :
					 FA_UNKNOWN;
		}
		else
		{
			attrib = (entry->d_type == DT_DIR) ? IterateDirectory::FA_DIRECTORY :
					 (entry->d_type == DT_REG) ? IterateDirectory::FA_FILE      :
					 FA_UNKNOWN;
		}
		if(attrib == FA_UNKNOWN) VFS_LOG_WARNING(_BS(L"Could not determine type of path : ") << filename << _BS::wget);

		filename = String(entry->d_name);

		return true;
	}
	return false;
#endif
}

bool vfs::OS::checkRealDirectory(vfs::Path const& dir)
{
#ifdef WIN32
	bool bDirExists = false;
	if(!Settings::getUseUnicode())
	{
		WIN32_FIND_DATAA fd;
		memset(&fd,0,sizeof(WIN32_FIND_DATAA));

		std::string s;
		String::narrow(dir.c_wcs(), s);

		HANDLE hFile = FindFirstFileA( s.c_str(), &fd);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			VFS_LOG_ERROR(_BS(L"Directory does not exist - ") << error << L" : " << dir << _BS::wget);
		}

		bDirExists = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

		FindClose(hFile);
	}
	else
	{
		WIN32_FIND_DATAW fd;
		memset(&fd,0,sizeof(WIN32_FIND_DATAW));
		HANDLE hFile = FindFirstFileW( dir.c_str(), &fd);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			VFS_LOG_ERROR(_BS(L"Directory does not exist - ") << error << L" : " << dir << _BS::wget);
		}

		bDirExists = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

		FindClose(hFile);
	}
	return bDirExists;
#else
	int result = access(dir.to_string().c_str(), F_OK /*0400*/);
	if(result == -1)
	{
		String err = strerror(errno);
		VFS_LOG_ERROR(_BS(L"Directory does not exist - ") << err << L" : " << dir << _BS::wget);
		return false;
	}
	return true;
#endif
}

bool vfs::OS::createRealDirectory(vfs::Path const& dir)
{
#ifdef WIN32
	BOOL success;
	String::str_t const& str = dir.c_wcs();
	success = Settings::getUseUnicode() ?
		CreateDirectoryW( dir.c_str(),NULL ) :
		CreateDirectoryA( String::narrow( str.c_str(), str.length() ).c_str(), NULL );
	if(success == 0)
	{
		DWORD error = GetLastError();
		if(error == ERROR_ALREADY_EXISTS)
		{
			return true;
		}

		VFS_LOG_ERROR(_BS(L"Could not create directory - ") << error << L" : " << dir << _BS::wget);

		return false;
	}
	return true;
#else
	int result = mkdir(dir.to_string().c_str(), S_IRWXU | S_IRGRP /*0777*/);
	if(result == -1 && errno != EEXIST)
	{
		String err = strerror(errno);
		VFS_LOG_ERROR(_BS(L"Could not create directory - ") << err << L" : " << dir << _BS::wget);
		return false;
	}
	return true;
#endif
}

bool vfs::OS::FileAttributes::getFileAttributes(vfs::Path const& dir, vfs::UInt32& uiAttribs)
{
	uiAttribs = 0;
#ifdef WIN32
	DWORD attribs = Settings::getUseUnicode() ?
		GetFileAttributesW(dir.c_str()) :
		GetFileAttributesA(String::narrow(dir.c_str(), dir.length()).c_str());

	if(attribs == INVALID_FILE_ATTRIBUTES)
	{
		DWORD error = GetLastError();
		VFS_LOG_ERROR(_BS(L"Invalid File Attributes - ") << error << L" : " << dir << _BS::wget);
		return false;
	}

	for(vfs::UInt32 attribMask = 0x80000000; attribMask > 0; attribMask >>= 1)
	{
		switch(attribs & attribMask)
		{
			case FILE_ATTRIBUTE_ARCHIVE:
				uiAttribs |= ATTRIB_ARCHIVE;
				break;
			case FILE_ATTRIBUTE_DIRECTORY:
				uiAttribs |= ATTRIB_DIRECTORY;
				break;
			case FILE_ATTRIBUTE_HIDDEN:
				uiAttribs |= ATTRIB_HIDDEN;
				break;
			case FILE_ATTRIBUTE_NORMAL:
				uiAttribs |= ATTRIB_NORMAL;
				break;
			case FILE_ATTRIBUTE_READONLY:
				uiAttribs |= ATTRIB_READONLY;
				break;
			case FILE_ATTRIBUTE_SYSTEM:
				uiAttribs |= ATTRIB_SYSTEM;
				break;
			case FILE_ATTRIBUTE_TEMPORARY:
				uiAttribs |= ATTRIB_TEMPORARY;
				break;
			case FILE_ATTRIBUTE_COMPRESSED:
				uiAttribs |= ATTRIB_COMPRESSED;
				break;
			case FILE_ATTRIBUTE_OFFLINE:
				uiAttribs |= ATTRIB_OFFLINE;
				break;
		}
	}
#else

	std::string filename = dir.to_string();

	struct stat STAT;
	int stat_test = stat( filename.c_str() , &STAT );
	if(stat_test == -1)
	{
		String err = strerror(errno);
		VFS_LOG_ERROR(_BS(L"Path does not exist - ") << err << L" : " << dir << _BS::wget);
		return false;
	}

	if ( S_ISREG(STAT.st_mode) ) uiAttribs |= ATTRIB_NORMAL;
	if ( S_ISDIR(STAT.st_mode) ) uiAttribs |= ATTRIB_DIRECTORY;

	// OK, 'stat' succeeded, so the file must exist.
	// Now only check read/write permissions

	bool writable = access(filename.c_str(), W_OK) == 0;
	bool readable = access(filename.c_str(), R_OK) == 0;
	if( readable && !writable )
	{
		uiAttribs |= ATTRIB_READONLY;
	}
	else if ( !(writable && readable) )
	{
		VFS_LOG_ERROR( _BS(L"Path has stats [readable=") << (readable ? 1 : 0) << L", writable=" << (writable ? 1 : 0)
				<< L"] which cannot be represented with an attribute value : " << dir
				<< _BS::wget );
	}
#endif
	return true;
}

bool vfs::OS::deleteRealFile(vfs::Path const& filename)
{
#ifdef WIN32
	BOOL del = Settings::getUseUnicode() ?
		DeleteFileW( filename.c_str() ) :
		DeleteFileA( String::narrow(filename.c_str(), filename.length()).c_str() );
	if(!del)
	{
		DWORD err = GetLastError();
		if(err != NO_ERROR)
		{
			VFS_LOG_ERROR(_BS(L"Could not delete file - ") << err << L" : " << filename << _BS::wget);
		}
	}
	return (del != FALSE);
#else
	int result = remove( String::as_utf8(filename()).c_str() );
	if(result == -1)
	{
		String err = strerror(errno);
		VFS_LOG_ERROR(_BS(L"Could not delete file - ") << err << L" : " << filename << _BS::wget);
		return false;
	}
	return true;
#endif
}

void vfs::OS::getExecutablePath(vfs::Path& dir, vfs::Path& file)
{
#ifdef WIN32
	DWORD error;
	if(!Settings::getUseUnicode())
	{
		char path[256];
		if( 0 != (error = ::GetModuleFileNameA(NULL, path, 256)) )
		{
			Path(String::widen(path,256)).splitLast(dir, file);
		}
	}
	else
	{
		wchar_t path[256];
		if( 0 != (error = ::GetModuleFileNameW(NULL, path, 256)) )
		{
			Path(path).splitLast(dir, file);
		}
	}
	if(error == 0)
	{
		DWORD code = GetLastError();
		VFS_THROW(_BS(L"Could not get path of executable file [") <<
			(!Settings::getUseUnicode() ? L"no unicode" : L"unicode") <<
			L"] - " << code << _BS::wget);
	}
#else
	char buf[256];
#ifdef __linux__
	ssize_t size = readlink("/proc/self/exe", buf, 256);
#elif __FreeBSD__
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	size_t bufsize = sizeof(buf);
	ssize_t size = sysctl(mib, 4, buf, &bufsize, NULL, 0);

	if(size != -1) size = bufsize;
#endif
	if(size == -1)
	{
		String err = strerror(errno);
		VFS_THROW(_BS(L"Could not get path of executable file - ") << err << _BS::wget);
	}
	buf[size] = 0;

	Path exedir(buf);
	exedir.splitLast(dir, file);
#endif
}

void vfs::OS::getCurrentDirectory(vfs::Path& dir)
{
#ifdef WIN32
	DWORD error;
	Path  path;
	if( !Settings::getUseUnicode() )
	{
		char path[256];
		if( 0 != (error = ::GetCurrentDirectoryA(256, path)) )
		{
			dir = Path(String::widen(path,256));
		}
	}
	else
	{
		wchar_t path[256];
		if( 0 != (error = ::GetCurrentDirectoryW(256, path)) )
		{
			dir = Path(path);
		}
	}
	if(error == 0)
	{
		DWORD code = GetLastError();
		VFS_THROW(_BS(L"Could not determine current directory [") <<
			(!Settings::getUseUnicode() ? L"no unicode" : L"unicode") <<
			L"] - " << code << _BS::wget);
	}
#else
	char* cwd = getcwd(NULL,0);
	if(!cwd)
	{
		String err = strerror(errno);
		VFS_THROW(_BS(L"Could not determine current directory - ") << err << _BS::wget);
	}
	dir = cwd;
	free(cwd);
#endif
}

void vfs::OS::setCurrentDirectory(vfs::Path const& dir)
{
#ifdef WIN32
	if(!Settings::getUseUnicode())
	{
		std::string str;
		String::narrow( dir.c_wcs(), str );
		VFS_THROW_IFF( ::SetCurrentDirectoryA( str.c_str() ) == TRUE,
			_BS(L"Could not set current directory [no unicode] : ") << dir << _BS::wget );
	}
	else
	{
		VFS_THROW_IFF( ::SetCurrentDirectoryW( dir.c_str() ) == TRUE,
			_BS(L"Could not set current directory [unicode] : ") << dir <<_BS::wget );
	}
#else
	if(chdir(dir.to_string().c_str()) != 0)
	{
		String err = strerror(errno);
		VFS_THROW(_BS(L"Could not set current directory - ") << err << L" : " << dir << _BS::wget);
	}
#endif
}

bool vfs::OS::getEnv(vfs::String const& key, vfs::String& value)
{
#ifdef _MSC_VER
	wchar_t *val_buf = NULL;
	::size_t buf_len;
	errno_t err = _wdupenv_s(&val_buf,&buf_len, key.c_str());
	if(err == 0 && val_buf)
	{
		// success
		value = val_buf;
		free(val_buf);
		return !value.empty();
	}
	return false;
#else
	char* val_buf = getenv(key.utf8().c_str());
	if(val_buf)
	{
		value = val_buf;
		return !value.empty();
	}
	return false;
#endif
}
