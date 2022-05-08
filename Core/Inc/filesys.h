#ifndef __FILESYS_H
#define __FILESYS_H

#include "main.h"
#include "w25n_api.h"

#define INIT_KEY							0xAD2021FE
#define FS_HEADER_KEY						0xCE2021DC

#define FS_NOT_FIRST_INITED					0
#define FS_FIRST_INITED                 	1

#define ERR_Q_EMPTY							0
#define Q_OK								1

#define	FS_FAIL								0
#define FS_OK								1

#define FS_FAIL_2B							0xFFFF

#define FAT_FREE_LINK						0xFFFF
#define FAT_EOF_MARK						0xFF00
#define FAT_FS_HEADER_MARK					0xFFAA
#define FAT_JOUR_MARK						0xFFBB

#define BTT_FREE_LINK						0xFFFF

#define FS_NAME_FREE						0xFFFF

#define NR_BLOCKS							1024
#define PAGES_PER_BLOCK						64
#define BYTES_PER_PAGE						2048
#define BYTES_PER_PAGE_EFF					2040

#define SUBPAGES_PER_PAGE					4
#define SUBPAGES_PER_BLOCK					( SUBPAGES_PER_PAGE * PAGES_PER_BLOCK )

#define BYTES_PER_BLOCK						( BYTES_PER_PAGE * PAGES_PER_BLOCK )
#define BYTES_PER_BLOCK_EFF					( BYTES_PER_PAGE_EFF * PAGES_PER_BLOCK )

#define BYTES_PER_SUBPAGE					512
#define BYTES_PER_SUBPAGE_EFF				510

#define FS_TEST_BIT(WORD,BIT) 				( ((WORD)>>(BIT)) & 0x00000001 )
#define FS_SET_BIT(WORD,BIT)				W25N_SET_BIT(WORD,BIT)
#define FS_RESET_BIT(WORD,BIT)				W25N_RESET_BIT(WORD,BIT)

#define GET_BLOCK(SUBPAGE_POINTER)			( (SUBPAGE_POINTER)/SUBPAGES_PER_BLOCK )
#define GET_PAGE(SUBPAGE_POINTER)			( ((SUBPAGE_POINTER)/SUBPAGES_PER_PAGE) % PAGES_PER_BLOCK )			//within block
#define GET_SUBPAGE(SUBPAGE_POINTER)		( (SUBPAGE_POINTER) % SUBPAGES_PER_PAGE )							//within page
#define GET_SUBPAGE_IBL(SUBPAGE_POINTER)	( (SUBPAGE_POINTER)%SUBPAGES_PER_BLOCK )							//within block

#define GET_NR_BLOCKS(NR_BYTES)				( ((NR_BYTES)/BYTES_PER_BLOCK_EFF) + ( ((NR_BYTES)%BYTES_PER_BLOCK_EFF) ? 1 : 0 ) )
#define GET_NR_PAGES(NR_BYTES)				( ((NR_BYTES)/BYTES_PER_PAGE_EFF) + ( ((NR_BYTES)%BYTES_PER_PAGE_EFF) ? 1 : 0 ) )
#define GET_NR_SUBPAGES(NR_BYTES)			( ((NR_BYTES)/BYTES_PER_SUBPAGE_EFF) + ( ((NR_BYTES)%BYTES_PER_SUBPAGE_EFF) ? 1 : 0 ) )

#define LAST_SUBPAGE						( SUBPAGES_PER_PAGE - 1 )
#define LAST_PAGE							( PAGES_PER_BLOCK - 1 )

#define FILE_IS_WRONLY						0b10000000
#define FILE_IS_RDONLY						0b01000000
#define FILE_JOUR_MODE_MASK					0b00000010
#define FILE_IS_OPEN_MASK					0b00000001

#define FILE_CHECK_WRONLY(SF)				((SF) & FILE_IS_WRONLY)
#define FILE_CHECK_RDONLY(SF)				((SF) & FILE_IS_RDONLY)
#define FILE_GET_JMODE(SF)					( ((SF) & FILE_JOUR_MODE_MASK) ? 1 : 0 )//0==WEAK, 1==STRONG
#define FILE_CHECK_OPEN(SF)					((SF) & FILE_IS_OPEN_MASK)

