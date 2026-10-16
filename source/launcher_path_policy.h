#ifndef LAUNCHER_PATH_POLICY_H
#define LAUNCHER_PATH_POLICY_H

#include <stddef.h>
#include <string.h>

#define LAUNCHER_SAVE_BACKUP_PATH_EXTRA 32U
#define LAUNCHER_SAVE_NAME_MAX_BYTES 255U

/* Older launcher indexes and saved paths could retain at most 99 bytes from
   any one name. A component at that boundary cannot be distinguished from a
   longer LFN whose prefix was stored. The old favourite cache also held at
   most path_capacity - 1 bytes, so a path at that length may be truncated. */
static inline int LauncherPathMayBeLegacyTruncated(const char *path, size_t path_capacity)
{
	const char *p;
	size_t component_length = 0;
	size_t path_length;

	if(!path || path[0] != '/' || path_capacity < 2)
		return 1;

	path_length = strlen(path);
	if(path_length >= path_capacity - 1)
		return 1;

	for(p = path + 1; ; p++)
	{
		if(*p == '/' || *p == '\0')
		{
			if(component_length >= 99)
				return 1;
			if(*p == '\0')
				break;
			if(p[1] == '/' || p[1] == '\0')
				return 1;
			component_length = 0;
		}
		else
		{
			component_length++;
		}
	}

	return 0;
}

/* Build the backup directory path without limiting the save basename to the
   launcher's ordinary 255-byte path buffer. The basename must be one path
   component; FatFs resolves each component independently. */
static inline int LauncherBuildSaveBackupPath(const char *backup_dir,
		const char *save_name, char *out, size_t out_capacity)
{
	size_t dir_length;
	size_t name_length;
	size_t needed;

	if(!backup_dir || !save_name || !out || out_capacity == 0)
		return 0;
	out[0] = '\0';
	if(backup_dir[0] == '\0' || save_name[0] == '\0')
		return 0;
	if(strchr(save_name, '/') != NULL || strchr(save_name, '\\') != NULL)
		return 0;

	dir_length = strlen(backup_dir);
	name_length = strlen(save_name);
	if(name_length > (size_t)-1 - 2 ||
		dir_length > (size_t)-1 - name_length - 2)
		return 0;
	needed = dir_length + 1 + name_length + 1;
	if(needed > out_capacity)
		return 0;

	memcpy(out, backup_dir, dir_length);
	out[dir_length] = '/';
	memcpy(out + dir_length + 1, save_name, name_length + 1);
	return 1;
}

/* Numbered copies normally append the generation digit, preserving the
   existing <save-name>0 convention. A maximum-length FatFs component has no
   room for that byte, so encode its generation in the final 'v' of .sav/.esv. */
static inline int LauncherBuildSaveBackupGenerationPath(const char *backup_dir,
		const char *save_name, char generation, char *out, size_t out_capacity)
{
	size_t name_length;
	size_t path_length;

	if(generation < '0' || generation > '4' ||
		!LauncherBuildSaveBackupPath(backup_dir, save_name, out, out_capacity))
		return 0;

	name_length = strlen(save_name);
	path_length = strlen(out);
	if(name_length < LAUNCHER_SAVE_NAME_MAX_BYTES)
	{
		if(path_length >= out_capacity - 1)
		{
			out[0] = '\0';
			return 0;
		}
		out[path_length] = generation;
		out[path_length + 1] = '\0';
		return 1;
	}

	if(name_length != LAUNCHER_SAVE_NAME_MAX_BYTES ||
		path_length == 0 || save_name[name_length - 4] != '.' ||
		save_name[name_length - 1] != 'v' ||
		!((save_name[name_length - 3] == 's' && save_name[name_length - 2] == 'a') ||
		(save_name[name_length - 3] == 'e' && save_name[name_length - 2] == 's')))
	{
		out[0] = '\0';
		return 0;
	}

	out[path_length - 1] = generation;
	return 1;
}

#endif
