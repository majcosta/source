/*
 * bfVFS : vfs/Tools/vfs_property_container.cpp
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

#include <vfs/vfs_config.h>

#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs_os_functions.h>
#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/File/vfs_buffer_file.h>

#include <vfs/Tools/vfs_tools.h>
#include <vfs/Tools/vfs_parser_tools.h>
#include <vfs/Tools/vfs_property_container.h>

#include <vfs/Aspects/vfs_logging.h>

#include <sstream>
#include <vector>

/*************************************************************************************/
/*************************************************************************************/

bool vfs::PropertyContainer::Section::has(vfs::String const& key)
{
	return mapProps.find(key) != mapProps.end();
}

bool vfs::PropertyContainer::Section::add(vfs::String const& key, vfs::String const& value)
{
	if(!mapProps[key].empty())
	{
		mapProps[key] += L", ";
	}
	mapProps[key] += value;
	return true;
}

vfs::String& vfs::PropertyContainer::Section::value(vfs::String const& key)
{
	return mapProps[key];
}
bool vfs::PropertyContainer::Section::value(vfs::String const& key, vfs::String& value)
{
	Props_t::iterator sit = mapProps.find(key);
	if(sit != mapProps.end())
	{
		value.r_wcs().assign(sit->second.c_wcs());
		return true;
	}
	return false;
}
void vfs::PropertyContainer::Section::print(std::ostream& out, vfs::String::str_t sPrefix)
{
	Props_t::iterator sit = mapProps.begin();
	for(; sit != mapProps.end(); ++sit)
	{
		out << String::as_utf8(sPrefix) << sit->first.utf8() << " = " << sit->second.utf8() << "\r\n";
	}
}

void vfs::PropertyContainer::Section::clear()
{
	mapProps.clear();
}

/*************************************************************************************/
/*************************************************************************************/
void vfs::PropertyContainer::clearContainer()
{
	Sections_t::iterator it = m_mapProps.begin();
	for(;it != m_mapProps.end(); ++it)
	{
		it->second.clear();
	}
	m_mapProps.clear();
}

bool vfs::PropertyContainer::extractSection(vfs::String::str_t const& readStr, vfs::size_t startPos, vfs::String::str_t& section)
{
	// extract section name
	vfs::size_t close = readStr.find_first_of(L"]", startPos);
	if( close != vfs::npos && close > startPos)
	{
		startPos += 1;
		section   = trimString(readStr,startPos,(vfs::size_t)(close-1));
		return true;
	}
	return false;
}

vfs::PropertyContainer::EOperation vfs::PropertyContainer::extractKeyValue(vfs::String::str_t const &readStr, vfs::size_t startPos, vfs::String::str_t& key, vfs::String::str_t& value)
{
	vfs::size_t iEqual = readStr.find_first_of(L"+=", startPos);
	if(iEqual == vfs::npos)
	{
		VFS_LOG_WARNING(_BS("WARNING : could not extract key-value pair : ") << readStr << _BS::wget);
		return vfs::PropertyContainer::Error;
	}
	// extract key
	key           = trimString(readStr,0,iEqual-1);
	// extract value
	EOperation op = vfs::PropertyContainer::Set;
	if( readStr.at(iEqual) == L'+' )
	{
		if( (iEqual+1) < readStr.size() && (readStr.at(iEqual+1) == L'=') )
		{
			iEqual += 1;
			op      = PropertyContainer::Add;
		}
	}
	value = trimString(readStr,iEqual+1,readStr.size());
	return op;
}


bool vfs::PropertyContainer::initFromIniFile(vfs::Path const& filename)
{
	// try to open via VirtualFileSystem
	if(getVFS()->fileExists(filename))
	{
		return initFromIniFile(getVFS()->getReadFile(filename));
	}
	else
	{
		File::SP file( VFS_NEW1(File, filename) );
		if(file->openRead())
		{
			return initFromIniFile(ReadableFile_t::cast(file));
		}
		return false;
	}
}