#define FILE_SET_WRONLY(SF)					((SF) |= FILE_IS_WRONLY)
#define FILE_SET_RDONLY(SF)					((SF) |= FILE_IS_RDONLY)
#define FILE_SET_JMODE(SF,JMODE)			( (JMODE) ? ((SF)|=FILE_JOUR_MODE_MASK) : ((SF)&=(~FILE_JOUR_MODE_MASK)) ) //JMODE 0==WEAK, 1==STRONG
#define FILE_SET_OPEN(SF)					((SF) |= FILE_IS_OPEN_MASK)

#define FILE_CLOSE(SF)						( (SF) &= (~(FILE_IS_OPEN_MASK | FILE_IS_WRONLY | FILE_IS_RDONLY)) )

#define JOURNAL_ENTRY_LEN					1024 		//can be increased or decreased if necessary

#define FS_HEADER_LBA						0x0000
#define JOURNAL_LBA							0x0001
#define SEC_JOURNAL_LBA						0x0002

#define ENQUEUE_BLOCK						0x01
#define DEQUEUE_BLOCK						0x02

#define PRIMARY								0
#define SECONDARY							1

#define NO_ENQUEUE							0
#define ENQUEUE								1

#define JOUR_OP_MAX_BYTES					7

#define CHECK_ENTRY_SPACE(NR_BYTES,IND)		( (NR_BYTES)+(IND)>JOURNAL_ENTRY_LEN ? 0 : 1 )

typedef enum{
	FS_UNSTABLE_A,
	FS_UNSTABLE_C,
	FS_UNSTABLE_D,
	FS_STABLE

} stability_state;

typedef enum{
	UNDEFINED,
	UNDEFINED_OPERATION,
	FS_HEADER_MISSING,
	HEAD_BB_DETECTED,
	WRITE_BB_DETECTED,
	ERASE_BB_DETECTED,
	BBM_OUT_OF_MEMORY,
	OUT_OF_MEMORY,
	CANNOT_SAVE_FS_HEADER,
	OUT_OF_BOUND_READ,
	SUBPAGE_SIZE_ERR,
	FILE_NOT_OPEN,
	ILLEGAL_OPERATION,
	FILE_DOESNT_EXIST,
	RDWR_CONFLICT,
	FILE_ALREADY_OPEN,
	FILE_ALREADY_EXIST,
	COPYBACK_OUT_OF_MEMORY,
	SEC_JOURNAL_OVERFLOW,
	INVALID_FILEDES,
	INVALID_FILE_NAME

} FS_errno;

typedef enum{
	BTT_MODIFY						=0,
	QUEUE_MODIFY					=1,
	FAT_MODIFY						=2,
	BBBT_UPDATE						=3,
	BBBT_SOFT_REWRITE				=4,
	HEADER_LBA_MODIFY				=5,
	FNITT_UPDATE					=6,
	FD_NR_BYTES_MODIFY				=7,
	FD_NR_SUBPAGES_MODIFY			=8,
	FD_NR_BYTES_INCREMENT			=9,
	FD_NR_SUBPAGES_INCREMENT		=10,
	FD_TIME_CREATED_MODIFY			=11,
	FD_SFLAG_MODIFY					=12,
	MARK_FILE_FOR_DELETION			=13,
	FILE_DELETION_COMPLETED			=14,
	MARK_PBLOCK_USED				=15,
	OLD_FSS_BLOCKS_DELETED			=16,
	FD_DEFAULT_VALUES				=17,
	COPYBACK_BEGIN					=18,
	COPYBACK_COMPLETE				=19,
	WRITE_TRANSACTION_BEGIN			=20,
	WRITE_TRANSACTION_COMPLETE		=21,
	BB_COPYBACK_BEGIN				=22,
	BB_COPYBACK_COMPLETE			=23

} journal_ops;

