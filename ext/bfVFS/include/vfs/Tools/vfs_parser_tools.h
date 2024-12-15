/*
 * bfVFS : vfs/Tools/vfs_parser_tools.h
 *  - read file line-wise,
 *  - split string into tokens,
 *  - simple pattern matching
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

#ifndef _VFS_PARSER_TOOLS_H_
#define _VFS_PARSER_TOOLS_H_

#include <vfs/Core/vfs_types.h>
#include <vfs/Core/Interface/vfs_file_interface.h>

#include <list>

namespace vfs
{
	class VFS_API ReadLine
	{
		static const vfs::size_t BUFFER_SIZE = 1024;

	public:
		ReadLine(ReadableFile_t* file);
		~ReadLine();

		bool fillBuffer();
		bool fromBuffer(std::string& line);
		bool getLine   (std::string& line);

	private:
		vfs::Byte           _buffer[BUFFER_SIZE+1];
		ReadableFile_t::SP  _file;
		vfs::size_t         _bytes_left;
		vfs::size_t         _buffer_pos;
		vfs::size_t	        _buffer_last;
		bool                _eof;

		void operator=(ReadLine const& rl);
	};

	/**************************************************************/
	/**************************************************************/

	class VFS_API Tokenizer
	{
	public:
		Tokenizer(String const& str);
		~Tokenizer();

		bool next(String& token, String::char_t delimeter = L',');

	private:
		const String	m_list;
		String::size_t	m_current, m_next;

		void operator=(Tokenizer const& str);
	};

	VFS_API bool matchPattern(String const& pattern, String        const& str);
	VFS_API bool matchPattern(String const& pattern, String::str_t const& str);

} // namespace vfs

#endif // _VFS_PARSER_TOOLS_H_

