/*
 * bfVFS : vfs/Core/vfs_smartpointer.cpp
 *  - weak- and smart-poiner classes
 *  - weakpointer only knows that pointer to object is valid; can be converted to a smart pointer
 *  - macro VFS_SMARTPOINTER(..) defined weak- and smartpointer types for a class; makes smartpointer a friend class
 *  - create objects via static New in SmartPointer to register them for memory debugging
 *  - macro VFS_NEWx(..) creates object via static New and registers it with file and line of the object creation
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

#include <vfs/Core/vfs_smartpointer.h>
#include <vfs/Core/vfs_object.h>


vfs::WeakPointerBase::WeakPointerBase()
: prev(0), next(0), valid(false)
{};

vfs::WeakPointerBase::~WeakPointerBase()
{};

void vfs::WeakPointerBase::_register(vfs::ObjectBase* base)
{
	if(base) base->_register_weak(this);
}

void vfs::WeakPointerBase::_unregister(vfs::ObjectBase* base)
{
	if(base) base->_unregister_weak(this);
}


void vfs::WeakPointerBase::clearPrev()
{
	WeakPointerBase* _p = prev;

	prev  = 0;
	next  = 0;
	valid = false;

	if(_p /*&& valid*/)
	{
		_p->clearPrev();
	}
}
