#include "filesys.h"

struct filesys_header fs;

struct filesys_header_volatile fsv;

FS_errno errno;

struct journal_entry local_journal;

//uint8_t error_induced=0;

//{param0 nr bytes, param1 nr bytes}
const struct journal_operation operation_list[] = {
	{2,2},		//BTT_MODIFY=0
	{1,2},		//QUEUE_MODIFY=1
	{2,2},		//FAT_MODIFY=2
	{2,0},		//BBBT_UPDATE=3
	{1,4},	    //BBBT_SOFT_REWRITE=4
	{2,0},		//HEADER_LBA_MODIFY=5
	{2,2},		//FNITT_UPDATE=6
	{2,4},		//FD_NR_BYTES_MODIFY=7
	{2,4},		//FD_NR_SUBPAGES_MODIFY=8
	{2,4},		//FD_NR_BYTES_INCREMENT=9
	{2,4},		//FD_NR_SUBPAGES_INCREMENT=10
	{2,4},		//FD_TIME_CREATED_INCREMENT=11
	{2,1},		//FD_SFLAG_MODIFY=12
	{2,0}, 		//MARK_FILE_FOR_DELETION=13
	{0,0},		//FILE_DELETION_COMPLETED=14
	{2,0},		//MARK_PBLOCK_USED=15
	{0,0},		//OLD_FSS_BLOCKS_DELETED=16
	{2,0},		//FD_DEFAULT_VALUES=17
	{0,0},		//COPYBACK_BEGIN=18
	{1,2},		//COPYBACK_COMPLETE=19
	{1,2},		//WRITE_TRANSACTION_BEGIN=20
	{0,0},		//WRITE_TRANSACTION_COMPLETE=21
	{2,2},		//BB_COPYBACK_BEGIN=21
	{0,0}		//BB_COPYBACK_COMPLETE=22
};

//must be called after STM reset
uint8_t file_system_init(void){
	fsv.first_inited = FS_NOT_FIRST_INITED;

	if ( check_filesys_first_inited()==FS_NOT_FIRST_INITED ){
		W25N_scan_factory_BB( fs.BBBT );

		init_fs_head();

		mark_filesys_first_inited();

		return save_fs_header_to_flash();			//should not fail

	} else {
		fsv.first_inited = FS_FIRST_INITED;

		if( get_fs_header_from_mem()==FS_FAIL ){
			//errno FS_HEADER_MISSING
			//either system crashed after mark_first_inited() but before save_header() completed
			//or the header was erased but the new one was not saved (crash or BB memory runout)
			fsv.first_inited = FS_NOT_FIRST_INITED;
			wipe_memory(fs.BBBT);
			init_fs_head();

			return save_fs_header_to_flash();
		} else {
			return replay_journal();
		}
	}
}

//to be called when a critical error occurs, e.g. errno==CANNOT_SAVE_FS_HEADER
uint8_t fs_critical_error_recovery(void){
	uint32_t readout, max_version=0;
	uint16_t pba;

	wipe_memory(fs.BBBT);
	init_fs_head();

	//check if there exists an old filesystem header cemented in a bad block
	for( pba=0; pba<NR_BLOCKS; pba++ ){
		while( W25N_page_data_read( W25N_BNPN_TO_PA(pba,LAST_PAGE) ) == W25N_ERR_BUSY );
		while( W25N_read_data( 0, (uint8_t *)(&readout), 4) == W25N_ERR_BUSY );

		if( readout == FS_HEADER_KEY) {
			while( W25N_page_data_read( W25N_BNPN_TO_PA(pba,LAST_PAGE-1) ) == W25N_ERR_BUSY );
			while( W25N_read_data( 0, (uint8_t *)(&readout), 4) == W25N_ERR_BUSY );
			if( (~readout)>=max_version ){
				max_version = ~readout;
			}
		}
	}
	return save_fs_header_to_flash();
}

void wipe_memory(uint32_t * BBBT){
	uint16_t pba;

	for ( pba=0; pba<NR_BLOCKS; pba++){
		while( W25N_block_erase(pba)==W25N_ERR_BUSY );
		while( W25N_check_busy() );
		//detect bad blocks
		if( W25N_check_erase_fail() ){
			FS_SET_BIT( BBBT[pba/32],pba%32);
		} else {
			FS_RESET_BIT( BBBT[pba/32], pba%32);
		}
	}
	return;
}

void init_fs_head(void){
	uint16_t i=0, j=0;

	queue_init( &fs.freePB );

	for ( i=0; i<32; i++){
		for ( j=0; j<32; j++){
			fs.FNITT[i*32+j] = FS_NAME_FREE;
			fs.FAT[i*32+j] = FAT_FREE_LINK;
			fs.BTT[i*32+j] = BTT_FREE_LINK;
			fs.FDT[i*32+j].sflag = 0;

			if ( !FS_TEST_BIT( fs.BBBT[i], j) ){
				enqueue_block( &fs.freePB, (uint16_t)(i*32+j));
			}

			fsv.FDT[i*32+j].pointer = 0;
			fsv.FDT[i*32+j].subpage_pointer = 0;
			fsv.FDT[i*32+j].subpage_read_offset = 0;
		}
	}
	//all valid blocks should be free
	dequeue_block( &fs.freePB, &fs.BTT[FS_HEADER_LBA] );
	fs.FAT[FS_HEADER_LBA] = FAT_FS_HEADER_MARK;

	dequeue_block( &fs.freePB, &fs.BTT[JOURNAL_LBA] );
	fs.FAT[JOURNAL_LBA] = FAT_JOUR_MARK;

	dequeue_block( &fs.freePB, &fs.BTT[SEC_JOURNAL_LBA] );
	fs.FAT[SEC_JOURNAL_LBA] = FAT_JOUR_MARK;

	fs.old_fs_header_pba = 0xFFFF;
	fs.old_jour_pba[PRIMARY] = 0xFFFF;
	fs.old_jour_pba[SECONDARY] = 0xFFFF;

	fs.file_marked_for_deletion = 0xFFFF;

	fs.copyback_lba = 0xFFFF;
	fs.copyback_last_valid_subpage = 0xFFFF;

	fs.ufile.file_id = 0xFFFF;
	fs.ufile.bb_lba = 0xFFFF;
	fs.ufile.bb_pba = 0xFFFF;
	fs.ufile.sc_lba = 0xFFFF;
	fs.ufile.sc_pba = 0xFFFF;

	fsv.journal_page[PRIMARY] = 0;
	fsv.journal_page[SECONDARY] = 0;
	init_local_journal_entry();

	fsv.header_version = 0;
	fsv.blocks_dequeued = 0;

	fs.stability = FS_STABLE;

	return;
}

uint8_t check_filesys_first_inited(void){
	uint32_t key_read;

	W25N_enter_OTP_mode();
	while( !W25N_check_OTP_mode() ) {
		W25N_enter_OTP_mode();
	}

	//OTP page [0], address 02h should hold key that indicates whether the filesystem has been initialized
	while( W25N_page_data_read(0x0002) == W25N_ERR_BUSY );

	while( W25N_read_data( 0, (uint8_t *)(&key_read), 4) == W25N_ERR_BUSY );

	W25N_exit_OTP_mode();
	while( W25N_check_OTP_mode() );

	if ( key_read == INIT_KEY ) return FS_FIRST_INITED;
	else return FS_NOT_FIRST_INITED;
}

//warning: this function will OTP lock filesystem initialized key in OTP pages area, as well as OTP lock
//block protect bits in disabled state(latter not in current version ). make sure that status registers are at their reset value.
void mark_filesys_first_inited(void){
	uint32_t key = INIT_KEY;

	W25N_disable_block_protect();
	while( W25N_block_protect_status() );

	W25N_enter_OTP_mode();
	while( !W25N_check_OTP_mode() );

	//OTP page [0], address 02h should hold key that indicates whether the filesystem has been initialized
	while( W25N_page_data_read(0x0002) == W25N_ERR_BUSY );

	while( W25N_prog_data_load( 0, (uint8_t *)(&key), 4) == W25N_ERR_BUSY );

	while( W25N_prog_execute(0x0002) == W25N_ERR_BUSY );

	while( W25N_check_busy() );

	W25N_set_OTP_lock();

	wait_for_50_ns();		//OTP-L read behaviour not defined, so wait is used (50 ns status register write time)

	W25N_prog_execute(0x0000);	//should not be busy at this point

	while( W25N_check_busy() );

	W25N_exit_OTP_mode();
	while( W25N_check_OTP_mode() );

	return;
}

//wrapper function of save_fs_header_to_flash_low to ensure BB management
uint8_t save_fs_header_to_flash(void){
	//uint32_t should not overflow considering the lifetime number of write/erase cycles for the memory
	if( fsv.first_inited==FS_FIRST_INITED ) fsv.header_version++;

	fs.old_fs_header_pba = 0xFFFF;

	if( save_fs_header_to_flash_low()==FS_FAIL ){
		if( errno==HEAD_BB_DETECTED ){
			return save_fs_header_to_flash();
		} else {
			//errno CANNOT_SAVE_FS_HEADER
			return FS_FAIL;
		}
	} else {
		//while(1);
		return FS_OK;
	}
}

