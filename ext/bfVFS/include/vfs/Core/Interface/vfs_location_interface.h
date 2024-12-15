/*
 * bfVFS : vfs/Core/Interface/vfs_location_interface.h
 *  - generic Location interface that allows retrieval of a file from a real location
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

#ifndef _VFS_LOCATION_INTERFACE_H_
#define _VFS_LOCATION_INTERFACE_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_smartpointer.h>
#include <vfs/Core/Interface/vfs_file_interface.h>
#include <vfs/Core/Interface/vfs_iterator_interface.h>

#include <map>
#include <list>
#include <typeinfo>

namespace vfs
{
	class VirtualProfile;

	class VFS_API IBaseLocation : public vfs::ObjectBase
	{
	protected:
		IBaseLocation() {};

	public:
		VFS_SMARTPOINTER(IBaseLocation);

		typedef TIterator<IBaseFile> Iterator;

		virtual ~IBaseLocation()
		{};

		virtual bool            implementsWritable() = 0;
		virtual	bool            implementsReadable() = 0;

		virtual Path const&     getPath           () = 0;
		virtual bool            fileExists        (vfs::Path const& filename, const VirtualProfile* profile) = 0;
		virtual IBaseFile*      getFile           (vfs::Path const& filename, const VirtualProfile* profile) = 0;

		virtual Iterator        begin             (const VirtualProfile* profile) = 0;
		virtual void            getSubDirList     (std::list<vfs::Path>& sub_dirs, const VirtualProfile* profile) = 0;
	};

	/**
	 *  TLocation
	 */
	template<typename ReadType, typename WriteType>
	class VFS_API TLocationTemplate : public IBaseLocation
	{
	protected:
		TLocationTemplate(Path const& mountPoint)
			: m_mountPoint(mountPoint)
		{};

	public:
		VFS_SMARTPOINTER(TLocationTemplate);

		typedef TLocationTemplate<ReadType,WriteType>   LocationType_t;
		typedef TFileTemplate<ReadType,WriteType>       FileType_t;
		typedef ReadType                                ReadType_t;
		typedef WriteType                               WriteType_t;
		typedef std::list<std::pair<FileType_t*,Path> > ListFilesWithPath_t;

		friend class UncompressedLibraryBase;

	public:
		virtual ~TLocationTemplate()
		{};

		// has to be virtual , or the types of the caller (not the real object) will be tested
		virtual bool implementsWritable()
		{
			return typeid(WriteType_t) == typeid(IWritable);
		}
		virtual bool implementsReadable()
		{
			return typeid(ReadType_t)  == typeid(IReadable);
		}

		Path const& getMountPoint()
		{
			return m_mountPoint;
		}

		/**
		 *  IBaseLocation interface
		 */
		virtual Path const&     getPath()
		{
			return m_mountPoint;
		}

		virtual bool            fileExists  (Path const& filename, const VirtualProfile* profile) = 0;
		virtual IBaseFile*      getFile     (Path const& filename, const VirtualProfile* profile) = 0;
		virtual FileType_t*     getFileTyped(Path const& filename, const VirtualProfile* profile) = 0;

	protected:
		Path m_mountPoint;
	};

/**************************************************************************************/
/**************************************************************************************/

	template<typename WriteType=IWriteType>
	class TReadLocation : public TLocationTemplate<IReadable,WriteType>
	{
	public:
		VFS_SMARTPOINTER(TReadLocation);

		typedef TReadLocation<WriteType> LocationType_t;

		static LocationType_t* cast(IBaseLocation* bl)
		{
			if(bl && bl->implementsReadable())
			{
				return static_cast<LocationType_t*>(bl);
			}
			return NULL;
		}

	public:
		TReadLocation(Path const& sLocalPath)
			: TLocationTemplate<IReadable,WriteType>(sLocalPath)
		{};
		virtual ~TReadLocation(){};
	};

	template<typename ReadType=IReadType>
	class TWriteLocation : public TLocationTemplate<ReadType,IWritable>
	{
	public:
		VFS_SMARTPOINTER(TWriteLocation);

		typedef TWriteLocation<ReadType> LocationType_t;

		static LocationType_t* cast(IBaseLocation* bl)
		{
			if(bl && bl->implementsWritable())
			{
				return static_cast<LocationType_t*>(bl);
			}
			return NULL;
		}

	public:
		TWriteLocation(Path const& sLocalPath)
			: TLocationTemplate<ReadType,IWritable>(sLocalPath)
		{};
		virtual ~TWriteLocation(){};
	};

	/**************************************************************************************/
	/**************************************************************************************/

	typedef TReadLocation <IWriteType> ReadLocation_t;
	typedef TWriteLocation<IReadType>  WriteLocation_t;

} // end namespace

#endif // _VFS_LOCATION_INTERFACE_H_

