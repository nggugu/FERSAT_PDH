#include "filesys_api.h"

//returns file descriptor "filedes" (aka "file_id") for given file name
//after this function is called, read and write operations can be performed using the filedes
//params: 	- file_name - must be an integer between 0 and 1023
//			- oflag - possible values: 	- O_RDONLY - file is open in read only mode
//										- O_WRONLY - file is open in write only mode
//										- O_CREAT - used for file creation (see comment below)
//										- O_JSTRONG - journaling mode STRONG
//										- O_JWEAK - journaling mode WEAK
//					- if file does not exist, use O_WRONLY and bitwise OR it with O_CREAT
//					- during creation, journaling mode is specified (use bitwise OR) by O_JSTRONG or O_JWEAK
//					- if the journaling flag is not inputed, WEAK journaling mode is used
//note: -when open in read only mode, only sequential reads from the beginning of the file are possible (the
//		file pointer cannot be moved at will)
//		-when open in write only mode, only append operations are supported
//ret val: 	-file_id (integer between 0 and 1023) - note: this is NOT the same as file_name
//			-0xFFFF if an error occurs (e.g. incorrect flag use, trying to create a file that already exists etc.)
//
uint16_t open(uint16_t file_name, uint8_t oflag){
	uint16_t file_id;

	if( file_name>1024 ){
		errno = INVALID_FILE_NAME;
		return 0xFFFF;
	}

	if ( oflag & O_RDONLY ){
		//read only
		if ( oflag & O_WRONLY ){
			errno = RDWR_CONFLICT;
			return 0xFFFF;
		} else {
			if ( fs.FNITT[file_name]== FS_NAME_FREE ){
				errno = FILE_DOESNT_EXIST;
				return 0xFFFF;
			} else {
				//file exists
				file_id = fs.FNITT[file_name];
				if( FILE_CHECK_OPEN(fs.FDT[file_id].sflag) ){
					//file already open, close it first
					FILE_CLOSE(fs.FDT[file_id].sflag);
				}
				//set file open in read only mode
				FILE_SET_OPEN(fs.FDT[file_id].sflag);
				FILE_SET_RDONLY(fs.FDT[file_id].sflag);

				fsv.FDT[file_id].subpage_read_offset = 0;
				fsv.FDT[file_id].subpage_pointer = 0;
				fsv.FDT[file_id].pointer = 0;

				return file_id;
			}
		}
	} else if ( oflag & O_WRONLY ){
		//write only
		if ( oflag & O_CREAT ){
			//create file
			if ( fs.FNITT[file_name]!= FS_NAME_FREE ){
				errno = FILE_ALREADY_EXIST;
				return 0xFFFF;
			} else {
				//file does not exist, create it
				if( oflag & O_JSTRONG ){
					file_id = create_file(JOUR_STRONG);
				} else {
					file_id = create_file(JOUR_WEAK);
				}
				if ( file_id == FS_FAIL_2B ){
					//out of memory or journal commit fail
					return 0xFFFF;
				} else {
					//creation ok
					fs.FNITT[file_name] = file_id;
					write_journal(FNITT_UPDATE,file_name,file_id);
					if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return 0xFFFF;
					FILE_SET_OPEN(fs.FDT[file_id].sflag);
					FILE_SET_WRONLY(fs.FDT[file_id].sflag);
					return file_id;
				}
			}
		} else {
			//do not create file
			if ( fs.FNITT[file_name]== FS_NAME_FREE ){
				errno = FILE_DOESNT_EXIST;
				return 0xFFFF;
			} else {
				//file exists
				file_id = fs.FNITT[file_name];
				if( FILE_CHECK_OPEN(fs.FDT[file_id].sflag) ){
					//file already open, close it first
					FILE_CLOSE(fs.FDT[file_id].sflag);
				}
				//set file open in write only mode
				FILE_SET_OPEN(fs.FDT[file_id].sflag);
				FILE_SET_WRONLY(fs.FDT[file_id].sflag);
				return file_id;
			}
		}
	} else {
		errno = UNDEFINED_OPERATION;
		return 0xFFFF;
	}

}

