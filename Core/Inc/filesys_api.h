#ifndef __FILESYS_API_H
#define __FILESYS_API_H

#include "filesys.h"

#define O_RDONLY		0b00000001
#define O_WRONLY		0b00000010
#define O_CREAT			0b00000100
#define O_JSTRONG		0b00001000
#define O_JWEAK			0b00000000

uint16_t open(uint16_t file_name, uint8_t oflag);
uint32_t write(uint16_t filedes, void * buf, uint32_t nbyte);
uint32_t read(uint16_t filedes, void * buf, uint32_t nbyte);
uint8_t close(uint16_t filedes);
uint8_t remove_(uint16_t file_name);

//returns file size in bytes for given file descriptor
//params:  - filedes a.k.a. file_id
//ret val: - file size if successful
//         - 0xFFFFFFFF if filedes invalid
//note: when failure occurs, error type is visible in errno
//
__STATIC_INLINE uint32_t get_file_size(uint16_t filedes){
	if(filedes>1024){
		errno = INVALID_FILEDES;
		return 0xFFFFFFFF;
	} else{
		return fs.FDT[filedes].nr_bytes;
	}
}

//used to initialize filesystem parameters after reset
//ret val: - FS_OK if successful
//         - FS_FAIL if failed
//note: when failure occurs, error type is visible in errno
//
__STATIC_INLINE uint8_t start_fs(void){
	return file_system_init();
}

//must be called before powering off the processor
//ret val: - FS_OK if successful
//         - FS_FAIL if failed
//note: when failure occurs, error type is visible in errno
//
__STATIC_INLINE uint8_t suspend_fs(void){
	return checkpoint_journal();
}

//to be called when a critical error occurs
//ret val: - FS_OK if successful
//         - FS_FAIL if failed
__STATIC_INLINE uint8_t reinit_fs(void){
	return fs_critical_error_recovery();
}

#endif //__FILESYS_API_H