//do not change positions of JOUR_WEAK and JOUR_STRONG
typedef enum{
	JOUR_WEAK,						//use for large, multi-block files
	JOUR_STRONG,					//use for small files with incremental writes
	JOUR_DISABLE					//no entries recorded on journal
} journaling_mode;

struct journal_entry{
	uint8_t entry[JOURNAL_ENTRY_LEN];
	uint16_t entry_index;

};

struct journal_operation{
	uint8_t param0_bytes;
	uint8_t param1_bytes;

};

//file descriptor non-volatile
struct file_desc{
	uint32_t nr_bytes;
	uint32_t nr_subpages;
	uint32_t time_created;
	//status flag - sflag[7] write open; sflag[6] read open; sflag[1] journal mode - 0 WEAK, 1 STRONG; sflag[0] is open
	uint8_t sflag;

} __PACKED;

//file descriptor volatile
struct file_decs_vol{
	uint32_t pointer;
	uint32_t subpage_pointer;
	uint16_t subpage_read_offset;

} __PACKED;

//queue of free physical blocks
struct queue{
	int16_t head;
	int16_t tail;
	uint16_t blocks[NR_BLOCKS+1];

} __PACKED;

struct unstable_file{
	uint16_t file_id;
	uint16_t bb_lba;
	uint16_t bb_pba;
	uint16_t sc_lba; //stable copyback block
	uint16_t sc_pba;

} __PACKED;


struct filesys_header{
	//block translation table - BTT
	//LBA to PBA
	uint16_t BTT[NR_BLOCKS];

	//queue of free physical blocks
	//blocks with least erase cycles first
	struct queue freePB;

	//file allocation table - FAT
	uint16_t FAT[NR_BLOCKS];

	//file descriptor table - FDT - non-volatile part
	struct file_desc FDT[NR_BLOCKS];

	//bad block bit table - BBBT
	//refers to physical blocks
	//note: bits start from LSB
	uint32_t BBBT[32];

	//file name to file id translation table
	//file name is an integer  [0,1023]
	uint16_t FNITT[NR_BLOCKS];

	//PBAs of previous primary and secondary journal block
	uint16_t old_jour_pba[2];

	//PBA of previos fs_header block
	uint16_t old_fs_header_pba;

	//file_id of file marked for deletion
	uint16_t file_marked_for_deletion;

	//lba marked for copyback
	uint16_t copyback_lba;

	//last valid subpage page of block with pending copyback operation
	uint16_t copyback_last_valid_subpage;

	//state of filesystem stabilty upon fs_header save operation
	stability_state stability;

	//unstable file paramaters
	struct unstable_file ufile;

} __PACKED;


struct filesys_header_volatile{
	//primary and secondary journal block current page (each entry takes two pages)
	uint16_t journal_page[2];

	//indicates whether the previous pblocks holding the filesys structures have been cleared
	//(fs_header and journal blocks) - used during journal replay
	uint8_t old_fss_blocks_deleted;

	//non-volatile current header version
	uint32_t header_version;

	//at reset assume not initialized
	uint8_t first_inited;

	//most recent pba BB managed
	uint16_t last_pba_BB_managed;

	//used during journal replay, marks ongoing strong journaled write transaction
	uint8_t strong_transaction_ongoing;

	//used during journal replay, marks ongoing weak journaled write transaction
	uint8_t weak_transaction_ongoing;

	//used to signal between expand_file() and write_data_to_file() for
	//weak journaled transactions, when new blocks have been allocated
	uint8_t weak_transaction_nb;

	//used during journal replay
	//stage=0 no copyback action
	//stage=1 block copy stage
	//stage=2 old block delete stage
	uint8_t copyback_stage;

	//if crash occured during stage 2 of copyback, here is the pba to delete
	uint8_t pba_to_delete;

	//volatile part of FDT
	struct file_decs_vol FDT[NR_BLOCKS];

	//file upon which strong transaction is carried on
	struct unstable_file ufile;

	//list of pblock that are strong journaled and might or might not be subject to deletion
	//depending on transaction status upon journal replay end
	uint16_t marked_pblocks[32];