bool vfs::PropertyContainer::initFromIniFile(vfs::ReadableFile_t *file)
{
	if(!file)
	{
		return false;
	}

	std::string   buffer;
	String::str_t current_section;
	int           line_counter = 0;
	ReadLine      rline(file);

	while(rline.getLine(buffer))
	{
		line_counter++;
		// very simple parsing : key = value
		if(!buffer.empty())
		{
			// remove leading white spaces
			::size_t iStart = buffer.find_first_not_of(" \t",0);
			if(iStart == std::string::npos)
			{
				// only white space characters
				continue;
			}
			char first = buffer.at(iStart);
			switch(first)
			{
			case '!':
			case ';':
			case '#':
				// comment -> do nothing
				break;
			case '[':
				{
					vfs::String u8s;
					try
					{
						String::as_utfW(buffer.substr(iStart, buffer.length()-iStart), u8s.r_wcs());
					}
					catch(std::exception& ex)
					{
						VFS_RETHROW( _BS(L"Conversion error in file \"") << file->getPath() << L"\", line " << line_counter << _BS::wget, ex);
					}
					if(this->extractSection(u8s.c_wcs(), 0, current_section))
					{
						m_mapProps[current_section];
					}
					else
					{
						VFS_LOG_WARNING(_BS("WARNING : could not extract section name : ") << buffer << _BS::wget);
					}
				}
				break;
			default:
				{
					// probably key-value pair
					String::str_t u8s;
					try
					{
						String::as_utfW(buffer.substr(iStart, buffer.length()-iStart), u8s);
					}
					catch(std::exception& ex)
					{
						VFS_RETHROW( _BS(L"Conversion error in file \"") << file->getPath() << L"\", line " << line_counter << _BS::wget, ex);
					}
					String::str_t sKey, sValue;
					EOperation op = this->extractKeyValue(u8s, 0, sKey, sValue);
					if(op != Error)
					{
						// add key-value pair to map
						if(m_mapProps.find(current_section) != m_mapProps.end())
						{
							if(op == Set)
							{
								this->section(current_section).value(sKey) = sValue;
							}
							else if(op == Add)
							{
								this->section(current_section).add(sKey, sValue);
							}
						}
						else
						{
							VFS_LOG_WARNING(_BS(L"ERROR : could not find section [") << current_section << L"] in container" << _BS::wget);
						}
					}
				}
				break;
			}; // end switch
		} // end if (empty)
	} // end while(!eof)
	return true;
}

static vfs::UByte utf8bom[4] = {0xef,0xbb,0xbf,0x0};

