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

#include <vfs/Core/vfs_object.h>
#include <vfs/Core/vfs_smartpointer.h>
#include <vfs/Tools/vfs_memory_register.h>

#ifdef WIN32
#	include <Windows.h>
#endif


vfs::ObjectBase::ObjectBase() : m_ref_counter(0), m_weak_ref(0)
{
};

vfs::ObjectBase::~ObjectBase()
{
}

void vfs::ObjectBase::_register()
{
#if defined(WIN32)
	InterlockedIncrement(&m_ref_counter);
#else
	__sync_add_and_fetch(&m_ref_counter,1);
#endif
}

bool vfs::ObjectBase::_unregister()
{
#if defined(WIN32)
	if(InterlockedDecrement(&m_ref_counter) <= 0)
#else
	if(__sync_sub_and_fetch(&m_ref_counter,1) <= 0)
#endif
	{
		if(m_weak_ref && m_weak_ref->valid)
		{
			m_weak_ref->clearPrev();
			m_weak_ref = 0;
		}
		delete this;
		return true;
	}
	return false;
}

long vfs::ObjectBase::_getcount() const
{
	return m_ref_counter;
}

void vfs::ObjectBase::_register_weak(vfs::WeakPointerBase* wptr)
{
	if(wptr)
	{
		wptr->valid = true;
#if   defined WIN32
		InterlockedExchangePointer(reinterpret_cast<void**>(&wptr->prev), m_weak_ref);
		if(wptr->prev)
		{
			InterlockedExchangePointer(reinterpret_cast<void**>(&wptr->prev->next), wptr);
		}
		InterlockedExchangePointer(reinterpret_cast<void**>(&m_weak_ref), wptr);
#elif defined __GNUC__ || defined __clang__
		__sync_lock_test_and_set(&wptr->prev, m_weak_ref);
		if(wptr->prev)
		{
			__sync_lock_test_and_set(&wptr->prev->next, wptr);
		}
		__sync_lock_test_and_set(&m_weak_ref, wptr);
#else
		wptr->prev = m_weak_ref;
		if(wptr->prev)
		{
			wptr->prev->next = wptr;
		}
		m_weak_ref = wptr;
#endif
		// if(m_weak_ref) m_weak_ref->valid = true;
	}
}

void vfs::ObjectBase::_unregister_weak(vfs::WeakPointerBase* wptr)
{
	if(wptr && wptr->valid)
	{
		if(wptr->prev)
		{
#if   defined WIN32
			InterlockedExchangePointer(reinterpret_cast<void**>(&wptr->prev->next), wptr->next);
#elif defined __GNUC__ || defined __clang__
			__sync_lock_test_and_set(&wptr->prev->next, wptr->next);
#else
			wptr->prev->next = wptr->next;
#endif
		}
		if(wptr->next)
		{
#if   defined WIN32
			InterlockedExchangePointer(reinterpret_cast<void**>(&wptr->next->prev), wptr->prev);
#elif defined __GNUC__ || defined __clang__
			__sync_lock_test_and_set(&wptr->next->prev, wptr->prev);
#else
			wptr->next->prev = wptr->prev;
#endif
		}
		else
		{
#if   defined WIN32
			InterlockedExchangePointer(reinterpret_cast<void**>(&m_weak_ref), wptr->prev);
#elif defined __GNUC__ || defined __clang__
			__sync_lock_test_and_set(&m_weak_ref, wptr->prev);
#else
			m_weak_ref = wptr->prev;
#endif
		}

		wptr->valid = false;
		wptr->prev  = 0;
		wptr->next  = 0;
	}
}

