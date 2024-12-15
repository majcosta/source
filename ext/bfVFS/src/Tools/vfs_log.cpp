/*
 * bfVFS : vfs/Tools/vfs_log.cpp
 *  - simple file logger
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

#include <vfs/Tools/vfs_log.h>
#include <cstring>

#ifdef WIN32
static const char ENDL[] = "\r\n";
#else
static const char ENDL[] = "\n";
#endif

static vfs::Log::LogList_t* __logs;

vfs::Log::LogList_t& vfs::Log::_logs()
{
	if(!__logs)
	{
		__logs = new LogList_t;
	}
	return *__logs;
}

vfs::Log::SP vfs::Log::create(vfs::Path const& filename, bool append, EFlushMode fmode)
{
	_logs().push_back( VFS_NEW4( Log, filename, true, append, fmode));
	return _logs().back();
}
vfs::Log::SP vfs::Log::create(vfs::WritableFile_t* file, bool append, EFlushMode fmode)
{
	_logs().push_back( VFS_NEW3(Log, file, append, fmode));
	return _logs().back();
}

void vfs::Log::flushDeleteAll()
{
	LogList_t::iterator it = _logs().begin();
	for(; it != _logs().end(); ++it)
	{
		(*it)->flush();
		(*it)->_merge_buffers();
		(*it)->flush();

		(*it).null();
	}
	_logs().clear();
}

void vfs::Log::flushReleaseAll()
{
	LogList_t::iterator it = _logs().begin();
	for(; it != _logs().end(); ++it)
	{
		(*it)->releaseFile();
	}
}

vfs::String vfs::Log::_shared_id_str;

vfs::String const& vfs::Log::getSharedString()
{
	return _shared_id_str;
}

void vfs::Log::setSharedString(vfs::String const& str)
{
	_shared_id_str = str;
}


vfs::Log::Log(vfs::Path const& filename, bool use_vfs_file, bool append, EFlushMode fmode)
:	_filename(filename), _file(NULL),
	_first_write(true), _flush_mode(fmode), _append(append),
	_buffer_test_size(8192), _flushing(false),
	_buffer(&_front_buffer)
{
};

vfs::Log::Log(vfs::WritableFile_t* file, bool append, EFlushMode fmode)
:	_file(file),
	_first_write(true), _flush_mode(fmode), _append(append),
	_buffer_test_size(8192), _flushing(false),
	_buffer(&_front_buffer)
{
}


vfs::Log::~Log()
{
	// the final flush
	flush();

	_file.null();

	// one extra unlock wouldn't hurt
	_mutex.unlock();
}

void vfs::Log::destroy()
{
	// no need to lock here as 'flush' and 'Release' do it themselves
	this->flush();
	this->_merge_buffers();
	this->flush();

	if(this->_getcount() <= 2)
	{
		// object is about to be deleted, so remove it now from the static list
		_logs().remove( SP(this) );
	}
}

void vfs::Log::releaseFile()
{
	VFS_LOCK(_mutex);

	flush();
	_merge_buffers();
	flush();

	this->_file = NULL;
}

vfs::Log& vfs::Log::operator<<(vfs::UInt64 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::UInt32 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::UInt16 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::UInt8 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}

vfs::Log& vfs::Log::operator<<(vfs::Int64 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::Int32 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::Int16 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(vfs::Int8 const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
#ifdef _MSC_VER
	vfs::Log& vfs::Log::operator<<(DWORD const& t)
	{
		VFS_LOCK(_mutex);
		return pushNumber(t);
	}
#endif
vfs::Log& vfs::Log::operator<<(float const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}
vfs::Log& vfs::Log::operator<<(double const& t)
{
	VFS_LOCK(_mutex);
	return pushNumber(t);
}

vfs::Log& vfs::Log::operator<<(const char* t)
{
	VFS_LOCK(_mutex);
	(*_buffer).buffer      << t;
	(*_buffer).buffer_size += strlen(t);
	_test_flush();
	return *this;
}
vfs::Log& vfs::Log::operator<<(const wchar_t* t)
{
	VFS_LOCK(_mutex);
	std::string s           = vfs::String::as_utf8(t);
	(*_buffer).buffer      << s;
	(*_buffer).buffer_size += s.length();
	_test_flush();
	return *this;
}
vfs::Log& vfs::Log::operator<<(std::string const& t)
{
	VFS_LOCK(_mutex);
	(*_buffer).buffer      << t;
	(*_buffer).buffer_size += t.length();
	_test_flush();
	return *this;
}
vfs::Log& vfs::Log::operator<<(std::wstring const& t)
{
	VFS_LOCK(_mutex);
	std::string s = vfs::String::as_utf8(t);
	(*_buffer).buffer      << s;
	(*_buffer).buffer_size += s.length();
	_test_flush();
	return *this;
}
vfs::Log& vfs::Log::operator<<(vfs::String const& t)
{
	VFS_LOCK(_mutex);
	std::string s           = t.utf8();
	(*_buffer).buffer      << s;
	(*_buffer).buffer_size += s.length();
	_test_flush();
	return *this;
}

vfs::Log& vfs::Log::operator<<(void* const& t)
{
	VFS_LOCK(_mutex);
	(*_buffer).buffer      << t;
	(*_buffer).buffer_size += sizeof(void*);
	return *this;
}

vfs::Log& vfs::Log::operator<<(vfs::Log::_endl const& endl)
{
	VFS_LOCK(_mutex);
	(*_buffer).buffer      << ENDL;
	(*_buffer).buffer_size += sizeof(ENDL)-1;

	if(_buffer == &_front_buffer && _back_buffer.buffer_size > 0)
	{
		_merge_buffers();
	}

	if(_flush_mode == vfs::Log::FLUSH_ON_ENDL) flush();
	return *this;
}

void vfs::Log::_merge_buffers()
{
	std::string s = _back_buffer.buffer.str();
	_front_buffer.buffer      << s;
	_front_buffer.buffer_size += s.length();

	_back_buffer.buffer.str("");
	_back_buffer.buffer_size   = 0;
}


/*
vfs::Log& vfs::Log::endl()
{
	_buffer << ENDL;
	_buffer_size += sizeof(ENDL)-1;
	_test_flush();
	return *this;
}
*/
void vfs::Log::setAppend(bool append)
{
	VFS_LOCK(_mutex);
	_append = append;
}

