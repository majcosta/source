/*
 * bfVFS : vfs/Core/vfs_init.cpp
 *  - initialization functions/classes
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

#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_init.h>
#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/File/vfs_buffer_file.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Core/Interface/vfs_library_interface.h>

#ifdef VFS_WITH_SLF
#	include <vfs/Ext/slf/vfs_slf_library.h>
#endif
#ifdef VFS_WITH_7ZIP
#	include <vfs/Ext/7z/vfs_7z_library.h>
#	include <vfs/Ext/7z/vfs_create_7z_library.h>
#endif

#include <vfs/Tools/vfs_property_container.h>
#include <vfs/Tools/vfs_log.h>

#include <vfs/Aspects/vfs_logging.h>
#include <vfs/Aspects/vfs_settings.h>

/////////////////////////////////

vfs_init::Location::Location()
: m_optional(false)
{
};

vfs_init::Location::~Location()
{
};

/////////////////////////////////

vfs_init::Profile::Profile()
: m_writable(false)
{
};

vfs_init::Profile::~Profile()
{
	locations_t::iterator it = locations.begin();
	for(;it != locations.end(); ++it)
	{
		it->null();
	}
	locations.clear();
}

void vfs_init::Profile::addLocation(Location::SP loc)
{
	locations.push_back(loc);
}

/////////////////////////////////

vfs_init::VfsConfig::~VfsConfig()
{
	profiles_t::iterator it = profiles.begin();
	for(;it != profiles.end(); ++it)
	{
		it->null();
	}
	profiles.clear();
}

void vfs_init::VfsConfig::addProfile(Profile::SP prof)
{
	profiles.push_back(prof);
}

void vfs_init::VfsConfig::appendConfig(VfsConfig::SP conf)
{
	VfsConfig::profiles_t::iterator it = conf->profiles.begin();
	for(; it != conf->profiles.end(); ++it)
	{
		// even if the other object owns its profiles, this one does not
		profiles.push_back(*it);
	}
}

/********************************************************************/
/********************************************************************/

bool vfs_init::initVirtualFileSystem(vfs::Path const& vfs_ini)
{
	std::list<vfs::Path> li;
	li.push_back(vfs_ini);
	return initVirtualFileSystem(li);
}
bool vfs_init::initVirtualFileSystem(std::list<vfs::Path> const& vfs_ini_list)
{
	vfs::PropertyContainer oVFSProps;
	std::list<vfs::Path>::const_iterator clit = vfs_ini_list.begin();
	for(; clit != vfs_ini_list.end(); ++clit)
	{
		oVFSProps.initFromIniFile(*clit);
	}
	return initVirtualFileSystem(oVFSProps);
}
bool vfs_init::initVirtualFileSystem(vfs::PropertyContainer& oVFSProps)
{
	VFS_LOG_INFO(L"Processing VFS configuration");
	VfsConfig::SP conf = VFS_NEW(VfsConfig);

	std::list<vfs::String> lProfiles, lLocSections;

	oVFSProps.getStringListProperty(L"vfs_config",L"PROFILES",lProfiles,L"");
	if(lProfiles.empty())
	{
		VFS_LOG_ERROR(L"no profiles specified");
		return false;
	}

	std::list<vfs::String>::const_iterator prof_cit = lProfiles.begin();
	for(; prof_cit != lProfiles.end(); ++prof_cit)
	{
		vfs::String sProfSection = vfs::String("PROFILE_") + vfs::String(*prof_cit);

		Profile::SP prof = VFS_NEW(Profile);
		prof->m_name     = oVFSProps.getStringProperty(sProfSection,L"NAME",L"");
		prof->m_root     = oVFSProps.getStringProperty(sProfSection,L"PROFILE_ROOT",L"");
		prof->m_writable = oVFSProps.getBoolProperty  (sProfSection,L"WRITE",false);

		lLocSections.clear();
		oVFSProps.getStringListProperty(sProfSection,L"LOCATIONS",lLocSections,L"");

		std::list<vfs::String>::iterator loc_it = lLocSections.begin();
		for(; loc_it != lLocSections.end(); ++loc_it)
		{
			vfs::String sLocSection = vfs::String("LOC_") + vfs::String(*loc_it);

			Location::SP loc   = VFS_NEW(Location);
			loc->m_path        = oVFSProps.getStringProperty(sLocSection,L"PATH",L"");
			loc->m_vfs_path    = oVFSProps.getStringProperty(sLocSection,L"VFS_PATH",L"");
			loc->m_mount_point = oVFSProps.getStringProperty(sLocSection,L"MOUNT_POINT",L"");
			loc->m_type        = oVFSProps.getStringProperty(sLocSection,L"TYPE",L"NOT_FOUND");
			loc->m_optional    = oVFSProps.getBoolProperty  (sLocSection,L"OPTIONAL",false);

			prof->addLocation(loc);
		}
		conf->addProfile(prof);
	}
	return initVirtualFileSystem(conf);
}

