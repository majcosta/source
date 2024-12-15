/*
 * bfVFS : vfs/Core/vfs_object.cpp
 *  - base class that implements lockfree reference counting
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

#ifndef VFS_OBJECT_H_
#define VFS_OBJECT_H_

#include <vfs/vfs_config.h>
#include <vfs/Core/vfs_smartpointer.h>

namespace vfs
{
	class WeakPointerBase;

	class VFS_API ObjectBase
	{
		long m_ref_counter;

	public:
		VFS_SMARTPOINTER(ObjectBase);

		ObjectBase();
		virtual ~ObjectBase();

		void _register  ();
		bool _unregister();

		long _getcount  () const;

	private:

		friend class WeakPointerBase;

		void _register_weak  (WeakPointerBase* wptr);
		void _unregister_weak(WeakPointerBase* wptr);

		WeakPointerBase* m_weak_ref;
	};
}

#endif /* VFS_OBJECT_H_ */