bool vfs::PropertyContainer::writeToIniFile(vfs::Path const& filename, bool createNew)
{
#ifdef WIN32
	const char ENDL[] = "\r\n";
#else
	const char ENDL[] = "\n";
#endif
	if(createNew)
	{
		WritableFile_t::SP file;
		try
		{
			OpenWriteFile wfile(filename,true,true);
			file = wfile.file();
			wfile.release();
		}
		catch(std::exception& ex)
		{
			VFS_LOG_WARNING(ex.what());
			// vfs not initialized?

			File::SP cfile( VFS_NEW1(File, filename) );
			cfile->openWrite(true,true);
			file = WritableFile_t::cast(cfile);
		}

		Sections_t::iterator sit = m_mapProps.begin();
		std::stringstream    ss;
		std::string          str;

		ss << (char*)utf8bom;
		for(; sit != m_mapProps.end(); ++sit)
		{
			ss.str("");
			ss << "[" << sit->first.utf8() << "]" << ENDL;
			str = ss.str();
			file->write(str.c_str(), str.length());

			ss.clear();
			ss.str("");
			Section& section = sit->second;
			section.print(ss);
			ss << ENDL;
			str = ss.str();
			file->write(str.c_str(),str.length());
		}
		file->close();
		return true;
	}
	else
	{
		// try to open via VirtualFileSystem
		BufferFile::SP rfile( VFS_NEW(BufferFile) );
		if(getVFS()->fileExists(filename))
		{
			OpenReadFile rf(filename);
			rfile->copyToBuffer(rf.file());
		}
		else if(getVFS()->createNewFile(filename))
		{
			ReadableFile_t::SP file = getVFS()->getReadFile(filename);
			if(!file.isNull() && file->openRead())
			{
				rfile->copyToBuffer(file);
				file->close();
			}
		}
		else
		{
			// file doesn't exist or VFS not initialized yet
			File::SP file( VFS_NEW1(File,filename) );
			rfile->copyToBuffer(ReadableFile_t::cast(file));
		}

		std::stringstream  outbuffer;
		std::string        buffer;
		vfs::String::str_t current_section;

		std::set<String> setKeys;
		std::set<String> setSections;
		Sections_t::iterator sit = m_mapProps.begin();
		for(; sit != m_mapProps.end(); ++sit)
		{
			setSections.insert(sit->first);
		}

		ReadLine rline(ReadableFile_t::cast(rfile));
		outbuffer << (char*)(utf8bom);
		vfs::UInt32 line_counter = 0;
		while(rline.getLine(buffer))
		{
			line_counter++;
			if(!buffer.empty())
			{
				// remove leading white spaces
				size_t iStart = buffer.find_first_not_of(" \t",0);
				char   first  = buffer.at(iStart);
				switch(first)
				{
				case '!':
				case ';':
				case '#':
					outbuffer << buffer << ENDL;
					break;
				case '[':
					{
						String u8s;
						try
						{
							String::as_utfW(buffer.substr(iStart, buffer.length()-iStart), u8s.r_wcs());
						}
						catch(std::exception& ex)
						{
							VFS_RETHROW(_BS(L"Conversion error in file \"") << filename << L"\", line " << line_counter << _BS::wget, ex);
						}
						String::str_t oldSection = current_section;
						if(this->extractSection(u8s.c_wcs(), 0, current_section))
						{
							if(setSections.find(current_section) == setSections.end())
							{
								// section already handled ?!?!?!
								// just print duplicate version
								outbuffer << String::as_utf8(buffer) << ENDL;
								break;
							}
							if(!setKeys.empty())
							{
								// there are new keys in the previous section
								Section& oldsec = m_mapProps[oldSection];

								std::set<String>::iterator kit = setKeys.begin();
								for(; kit != setKeys.end(); ++kit)
								{
									outbuffer << String::as_utf8(*kit) << " = " << String::as_utf8(oldsec.value(*kit)) << ENDL;
								}
								// all remaining keys were written, clear set
								setKeys.clear();
								outbuffer << ENDL;
							}
							Section& sec = m_mapProps[current_section];
							Section::Props_t::iterator it = sec.mapProps.begin();
							for(; it != sec.mapProps.end(); ++it)
							{
								setKeys.insert(it->first);
							}
						}
						outbuffer << String::as_utf8(buffer) << ENDL;
					}
					break;
				default:
					{
						// probably key-value pair
						String u8s;
						try
						{
							String::as_utfW(buffer.substr(iStart, buffer.length()-iStart), u8s.r_wcs());
						}
						catch(std::exception& ex)
						{
							VFS_RETHROW(_BS(L"Conversion error in file \"") << filename << L"\", line " << line_counter << _BS::wget, ex);
						}
						String::str_t key, value;
						if(this->extractKeyValue(u8s.c_wcs(), 0, key, value))
						{
							if(setKeys.find(key) != setKeys.end())
							{
								outbuffer << String::as_utf8(key) << " = " << String::as_utf8(m_mapProps[current_section].value(key)) << "\r\n";
								setKeys.erase(key);
							}
							else
							{
								outbuffer << String::as_utf8(buffer) << ENDL;
							}
							if(setKeys.empty())
							{
								setSections.erase(current_section);
							}
						}
					}
					break;
				}; // end switch
			}
			else
			{
				outbuffer << ENDL;
			}
		}
		if(!setKeys.empty())
		{
			Section& sec = m_mapProps[current_section];
			std::set<String>::iterator kit = setKeys.begin();
			for(; kit != setKeys.end(); ++kit)
			{
				outbuffer << String::as_utf8(*kit) << " = " << String::as_utf8(sec.value(*kit)) << ENDL;
			}
			setKeys.clear();
			if(setKeys.empty())
			{
				setSections.erase(current_section);
			}
		}
		std::set<String>::iterator it = setSections.begin();
		for(; it != setSections.end(); ++it)
		{
			outbuffer << ENDL << "[" << String::as_utf8(*it) << "]" << ENDL;
			std::stringstream ss;
			m_mapProps[*it].print(outbuffer);
		}

		try
		{
			OpenWriteFile wfile(filename,true,true);
			wfile->write(outbuffer.str().c_str(),(vfs::size_t)outbuffer.str().length());
		}
		catch(std::exception& ex)
		{
			VFS_LOG_WARNING(ex.what());
			File::SP file( VFS_NEW1(File, filename) );
			if(file->openWrite(true,true))
			{
				file->write(outbuffer.str().c_str(),(vfs::size_t)outbuffer.str().length());
				file->close();
			}
		}
		return true;
	}
}


void vfs::PropertyContainer::printProperties(std::ostream &out)
{
	Sections_t::iterator pit = m_mapProps.begin();
	for(;pit != m_mapProps.end(); ++pit)
	{
		out << "[" << pit->first.utf8() << "]\n";
		pit->second.print(out, L"  ");
		out << std::endl;
	}
}


