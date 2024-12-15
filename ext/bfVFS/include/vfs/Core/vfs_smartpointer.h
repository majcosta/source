/*
 * bfVFS : vfs/Core/vfs_smartpointer.h
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


#ifndef VFS_SMARTPOINTER_H_
#define VFS_SMARTPOINTER_H_

#include <vfs/vfs_config.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Tools/vfs_classname.h>
#include <vfs/Tools/vfs_memory_register.h>

namespace vfs
{
	/*
	 *
	 */
	class VFS_API WeakPointerBase
	{
		friend class ObjectBase;

	protected:
		WeakPointerBase *prev, *next;
		bool valid;

		void _register  (ObjectBase* base);
		void _unregister(ObjectBase* base);

	public:
		WeakPointerBase();
		virtual ~WeakPointerBase();

		void clearPrev();
	};


	/*
	 *
	 */
	template<typename T>
	class SmartPointer
	{
		T*   m_pointer;

		template<typename T2>
		inline void swap(T2* ptr)
		{
			if(m_pointer && m_pointer->_unregister())
			{
				VFS_MEM_UNREGISTER(m_pointer);
			}
			m_pointer = ptr;
			if(m_pointer)
			{
				m_pointer->_register();
			}
		}

	public:

		enum _guard { _GUARD = -1 };

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		SmartPointer() : m_pointer(NULL)
		{
		}

//		explicit
		SmartPointer(T* ptr) : m_pointer(NULL)
		{
			swap(ptr);
		}

		SmartPointer(SmartPointer<T> const& ptr2) : m_pointer(NULL)
		{
			swap(ptr2.get());
		}

		template<typename T2>
		SmartPointer(SmartPointer<T2> const& ptr2) : m_pointer(NULL)
		{
			swap(ptr2.get());
		}

		SmartPointer& operator=(SmartPointer<T> const& ptr2)
		{
			if(m_pointer != ptr2.get())
			{
				swap(ptr2.get());
			}
			return *this;
		}

		template<typename T2>
		SmartPointer& operator=(SmartPointer<T2> const& ptr2)
		{
			if(m_pointer != ptr2.get())
			{
				swap(ptr2.get());
			}
			return *this;
		}

		~SmartPointer()
		{
			null();
		}

		void null()
		{
			swap((T*)NULL);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		bool isNull() const { return m_pointer == NULL; }

		T* get() const { return m_pointer; }

		operator T* () const { return m_pointer; }

		T* operator->() { return m_pointer; }

		const T* operator->() const { return m_pointer; }

		T& operator*()
		{
			return *m_pointer;
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		static SmartPointer<T> New(const char* file = "", int line = -1)
		{
			T* obj = new T();
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1>
		static SmartPointer<T> New(T1 const& p1, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1>
		static SmartPointer New(T1& p1, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2>
		static SmartPointer New(T1 const& p1, T2 const& p2, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2>
		static SmartPointer New(T1& p1, T2& p2, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, T6 const& p6, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, T6& p6, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, T6 const& p6, T7 const& p7, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, T6& p6, T7& p7, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, T6 const& p6, T7 const& p7, T8 const& p8, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, T6& p6, T7& p7, T8& p8, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, T6 const& p6, T7 const& p7, T8 const& p8, T9 const& p9, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8,p9);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, T6& p6, T7& p7, T8& p8, T9& p9, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8,p9);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
		static SmartPointer New(T1 const& p1, T2 const& p2, T3 const& p3, T4 const& p4, T5 const& p5, T6 const& p6, T7 const& p7, T8 const& p8, T9 const& p9, T10 const& p10, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}
		template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
		static SmartPointer New(T1& p1, T2& p2, T3& p3, T4& p4, T5& p5, T6& p6, T7& p7, T8& p8, T9& p9, T10& p10, _guard g = _GUARD, const char* file = "", int line = -1)
		{
			T* obj = new T(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10);
			VFS_MEM_REGISTER(T, obj, file, line);
			return SmartPointer<T>(obj);
		}

		/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */
	};

	template<typename T>
	class WeakPointer : public WeakPointerBase
	{
	protected:
		T*  m_ptr;

		inline void swap(T* ptr)
		{
			if(m_ptr)
			{
				_unregister(m_ptr);
			}
			m_ptr = ptr;
			if(m_ptr)
			{
				_register(m_ptr);
			}
			else
			{
				valid = false;
				prev = next = 0;
			}
		}

	public:

		WeakPointer() : WeakPointerBase(), m_ptr(NULL)
		{};

		WeakPointer(T* ptr) : WeakPointerBase(), m_ptr(NULL)
		{
			swap(ptr);
		};

		WeakPointer(WeakPointer const& wptr) : WeakPointerBase(), m_ptr(NULL)
		{
			swap(wptr.m_ptr);
		}

		template<typename T2>
		WeakPointer(WeakPointer<T2> const& wptr) : WeakPointerBase(), m_ptr(NULL)
		{
			swap(wptr.m_ptr);
		}

		template<typename T2>
		WeakPointer(SmartPointer<T2> const& sptr) : WeakPointerBase(), m_ptr(NULL)
		{
			swap(sptr.get());
		}

		WeakPointer& operator=(WeakPointer const& wptr2)
		{
			if(this != &wptr2)
			{
				swap(wptr2.m_ptr);
			}
			return *this;
		}

		template<typename T2>
		WeakPointer& operator=(SmartPointer<T2> const& sptr)
		{
			if(m_ptr != sptr.get())
			{
				swap(sptr.get());
			}
			return *this;
		}

		virtual ~WeakPointer()
		{
			// _unregister tests for validity of pointer
			_unregister(m_ptr);

			valid = false;
			prev  = 0;
			next  = 0;
		}

		SmartPointer<T> lock() const
		{
			return valid ? SmartPointer<T>(m_ptr) : SmartPointer<T>();
		}

		SmartPointer<T> safe_lock() const
		{
			VFS_THROW_IFF(valid, L"Could not acquire SmartPointer from a WeakPoiner!");
			return SmartPointer<T>(m_ptr);
		}

		bool operator<(const WeakPointer& wp) const
		{
			return m_ptr < wp.m_ptr;
		}
	};
} // end namespace


#define VFS_SMARTPOINTER(classname)       \
	typedef classname               Self; \
	typedef vfs::SmartPointer<Self> SP;   \
	typedef vfs::WeakPointer <Self> WP;   \
	friend class vfs::SmartPointer<classname>;


#define VFS_NEW(Type) 				                        Type::SP::New(__FILE__, __LINE__)
#define VFS_NEW1(Type, P1)                                  Type::SP::New(P1, Type::SP::_GUARD,  __FILE__, __LINE__)
#define VFS_NEW2(Type, P1,P2)                               Type::SP::New(P1,P2, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW3(Type, P1,P2,P3)                            Type::SP::New(P1,P2,P3, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW4(Type, P1,P2,P3,P4)                         Type::SP::New(P1,P2,P3,P4, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW5(Type, P1,P2,P3,P4,P5)                      Type::SP::New(P1,P2,P3,P4,P5, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW6(Type, P1,P2,P3,P4,P5,P6)                   Type::SP::New(P1,P2,P3,P4,P5,P6, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW7(Type, P1,P2,P3,P4,P5,P6,P7)                Type::SP::New(P1,P2,P3,P4,P5,P6,P7, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW8(Type, P1,P2,P3,P4,P5,P6,P7,P8)             Type::SP::New(P1,P2,P3,P4,P5,P6,P7,P8, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW9(Type, P1,P2,P3,P4,P5,P6,P7,P8,P9)          Type::SP::New(P1,P2,P3,P4,P5,P6,P7,P8,P9, Type::SP::_GUARD, __FILE__, __LINE__)
#define VFS_NEW10(Type, P1,P2,P3,P4,P5,P6,P7,P8,P9,P10)     Type::SP::New(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10, Type::SP::_GUARD, __FILE__, __LINE__)


#endif /* VFS_SMARTPOINTER_H_ */