uint8_t save_fs_header_to_flash_low(void){
	uint16_t pba, current_page, prev_header_pba;
	uint32_t nr_bytes, nr_sent, write_val;
	uint8_t * fs_header = (uint8_t *)(&fs);

	nr_bytes = sizeof(fs);
	prev_header_pba = fs.BTT[FS_HEADER_LBA];

	if( fsv.first_inited==FS_FIRST_INITED ){
		if( dequeue_block( &fs.freePB, &fs.BTT[FS_HEADER_LBA] )==ERR_Q_EMPTY ){
			//no free blocks, delete current fs_header in memory to free up space
			//check if block is good (could have been marked bad one one of the prevoius recursive iterations, if write failed)
			if( FS_TEST_BIT( fs.BBBT[fs.BTT[FS_HEADER_LBA]/32], fs.BTT[FS_HEADER_LBA]%32) ){
				errno = CANNOT_SAVE_FS_HEADER;
				return FS_FAIL;
			} else {
				if( delete_pb(prev_header_pba, NO_ENQUEUE)==FS_FAIL ){
					// errno ERASE_BB_DETECTED
					errno = CANNOT_SAVE_FS_HEADER; 	//critical error
					return FS_FAIL;
				}
			}
		} else {
			//a new block will be writen, note on secondary journal
			if( sec_journal_write_and_commit(fs.BTT[FS_HEADER_LBA],fsv.blocks_dequeued)==FS_FAIL ){
				if( errno==SEC_JOURNAL_OVERFLOW  ){
					errno = CANNOT_SAVE_FS_HEADER;
					return FS_FAIL;
				} else {
					//errno WRITE_BB_DETECTED - secondary journal block is bad
					//try to write fs header on the same block as the old one
					FS_SET_BIT( fs.BBBT[fs.old_jour_pba[SECONDARY]/32], fs.old_jour_pba[SECONDARY]%32);
					//check if block is good (could have been marked bad one one of the prevoius recursive iterations, if write failed)
					if( FS_TEST_BIT( fs.BBBT[prev_header_pba/32], prev_header_pba%32) ){
						errno = CANNOT_SAVE_FS_HEADER;
						return FS_FAIL;
					} else {
						if( delete_pb(prev_header_pba, NO_ENQUEUE)==FS_FAIL ){
							// errno ERASE_BB_DETECTED
							errno = CANNOT_SAVE_FS_HEADER; 	//critical error
							return FS_FAIL;
						} else {
							//old fs_head block delete successful, return the allocated unused block back to queue
							enqueue_block( &fs.freePB, fs.BTT[FS_HEADER_LBA] );
							fs.BTT[FS_HEADER_LBA] = prev_header_pba;
						}
					}
				}
			} else {
				//secondary journal commit ok
				fsv.blocks_dequeued = 0;	//to prevent writing this on more than one entry
				fs.old_fs_header_pba = prev_header_pba;
			}
		}
	}

	pba = fs.BTT[FS_HEADER_LBA];
	if( fsv.first_inited==FS_NOT_FIRST_INITED ) fsv.first_inited = FS_FIRST_INITED;

	nr_sent = 0;
	current_page = 0;
	while( nr_sent!=nr_bytes ){
		if ( (nr_bytes-nr_sent) < BYTES_PER_PAGE ){
			while( W25N_prog_data_load(0, (uint8_t *)(fs_header + nr_sent), nr_bytes) == W25N_ERR_BUSY );
			nr_sent = nr_bytes;
		} else {
			while( W25N_prog_data_load(0, (uint8_t *)(fs_header + nr_sent), BYTES_PER_PAGE) == W25N_ERR_BUSY );
			nr_sent += BYTES_PER_PAGE;
		}
		while( W25N_prog_execute(W25N_BNPN_TO_PA(pba,current_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );
		if( W25N_check_prog_fail() ){
			fs.BTT[FS_HEADER_LBA] = prev_header_pba;
			errno=HEAD_BB_DETECTED;
			return FS_FAIL;
		}
		current_page++;
	}
	//write version identifier to second to last page, use negative logic
	write_val = ~fsv.header_version;
	while( W25N_prog_data_load(0, (uint8_t *)(&write_val), 4) == W25N_ERR_BUSY );
	while( W25N_prog_execute(W25N_BNPN_TO_PA(pba,LAST_PAGE-1)) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		FS_SET_BIT( fs.BBBT[fs.BTT[FS_HEADER_LBA]/32], fs.BTT[FS_HEADER_LBA]%32);
		fs.BTT[FS_HEADER_LBA] = prev_header_pba;
		errno=HEAD_BB_DETECTED;
		return FS_FAIL;
	}
	//write special key to the last page, indicating later on that the write operation was successful
	write_val = FS_HEADER_KEY;
	while( W25N_prog_data_load(0, (uint8_t *)(&write_val), 4) == W25N_ERR_BUSY );
	while( W25N_prog_execute(W25N_BNPN_TO_PA(pba,LAST_PAGE)) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		FS_SET_BIT( fs.BBBT[fs.BTT[FS_HEADER_LBA]/32], fs.BTT[FS_HEADER_LBA]%32);
		fs.BTT[FS_HEADER_LBA] = prev_header_pba;
		errno=HEAD_BB_DETECTED;
		return FS_FAIL;
	}
	return FS_OK;
}

//param: enq - enqueue block to freePB queue or not
uint8_t delete_pb(uint16_t pba, uint8_t enq){
	uint8_t bb_detected=0;

	while( W25N_block_erase(pba)==W25N_ERR_BUSY );
	while( W25N_check_busy() );

	//detect bad blocks
	if( W25N_check_erase_fail() ){
		FS_SET_BIT( fs.BBBT[pba/32],pba%32);
		bb_detected = 1;
	} else {
		if(enq==ENQUEUE) enqueue_block( &fs.freePB, pba );
	}

	if( !bb_detected ){
		return FS_OK;
	} else {
		errno = ERASE_BB_DETECTED;
		return FS_FAIL;
	}
}

uint8_t get_fs_header_from_mem(void){
	uint32_t readout, max_version=0;
	uint16_t pba, target_pba=0xFFFF ;
	uint8_t * fs_header = (uint8_t *)(&fs);

	for( pba=0; pba<NR_BLOCKS; pba++ ){
		while( W25N_page_data_read( W25N_BNPN_TO_PA(pba,LAST_PAGE) ) == W25N_ERR_BUSY );
		while( W25N_read_data( 0, (uint8_t *)(&readout), 4) == W25N_ERR_BUSY );

		if( readout == FS_HEADER_KEY) {
			while( W25N_page_data_read( W25N_BNPN_TO_PA(pba,LAST_PAGE-1) ) == W25N_ERR_BUSY );
			while( W25N_read_data( 0, (uint8_t *)(&readout), 4) == W25N_ERR_BUSY );
			if( (~readout)>=max_version ){
				max_version = ~readout;
				target_pba = pba;
			}
		}
	}

	if( target_pba!=0xFFFF ){
		//header found
		//use continuous read for improved speed
		W25N_exit_BUF_mode();
		while( W25N_check_BUF_mode() );

		while( W25N_page_data_read( W25N_BNPN_TO_PA(target_pba,0) ) == W25N_ERR_BUSY );
		while( W25N_read_data( 0, fs_header, sizeof(fs) ) == W25N_ERR_BUSY );

		W25N_enter_BUF_mode();
		while( !W25N_check_BUF_mode() );

		fsv.header_version=max_version;
		return FS_OK;
	} else {
		errno = FS_HEADER_MISSING;
		return FS_FAIL;
	}
}

//returns file_id
uint16_t create_file(journaling_mode jmode){
	uint16_t i=0;

	if ( get_queue_size(&fs.freePB)==0 ){
		errno = OUT_OF_MEMORY;
		return FS_FAIL_2B;
	}

	for( i=0; i<NR_BLOCKS; i++){
		if( fs.BTT[i] == BTT_FREE_LINK ){
			break;
		}
	}
	//nr_bytes=29 - include FNITT update in filesys_api.c
	if( !CHECK_ENTRY_SPACE(29,local_journal.entry_index) ){
		if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL_2B;
	}

	dequeue_block( &fs.freePB, &fs.BTT[i] );
	fs.FAT[i] = FAT_EOF_MARK;

	fsv.FDT[i].pointer = 0;
	fsv.FDT[i].subpage_pointer = 0;
	fsv.FDT[i].subpage_read_offset = 0;
	fs.FDT[i].nr_bytes = 0;
	fs.FDT[i].nr_subpages = 0;
	FILE_SET_JMODE( fs.FDT[i].sflag, jmode);

	write_journal(QUEUE_MODIFY, DEQUEUE_BLOCK, NULL);
	write_journal(BTT_MODIFY, i, fs.BTT[i]);
	write_journal(FAT_MODIFY, i, FAT_EOF_MARK);
	write_journal(FD_DEFAULT_VALUES, i, NULL);
	write_journal(FD_SFLAG_MODIFY, i, fs.FDT[i].sflag);

	return i;
}

uint8_t expand_file(uint16_t file_id, uint32_t bytes_more ){
	uint32_t subpages_need, last_block_subpages_free, nr_bytes;
	uint16_t last_lba, nr_blocks, i, j=0;
	uint16_t previous = 0xFFFF;
	uint8_t jour_space;
	journaling_mode jmode = FILE_GET_JMODE(fs.FDT[file_id].sflag);

	subpages_need = GET_NR_SUBPAGES(bytes_more);
	last_block_subpages_free = (SUBPAGES_PER_BLOCK - (fs.FDT[file_id].nr_subpages % SUBPAGES_PER_BLOCK));
	if( last_block_subpages_free==SUBPAGES_PER_BLOCK && fs.FDT[file_id].nr_subpages!=0 ) last_block_subpages_free=0;

	if( subpages_need <= last_block_subpages_free ){
		//still fits on same block
		//mark new nr_subpages/bytes, FDT to be updated after data is written
		fsv.new_nr_subpages = fs.FDT[file_id].nr_subpages+subpages_need;
		fsv.new_nr_bytes = fs.FDT[file_id].nr_bytes+bytes_more;

		if( jmode==JOUR_STRONG ){
			if( write_journal(WRITE_TRANSACTION_BEGIN, JOUR_STRONG, file_id)==FS_FAIL) return FS_FAIL;
			fs.ufile.file_id = file_id;
			fs.stability = FS_UNSTABLE_A;
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
		}
		fsv.weak_transaction_nb = 0;
		return FS_OK;

	} else {
		//need to allocate additional blocks
		nr_bytes = bytes_more - last_block_subpages_free * BYTES_PER_SUBPAGE_EFF;
		//check if there is enough memory
		nr_blocks = GET_NR_BLOCKS(nr_bytes);
		if ( nr_blocks > get_queue_size(&fs.freePB) ){
			errno = OUT_OF_MEMORY;
			return FS_FAIL;
		}
		last_lba = get_current_lba( file_id, fs.FDT[file_id].nr_subpages-1 );

		if( write_journal(WRITE_TRANSACTION_BEGIN, jmode, file_id)==FS_FAIL ) return FS_FAIL;
		fs.ufile.file_id = file_id;
		fs.stability = FS_UNSTABLE_A;

		//allocate
		for( i=0; i<NR_BLOCKS && j<nr_blocks; i++){
			if( fs.BTT[i] == BTT_FREE_LINK ){
				//check how much space is needed on the journal
				if( (j+1)==nr_blocks ) jour_space = 22;
				else jour_space = 17;

				if( !CHECK_ENTRY_SPACE(jour_space,local_journal.entry_index) ){
					if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
				}

				dequeue_block( &fs.freePB, &fs.BTT[i] );
				//update FAT table
				if ( previous != 0xFFFF){
					fs.FAT[previous] = i;
					write_journal(FAT_MODIFY,previous,i);
				} else {
					fs.FAT[last_lba] = i;
					write_journal(FAT_MODIFY,last_lba,i);
				}
				previous = i;
				if( (j+1)==nr_blocks ){
					fs.FAT[i] = FAT_EOF_MARK;
					write_journal(FAT_MODIFY,i,FAT_EOF_MARK);
				}
				j++;

				write_journal(BTT_MODIFY,i,fs.BTT[i]);
				write_journal(QUEUE_MODIFY, DEQUEUE_BLOCK, NULL);
				write_journal(MARK_PBLOCK_USED, fs.BTT[i], NULL);
				if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			}
		}

		fsv.new_nr_subpages = fs.FDT[file_id].nr_subpages+subpages_need;
		fsv.new_nr_bytes = fs.FDT[file_id].nr_bytes+bytes_more;

		if(jmode==JOUR_WEAK) fsv.weak_transaction_nb=1;
		else fsv.weak_transaction_nb=0;

		return FS_OK;
	}
}

//same as write_data_to_file_low(), but implements BB error recovery
uint8_t write_data_to_file(uint16_t file_id, uint32_t nr_bytes, uint8_t * data){
	uint32_t delta_pointer = fsv.FDT[file_id].pointer;
	if( write_data_to_file_low(file_id, nr_bytes, data)==FS_FAIL ){
		//errno==WRITE_BB_DETECTED
		if( BB_error_recovery( file_id )==FS_OK ){
			delta_pointer = fsv.FDT[file_id].pointer - delta_pointer;
			return write_data_to_file(file_id, nr_bytes - delta_pointer, (uint8_t *)(data + delta_pointer));
		} else {
			//errno BBM_OUT_OF_MEMORY or joural commit fail
			return FS_FAIL;
		}
	} else {
		//all ok
		fs.FDT[file_id].nr_subpages = fsv.new_nr_subpages;
		fs.FDT[file_id].nr_bytes = fsv.new_nr_bytes;
		fs.stability = FS_STABLE;
		if( FILE_GET_JMODE(fs.FDT[file_id].sflag)==JOUR_STRONG){
			//JOUR_STRONG
			if( !CHECK_ENTRY_SPACE(11,local_journal.entry_index) ){
				if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			}
			write_journal(WRITE_TRANSACTION_COMPLETE, NULL, NULL);
			write_journal(FD_NR_SUBPAGES_MODIFY, file_id, fs.FDT[file_id].nr_subpages);
			write_journal(FD_NR_BYTES_MODIFY, file_id, fs.FDT[file_id].nr_bytes);

			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			return FS_OK;
		} else {
			//JOUR_WEAK
			if( fsv.weak_transaction_nb==1 ){
				//weak transaction with new block(s) allocated
				if( !CHECK_ENTRY_SPACE(11,local_journal.entry_index) ){
					if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
				}
				write_journal(WRITE_TRANSACTION_COMPLETE, NULL, NULL);
				write_journal(FD_NR_SUBPAGES_MODIFY, file_id, fs.FDT[file_id].nr_subpages);
				write_journal(FD_NR_BYTES_MODIFY, file_id, fs.FDT[file_id].nr_bytes);

				if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;

				fsv.weak_transaction_nb=0;
				return FS_OK;
			}
			return FS_OK;
		}
	}
}

//be careful not to write more data than that has been allocated
//this function should be used together with create/expand file, using the same nr_bytes
//if writing less then allocated, write only a multiple of 510 bytes
uint8_t write_data_to_file_low(uint16_t file_id, uint32_t nr_bytes, uint8_t * data){
	uint32_t pointer, subpage_pointer;
	uint16_t current_block_pba, current_block_lba, subpage_bytes_to_write, page_bytes_written;
	uint8_t current_subpage, current_page, subpage_pointer_inc;

	pointer = fsv.FDT[file_id].pointer;
	subpage_pointer = fsv.FDT[file_id].subpage_pointer;
	current_block_lba = get_current_lba(file_id, subpage_pointer);
	current_block_pba = fs.BTT[current_block_lba];

	current_page = GET_PAGE(subpage_pointer);
	current_subpage = GET_SUBPAGE(subpage_pointer);

	while( 1 ){
		page_bytes_written = 0;
		subpage_pointer_inc = 0;

		//preload page in data buffer, if subpage not 0
		if( current_subpage!=0 ){
			while( W25N_page_data_read(W25N_BNPN_TO_PA(current_block_pba,current_page)) == W25N_ERR_BUSY );
		}

		while( current_subpage<=LAST_SUBPAGE ){
			if( nr_bytes>BYTES_PER_SUBPAGE_EFF ) subpage_bytes_to_write=BYTES_PER_SUBPAGE_EFF;
			else subpage_bytes_to_write=nr_bytes;

			//write subpage size indicator bytes
			if( current_subpage==0 ){
				while( W25N_prog_data_load(current_subpage*BYTES_PER_SUBPAGE, (uint8_t *)(&subpage_bytes_to_write), 2) == W25N_ERR_BUSY );
			} else {
				while( W25N_rand_prog_data_load(current_subpage*BYTES_PER_SUBPAGE, (uint8_t *)(&subpage_bytes_to_write), 2) == W25N_ERR_BUSY );
			}
			//write actual data
			while( W25N_rand_prog_data_load(current_subpage*BYTES_PER_SUBPAGE+2, data, subpage_bytes_to_write) == W25N_ERR_BUSY );

			current_subpage++;
			data = data + subpage_bytes_to_write;
			page_bytes_written = page_bytes_written + subpage_bytes_to_write;
			nr_bytes = nr_bytes - subpage_bytes_to_write;
			subpage_pointer_inc++;

			if( nr_bytes==0 ) break;
		}

		while( W25N_prog_execute(W25N_BNPN_TO_PA(current_block_pba,current_page)) == W25N_ERR_BUSY );

		//wait for program operation to complete
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			fsv.FDT[file_id].pointer = pointer;
			fsv.FDT[file_id].subpage_pointer = subpage_pointer;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		} else {
			pointer = pointer + page_bytes_written;
			subpage_pointer = subpage_pointer + subpage_pointer_inc;
		}

		if( nr_bytes==0 ){
			fsv.FDT[file_id].pointer = pointer;
			fsv.FDT[file_id].subpage_pointer = subpage_pointer;
			return FS_OK;

		} else {
			current_subpage = 0;
			current_page++;

			if( current_page > LAST_PAGE ){
				current_block_lba = fs.FAT[current_block_lba];
				current_block_pba = fs.BTT[current_block_lba];

				current_page = 0;
			}
		}
	}
}

uint8_t read_data_from_file(uint16_t file_id, uint32_t nr_bytes, uint8_t * data){
	uint32_t pointer, subpage_pointer;
	uint16_t current_block_pba, current_block_lba, subpage_read_offset;
	volatile uint16_t bytes_per_subpage;
	uint8_t current_page, current_subpage;

	pointer = fsv.FDT[file_id].pointer;
	subpage_pointer = fsv.FDT[file_id].subpage_pointer;

	if ( pointer+nr_bytes > fs.FDT[file_id].nr_bytes ) {
		errno = OUT_OF_BOUND_READ;
		return FS_FAIL;
	}

	current_block_lba = get_current_lba(file_id, subpage_pointer);
	current_block_pba = fs.BTT[current_block_lba];

	current_page = GET_PAGE(subpage_pointer);
	current_subpage = GET_SUBPAGE(subpage_pointer);

	subpage_read_offset = fsv.FDT[file_id].subpage_read_offset;

	while( 1 ){
		//preload page in data buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(current_block_pba,current_page)) == W25N_ERR_BUSY );

		while (current_subpage<=LAST_SUBPAGE) {
			//read subpage size indicator bytes
			while( W25N_read_data(current_subpage*BYTES_PER_SUBPAGE, (uint8_t *)(&bytes_per_subpage), 2) == W25N_ERR_BUSY );
			//read data
			if( (bytes_per_subpage)-subpage_read_offset>510 ){
				//sanity check fail
				errno = SUBPAGE_SIZE_ERR;
				return FS_FAIL;
			} else if( nr_bytes < bytes_per_subpage-subpage_read_offset ){
				//partial subpage read
				fsv.FDT[file_id].subpage_read_offset = subpage_read_offset + nr_bytes;

				while( W25N_read_data(current_subpage*BYTES_PER_SUBPAGE+2+subpage_read_offset, data, nr_bytes) == W25N_ERR_BUSY );
				data = data + nr_bytes;
				pointer = pointer + nr_bytes;
				nr_bytes = 0;

			} else {
				//subpage read to the last byte (included)
				while( W25N_read_data(current_subpage*BYTES_PER_SUBPAGE+2+subpage_read_offset, data, bytes_per_subpage-subpage_read_offset) == W25N_ERR_BUSY );
				current_subpage++;
				data = data + (bytes_per_subpage-subpage_read_offset);
				pointer = pointer + (bytes_per_subpage-subpage_read_offset);
				subpage_pointer++;
				nr_bytes = nr_bytes - (bytes_per_subpage-subpage_read_offset);

				fsv.FDT[file_id].subpage_read_offset = subpage_read_offset = 0;
			}

			if (nr_bytes==0) break;
		}


		if (nr_bytes==0){
			fsv.FDT[file_id].pointer = pointer;
			fsv.FDT[file_id].subpage_pointer = subpage_pointer;
			return FS_OK;

		} else {
			current_subpage = 0;
			current_page++;

			if (current_page > LAST_PAGE){
				current_block_lba = fs.FAT[current_block_lba];
				current_block_pba = fs.BTT[current_block_lba];

				current_page = 0;
			}
		}

	}
}

uint8_t BB_error_recovery(uint16_t file_id){
	if( BB_manage( get_current_lba(file_id,fsv.FDT[file_id].subpage_pointer ), GET_SUBPAGE_IBL(fsv.FDT[file_id].subpage_pointer), FILE_GET_JMODE(fs.FDT[file_id].sflag) )==FS_FAIL){
		if( errno==WRITE_BB_DETECTED ){
			return BB_error_recovery(file_id);
		} else {
			//errno BBM_OUT_OF_MEMORY or journal commit fail
			return FS_FAIL;
		}
	} else {
		return FS_OK;
	}
}

//provides new physical block to given logical block with bb issue, then calls BB_copyback
uint8_t BB_manage(uint16_t lba, uint8_t first_invalid_subpage, journaling_mode jmode){
	uint16_t bb_pba = fs.BTT[lba];

	FS_SET_BIT( fs.BBBT[bb_pba/32], bb_pba%32);
	if( write_journal(BBBT_UPDATE, bb_pba, NULL)==FS_FAIL) return FS_FAIL;

	if ( get_queue_size(&fs.freePB)==0 ){
		errno = BBM_OUT_OF_MEMORY;
		return FS_FAIL;

	} else {
		fsv.last_pba_BB_managed = bb_pba;

		if( !CHECK_ENTRY_SPACE(17,local_journal.entry_index) ){
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
		}
		//mark bb_pba for unstable fs_header scenario
		fs.ufile.bb_lba = lba;
		fs.ufile.bb_pba = bb_pba;

		dequeue_block( &fs.freePB, &fs.BTT[lba] );
		write_journal(BB_COPYBACK_BEGIN, lba, bb_pba);
		write_journal(QUEUE_MODIFY, DEQUEUE_BLOCK, NULL);
		write_journal(MARK_PBLOCK_USED, fs.BTT[lba], NULL);
		write_journal(BTT_MODIFY, lba, fs.BTT[lba]);
		if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;

		if( first_invalid_subpage!=0 ){
			return BB_copyback( lba, first_invalid_subpage-1);
		} else {
			//no copying required, just a new block
			fs.ufile.bb_lba = 0xFFFF;
			fs.ufile.bb_pba = 0xFFFF;
			//no need to check free journal space since commit was just made
			write_journal(BB_COPYBACK_COMPLETE, NULL, NULL);
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			else return FS_OK;
		}
	}
}

//use this function to salvage valid programmed pages from a bad block
uint8_t BB_copyback(uint16_t lba, uint8_t last_valid_subpage){
	uint8_t current_page = 0, last_valid_page;

	last_valid_page = GET_PAGE(last_valid_subpage);

	//copyback on all but the last page
	for( current_page=0; current_page<last_valid_page; current_page++ ){
		//copy bad block page to data buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fsv.last_pba_BB_managed,current_page)) == W25N_ERR_BUSY );
		//write data budder to valid block
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],current_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			fs.BTT[lba] = fsv.last_pba_BB_managed;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	}
	//last page copyback
	//check if partially programmed
	if( last_valid_subpage%SUBPAGES_PER_PAGE==3 ){
		//no partial programming
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fsv.last_pba_BB_managed,last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			fs.BTT[lba] = fsv.last_pba_BB_managed;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	} else {
		//last page partially programmed, use uC RAM as buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fsv.last_pba_BB_managed,last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_read_data(0, fsv.copyback_local_page, ((last_valid_subpage+1)%SUBPAGES_PER_PAGE)*BYTES_PER_SUBPAGE ) == W25N_ERR_BUSY );
		while( W25N_prog_data_load(0, fsv.copyback_local_page, ((last_valid_subpage+1)%SUBPAGES_PER_PAGE)*BYTES_PER_SUBPAGE ) == W25N_ERR_BUSY );
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			fs.BTT[lba] = fsv.last_pba_BB_managed;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	}

	fs.ufile.bb_lba = 0xFFFF;
	fs.ufile.bb_pba = 0xFFFF;
	//no need to check free journal space since commit was in BB_manage
	write_journal(BB_COPYBACK_COMPLETE, NULL, NULL);
	if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;

	return FS_OK;
}

