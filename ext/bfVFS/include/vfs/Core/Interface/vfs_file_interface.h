/*
 * bfVFS : vfs/Core/Interface/vfs_file_interface.h
 *  - generic interface for read/write files
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

#ifndef _VFS_FILE_INTERFACE_H_
#define _VFS_FILE_INTERFACE_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/vfs_path.h>
#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_smartpointer.h>

#include <typeinfo>

namespace vfs
{
	/**
	 *  FileAttributes
	 */
	class VFS_API FileAttributes
	{
	public:
		enum Attributes
		{
			ATTRIB_INVALID          = 0,
			ATTRIB_ARCHIVE          = 1,
			ATTRIB_DIRECTORY        = 2,
			ATTRIB_HIDDEN           = 4,
			ATTRIB_NORMAL           = 8,
			ATTRIB_READONLY         = 16,
			ATTRIB_SYSTEM           = 32,
			ATTRIB_TEMPORARY        = 64,
			ATTRIB_COMPRESSED       = 128,
			ATTRIB_OFFLINE          = 256,
		};
		enum LocationType
		{
			LT_NONE                 = 0,
			LT_LIBRARY              = 1,
			LT_DIRECTORY            = 2,
			LT_READONLY_DIRECTORY   = 4,
		};
	public:
		FileAttributes();
		FileAttributes(vfs::UInt32 attribs, LocationType location);

		vfs::UInt32 getAttrib     () const;
		vfs::UInt32 getLocation   () const;

		bool		isAttribSet   (vfs::UInt32 attribs) const;
		bool		isAttribNotSet(vfs::UInt32 attribs) const;

		bool		isLocation    (vfs::UInt32 location) const;
	private:
		void		operator=     (vfs::FileAttributes const& attr);

		const vfs::UInt32	_attribs;
		const vfs::UInt32	_location;
	};

	/**
	 *  IBaseFile
	 */
	class VFS_API IBaseFile : public ObjectBase
	{
	public:
		enum ESeekDir
		{
			SD_BEGIN,
			SD_CURRENT,
			SD_END,
		};

	public:
		VFS_SMARTPOINTER(IBaseFile);

		virtual ~IBaseFile();

		virtual FileAttributes  getAttributes     () = 0;

		Path const&             getName           () const;
		void                    getName(Path& name ) const;

		virtual Path            getPath           () const;
		virtual void            getPath(Path& dir, Path& file) const;

		virtual bool            implementsWritable() = 0;
		virtual bool            implementsReadable() = 0;

		virtual void            close             () = 0;
		virtual vfs::size_t     getSize           () = 0;

		virtual bool            _getRealPath      (Path& path);

	protected:
		IBaseFile(Path const& filename);

		Path                    m_filename;
	};

	/**
	 *  IReadType , IReadable
	 */
	class IReadType{};
	class VFS_API IReadable : public IReadType
	{
	public:
		virtual ~IReadable() {};

		virtual bool            isOpenRead      () = 0;
		virtual bool            openRead        () = 0;
		virtual vfs::size_t     read            (vfs::Byte* data, vfs::size_t bytesToRead) = 0;

		virtual vfs::size_t     getReadPosition () = 0;
		virtual void            setReadPosition (vfs::size_t positionInBytes) = 0;
		virtual void            setReadPosition (vfs::offset_t offsetInBytes, vfs::IBaseFile::ESeekDir seekDir) = 0;
	};
	//class NonReadable : public IReadType{};

	/**
	 *  IWriteType , IWritable
	 */
	class IWriteType{};
	class VFS_API IWritable : public IWriteType
	{
	public:
		virtual ~IWritable() {};

		virtual bool            isOpenWrite     () = 0;
		virtual bool            openWrite       (bool createWhenNotExist = false, bool truncate = false) = 0;
		virtual vfs::size_t     write           (const vfs::Byte* data, vfs::size_t bytesToWrite) = 0;

		virtual vfs::size_t     getWritePosition() = 0;
		virtual void            setWritePosition(vfs::size_t positionInBytes) = 0;
		virtual void            setWritePosition(vfs::offset_t offsetInBytes, vfs::IBaseFile::ESeekDir seekDir) = 0;

		virtual bool            deleteFile      () = 0;
	};
	//class NonWritable: public IWriteType{};

	/******************************************************************/
	/******************************************************************/

	/**
	 *  IFileTemplate
	 */
	template<typename ReadType=IReadType, typename WriteType=IWriteType>
	class VFS_API TFileTemplate : public IBaseFile, public ReadType, public WriteType
	{
	public:
		typedef ReadType                             ReadType_t;
		typedef WriteType                            WriteType_t;

		typedef TFileTemplate<ReadType_t,IWritable>  WriteFile_t;
		typedef TFileTemplate<IReadable,WriteType_t> ReadFile_t;

	protected:
		TFileTemplate(Path const& fileName)
			: IBaseFile(fileName), ReadType(), WriteType()
		{};

	public:
		VFS_SMARTPOINTER(TFileTemplate);

		virtual ~TFileTemplate()
		{};
		virtual bool implementsWritable()
		{
			return typeid(WriteType_t) == typeid(IWritable);
		}
		virtual bool implementsReadable()
		{
			return typeid(ReadType_t)  == typeid(IReadable);
		}
	};

	/**
	 *  TReadableFile
	 */
	template<class WriteType=IWriteType>
	class TReadableFile : public vfs::TFileTemplate<IReadable,WriteType>
	{
		typedef TFileTemplate<IReadable,WriteType> BaseClass_t;

	public:
		VFS_SMARTPOINTER(TReadableFile);

		typedef TReadableFile<WriteType>           ReadFile_t;

		/////////////////////////////////////////
		virtual ~TReadableFile(){};

		/////////////////////////////////////////
		static ReadFile_t* cast(IBaseFile* bf)
		{
			if(bf && bf->implementsReadable())
			{
				return static_cast<ReadFile_t*>(bf);
			}
			return NULL;
		}

	protected:
		TReadableFile(Path const& sFilename)
			: BaseClass_t(sFilename)
		{};

		TReadableFile();
	};

	/**
	 *  TWritableFile
	 */
	template<class ReadType=IReadType>
	class TWritableFile : public TFileTemplate<ReadType,IWritable>
	{
		typedef TFileTemplate<ReadType,IWritable> BaseClass_t;

	public:
		VFS_SMARTPOINTER(TWritableFile);

		typedef TWritableFile<ReadType>           WriteFile_t;

		/////////////////////////////////////////
		virtual ~TWritableFile(){};

		/////////////////////////////////////////
		static WriteFile_t* cast(IBaseFile* bf)
		{
			if(bf && bf->implementsWritable())
			{
				return static_cast<WriteFile_t*>(bf);
			}
			return NULL;
		}

	protected:
		TWritableFile(Path const& sFilename)
			: BaseClass_t(sFilename)
		{};

		TWritableFile();
	};


	/******************************************************************/
	/******************************************************************/

	/**
	 *  typedef's
	 */
	typedef TReadableFile<IWriteType>   ReadableFile_t;
	typedef TWritableFile<IReadable>    WritableFile_t;

} // end namespace

#endif // _VFS_FILE_INTERFACE_H_