vfs::PropertyContainer::Section& vfs::PropertyContainer::section(vfs::String const& _section)
{
	return m_mapProps[_section];
}

bool vfs::PropertyContainer::getValueForKey(vfs::String const& _section, vfs::String const& _key, vfs::String &value)
{
	Sections_t::iterator pit = m_mapProps.find(trimString(_section,0,(vfs::size_t)_section.length()));
	if( pit != m_mapProps.end() )
	{
		return pit->second.value( trimString(_key,0,(vfs::size_t)_key.length()), value );
	}
	return false;
}

bool vfs::PropertyContainer::hasProperty(vfs::String const& _section, vfs::String const& _key)
{
	Sections_t::iterator pit = m_mapProps.find(trimString(_section,0,(vfs::size_t)_section.length()));
	if( pit != m_mapProps.end() )
	{
		return pit->second.has(trimString(_key,0,_key.length()));
	}
	return false;
}

vfs::String const& vfs::PropertyContainer::getStringProperty(vfs::String const& _section, vfs::String const& _key, vfs::String const& default_value)
{
	Sections_t::iterator sit = m_mapProps.find(_section);
	if(sit != m_mapProps.end())
	{
		Section::Props_t::iterator pit = sit->second.mapProps.find(_key);
		if(pit != sit->second.mapProps.end())
		{
			return pit->second;
		}
	}
	return default_value;
}

bool vfs::PropertyContainer::getStringProperty(vfs::String const& _section, vfs::String const& _key, vfs::String& value, vfs::String const& default_value)
{
	if(getValueForKey(_section,_key,value))
	{
		return true;
	}
	value = default_value;
	return false;
}

bool vfs::PropertyContainer::getStringProperty(vfs::String const& _section, vfs::String const& _key, vfs::String::char_t* value, vfs::size_t len, vfs::String const& default_value)
{
	String s;
	if(getValueForKey(_section, _key, s))
	{
		vfs::size_t l = std::min<vfs::size_t>(s.length(), len-1);
		wcsncpy(value,s.c_str(), l);
		value[l] = 0;
		return true;
	}
	vfs::size_t l = std::min<vfs::size_t>(default_value.length(), len-1);
	wcsncpy(value,default_value.c_str(), l);
	value[l] = 0;
	return false;
}

vfs::Int64 vfs::PropertyContainer::getIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::Int64 default_value, vfs::Int64 min_value, vfs::Int64 max_value)
{
	return std::min<vfs::Int64>(max_value, std::max<vfs::Int64>(min_value, this->getIntProperty(_section, _key, default_value)));
}

vfs::Int64 vfs::PropertyContainer::getIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::Int64 default_value)
{
	String value;
	if(getValueForKey(_section,_key,value))
	{
		vfs::Int64 result;
		if(convertTo<vfs::Int64>(value,result))
		{
			return result;
		}
	}
	return default_value;
}

vfs::UInt64 vfs::PropertyContainer::getUIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::UInt64 default_value, vfs::UInt64 min_value, vfs::UInt64 max_value)
{
	return std::min<vfs::UInt64>(max_value, std::max<vfs::UInt64>(min_value, this->getIntProperty(_section, _key, default_value)));
}

vfs::UInt64 vfs::PropertyContainer::getUIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::UInt64 default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		vfs::UInt64 result;
		if(convertTo<vfs::UInt64>(value,result))
		{
			return result;
		}
	}
	return default_value;
}

double vfs::PropertyContainer::getFloatProperty(vfs::String const& _section, vfs::String const& _key, double default_value, double  min_value, double max_value)
{
	return std::min<double>(max_value, std::max<double>(min_value, this->getFloatProperty(_section, _key, default_value)));
}

double vfs::PropertyContainer::getFloatProperty(vfs::String const& _section, vfs::String const& _key, double default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		double result;
		if(convertTo<double>(value,result))
		{
			return result;
		}
	}
	return default_value;
}

bool vfs::PropertyContainer::getBoolProperty(vfs::String const& _section, vfs::String const& _key, bool default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		vfs::Int32 result;
		if( StrCmp::Equal(value,L"true") || ( convertTo<>(value,result) && (result != 0) ) )
		{
			return true;
		}
		else if( StrCmp::Equal(value,"false") || ( convertTo<>(value,result) && (result == 0) ) )
		{
			return false;
		}
		// else return bDefaultValue
	}
	return default_value;
}

