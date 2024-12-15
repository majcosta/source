/*
 * bfVFS : vfs/Tools/vfs_property_container.h
 *  - <string,string> key-value map with capability to convert values to other types
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

#ifndef _VFS_PROPERTY_CONTAINER_H_
#define _VFS_PROPERTY_CONTAINER_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/Interface/vfs_file_interface.h>

#include <map>
#include <string>
#include <list>
#include <set>
#include <ostream>

namespace vfs
{
	class VFS_API PropertyContainer
	{
	public:
		class TagMap
		{
			typedef std::map<String,String> TagMap_t;

		public:
			TagMap();
			String const& container(String::char_t* container  = NULL);
			String const& section  (String::char_t* section    = NULL);
			String const& sectionID(String::char_t* section_id = NULL);
			String const& key      (String::char_t* key        = NULL);
			String const& keyID    (String::char_t* key_id     = NULL);

		private:
			TagMap_t _map;
		};

	public:
		PropertyContainer(){};
		~PropertyContainer(){};

		void            clearContainer();

		bool            initFromIniFile      (Path const& filename);
		bool            initFromIniFile      (ReadableFile_t *file);
		bool            writeToIniFile       (Path const& filename, bool create_new = false);

		// these methods are not implemented
		bool            initFromXMLFile      (Path const& filename, TagMap& tagmap);
		bool            writeToXMLFile       (Path const& filename, TagMap& tagmap);

		void            printProperties      (std::ostream &out);

		bool            hasProperty          (String const& section, String const& key);
		//
		String const&   getStringProperty    (String const& section, String const& key, vfs::String const&     default_value=L"");
		bool            getStringProperty    (String const& section, String const& key, vfs::String&           value,         String const& default_value=L"");
		bool            getStringProperty    (String const& section, String const& key, vfs::String::char_t*   value,         vfs::size_t   len, String const& default_value=L"");
		//
		vfs::Int64      getIntProperty       (String const& section, String const& key, vfs::Int64             default_value);
		vfs::Int64      getIntProperty       (String const& section, String const& key, vfs::Int64             default_value, vfs::Int64  min_value, vfs::Int64  max_value);
		//
		vfs::UInt64     getUIntProperty      (String const& section, String const& key, vfs::UInt64            default_value);
		vfs::UInt64     getUIntProperty      (String const& section, String const& key, vfs::UInt64            default_value, vfs::UInt64 min_value, vfs::UInt64 max_value);
		//
		double          getFloatProperty     (String const& section, String const& key, double                 default_value);
		double          getFloatProperty     (String const& section, String const& key, double                 default_value, double      min_value, double      max_value);
		//
		bool            getBoolProperty      (String const& section, String const& key, bool                   default_value);
		//
		bool            getStringListProperty(String const& section, String const& key, std::list<String>      &value_list,   String      default_value);
		bool            getIntListProperty   (String const& section, String const& key, std::list<vfs::Int64>  &value_list,   vfs::Int64  default_value);
		bool            getUIntListProperty  (String const& section, String const& key, std::list<vfs::UInt64> &value_list,   vfs::UInt64 default_value);
		bool            getFloatListProperty (String const& section, String const& key, std::list<double>      &value_list,   double      default_value);
		bool            getBoolListProperty  (String const& section, String const& key, std::list<bool>        &value_list,   bool        default_value);
		//
		void            setStringProperty    (String const& section, String const& key, String                 const& value);
		//
		void            setIntProperty       (String const& section, String const& key, vfs::Int64             const& value);
		void            setUIntProperty      (String const& section, String const& key, vfs::UInt64            const& value);
		void            setFloatProperty     (String const& section, String const& key, double                 const& value);
		void            setBoolProperty      (String const& section, String const& key, bool                   const& value);
		//
		void            setStringListProperty(String const& section, String const& key, std::list<String>      const& value);
		void            setIntListProperty   (String const& section, String const& key, std::list<vfs::Int64>  const& value);
		void            setUIntListProperty  (String const& section, String const& key, std::list<vfs::UInt64> const& value);
		void            setFloatListProperty (String const& section, String const& key, std::list<double>      const& value);
		void            setBoolListProperty  (String const& section, String const& key, std::list<bool>        const& value);

	private:
		enum EOperation
		{
			Error, Set, Add,
		};
		bool            extractSection       (String::str_t const& readStr, vfs::size_t startPos, vfs::String::str_t& section);
		EOperation      extractKeyValue      (String::str_t const& readStr, vfs::size_t startPos, vfs::String::str_t& key, vfs::String::str_t& value);

	private:
		class Section
		{
			friend  class PropertyContainer;
			typedef std::map<String,String, String::Less> Props_t;

		public:
			bool        has  (String const& key);
			bool        add  (String const& key, String const& value);
			bool        value(String const& key, String& value);
			String&     value(String const& key);
			void        print(std::ostream& out, String::str_t sPrefix = L"");
			void        clear();

		private:
			Props_t     mapProps;
		};
		typedef std::map<String, Section, String::Less> Sections_t;

		bool            getValueForKey(String const& section, String const& key, String &value);

		Section&        section(String const& section);

		Sections_t      m_mapProps;
	};

} // namespace vfs

#endif // _VFS_PROPERTY_CONTAINER_H_