void vfs::Log::setBufferSize(vfs::UInt32 bufferSize)
{
	VFS_LOCK(_mutex);
	_buffer_test_size = bufferSize;
}

void vfs::Log::_test_flush(bool force)
{
	if( (_flush_mode == FLUSH_IMMEDIATELY) ||
		(_flush_mode == FLUSH_BUFFER && (*_buffer).buffer_size > _buffer_test_size) ||
		(/*_flush_mode == FLUSH_ON_DELETE &&*/ force == true) )
	{
		flush();
	}
}


vfs::Log::EFlushMode vfs::Log::flushMode()
{
	return _flush_mode;
}
void vfs::Log::flushMode(vfs::Log::EFlushMode fmode)
{
	_flush_mode = fmode;
}

#include <ctime>
#include <vfs/Core/vfs.h>

struct _ScopeTest
{
	bool& ref;

	_ScopeTest(bool& var) : ref(var)
	{
		ref = true;
	}
	~_ScopeTest()
	{
		ref = false;
	}
};

void vfs::Log::flush()
{
	VFS_LOCK(_mutex);

	if(_flushing)
	{
		return;
	}
	_ScopeTest st(_flushing);

	::size_t buflen = _front_buffer.buffer.str().length();
	if(buflen == 0)
	{
		return;
	}

	_buffer = &_back_buffer;


	if(_file.isNull())
	{
		VFS_THROW_IFF(!_filename.empty(), L"_file is NULL and _filename is empty");

		if(vfs::canWrite())
		{
			try
			{
				OpenWriteFile wfile(_filename,true,!_append);
				_file = wfile.file();
				wfile.release();
			}
			catch(...)
			{
			}
		}
		else
		{
			try
			{
				_file = WritableFile_t::cast( VFS_NEW1(File, _filename) );
				_file->openWrite(true,!_append);
			}
			catch(...)
			{
			}
		}
	}

	try
	{
		OpenWriteFile wfile(_file, _append);
		if(_append)
		{
			wfile->setWritePosition(0, IBaseFile::SD_END);
		}

		if(_first_write)
		{
			time_t rawtime;
			time ( &rawtime );
			std::string datetime(ctime(&rawtime));
			std::string s_out;

			vfs::size_t wloc = wfile->getWritePosition();
			if(wloc > 0)
			{
				s_out = ENDL;
			}
			s_out += " *** ";
			s_out += datetime.substr(0,datetime.length()-1);
			s_out += " *** ";
			s_out += ENDL;
			s_out += "[ ";
			s_out += _shared_id_str.utf8();
			s_out += " ]";
			s_out += ENDL;
			s_out += ENDL;

			wfile->write(s_out.c_str(), s_out.length());
			_first_write = false;
		}

		wfile->write(_front_buffer.buffer.str().c_str(), buflen);
		_front_buffer.buffer.str("");
		_front_buffer.buffer.clear();
		_front_buffer.buffer_size = 0;

		_append = true;

		_buffer = &_front_buffer;
	}
	catch(vfs::Exception const& ex)
	{
		// it is possible that the file cannot be opened anymore, e.g. when it the VFS was shut down and the Log object remained
		// just ignore the error and return
	}
}

void vfs::Log::lock()
{
	_mutex.lock();
}
void vfs::Log::unlock()
{
	_mutex.unlock();
}
