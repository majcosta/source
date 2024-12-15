/*
 * bfVFS : vfs/Aspects/vfs_logging.cpp
 *  - Logging interface and macros that will be used to report errors/warnings to the using program
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
#include <vfs/Core/vfs_string.h>

static bool s_logger_active = true;

void vfs::Aspects::startLogging()
{
	s_logger_active = true;
}
void vfs::Aspects::stopLogging()
{
	s_logger_active = false;
}
bool vfs::Aspects::isLogging()
{
	return s_logger_active;
}

struct _Logs
{
	~_Logs()
	{
		s_LogInfo.null();
		s_LogWarning.null();
		s_LogError.null();
		s_LogDebug.null();
	}
	vfs::Aspects::ILogger::SP s_LogInfo;
	vfs::Aspects::ILogger::SP s_LogWarning;
	vfs::Aspects::ILogger::SP s_LogError;
	vfs::Aspects::ILogger::SP s_LogDebug;
};
static _Logs __logs;

void vfs::Aspects::setLogger( ILogger::SP info_logger, ILogger::SP warning_logger, ILogger::SP error_logger, ILogger::SP debug_logger )
{
	setLogger(LOG_INFO,     info_logger);
	setLogger(LOG_WARNING,  warning_logger);
	setLogger(LOG_ERROR,    error_logger);
	setLogger(LOG_DEBUG,    debug_logger);
}

void vfs::Aspects::setLogger(vfs::Aspects::LogType type, vfs::Aspects::ILogger::SP logger)
{
	switch (type)
	{
	case Aspects::LOG_INFO :
		__logs.s_LogInfo = logger;
		break;
	case Aspects::LOG_WARNING :
		__logs.s_LogWarning = logger;
		break;
	case Aspects::LOG_ERROR :
		__logs.s_LogError = logger;
		break;
	case Aspects::LOG_DEBUG :
		__logs.s_LogDebug = logger;
		break;
	}
}

vfs::Aspects::ILogger::SP vfs::Aspects::getLogger(vfs::Aspects::LogType type)
{
	switch (type)
	{
	case Aspects::LOG_DEBUG   : return __logs.s_LogDebug;
	case Aspects::LOG_INFO    : return __logs.s_LogInfo;
	case Aspects::LOG_WARNING : return __logs.s_LogWarning;
	case Aspects::LOG_ERROR   : return __logs.s_LogError;
	}
	return ILogger::SP();
}

static inline bool is_logger_valid( vfs::Aspects::ILogger::SP const& logger)
{
	vfs::Aspects::ILogger* _log = logger;

	// object might be used while it is being deleted, therefore the reference counter is already 0, but the pointer is still valid (!NULL)
	return (_log != NULL) && (_log->_getcount() > 0);
}

void vfs::Aspects::Debug(vfs::String const& msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogDebug))
	{
		__logs.s_LogDebug->Msg(msg.c_str());
	}
}
void vfs::Aspects::Debug(const wchar_t* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogDebug))
	{
		__logs.s_LogDebug->Msg(msg);
	}
}
void vfs::Aspects::Debug(const char* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogDebug))
	{
		__logs.s_LogDebug->Msg(msg);
	}
}

void vfs::Aspects::Info(vfs::String const& msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogInfo))
	{
		__logs.s_LogInfo->Msg(msg.c_str());
	}
}
void vfs::Aspects::Info(const wchar_t* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogInfo))
	{
		__logs.s_LogInfo->Msg(msg);
	}
}
void vfs::Aspects::Info(const char* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogInfo))
	{
		__logs.s_LogInfo->Msg(msg);
	}
}

void vfs::Aspects::Warning(vfs::String const& msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogWarning))
	{
		__logs.s_LogWarning->Msg(msg.c_str());
	}
}
void vfs::Aspects::Warning(const wchar_t* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogWarning))
	{
		__logs.s_LogWarning->Msg(msg);
	}
}
void vfs::Aspects::Warning(const char* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogWarning))
	{
		__logs.s_LogWarning->Msg(msg);
	}
}

void vfs::Aspects::Error(vfs::String const& msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogError))
	{
		__logs.s_LogError->Msg(msg.c_str());
	}
}
void vfs::Aspects::Error(const wchar_t* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogError))
	{
		__logs.s_LogError->Msg(msg);
	}
}
void vfs::Aspects::Error(const char* msg)
{
	if(s_logger_active && is_logger_valid(__logs.s_LogError))
	{
		__logs.s_LogError->Msg(msg);
	}
}
