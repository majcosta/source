/*
 * bfVFS : vfs/Aspects/vfs_logging.h
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

#ifndef VFS_LOGGING_H
#define VFS_LOGGING_H

#include <vfs/vfs_config.h>
#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_smartpointer.h>

namespace vfs
{
	class String;
	namespace Aspects
	{
		enum LogType { LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG };

		class VFS_API ILogger : public vfs::ObjectBase
		{
		public:
			VFS_SMARTPOINTER(ILogger);

			virtual ~ILogger() {};

			virtual void Msg(const wchar_t* msg) = 0;
			virtual void Msg(const char* msg)    = 0;
		};

		VFS_API void        startLogging();
		VFS_API void        stopLogging();
		VFS_API bool        isLogging();

		VFS_API ILogger::SP getLogger(LogType     type);
		VFS_API void        setLogger(LogType     type,        ILogger::SP logger);
		VFS_API void        setLogger(ILogger::SP info_logger, ILogger::SP warning_logger, ILogger::SP error_logger, ILogger::SP debug_logger );

		void                Debug    (String const&  msg);
		void                Debug    (const wchar_t* msg);
		void                Debug    (const char*    msg);

		void                Info     (String const&  msg);
		void                Info     (const wchar_t* msg);
		void                Info     (const char*    msg);

		void                Warning  (String const&  msg);
		void                Warning  (const wchar_t* msg);
		void                Warning  (const char*    msg);

		void                Error    (String const&  msg);
		void                Error    (const wchar_t* msg);
		void                Error    (const char*    msg);
	};
};

#if !defined VFS_DISABLE_LOGGING

#	if !defined VFS_LOG_DEBUG
#		define VFS_LOG_DEBUG(msg)       (vfs::Aspects::Debug(msg))
#	endif

#	if !defined VFS_LOG_INFO
#		define VFS_LOG_INFO(msg)        (vfs::Aspects::Info(msg))
#	endif

#	if !defined VFS_LOG_WARNING
#		define VFS_LOG_WARNING(msg)     (vfs::Aspects::Warning(msg))
#	endif

#	if !defined VFS_LOG_ERROR
#		define VFS_LOG_ERROR(msg)       (vfs::Aspects::Error(msg))
#	endif

#else

#	if !defined VFS_LOG_DEBUG
#		define VFS_LOG_DEBUG(msg)
#	endif

#	if !defined VFS_LOG_INFO
#		define VFS_LOG_INFO(msg)
#	endif

#	if !defined VFS_LOG_WARNING
#		define VFS_LOG_WARNING(msg)
#	endif

#	if !defined VFS_LOG_ERROR
#		define VFS_LOG_ERROR(msg)
#	endif

#endif // VFS_DISABLE_LOGGING

#endif // VFS_LOGGING_H
