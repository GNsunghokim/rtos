#include <stdio.h>
#include <stdlib.h>
#include <driver/fs.h>
#include "hai.h"
#include "tffs.h"
#include "debug.h"
#include "dirent.h"
#include "crtdef.h"
#include "fat.h"
#include "cache.h"

static int fatfs_mount(FileSystemDriver* driver, DiskDriver* disk_driver, uint32_t lba, size_t size) {
	printf("Mounting FAT filesystem...\n");
	driver->driver = disk_driver;
	int ret = TFFS_mount(driver, lba);

	tffs_t* priv = driver->priv;
	priv->read_buffers = list_create(NULL);
	priv->write_buffers = list_create(NULL);
	priv->wait_lists = list_create(NULL);
	priv->fat_sync = list_create(NULL);
	priv->undone = 0;
	priv->event_id = 0;

	return ret;
}

static int fatfs_open(FileSystemDriver* driver, const char* file_name, char* flags, void** priv) {
	tfile_handle_t* hfile;
	int ret = TFFS_fopen((tffs_handle_t*)driver->priv, (char*)file_name, flags, &hfile);
	if(ret != TFFS_OK) {
		*priv = NULL;
	} else {
		*priv = (void*)hfile;
	}

	return ret;
}

static int fatfs_read(FileSystemDriver* driver, void* _file, void* buffer, size_t size) {
	tfile_handle_t* hfile = (tfile_handle_t*)_file;
	int ret = TFFS_fread(hfile, size, (ubyte*)buffer);
	return ret;
}

static int fatfs_write(FileSystemDriver* driver, void* _file, void* buffer, size_t size) {
	tfile_handle_t* hfile = (tfile_handle_t*)_file;
	int ret = TFFS_fwrite(hfile, size, (ubyte*)buffer);
	return ret;
}

static int fatfs_read_async(FileSystemDriver* driver, void* _file, size_t size, bool(*callback)(List* blocks, int success, void* context), void* context) {
	tfile_handle_t* hfile = (tfile_handle_t*)_file;
	int ret = TFFS_fread_async(hfile, size, callback, context);
	return ret;
}

static int fatfs_write_async(FileSystemDriver* driver, void* _file, void* buffer, size_t size, void(*callback)(void* buffer, int success, void* context), void* context, void(*sync_callback)(int errno, void* context), void* context2) {
	tfile_handle_t* hfile = (tfile_handle_t*)_file;
	int ret = TFFS_fwrite_async(hfile, buffer, size, callback, context, sync_callback, context2);
	return ret;
}

static int fatfs_close(FileSystemDriver* driver, void* _file) {
	tfile_handle_t* hfile = (tfile_handle_t*)_file;
	int ret = TFFS_fclose(hfile);
	return ret;
}

static void* fatfs_opendir(FileSystemDriver* driver, const char* dir_name) {
	tdir_handle_t* hdir;
	int ret = TFFS_opendir((tffs_handle_t*)driver->priv, (char*)dir_name, &hdir);
	if(ret != TFFS_OK) {
		return NULL;
	} else {
		return (void*)hdir;
	}
}

static int fatfs_readdir(FileSystemDriver* driver, void* _dir, Dirent* dirent) {
	dirent_t dir;
	tdir_handle_t* hdir = (tdir_handle_t*)_dir;

	int ret = TFFS_readdir(hdir, &dir);
	if(ret == TFFS_OK)
		strcpy(dirent->name, dir.d_name);
	
	return ret;
}

static int fatfs_closedir(FileSystemDriver* driver, void* _dir) {
	tdir_handle_t* dir = (tdir_handle_t*)_dir;
	int ret = TFFS_closedir(dir);
	return ret;
}

static int fatfs_file_size(void* _file) {
	tfile_t* file = (tfile_t*)_file;
	return file->file_size;
}

static bool fatfs_check_sync(void* _file) {
	tfile_t* file = (tfile_t*)_file;
	if(file->event_id != 0)
		return false;
	
	return true;
}

FileSystemDriver filesystem_driver= {
	.type = FS_TYPE_FAT,
	.mount = fatfs_mount,
	
	.open = fatfs_open,
	.read = fatfs_read,
	.write = fatfs_write,
	.read_async = fatfs_read_async,
	.write_async = fatfs_write_async,
	.close = fatfs_close,

	.opendir = fatfs_opendir,
	.readdir = fatfs_readdir,
	.closedir = fatfs_closedir,

	.file_size = fatfs_file_size,
	.check_sync = fatfs_check_sync,
};

bool fatfs_init() {
	return fs_register(&filesystem_driver);
}