uint16_t get_current_lba(uint16_t file_id, uint32_t subpage_pointer){
	uint16_t i=0, lba_next, nr_blocks;
	nr_blocks = GET_BLOCK(subpage_pointer) + 1;

	lba_next = file_id;
	for(i=0; (i+1)<nr_blocks; i++){
		lba_next = fs.FAT[lba_next];
	}

	return lba_next;
}

uint8_t copyback(uint16_t lba, uint16_t last_valid_subpage){
	if( write_journal(COPYBACK_BEGIN, NULL, NULL)==FS_FAIL) return FS_FAIL;
	if( copyback_low(lba,last_valid_subpage)==FS_FAIL){
		if(errno==WRITE_BB_DETECTED){
			return copyback(lba, last_valid_subpage);
		} else {
			//COPYBACK_OUT_OF_MEMORY
			return FS_FAIL;
		}
	} else {
		return FS_OK;
	}
}

uint8_t copyback_low(uint16_t lba, uint16_t last_valid_subpage){
	uint8_t current_page, last_valid_page;
	uint16_t old_pba = fs.BTT[lba];

	if ( get_queue_size(&fs.freePB)==0 ){
		errno = COPYBACK_OUT_OF_MEMORY;
		return FS_FAIL;
	} else {
		if( last_valid_subpage==0xFFFF){
			//no copying required, only new block allocation
			if( !CHECK_ENTRY_SPACE(7,local_journal.entry_index) ){
				if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			}
			dequeue_block( &fs.freePB, &fs.BTT[lba] );
			write_journal(QUEUE_MODIFY, DEQUEUE_BLOCK, NULL);
			write_journal(BTT_MODIFY, lba, fs.BTT[lba]);
			write_journal(COPYBACK_COMPLETE, 2, NULL);	//stage 2 complete
			fs.copyback_lba = 0xFFFF;
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			else return FS_OK;
		} else {
			//copying required
			last_valid_page = GET_PAGE(last_valid_subpage);
			//journal ops begin
			if( !CHECK_ENTRY_SPACE(7,local_journal.entry_index) ){
				if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			}
			fs.ufile.sc_lba = lba;
			fs.ufile.sc_pba = old_pba;
			fs.stability = FS_UNSTABLE_C;
			dequeue_block( &fs.freePB, &fs.BTT[lba] );
			write_journal(QUEUE_MODIFY, DEQUEUE_BLOCK, NULL);
			write_journal(MARK_PBLOCK_USED, fs.BTT[lba], NULL);
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
			//journal ops end
		}
	}

	//copyback on all but the last page
	for( current_page=0; current_page<last_valid_page; current_page++ ){
		//copy bad block page to data buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(old_pba,current_page)) == W25N_ERR_BUSY );
		//write data buffer to valid block
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],current_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			FS_SET_BIT( fs.BBBT[fs.BTT[lba]/32], fs.BTT[lba]%32);
			if( write_journal(BBBT_UPDATE, fs.BTT[lba], NULL)==FS_FAIL) return FS_FAIL;
			fs.BTT[lba] = old_pba;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	}

	//last page copyback
	//check if partially programmed
	if( last_valid_subpage%SUBPAGES_PER_PAGE==3 ){
		//no partial programming
		while( W25N_page_data_read(W25N_BNPN_TO_PA(old_pba,last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			FS_SET_BIT( fs.BBBT[fs.BTT[lba]/32], fs.BTT[lba]%32);
			if( write_journal(BBBT_UPDATE, fs.BTT[lba], NULL)==FS_FAIL) return FS_FAIL;//
			fs.BTT[lba] = old_pba;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	} else {
		//last page partially programmed, use uC RAM as buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(old_pba,last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_read_data(0, fsv.copyback_local_page, ((last_valid_subpage+1)%SUBPAGES_PER_PAGE)*BYTES_PER_SUBPAGE ) == W25N_ERR_BUSY );
		while( W25N_prog_data_load( 0, fsv.copyback_local_page, ((last_valid_subpage+1)%SUBPAGES_PER_PAGE)*BYTES_PER_SUBPAGE ) == W25N_ERR_BUSY );
		while( W25N_prog_execute(W25N_BNPN_TO_PA(fs.BTT[lba],last_valid_page)) == W25N_ERR_BUSY );
		while( W25N_check_busy() );

		if( W25N_check_prog_fail() ){
			FS_SET_BIT( fs.BBBT[fs.BTT[lba]/32], fs.BTT[lba]%32);
			if( write_journal(BBBT_UPDATE, fs.BTT[lba], NULL)==FS_FAIL) return FS_FAIL;//
			fs.BTT[lba] = old_pba;
			errno = WRITE_BB_DETECTED;
			return FS_FAIL;
		}
	}

	//journal ops begin
	fs.stability = FS_UNSTABLE_D;
	fs.ufile.sc_lba = 0xFFFF;
	fs.copyback_lba = 0xFFFF;
	//non-volatile pba to delete store here, relevant only in UNSTABLE_D state
	fs.ufile.sc_pba = old_pba;

	if( !CHECK_ENTRY_SPACE(7,local_journal.entry_index) ){
		if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
	}
	write_journal(BTT_MODIFY, lba, fs.BTT[lba]);
	write_journal(COPYBACK_COMPLETE, 1, old_pba);	//stage 1 complete
	if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
	//journal ops end
	//erase block on memory module
	while( W25N_block_erase(old_pba)==W25N_ERR_BUSY );
	while( W25N_check_busy() );

	//detect bad blocks
	//no need to check journal space since there was a commit just now
	if( W25N_check_erase_fail() ){
		FS_SET_BIT( fs.BBBT[old_pba/32], old_pba%32);
		write_journal(BBBT_UPDATE, old_pba, NULL);
	} else {
		enqueue_block( &fs.freePB, old_pba);
		write_journal(QUEUE_MODIFY, ENQUEUE_BLOCK, old_pba);
	}
	write_journal(COPYBACK_COMPLETE, 2, NULL);	//stage 2 complete
	fs.ufile.sc_pba = 0xFFFF;
	fs.stability = FS_STABLE;
	if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
	return FS_OK;
}

uint8_t delete_file(uint16_t file_id, journaling_mode jmode, uint16_t file_name){
	uint16_t current_block_lba = file_id, prev_block_lba, current_block_pba;
	uint16_t i;

	if( jmode!=JOUR_DISABLE ){
		if( !CHECK_ENTRY_SPACE(8,local_journal.entry_index) ){
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
		}
		fs.file_marked_for_deletion = file_id;
		fs.FNITT[file_name] = FS_NAME_FREE;
		write_journal(MARK_FILE_FOR_DELETION, file_id, NULL);
		write_journal(FNITT_UPDATE, file_name, FS_NAME_FREE);
		if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
	}

	while( current_block_lba!=FAT_EOF_MARK && current_block_lba!=FAT_FREE_LINK ){
		//erase block on memory module
		while( W25N_block_erase(fs.BTT[current_block_lba])==W25N_ERR_BUSY );

		while( W25N_check_busy() );

		//detect bad blocks
		if( W25N_check_erase_fail() ){
			current_block_pba = fs.BTT[current_block_lba];
			FS_SET_BIT( fs.BBBT[current_block_pba/32], current_block_pba%32);
		} else {
			enqueue_block( &fs.freePB, fs.BTT[current_block_lba]);
			fs.BTT[current_block_lba] = BTT_FREE_LINK;
		}

		//clear FAT entry
		prev_block_lba = current_block_lba;
		current_block_lba = fs.FAT[prev_block_lba];
		fs.FAT[prev_block_lba] = FAT_FREE_LINK;
	}

	if( jmode!=JOUR_DISABLE ){
		fs.file_marked_for_deletion = 0xFFFF;
		if( !CHECK_ENTRY_SPACE(6*32+1,local_journal.entry_index) ){
			if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
		}
		write_journal(FILE_DELETION_COMPLETED, NULL, NULL);
		for( i=0; i<32; i++){
			write_journal(BBBT_SOFT_REWRITE, i, fs.BBBT[i]);
		}
		if( write_and_commit_journal_entry_to_flash()==FS_FAIL ) return FS_FAIL;
	}
	return FS_OK;
}

//deletes file logically from the descriptor tables, but does not perform physical block erase
//to be used when replaying a journal entry of file deletion that is known to have been performed physically
void delete_file_logical(uint16_t file_id){
	uint16_t current_block_lba = file_id, prev_block_lba, current_block_pba;

	while( current_block_lba!=FAT_EOF_MARK && current_block_lba!=FAT_FREE_LINK ){
		current_block_pba = fs.BTT[current_block_lba];
		if(	!FS_TEST_BIT( fs.BBBT[current_block_pba/32], current_block_pba%32) ){
			enqueue_block( &fs.freePB, current_block_pba);
			fs.BTT[current_block_lba] = BTT_FREE_LINK;
		}
		//clear FAT entry
		prev_block_lba = current_block_lba;
		current_block_lba = fs.FAT[prev_block_lba];
		fs.FAT[prev_block_lba] = FAT_FREE_LINK;
	}
}

void init_local_journal_entry(void){
	local_journal.entry[0] = 0xE2;	//Transaction begin marker
	local_journal.entry[1] = 0x00;

	local_journal.entry_index = 2;

	return;
}

//this function works only with little endian format
uint8_t write_journal(journal_ops operation, uint16_t param0, uint32_t param1){
	uint8_t i;

	local_journal.entry[local_journal.entry_index++] = (uint8_t)operation;

	for( i=0; i<(operation_list[operation].param0_bytes); i++ ){
		local_journal.entry[local_journal.entry_index++] = *((uint8_t *)(&param0)+i);
	}
	for( i=0; i<(operation_list[operation].param1_bytes); i++ ){
		local_journal.entry[local_journal.entry_index++] = *((uint8_t *)(&param1)+i);
	}

	if( local_journal.entry_index+JOUR_OP_MAX_BYTES>JOURNAL_ENTRY_LEN ){
		return write_and_commit_journal_entry_to_flash();
	} else {
		return FS_OK;
	}
}

uint8_t write_and_commit_journal_entry_to_flash(void){
	uint16_t TxE = 0x00E3;

	//check if the entry is empty, if so, return
	if( local_journal.entry_index==2 ) return FS_OK;

	//load TxB and metadata - first two supbages (can be expanded if necessary)
	while( W25N_prog_data_load( 0, local_journal.entry, local_journal.entry_index ) == W25N_ERR_BUSY );
	//load metadata size indicatior on last subpage
	while( W25N_rand_prog_data_load( BYTES_PER_SUBPAGE*3, (uint8_t *)(&local_journal.entry_index), 2 ) == W25N_ERR_BUSY );
	//write to flash
	while( W25N_prog_execute( W25N_BNPN_TO_PA(fs.BTT[JOURNAL_LBA], fsv.journal_page[PRIMARY]) ) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		//error WRITE_BB_DETECTED
		return checkpoint_journal();
	}
	fsv.journal_page[PRIMARY]++;

	//commit to journal - indicated by writing 0xE300 to next page
	while( W25N_rand_prog_data_load( 0, (uint8_t *)(&TxE), 2 ) == W25N_ERR_BUSY );
	while( W25N_prog_execute( W25N_BNPN_TO_PA(fs.BTT[JOURNAL_LBA], fsv.journal_page[PRIMARY]) ) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		//error WRITE_BB_DETECTED;
		return checkpoint_journal();
	}
	fsv.journal_page[PRIMARY]++;

	//reset local journal entry
	init_local_journal_entry();

	if( fsv.journal_page[PRIMARY]==PAGES_PER_BLOCK ){
		//reached end of block; find new block, checkpoint journal and erase current block
		return checkpoint_journal();
	} else{
		//still room left in the journal block
		return FS_OK;
	}
}

//target_pba - the block(s) that is(are - if BB occurs) intended to be written with fs_header data
uint8_t sec_journal_write_and_commit(uint16_t target_pba, uint16_t blocks_dequeued){
	uint16_t TxBM[3], TxE = 0x00E5;
	TxBM[0] = 0x00E4;
	TxBM[1] = target_pba;
	TxBM[2] = blocks_dequeued;

	if( fsv.journal_page[SECONDARY]==PAGES_PER_BLOCK ){
		//probability of this happening is extremely low
		errno = SEC_JOURNAL_OVERFLOW;
		return FS_FAIL;
	}

	//load TxB and metadata to first page
	while( W25N_prog_data_load( 0, (uint8_t *)TxBM, 6 ) == W25N_ERR_BUSY );
	//write to flash
	while( W25N_prog_execute( W25N_BNPN_TO_PA(fs.old_jour_pba[SECONDARY], fsv.journal_page[SECONDARY]) ) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		errno = WRITE_BB_DETECTED;
		return FS_FAIL;
	}
	fsv.journal_page[SECONDARY]++;
	//commit to journal - indicated by writing 0xE500 to next page
	while( W25N_rand_prog_data_load( 0, (uint8_t *)(&TxE), 2 ) == W25N_ERR_BUSY );
	while( W25N_prog_execute( W25N_BNPN_TO_PA(fs.old_jour_pba[SECONDARY], fsv.journal_page[SECONDARY]) ) == W25N_ERR_BUSY );
	while( W25N_check_busy() );
	if( W25N_check_prog_fail() ){
		errno = WRITE_BB_DETECTED;
		return FS_FAIL;
	}
	fsv.journal_page[SECONDARY]++;

	return FS_OK;
}

uint8_t checkpoint_journal(void){
	uint16_t pba1=0xFFFF, pba2=0xFFFF;
	uint8_t nr_free_pblocks, i;

	if( dequeue_block( &fs.freePB, &pba1 )==ERR_Q_EMPTY ){
		nr_free_pblocks = 0;
	} else {
		if( dequeue_block( &fs.freePB, &pba2 )==ERR_Q_EMPTY ){
			nr_free_pblocks = 1;
		} else {
			nr_free_pblocks = 2;
		}
	}

	fs.old_jour_pba[PRIMARY] = fs.BTT[JOURNAL_LBA];
	fs.old_jour_pba[SECONDARY] = fs.BTT[SEC_JOURNAL_LBA];
	//if there are no free blocks swap physical blocks between primary and secondary journal
	switch(nr_free_pblocks){
	case 0:
	{
		fs.BTT[JOURNAL_LBA] = fs.old_jour_pba[SECONDARY];
		fs.BTT[SEC_JOURNAL_LBA] = fs.old_jour_pba[PRIMARY];
	}
	case 1:
	{
		fs.BTT[JOURNAL_LBA] = pba1;
		fs.BTT[SEC_JOURNAL_LBA] = fs.old_jour_pba[PRIMARY];
	}
	case 2:
	{
		fs.BTT[JOURNAL_LBA] = pba1;
		fs.BTT[SEC_JOURNAL_LBA] = pba2;
	}
	}

	fsv.blocks_dequeued = nr_free_pblocks;
	if( save_fs_header_to_flash()==FS_FAIL ) return FS_FAIL;

	fsv.journal_page[PRIMARY] = 0;
	fsv.journal_page[SECONDARY] = 0;
	init_local_journal_entry();

	//no need to check journal free space as at this point it is empty
	for( i=0; i<2; i++){
		while( W25N_block_erase(fs.old_jour_pba[i])==W25N_ERR_BUSY );
		while( W25N_check_busy() );
		//detect bad blocks on erase / scan BBBT table for previous write fail
		if( W25N_check_erase_fail() || FS_TEST_BIT( fs.BBBT[fs.old_jour_pba[i]/32], fs.old_jour_pba[i]%32) ){
			FS_SET_BIT( fs.BBBT[fs.old_jour_pba[i]/32], fs.old_jour_pba[i]%32);
			if( nr_free_pblocks==0 || nr_free_pblocks==1 ){
				errno = OUT_OF_MEMORY; //critical error, nowhere to store journal
				return FS_FAIL;
			}
			write_journal(BBBT_UPDATE, fs.old_jour_pba[i], NULL);
		} else {
			enqueue_block( &fs.freePB, fs.old_jour_pba[i] );
			write_journal(QUEUE_MODIFY, ENQUEUE_BLOCK, fs.old_jour_pba[i]);
		}
	}

	//delete old fs_header, if a new pblock has been allocated for fs_header
	if( fs.old_fs_header_pba!=0xFFFF ){
		while( W25N_block_erase(fs.old_fs_header_pba)==W25N_ERR_BUSY );
		while( W25N_check_busy() );
		//detect bad blocks on erase
		if( W25N_check_erase_fail() ){
			FS_SET_BIT( fs.BBBT[fs.old_fs_header_pba/32], fs.old_fs_header_pba%32);
			write_journal(BBBT_UPDATE, fs.old_fs_header_pba, NULL);
		} else {
			enqueue_block( &fs.freePB, fs.old_fs_header_pba );
			write_journal(QUEUE_MODIFY, ENQUEUE_BLOCK, fs.old_fs_header_pba );
		}
	}

	write_journal(OLD_FSS_BLOCKS_DELETED, NULL, NULL);
	return write_and_commit_journal_entry_to_flash();
	//recursive loop might occur, but should have minimal iterations
}

uint8_t replay_journal(void){
	uint16_t current_page;
	uint16_t read_val, i=0, j=0;
	uint32_t subpage_pointer;
	uint8_t operation;

	fsv.old_fss_blocks_deleted = 0;
	fsv.strong_transaction_ongoing = 0;
	fsv.weak_transaction_ongoing = 0;
	fsv.marked_pblocks_len = 0;
	fsv.file_deletion_stage = 0;
	fsv.bb_copyback_ongoing = 0;
	fsv.copyback_stage = 0;
	fsv.ufile.file_id = 0xFFFF;

	//scan the primary journal block page by page
	for( current_page=0; current_page<PAGES_PER_BLOCK; current_page+=2 ){
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fs.BTT[JOURNAL_LBA],current_page+1)) == W25N_ERR_BUSY );

		//check for TxE marker
		while( W25N_read_data( 0, (uint8_t *)(&read_val), 2) == W25N_ERR_BUSY );
		if( read_val!=0x00E3 ) break; //exit point

		//entry found, use local_journal_entry as read buffer
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fs.BTT[JOURNAL_LBA],current_page)) == W25N_ERR_BUSY );
		while( W25N_read_data( BYTES_PER_SUBPAGE*3, (uint8_t *)(&local_journal.entry_index), 2) == W25N_ERR_BUSY );
		while( W25N_read_data( 0, local_journal.entry, local_journal.entry_index) == W25N_ERR_BUSY );

		//replay journal operation by operation; i=2 to skip 0xE200 begin marker
		i = 2;
		while( i<local_journal.entry_index ){
			operation = local_journal.entry[i];
			journal_to_fs_head( operation, &local_journal.entry[i+1] );

			i = i + 1 + operation_list[operation].param0_bytes + operation_list[operation].param1_bytes;
		}
	}

	//scan through the secondary journal: entries are to be found only if crash occured after new fs_header pba selection and
	//before saving was complete
	//if there is more than one entry in the journal, entires that are not last are bad blocks (write failed)
	//however, this is not checked spereately since (a) it is a rare occurance (b) delete block is then also expected to fail
	for( current_page=0; current_page<PAGES_PER_BLOCK; current_page+=2 ){
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fs.BTT[SEC_JOURNAL_LBA],current_page+1)) == W25N_ERR_BUSY );
		//check for TxE marker
		while( W25N_read_data( 0, (uint8_t *)(&read_val), 2) == W25N_ERR_BUSY );
		if( read_val!=0x00E5 ) break; //exit point

		//entry found
		while( W25N_page_data_read(W25N_BNPN_TO_PA(fs.BTT[SEC_JOURNAL_LBA],current_page)) == W25N_ERR_BUSY );
		//read number if dequeues made in checkpoint_journal() (pba1 and pba2), store number in read_val
		while( W25N_read_data( 4, (uint8_t *)(&read_val), 2) == W25N_ERR_BUSY );

		for(j=0;j<read_val;j++){
			//use i as buffer
			dequeue_block( &fs.freePB, &i );
			enqueue_block( &fs.freePB, i);
		}

		//header pba is next; note: i and read_val should be identical
		dequeue_block( &fs.freePB, &i );
		while( W25N_read_data( 2, (uint8_t *)(&read_val), 2) == W25N_ERR_BUSY );
		//erase block
		while( W25N_block_erase(read_val)==W25N_ERR_BUSY );
		while( W25N_check_busy() );

		//detect bad blocks
		if( W25N_check_erase_fail() ){
			FS_SET_BIT( fs.BBBT[read_val/32], read_val%32);
		} else {
			enqueue_block( &fs.freePB, read_val);
		}
	}

	//clear used (corrupt) pblocks, if any
	if( fsv.copyback_stage==1 || fsv.weak_transaction_ongoing || fsv.strong_transaction_ongoing || fsv.bb_copyback_ongoing ){
		for( i=0; i<fsv.marked_pblocks_len; i++){
			if( !FS_TEST_BIT(fs.BBBT[fsv.marked_pblocks[i]/32], fsv.marked_pblocks[i]%32) ){
				while( W25N_block_erase( fsv.marked_pblocks[i] )==W25N_ERR_BUSY );
				while( W25N_check_busy() );
				//detect bad blocks
				if( W25N_check_erase_fail() ){
					FS_SET_BIT( fs.BBBT[fsv.marked_pblocks[i]/32], fsv.marked_pblocks[i]%32);
				} else {
					enqueue_block( &fs.freePB, fsv.marked_pblocks[i] );
				}
			}
		}
	}

	//delete used block from a stage 2 interrupted copyback
	if( fsv.copyback_stage==2 ){
		if( !FS_TEST_BIT(fs.BBBT[fsv.pba_to_delete], fsv.pba_to_delete%32) ){
			while( W25N_block_erase(fsv.pba_to_delete)==W25N_ERR_BUSY );
			while( W25N_check_busy() );
			if( W25N_check_erase_fail() ){
				FS_SET_BIT( fs.BBBT[fsv.pba_to_delete/32], fsv.pba_to_delete%32);
			} else {
				enqueue_block( &fs.freePB, fsv.pba_to_delete);
			}
		}
	}

	//must go before if( fsv.strong_transaction_ongoing || fsv.weak_transaction_ongoing ){...
	if( fsv.bb_copyback_ongoing ){
		fs.BTT[fsv.ufile.bb_lba] = fsv.ufile.bb_pba;
	}

	//check for any interrupted write transactions
	//FAT and BTT repair; copyback (if applicable)
	if( fsv.strong_transaction_ongoing || fsv.weak_transaction_ongoing ){
		//journal hadn't recorded tranaction complete marker; potentially corrupt block, find new pb and perform copyback
		subpage_pointer = fs.FDT[fsv.ufile.file_id].nr_subpages;
		if(subpage_pointer!=0) subpage_pointer-=1;

		//repair FAT and BTT table
		repair_FAT_and_BTT(get_current_lba(fsv.ufile.file_id, subpage_pointer));

		if( fsv.strong_transaction_ongoing ){
			// mark parameters for copyback
			fs.copyback_lba = get_current_lba(fsv.ufile.file_id, subpage_pointer);
			if( fs.FDT[fsv.ufile.file_id].nr_subpages==0 ){
				fs.copyback_last_valid_subpage = 0xFFFF;
			} else {
				fs.copyback_last_valid_subpage = GET_SUBPAGE_IBL(subpage_pointer);
			}
		}
	}

	//stabilization
	if( stabilize_fs()==FS_FAIL ) return FS_FAIL;

	//check if there is a file whose delete sequence hadn't completed
	if( fs.file_marked_for_deletion!=0xFFFF ){
		if( fsv.file_deletion_stage==0	){
			//FILE_DELETION_COMPLETE not read
			delete_file(fs.file_marked_for_deletion, JOUR_DISABLE, NULL);
		} else{
			//deletion stage 1; perform only logical deletion
			delete_file_logical(fs.file_marked_for_deletion);
		}
		fs.file_marked_for_deletion = 0xFFFF;
	}

	//check if previous filesys structure blocks have been cleared
	if( fsv.old_fss_blocks_deleted==0 ){
		//journal pblocks
		if( fs.old_jour_pba[PRIMARY]!=0xFFFF ){ //secondary also
			for(i=0; i<2; i++){
				if( !FS_TEST_BIT( fs.BBBT[fs.old_jour_pba[i]/32], fs.old_jour_pba[i]%32) ){
					while( W25N_block_erase(fs.old_jour_pba[i])==W25N_ERR_BUSY );
					while( W25N_check_busy() );
					//detect bad blocks
					if(W25N_check_erase_fail()) FS_SET_BIT( fs.BBBT[fs.old_jour_pba[i]/32], fs.old_jour_pba[i]%32);
					else enqueue_block( &fs.freePB, fs.old_jour_pba[i] );
				}
			}
		}
		//fs_header pblock
		if( fs.old_fs_header_pba!=0xFFFF){
			if( !FS_TEST_BIT( fs.BBBT[fs.old_fs_header_pba/32], fs.old_fs_header_pba%32) ){
				while( W25N_block_erase(fs.old_fs_header_pba)==W25N_ERR_BUSY );
				while( W25N_check_busy() );
				//detect bad blocks
				if(W25N_check_erase_fail()) FS_SET_BIT( fs.BBBT[fs.old_fs_header_pba/32], fs.old_fs_header_pba%32);
				else enqueue_block( &fs.freePB, fs.old_fs_header_pba );
			}
		}
	}

	//must checkpoint journal because the journal block might be corrupt, or is full(crash occured during checkpointing)
	if( checkpoint_journal()==FS_FAIL ) return FS_FAIL;

	//after checkpointing, journal is available for new writes, can perform copyback if needed
	if( fs.copyback_lba!=0xFFFF) return copyback(fs.copyback_lba,fs.copyback_last_valid_subpage);

	return FS_OK;
}

