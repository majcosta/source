#ifndef _STRUCTURE_H_
#define _STRUCTURE_H_

#include <vfs/Core/Interface/vfs_file_interface.h>

namespace ja2xp
{
	bool ConvertStructure(vfs::ReadableFile_t* pStructureFile, vfs::WritableFile_t* pOutputFile);
};

#endif // _STRUCTURE_H_