//attempts to write nbyte bytes to a file designated by filedes from buffer buf.
//note: each write will result in (510 - nbyte%510) bytes unusable in memory. It is therefore
//recommended that nbyte be a multiple of 510, especially for small writes.
//params: 	- filedes - file descriptor (file_id)
//			- buf - pointer to data that will be written to file
//			- nbyte - number of bytes to write
//ret val: - number of bytes sucessfully written
//         - 0xFFFFFFFF - failure, zero bytes written
//note: when failure or less than desired number of bytes is returned, error type is visible in errno
//
uint32_t write(uint16_t filedes, void * buf, uint32_t nbyte){
	uint8_t *data = (uint8_t *) buf;

	if( filedes>1024 ){
		errno = INVALID_FILEDES;
		return 0xFFFFFFFF;
	}

	if ( !FILE_CHECK_OPEN(fs.FDT[filedes].sflag) ){
		errno = FILE_NOT_OPEN;
		return 0xFFFFFFFF;
	} else if ( !FILE_CHECK_WRONLY(fs.FDT[filedes].sflag) ){
		errno = ILLEGAL_OPERATION;
		return 0xFFFFFFFF;
	} else {
		//move pointers to end of file
		fsv.FDT[filedes].subpage_pointer = fs.FDT[filedes].nr_subpages;
		fsv.FDT[filedes].pointer = fs.FDT[filedes].nr_bytes;

		if ( expand_file(filedes, nbyte)==FS_FAIL ) return 0xFFFFFFFF;

		if ( write_data_to_file(filedes, nbyte, data)==FS_FAIL ) return 0xFFFFFFFF;
		else return nbyte;

	}
}

//reads nbyte bytes from a file designated by filedes to buffer buf.
//params: 	- filedes - file descriptor (file_id)
//			- buf - pointer to area where read data will be placed
//			- nbyte - number of bytes to read
//ret val: - number of bytes sucessfully read (always equal to nbyte)
//         - 0xFFFFFFFF - failure, zero bytes read
//note: when failure occurs, error type is visible in errno
//
uint32_t read(uint16_t filedes, void * buf, uint32_t nbyte){
	uint8_t * data = (uint8_t *) buf;

	if( filedes>1024 ){
		errno = INVALID_FILEDES;
		return 0xFFFFFFFF;
	}

	if ( !FILE_CHECK_OPEN(fs.FDT[filedes].sflag) ){
		errno = FILE_NOT_OPEN;
		return 0xFFFFFFFF;
	} else if ( !FILE_CHECK_RDONLY(fs.FDT[filedes].sflag) ){
		errno = ILLEGAL_OPERATION;
		return 0xFFFFFFFF;
	} else {
		if ( read_data_from_file( filedes, nbyte, data) == FS_FAIL) return 0xFFFFFFFF;
		else return nbyte;
	}

}

//close file given by file descriptor
//params: 	- filedes - file descriptor (file_id)
//ret val: - FS_OK if successful
//         - 0xFF if failed
//note: when failure occurs, error type is visible in errno
//
uint8_t close(uint16_t filedes){
	if( filedes>1024 ){
		errno = INVALID_FILEDES;
		return 0xFF;
	}

	if ( !FILE_CHECK_OPEN(fs.FDT[filedes].sflag) ){
		errno = FILE_NOT_OPEN;
		return 0xFF;
	}
	FILE_CLOSE(fs.FDT[filedes].sflag);
	return FS_OK;
}

//deletes file given by file_name from the file system
//both logically and physically (memory erase is performed)
//params: 	- file_name - file name
//ret val: - FS_OK if successful
//         - 0xFF if failed
//note: when failure occurs, error type is visible in errno
//
uint8_t remove_(uint16_t file_name){
	if( file_name>1024 ){
		errno = FILE_DOESNT_EXIST;
		return 0xFF;
	}

	if ( fs.FNITT[file_name]==FS_NAME_FREE ){
		errno = FILE_DOESNT_EXIST;
		return 0xFF;
	} else {
		delete_file(fs.FNITT[file_name], JOUR_STRONG, file_name);
		return FS_OK;
	}

}