bool vfs_init::initWriteProfile(vfs::VirtualProfile::SP profile)
{
	typedef vfs::TDirectory<vfs::IWritable> WDir_t;
	WDir_t::SP dir;
	vfs::IBaseLocation::SP loc = profile->getLocation(vfs::Path(vfs::Const::EMPTY()));
	if(!loc.isNull())
	{
		dir = dynamic_cast<WDir_t*>(loc.get());
	}
	else
	{
		VFS_LOG_WARNING(_BS(L"Could not find location (\"\") for profile '") << profile->cName << L"'" << _BS::wget );
		VFS_LOG_WARNING(_BS(L"Trying to initialize profile root : ")         << profile->cRoot         << _BS::wget );
		vfs::DirectoryTree::SP dir_tree = VFS_NEW2(vfs::DirectoryTree, vfs::Path(vfs::Const::EMPTY()), profile->cRoot );
		if(!dir_tree->init())
		{
			return false;
		}
		VFS_TRYCATCH_RETHROW( profile->addLocation(dir_tree), L"" );
		getVFS()->addLocation(dir_tree, profile);
		dir = dir_tree;
	}
	return !dir.isNull();
}


bool vfs_init::initVirtualFileSystem(vfs_init::VfsConfig::SP const& conf)
{
	VFS_LOG_INFO(L"Initializing Virtual File System");

	vfs::VirtualFileSystem *pVFS = getVFS();

	if(conf->profiles.empty())
	{
		return false;
	}

	VfsConfig::profiles_t::const_iterator prof_it = conf->profiles.begin();
	for(; prof_it != conf->profiles.end(); ++prof_it)
	{
		Profile::SP prof = *prof_it;
		VFS_LOG_INFO(_BS(L"  Creating profile : ") << prof->m_name << _BS::wget);

		vfs::Path profileRoot         = prof->m_root;
		bool bIsWritable              = prof->m_writable;

		vfs::ProfileStack::SP   pPS   = pVFS->getProfileStack();
		vfs::VirtualProfile::SP pProf = pPS-> getProfile(prof->m_name);
		if(pProf.isNull())
		{
			pProf = VFS_NEW3(vfs::VirtualProfile, prof->m_name, profileRoot, bIsWritable);
			pPS->pushProfile(pProf);
		}
		else
		{
			VFS_THROW_IFF(pProf->cWritable == bIsWritable, L"Profile already exists, but has different write properties");
			VFS_THROW_IFF(pProf->cRoot     == profileRoot, L"Profile already exists, but uses a different root directory");
			continue;
		}


		Profile::locations_t::const_iterator loc_it = prof->locations.begin();
		loc_it = prof->locations.begin();
		for(; loc_it != prof->locations.end(); ++loc_it)
		{
			Location::SP loc  = *loc_it;
			bool bOptional = loc->m_optional;

			if(vfs::StrCmp::Equal(loc->m_type,L"LIBRARY"))
			{
				vfs::ReadableFile_t::SP lib_file;

				vfs::Path fullpath = profileRoot + loc->m_path;

				if(!loc->m_path.empty())
				{
					// try regular file
					lib_file = vfs::ReadableFile_t::cast( VFS_NEW1(vfs::File, fullpath) );
					VFS_LOG_INFO( _BS(L"    library   : ") << fullpath << _BS::wget );
				}
				if(lib_file.isNull() && !loc->m_vfs_path.empty())
				{
					// if regular file doesn't exist, try to find it in the (partially initialized) VFS
					lib_file = pVFS->getReadFile(profileRoot + loc->m_vfs_path);
					VFS_LOG_INFO( _BS(L"    library   : ") << fullpath << L"  [vfs]" << _BS::wget );
				}
				if(!lib_file.isNull())
				{
					vfs::String ext;
					lib_file->getName().extension(ext);
					vfs::ILibrary::SP library;

					if(vfs::StrCmp::Equal(ext,L"slf"))
					{
#ifdef VFS_WITH_SLF
						library = VFS_NEW2(vfs::SLFLibrary, lib_file, loc->m_mount_point );
#else
						VFS_LOG_ERROR(L"      Initializing SLF library : SLF support disabled");
						continue;
#endif
					}
					else if(vfs::Uncompressed7zLibrary::checkSignature(lib_file))
					{
#ifdef VFS_WITH_7ZIP
						library = VFS_NEW2(vfs::Uncompressed7zLibrary, lib_file, loc->m_mount_point );
#else
						VFS_LOG_ERROR(L"      Initializing 7zip library : 7zip support disabled");
						continue;
#endif
					}
					else
					{
						VFS_THROW(_BS(L"File is not a SLF or 7z library : ") << loc->m_path << _BS::wget);
					}
					if(!library->init())
					{
						if(!bOptional)
						{
							VFS_THROW(_BS(L"Could not initialize library [ ") << loc->m_path << L" ]" <<
								L" in : profile [ " << prof->m_name << L" ]," <<
								L" path [ " << fullpath << L" ]" << _BS::wget);
						}
					}
					else
					{
						pProf->addLocation( library);
						pVFS ->addLocation( vfs::ReadLocation_t::cast(library), pProf);
					}
				}
				else
				{
					VFS_THROW(_BS(L"File not found : ") << loc->m_path << _BS::wget);
				}
			}
			else if(vfs::StrCmp::Equal(loc->m_type,L"DIRECTORY"))
			{
				vfs::Path fullpath = profileRoot + loc->m_path;
				VFS_LOG_INFO( _BS(L"    directory : ") << fullpath << _BS::wget );

				vfs::IBaseLocation::SP pDirLocation;
				bool init_success = false;
				if(bIsWritable)
				{
					vfs::DirectoryTree::SP pDirTree = VFS_NEW2(vfs::DirectoryTree, loc->m_mount_point, fullpath);
					init_success = pDirTree->init();
					pDirLocation = pDirTree;
				}
				else
				{
					vfs::ReadOnlyDirectoryTree::SP pDirTree = VFS_NEW2(vfs::ReadOnlyDirectoryTree, loc->m_mount_point, fullpath);
					init_success = pDirTree->init();
					pDirLocation = pDirTree;
				}
				if(!init_success)
				{
					VFS_THROW(_BS(L"Could not initialize directory [\"") << loc->m_path << L"\"]" <<
						L" in : profile [\"" << prof->m_name << L"\"]," <<
						L" path [\"" << fullpath << L"\"]" << _BS::wget);
				}
				else
				{
					pProf->addLocation(pDirLocation);
					pVFS ->addLocation(pDirLocation, pProf);
				}
			}
		}
		if(bIsWritable)
		{
			vfs::ProfileStack::SP   pPS   = pVFS->getProfileStack();
			vfs::VirtualProfile::SP pProf = pPS->getProfile(prof->m_name);
			if(pProf.isNull())
			{
				pProf = VFS_NEW3(vfs::VirtualProfile, prof->m_name, profileRoot, true);
				pPS->pushProfile(pProf);
			}
			else if(!pProf->cWritable)
			{
				VFS_THROW(_BS(L"Profile [") << prof->m_name << L"] is supposed to be writable!" << _BS::wget);
			}
			initWriteProfile(pProf);
		}
	}

	return true;
}