//the filesystem header is stable all metadata and data are consistent
//-the filesystem header is unstable A if metadata in a file had been modified and corresponding physical blocks
//may have been written,meaning that upon journal replay these blocks must be treated as corrupt
//-the filesystem header is unstable C if crash with incorrect metadata occured during copyback operation
//stage 1(non BB copyback)
//-the filesystem header is unstable D if crash with incorrect metadata occured during copyback operation
//stage 2(non BB copyback)
uint8_t stabilize_fs(void){
	uint16_t next_block_lba, current_block_pba, current_block_lba;
	uint32_t subpage_pointer;
	uint8_t jmode;

	if( fs.stability==FS_STABLE ){
		return FS_OK;
	} else if( fs.stability==FS_UNSTABLE_A ){
		subpage_pointer = (fs.FDT[fs.ufile.file_id].nr_subpages!=0) ? (fs.FDT[fs.ufile.file_id].nr_subpages-1) : 0 ;
		current_block_lba = get_current_lba( fs.ufile.file_id, subpage_pointer );
		jmode = FILE_GET_JMODE( fs.FDT[fs.ufile.file_id].sflag );
		//FAT and freePB queue scan and fix + block deletion
		//mark block for copyback also if necessary
		next_block_lba = fs.FAT[current_block_lba];
		fs.FAT[current_block_lba] = FAT_EOF_MARK;
		while( next_block_lba!=FAT_EOF_MARK && next_block_lba!=FAT_FREE_LINK ){
			current_block_lba = next_block_lba;
			current_block_pba = fs.BTT[current_block_lba];
			next_block_lba = fs.FAT[current_block_lba];
			//erase block on memory module
			//first check if a previous bb_manage instruction detected a bad block
			if( !FS_TEST_BIT( fs.BBBT[current_block_pba/32], current_block_pba%32) ){
				while( W25N_block_erase(current_block_pba)==W25N_ERR_BUSY );
				while( W25N_check_busy() );
				//detect bad blocks
				if( W25N_check_erase_fail() ){
					FS_SET_BIT( fs.BBBT[current_block_pba/32], current_block_pba%32);
				} else {
					enqueue_block( &fs.freePB, fs.BTT[current_block_lba]);
				}
			}
			fs.BTT[current_block_lba] = BTT_FREE_LINK;
			fs.FAT[current_block_lba] = FAT_FREE_LINK;
		}
		//check if crash occured during bad block manage instruction at such a moment
		//that the the last lba's BTT entry holds a pba of an empty block
		//in such a case the copyback operation would yield incorrect results
		if( fs.ufile.bb_pba!=0xFFFF && fs.ufile.bb_lba==get_current_lba(fs.ufile.file_id, subpage_pointer) ){
			fs.BTT[fs.ufile.bb_lba] = fs.ufile.bb_pba;
		}
		fs.ufile.bb_lba = 0xFFFF;
		fs.ufile.bb_pba = 0xFFFF;

		if( jmode==JOUR_STRONG ){
			fs.copyback_lba = get_current_lba(fs.ufile.file_id, subpage_pointer);
			if( fs.FDT[fs.ufile.file_id].nr_subpages==0 ){
				fs.copyback_last_valid_subpage = 0xFFFF;
			} else {
				fs.copyback_last_valid_subpage = GET_SUBPAGE_IBL(subpage_pointer);
			}
		}
		fs.stability = FS_STABLE;
		return FS_OK;
	} else if( fs.stability==FS_UNSTABLE_C ) {
		//FS_UNSTABLE_C
		if( fs.ufile.sc_lba!=0xFFFF && fs.BTT[fs.ufile.sc_lba]!=fs.ufile.sc_pba ){	//latter could happen in WRITE_BB_DETECTED scenario
			if( !FS_TEST_BIT( fs.BBBT[fs.BTT[fs.ufile.sc_lba]/32], fs.BTT[fs.ufile.sc_lba]%32) ){
				while( W25N_block_erase(fs.BTT[fs.ufile.sc_lba])==W25N_ERR_BUSY );
				while( W25N_check_busy() );
				if( W25N_check_erase_fail() ){
					FS_SET_BIT( fs.BBBT[fs.BTT[fs.ufile.sc_lba]/32], fs.BTT[fs.ufile.sc_lba]%32);
				} else {
					enqueue_block( &fs.freePB, fs.BTT[fs.ufile.sc_lba]);
				}
			}
			fs.BTT[fs.ufile.sc_lba] = fs.ufile.sc_pba;
			fs.ufile.sc_lba = 0xFFFF;
			fs.ufile.sc_pba = 0xFFFF;
		}
		fs.stability = FS_STABLE;
		return FS_OK;
	} else {
		//FS_UNSTABLE_D
		//crash occured during copyback stage 2
		//pba to delete stored in ufile.sc_pba
		fsv.pba_to_delete = fs.ufile.sc_pba;
		if( !FS_TEST_BIT( fs.BBBT[fsv.pba_to_delete/32], fsv.pba_to_delete%32) ){
			while( W25N_block_erase(fsv.pba_to_delete)==W25N_ERR_BUSY );
			while( W25N_check_busy() );
			if( W25N_check_erase_fail() ){
				FS_SET_BIT( fs.BBBT[fsv.pba_to_delete/32], fsv.pba_to_delete%32);
			} else {
				enqueue_block( &fs.freePB, fsv.pba_to_delete);
			}
		}
		fs.stability = FS_STABLE;
		return FS_OK;
	}
}

