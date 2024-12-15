/*
 * bfVFS : vfs/Core/vfs_dfebug.cpp
 *  - Exception class and throw macros, used to notify the using program of unexpected situations
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

#include <vfs/Aspects/vfs_logging.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Core/vfs_string.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/vfs_os_functions.h>
#include <vfs/Tools/vfs_log.h>

#include <sstream>
#include <ctime>

vfs::Exception::Exception(vfs::String const& text, vfs::String const& function, int line, const char* file)
: std::exception() //(text.utf8().c_str())
{
	VFS_LOG_ERROR( text.c_str() );

	time_t rawtime;
	time ( &rawtime );
	std::string datetime(ctime(&rawtime));

	SEntry en;
	en.message  = text;
	en.line     = line;
	en.file     = file;
	en.function = function;
	VFS_IGNOREEXCEPTION(en.time = String(datetime.substr(0,datetime.length()-1)), false);
	m_CallStack.push_back(en);
};

vfs::Exception::Exception(vfs::String const& text, vfs::String const& function, int line, const char* file, std::exception& ex)
: std::exception()
{
	VFS_LOG_ERROR( text.c_str() );

	if(dynamic_cast<Exception*>(&ex))
	{
		Exception& vfs_ex = *static_cast<Exception*>(&ex);
		m_CallStack.insert(m_CallStack.end(), vfs_ex.m_CallStack.begin(), vfs_ex.m_CallStack.end());
	}
	else
	{
		SEntry en;
		en.line     = -1;
		en.file     = "";
		en.function = "";
		en.time     = "";
		VFS_IGNOREEXCEPTION( en.message	= String(ex.what()), false );

		m_CallStack.push_back(en);
	}

	time_t rawtime;
	time ( &rawtime );
	std::string datetime(ctime(&rawtime));

	SEntry en;
	en.message  = text;
	en.line     = line;
	en.file     = file;
	en.function = function;
	VFS_IGNOREEXCEPTION(en.time = String(datetime.substr(0,datetime.length()-1)), false);

	m_CallStack.push_back(en);
};




vfs::Exception::~Exception() throw()
{
}

vfs::String vfs::Exception::getLastEntryString() const
{
	if(!m_CallStack.empty())
	{
		CALLSTACK::const_reverse_iterator rit = m_CallStack.rbegin();
		std::wstringstream wss;
		wss << rit->file.c_wcs()
			<< L" (l. "
			<< rit->line
			<< ") : ["
			<< rit->function.c_wcs()
			<< L"] - "
			<< rit->message.c_wcs();
		return wss.str();
	}
	return "";
}

const char* vfs::Exception::what() const throw()
{
	static std::string msg;
	msg = "";
	if(!m_CallStack.empty())
	{
		msg = this->getExceptionString().utf8();
	}
	return msg.c_str();
}


vfs::String vfs::Exception::getExceptionString() const
{
	if(!m_CallStack.empty())
	{
		std::wstringstream wss;
		CALLSTACK::const_reverse_iterator rit = m_CallStack.rbegin();
		for(; rit != m_CallStack.rend(); ++rit)
		{
			wss << L"========== "	<< rit->time		<< L" ==========\r\n";
			wss << L"File     :  "	<< rit->file		<< L"\r\n";
			wss << L"Line     :  "	<< rit->line		<< L"\r\n";
			wss << L"Location :  "	<< rit->function	<< L"\r\n\r\n";
			wss << L"   "			<< rit->message		<< L"\r\n";
		}
		return wss.str();
	}
	return L"";
}