bool vfs::PropertyContainer::getStringListProperty(vfs::String const& _section, vfs::String const& _key, std::list<vfs::String> &value_list, vfs::String default_value)
{
	String value;
	if(getValueForKey(_section,_key,value))
	{
		Tokenizer splitter(value);
		String    entry;
		while( splitter.next(entry, L',') )
		{
			value_list.push_back(vfs::trimString(entry,0,entry.length()));
		}
		return true;
	}
	return false;
}

bool vfs::PropertyContainer::getIntListProperty(vfs::String const& _section, vfs::String const& _key, std::list<vfs::Int64> &value_list, vfs::Int64 default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		String    entry;
		Tokenizer splitter(value);
		while( splitter.next(entry, L',') )
		{
			vfs::Int64 result;
			if(convertTo<vfs::Int64>(entry,result))
			{
				value_list.push_back(result);
			}
			else
			{
				value_list.push_back(default_value);
			}
		}
		return true;
	}
	return false;
}

bool vfs::PropertyContainer::getUIntListProperty(vfs::String const& _section, vfs::String const& _key, std::list<vfs::UInt64> &value_list, vfs::UInt64 default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		String    entry;
		Tokenizer splitter(value);
		while( splitter.next(entry, L',') )
		{
			vfs::UInt64 result;
			if(convertTo<vfs::UInt64>(entry,result))
			{
				value_list.push_back(result);
			}
			else
			{
				value_list.push_back(default_value);
			}
		}
		return true;
	}
	return false;
}
bool vfs::PropertyContainer::getFloatListProperty(vfs::String const& _section, vfs::String const& _key, std::list<double> &value_list, double default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		String    entry;
		Tokenizer splitter(value);
		while( splitter.next(entry, L',') )
		{
			double result;
			if(convertTo<double>(entry,result))
			{
				value_list.push_back(result);
			}
			else
			{
				value_list.push_back(default_value);
			}
		}
		return true;
	}
	return false;
}

bool vfs::PropertyContainer::getBoolListProperty(vfs::String const& _section, vfs::String const& _key, std::list<bool> &value_list, bool default_value)
{
	String value;
	if(getValueForKey(_section, _key, value))
	{
		String    entry;
		Tokenizer splitter(value);
		while( splitter.next(entry, L',') )
		{
			vfs::Int32 result;
			if( StrCmp::Equal(entry,L"true") || ( convertTo<>(entry,result) && (result != 0) ) )
			{
				value_list.push_back(true);
			}
			else if( StrCmp::Equal(entry,L"false") || ( convertTo<>(entry,result) && (result == 0) ) )
			{
				value_list.push_back(false);
			}
			else
			{
				value_list.push_back(default_value);
			}
		}
		return true;
	}
	return false;
}

void vfs::PropertyContainer::setStringProperty(vfs::String const& _section, vfs::String const& _key, vfs::String const& value)
{
	this->section(_section).value(_key) = value;
}

void vfs::PropertyContainer::setIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::Int64 const& value)
{
	this->section(_section).value(_key) = toString<wchar_t,vfs::Int64>(value);
}

void vfs::PropertyContainer::setUIntProperty(vfs::String const& _section, vfs::String const& _key, vfs::UInt64 const& value)
{
	this->section(_section).value(_key) = toString<wchar_t,vfs::UInt64>(value);
}

void vfs::PropertyContainer::setFloatProperty(vfs::String const& _section, vfs::String const& _key, double const& value)
{
	this->section(_section).value(_key) = toString<wchar_t,double>(value);
}

void vfs::PropertyContainer::setBoolProperty(vfs::String const& _section, vfs::String const& _key, bool const& value)
{
	this->section(_section).value(_key) = toString<wchar_t,bool>(value);
}

void vfs::PropertyContainer::setStringListProperty(vfs::String const& _section, vfs::String const& _key, std::list<vfs::String> const& value)
{
	this->section(_section).value(_key) = toStringList<vfs::String>(value);
}

void vfs::PropertyContainer::setIntListProperty(vfs::String const& _section, vfs::String const& _key, std::list<vfs::Int64> const& value)
{
	this->section(_section).value(_key) = toStringList<vfs::Int64>(value);
}

void vfs::PropertyContainer::setFloatListProperty(vfs::String const& _section, vfs::String const& _key, std::list<double> const& value)
{
	this->section(_section).value(_key) = toStringList<double>(value);
}

void vfs::PropertyContainer::setBoolListProperty(vfs::String const& _section, vfs::String const& _key, std::list<bool> const& value)
{
	this->section(_section).value(_key) = toStringList<bool>(value);
}

/**************************************************************************************************/
/**************************************************************************************************/