void repair_FAT_and_BTT(uint16_t last_good_lba){
	uint16_t next_block_lba, current_block_lba;

	current_block_lba = last_good_lba;

	next_block_lba = fs.FAT[current_block_lba];
	fs.FAT[current_block_lba] = FAT_EOF_MARK;
	while( next_block_lba!=FAT_EOF_MARK && next_block_lba!=FAT_FREE_LINK ){
		current_block_lba = next_block_lba;
		next_block_lba = fs.FAT[current_block_lba];
		fs.BTT[current_block_lba] = BTT_FREE_LINK;
		fs.FAT[current_block_lba] = FAT_FREE_LINK;
	}
	return;
}

void journal_to_fs_head(journal_ops operation, uint8_t * params){
	uint16_t dummy __attribute__((unused));

	switch(operation){
	case BTT_MODIFY:
	{
		fs.BTT[*((uint16_t *)params)] = *((uint16_t *)(params+2));
		break;
	}
	case QUEUE_MODIFY:
	{
		if (*params == ENQUEUE_BLOCK){
			enqueue_block( &fs.freePB, *((uint16_t *)(params+1)) );
		} else {
			dequeue_block( &fs.freePB, &dummy );
		}
		break;
	}
	case FAT_MODIFY:
	{
		fs.FAT[*((uint16_t *)params)] = *((uint16_t *)(params+2));
		break;
	}
	case BBBT_UPDATE:
	{
		FS_SET_BIT( fs.BBBT[(*((uint16_t *)params))/32], (*((uint16_t *)params))%32 );
		break;
	}
	case BBBT_SOFT_REWRITE:
	{
		fs.BBBT[*((uint8_t *)params)] |= *((uint32_t *)(params+1));
		break;
	}
	case FNITT_UPDATE:
	{
		fs.FNITT[*((uint16_t *)params)] = *((uint16_t *)(params+2));
		break;
	}
	case FD_NR_BYTES_MODIFY:
	{
		fs.FDT[*((uint16_t *)params)].nr_bytes = *((uint32_t *)(params+2));
		break;
	}
	case FD_NR_SUBPAGES_MODIFY:
	{
		fs.FDT[*((uint16_t *)params)].nr_subpages = *((uint32_t *)(params+2));
		break;
	}
	case FD_NR_BYTES_INCREMENT:
	{
		fs.FDT[*((uint16_t *)params)].nr_bytes += *((uint32_t *)(params+2));
		break;
	}
	case FD_NR_SUBPAGES_INCREMENT:
	{
		fs.FDT[*((uint16_t *)params)].nr_subpages += *((uint32_t *)(params+2));
		break;
	}
	case FD_TIME_CREATED_MODIFY:
	{
		fs.FDT[*((uint16_t *)params)].time_created = *((uint32_t *)(params+2));
		break;
	}
	case FD_SFLAG_MODIFY:
	{
		fs.FDT[*((uint16_t *)params)].sflag = *(params+2);
		break;
	}
	case MARK_FILE_FOR_DELETION:
	{
		fs.file_marked_for_deletion = *((uint16_t *)params);
		break;
	}
	case FILE_DELETION_COMPLETED:
	{
		fsv.file_deletion_stage = 1;
		break;
	}
	case MARK_PBLOCK_USED:
	{
		fsv.marked_pblocks[fsv.marked_pblocks_len] = *((uint16_t *)params);
		fsv.marked_pblocks_len++;
		break;
	}
	case OLD_FSS_BLOCKS_DELETED:
	{
		fsv.old_fss_blocks_deleted = 1;
		break;
	}
	case FD_DEFAULT_VALUES:
	{
		fsv.FDT[*((uint16_t *)params)].pointer = 0;
		fsv.FDT[*((uint16_t *)params)].subpage_pointer = 0;
		fsv.FDT[*((uint16_t *)params)].subpage_read_offset = 0;
		fs.FDT[*((uint16_t *)params)].nr_bytes = 0;
		fs.FDT[*((uint16_t *)params)].nr_subpages = 0;
		//fs.file_desc[*((uint16_t *)params)].time_created = get_time();
		break;
	}
	case COPYBACK_BEGIN:
	{
		//stage 1 start
		fsv.marked_pblocks_len = 0;
		fsv.copyback_stage = 1;
		break;
	}
	case COPYBACK_COMPLETE:
	{
		if( *params==1 ){
			//stage 1 complete, start stage 2
			fsv.copyback_stage = 2;
			fsv.marked_pblocks_len = 0;
			fsv.pba_to_delete = *((uint16_t *)(params+1));
			fs.copyback_lba = 0xFFFF;
			fs.copyback_last_valid_subpage = 0xFFFF;
			fs.ufile.sc_lba = 0xFFFF;
			fs.ufile.sc_pba = 0xFFFF;
		} else {
			//stage 2 complete
			fsv.copyback_stage = 0;
			fs.copyback_lba = 0xFFFF;
			fs.copyback_last_valid_subpage = 0xFFFF;
			fs.stability = FS_STABLE;
		}
		break;
	}
	case WRITE_TRANSACTION_BEGIN:
	{
		fsv.ufile.file_id = *((uint16_t *)(params+1));

		if( *params== JOUR_STRONG ){
			fsv.strong_transaction_ongoing = 1;
		} else {
			fsv.weak_transaction_ongoing = 1;
		}
		fsv.marked_pblocks_len = 0;

		break;
	}
	case WRITE_TRANSACTION_COMPLETE:
	{
		fsv.strong_transaction_ongoing = 0;
		fsv.weak_transaction_ongoing = 0;
		fsv.marked_pblocks_len = 0;
		fs.stability = FS_STABLE;
		break;
	}
	case BB_COPYBACK_BEGIN:
	{
		fsv.ufile.bb_lba = *((uint16_t *)params);
		fsv.ufile.bb_pba = *((uint16_t *)(params+2));
		fsv.bb_copyback_ongoing = 1;
		break;
	}
	case BB_COPYBACK_COMPLETE:
	{
		//remove last marked pblock if a non-journaled transaction is in question
		if( fsv.bb_copyback_ongoing && !fsv.strong_transaction_ongoing && !fsv.weak_transaction_ongoing) fsv.marked_pblocks_len--;
		fsv.bb_copyback_ongoing = 0;
		fs.ufile.bb_lba = 0xFFFF;
		fs.ufile.bb_pba = 0xFFFF;
		break;
	}
	default:
	{
		break;
	}
	}
	return;
}