	//number of marked_pblocks, based on journal block size
	//it cannot be greater than 32
	uint16_t marked_pblocks_len;

	//used for copyback when last page was partially programmed
	uint8_t copyback_local_page[BYTES_PER_SUBPAGE*3];

	//nr subpages predicted in expand_file(), to be updated in fs at the end of write_file();
	uint32_t new_nr_subpages;

	//nur bytes -||-
	uint32_t new_nr_bytes;

	//used during journal replay
	uint8_t file_deletion_stage;

	//used during journal replay
	uint8_t bb_copyback_ongoing;

	//how many blocks have been dequeud in checkpoint_journal(), to be written on secondary journal
	uint16_t blocks_dequeued;

};

uint8_t file_system_init(void);

uint8_t fs_critical_error_recovery(void);

void wipe_memory(uint32_t * BBBT);

void init_fs_head(void);

uint8_t check_filesys_first_inited(void);

void mark_filesys_first_inited(void);

uint8_t save_fs_header_to_flash(void);

uint8_t save_fs_header_to_flash_low(void);

uint8_t delete_pb(uint16_t fs_head_pba, uint8_t enq);

uint8_t get_fs_header_from_mem(void);

uint16_t create_file(journaling_mode jmode);

uint8_t expand_file(uint16_t file_id, uint32_t bytes_more);

uint8_t delete_file(uint16_t file_id, journaling_mode jmode, uint16_t file_name);

void delete_file_logical(uint16_t file_id);

uint8_t write_data_to_file(uint16_t file_id, uint32_t nr_bytes, uint8_t * data);

uint8_t write_data_to_file_low(uint16_t file_id, uint32_t nr_bytes, uint8_t * data);

uint8_t read_data_from_file(uint16_t file_id, uint32_t nr_bytes, uint8_t * data);

uint16_t get_current_lba(uint16_t file_id, uint32_t subpage_pointer);

uint8_t BB_manage(uint16_t lba, uint8_t first_invalid_subpage, journaling_mode jmode);

uint8_t BB_copyback(uint16_t lba, uint8_t last_valid_subpage);

uint8_t BB_error_recovery(uint16_t file_id);

uint8_t write_journal(journal_ops operation, uint16_t param0, uint32_t param1);

uint8_t write_and_commit_journal_entry_to_flash(void);

uint8_t sec_journal_write_and_commit(uint16_t target_pba, uint16_t blocks_dequeued);

uint8_t checkpoint_journal(void);

uint8_t replay_journal(void);

uint8_t stabilize_fs(void);

void repair_FAT_and_BTT(uint16_t last_good_lba);

void journal_to_fs_head(journal_ops operation, uint8_t * params);

void init_local_journal_entry(void);

uint8_t copyback(uint16_t lba, uint16_t last_valid_subpage);

uint8_t copyback_low(uint16_t lba, uint16_t last_valid_subpage);

__STATIC_INLINE void queue_init(struct queue *q){
	q->head = 0;
	q->tail = 0;
}

__STATIC_INLINE void enqueue_block(struct queue *q, uint16_t element) {
	q->head++;
	q->head %= (NR_BLOCKS+1);
	q->blocks[q->head] = element;
}

__STATIC_INLINE uint8_t dequeue_block(struct queue *q, uint16_t *element) {
	if(q->head == q->tail) return ERR_Q_EMPTY;
	q->tail++;
	q->tail %= (NR_BLOCKS+1);
	*element = q->blocks[q->tail];
	return Q_OK;
}

__STATIC_INLINE uint16_t get_queue_size(struct queue *q){
	if(q->head >= q->tail) {
		return (q->head - q->tail);
	} else {
		return (q->head - q->tail + NR_BLOCKS+1);
	}
}

//retuns file_id given file name
__STATIC_INLINE uint16_t get_file_id(uint16_t file_name, struct filesys_header fs){
	return fs.FNITT[file_name];
}

extern struct filesys_header fs;

extern struct filesys_header_volatile fsv;

extern FS_errno errno;

extern journaling_mode current_jour_mode;

#endif //__FILESYS_H
