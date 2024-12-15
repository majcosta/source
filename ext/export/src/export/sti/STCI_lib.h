#include "Types.h"
#include "himage.h"

#include <vfs/Core/File/vfs_file.h>

namespace ja2xp
{
	BOOLEAN LoadSTCIFileToImage(vfs::ReadableFile_t *pFile, HIMAGE hImage, UINT16 fContents );

	BOOLEAN IsSTCIETRLEFile(vfs::ReadableFile_t *pFile, CHAR8 * ImageFile );
};