bool vfs_init::initVirtualFileSystem(vfs::Path const& libname, vfs_init::VfsConfig::SP const& conf)
{
	VFS_LOG_INFO(_BS(L"Initializing Virtual File System from library : ") << libname << _BS::wget);

	if(conf->profiles.empty())
	{
		// trying to init profiles from a library, but configuration is empty
		return false;
	}

	// need to get the profile stack from the VFS object
	vfs::VirtualFileSystem *pVFS = getVFS();

	// get file for the filename
	vfs::ReadableFile_t::SP lib_file = vfs::ReadableFile_t::cast( VFS_NEW1(vfs::File, libname) );
	vfs::ILibrary::SP       library;

#ifdef VFS_WITH_7ZIP
	if(!vfs::Uncompressed7zLibrary::checkSignature(lib_file))
	{
		VFS_LOG_ERROR(_BS(L"  File is not a 7z archive : ") << libname << _BS::wget);
		return false;
	}
	library = VFS_NEW2(vfs::Uncompressed7zLibrary, lib_file, L"");
#else
	VFS_LOG_ERROR(L"  Initializing 7zip library : 7zip support disabled");
	return false;
#endif

	VFS_LOG_INFO(L"  Initializing library ..");

	if(!library->init())
	{
		VFS_LOG_ERROR(_BS(L"  Could not initialize library : ") << libname << _BS::wget);
		return false;
	}

	/*
	 *  library initialization didn't fail, now
	 *  1. create all profiles from the configuration,
	 *  2. map library paths to profile paths and then
	 *  3. iterate over locations in the profiles and add them to the VFS
	 *
	 *  For point 3. all directories in the library must already be mapped to profile locations (2.), or otherwise
	 *  they cannot be added to the VFS correctly. And for point 2. all profiles must exist or the mapping won't be
	 *  complete. Therefore the mapping cannot happen in point 3.
	 */

	VFS_LOG_INFO(L"  Mapping locations ..");

	// 1.
	VfsConfig::profiles_t::const_iterator prof_it = conf->profiles.begin();
	for(; prof_it != conf->profiles.end(); ++prof_it)
	{
		/*
		 * make sure profile object exists and has no duplicate (with the same name)
		 */

		Profile::SP             prof  = *prof_it;

		vfs::ProfileStack::SP   pPS   = pVFS->getProfileStack();
		vfs::VirtualProfile::SP pProf = pPS ->getProfile(prof->m_name);
		if(pProf.isNull())
		{
			pProf = VFS_NEW3(vfs::VirtualProfile, prof->m_name, prof->m_root, false);
			pPS->pushProfile(pProf);
		}
		else
		{
			VFS_THROW_IFF(pProf->cRoot == prof->m_root, L"Profile already exists, but uses a different root directory");
			continue;
		}
	}

	// 2.
	library->mapProfilePaths(conf);

	// 3.
	prof_it = conf->profiles.begin();
	for(; prof_it != conf->profiles.end(); ++prof_it)
	{
		Profile::SP             prof  = *prof_it;

		VFS_LOG_INFO(_BS(L"  Creating profile : ") << prof->m_name << _BS::wget);

		vfs::ProfileStack::SP   pPS   = pVFS->getProfileStack();
		vfs::VirtualProfile::SP pProf = pPS ->getProfile(prof->m_name);
		if(pProf.isNull())
		{
			VFS_THROW_IFF(pProf->cRoot == prof->m_root, L"Profile already exists, but uses a different root directory");
			continue;
		}

		// iterate over all locations of a profile
		Profile::locations_t::iterator loc_it = prof->locations.begin();
		for(; loc_it != prof->locations.end(); ++loc_it)
		{
			Location::SP loc = *loc_it;
			bool bOptional   =  loc->m_optional;

			if(vfs::StrCmp::Equal(loc->m_type,L"LIBRARY"))
			{
				// a library location is a technically an archive in an archive
				vfs::ReadableFile_t *pLibFile = NULL;
				bool bOwnFile                 = false;

				// assemble library filename
				vfs::Path fullpath            = prof->m_root + loc->m_path;

				if(!loc->m_path.empty())
				{
					// try file specified by a full library path
					pLibFile = vfs::ReadableFile_t::cast( library->getFile(fullpath, NULL) );
					bOwnFile = false;
					VFS_LOG_INFO( _BS(L"    library   : ") << fullpath << _BS::wget );
				}
				if(!pLibFile && !loc->m_vfs_path.empty())
				{
					// if file doesn't exist, try to find it in the (partially initialized) VFS
					// it might have been specified in a previously processed profile
					pLibFile = vfs::ReadableFile_t::cast( library->getFile(fullpath, pProf) );
					VFS_LOG_INFO( _BS(L"    library   : ") << fullpath << L"  [vfs]" << _BS::wget );
				}
				if(pLibFile)
				{
					vfs::Path full_str = pLibFile->getName();
					vfs::String ext;
					full_str.extension(ext);

					vfs::ILibrary::SP pLib;
					if(vfs::StrCmp::Equal(ext,L"slf"))
					{
#ifdef VFS_WITH_SLF
						pLib = VFS_NEW2(vfs::SLFLibrary, pLibFile, loc->m_mount_point);
#else
						VFS_LOG_ERROR(L"      Initializing SLF library : SLF support disabled");
						continue;
#endif
					}
					else if(vfs::Uncompressed7zLibrary::checkSignature(pLibFile))
					{
#ifdef VFS_WITH_7ZIP
						pLib = VFS_NEW2(vfs::Uncompressed7zLibrary, pLibFile, loc->m_mount_point);
#else
						VFS_LOG_ERROR(L"      Initializing 7zip library : 7zip support disabled");
						continue;
#endif
					}
					else
					{
						VFS_THROW(_BS(L"File is not a SLF or 7z library : ") << loc->m_path << _BS::wget);
					}
					if(!pLib->init())
					{
						if(!bOptional)
						{
							VFS_THROW(_BS(L"Could not initialize library [ ") << loc->m_path << L" ]" <<
								L" in : profile [ " << prof->m_name << L" ]," <<
								L" path [ "         << fullpath     << L" ]"  << _BS::wget);
						}
					}
					else
					{
						pProf->addLocation(pLib);
						pVFS ->addLocation(pLib, pProf);
					}
				}
				else
				{
					VFS_THROW(_BS(L"File not found : ") << loc->m_path << _BS::wget);
				}
			}
			else if(vfs::StrCmp::Equal(loc->m_type,L"DIRECTORY"))
			{
				// A directory location is a part of the library, therefore the library itself can be used as a stand-in for the location.
				// It must be ensured, though, that only the profile directories in the library are associated with the profile object.

				vfs::Path fullpath = prof->m_root + loc->m_path;
				VFS_LOG_INFO(_BS(L"    directory : ") << fullpath << _BS::wget );

				vfs::IBaseLocation::SP pLocation = library;

				pProf->addLocation(pLocation);
				pVFS-> addLocation(pLocation, pProf);
			}
		}
	}
	return true;
}
