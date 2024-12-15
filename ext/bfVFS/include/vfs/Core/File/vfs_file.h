/*
 * bfVFS : vfs/Core/File/vfs_file.h
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

#ifndef _VFS_FILE_H_
#define _VFS_FILE_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Aspects/vfs_synchronization.h>

#ifdef WIN32
#	include "windows.h"
#else
#	include <fstream>
#endif

typedef std::basic_fstream<wchar_t> wfstream;

namespace vfs
{
	/******************************************************************/
	/******************************************************************/

	template<typename WriteType=IWriteType>
	class VFS_API TFile : public TFileTemplate<IReadable,WriteType>
	{
		typedef TFileTemplate<IReadable,WriteType> BaseClass_t;

	public :
		VFS_SMARTPOINTER(TFile);

		virtual ~TFile();

		virtual FileAttributes  getAttributes    ();

		virtual void            close            ();
		virtual vfs::size_t     getSize          ();

		virtual bool            isOpenRead       ();
		virtual bool            openRead         ();
		virtual vfs::size_t     read             (vfs::Byte* data, vfs::size_t bytesToRead);

		virtual vfs::size_t     getReadPosition  ();
		virtual void            setReadPosition  (vfs::size_t positionInBytes);
		virtual void            setReadPosition  (vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir);

	protected:
		TFile(Path const& filename);

		bool                    _internalOpenRead(Path const& path);

	protected:
		bool                    m_isOpen_read;
#ifdef WIN32
		HANDLE                  m_file;
#else
		FILE*                   m_file;
#endif
		//vfs::Aspects::Mutex	m_mutex;
	};

	/******************************************************************/
	/******************************************************************/

	// implements the IWritable interface for TFile
	class VFS_API File : public TFile<IWritable>
	{
		typedef TFile<IWritable>	BaseClass_t;

	public :
		VFS_SMARTPOINTER(File);

		virtual ~File();

		virtual void        close             ();

		virtual bool        isOpenWrite       ();
		virtual bool        openWrite         (bool createWhenNotExist = false, bool truncate = false);
		virtual vfs::size_t write             (const vfs::Byte* data, vfs::size_t bytesToWrite);

		virtual vfs::size_t getWritePosition  ();
		virtual void        setWritePosition  (vfs::size_t positionInBytes);
		virtual void        setWritePosition  (vfs::offset_t offsetInBytes, IBaseFile::ESeekDir seekDir);

		virtual bool        deleteFile        ();

	protected:
		File(Path const& filename);

		bool                _internalOpenWrite(Path const& path, bool createWhenNotExist = false, bool truncate = false);

	protected:
		bool                m_isOpen_write;
	};

	/******************************************************************/
	/******************************************************************/

	typedef TFile<IWriteType>   ReadOnlyFile; // needs explicit template instantiation

} // end namespace

#endif // _VFS_FILE_H_

