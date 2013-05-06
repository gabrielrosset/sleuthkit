/*
** The Sleuth Kit
**
** Copyright (c) 2013 Basis Technology Corp.  All rights reserved
** Contact: Brian Carrier [carrier <at> sleuthkit [dot] org]
**
** This software is distributed under the Common Public License 1.0
**
*/

/**
 * \file fatxxfs.h
 * Contains the structures and function APIs for TSK FAT (FAT12, FAT16, FAT32, 
 * exFAT) file system support.
 */

#ifndef _TSK_FATFS_H
#define _TSK_FATFS_H

#include "tsk_fs_i.h"

// RJCTODO: Comment for Doxygen
// RJCTODO: these appear to be the wrong comments...
/* size of FAT to read into FATFS_INFO each time */
/* This must be at least 1024 bytes or else fat12 will get messed up */
#define FAT_CACHE_N		4       // number of caches
#define FAT_CACHE_B		4096
#define FAT_CACHE_S		8       // number of sectors in cache

// RJCTODO: Comment for Doxygen
#define FAT_BOOT_SECTOR_SIZE 512

// RJCTODO: Comment for Doxygen
/* MASK values for FAT entries */
#define FATFS_12_MASK	0x00000fff
#define FATFS_16_MASK	0x0000ffff
#define FATFS_32_MASK	0x0fffffff
#define EXFATFS_MASK	0x0fffffff

// RJCTODO: Comment for Doxygen
#define FATFS_CLUST_2_SECT(fatfs, c)	\
	(TSK_DADDR_T)(fatfs->firstclustsect + ((((c) & fatfs->mask) - 2) * fatfs->csize))

// RJCTODO: Comment for Doxygen
#define FATFS_SECT_2_CLUST(fatfs, s)	\
	(TSK_DADDR_T)(2 + ((s)  - fatfs->firstclustsect) / fatfs->csize)

#ifdef __cplusplus
extern "C" {
#endif

    // RJCTODO: Comment for Doxygen
    typedef struct
    {
        uint8_t data[FAT_BOOT_SECTOR_SIZE - 2];
        uint8_t magic[2];
    } FAT_BOOT_SECTOR_RECORD;

    // RJCTODO: Comment for Doxygen
    /* 
     * Internal TSK_FS_INFO derived structure for FATXX and exFAT file systems.  
     */
    typedef struct {
        TSK_FS_INFO fs_info;    /* super class */
        //TSK_DATA_BUF *table;      /* cached section of file allocation table */

        /* FAT cache */
        /* cache_lock protects fatc_buf, fatc_addr, fatc_ttl */
        tsk_lock_t cache_lock;
        char fatc_buf[FAT_CACHE_N][FAT_CACHE_B];        //r/w shared - lock
        TSK_DADDR_T fatc_addr[FAT_CACHE_N];     // r/w shared - lock
        uint8_t fatc_ttl[FAT_CACHE_N];  //r/w shared - lock

        /* FIrst sector of FAT */
        TSK_DADDR_T firstfatsect;

        /* First sector after FAT  - For TSK_FS_INFO_TYPE_FAT_12 and TSK_FS_INFO_TYPE_FAT_16, this is where the
         * root directory entries are.  For TSK_FS_INFO_TYPE_FAT_32, this is the the first 
         * cluster */
        TSK_DADDR_T firstdatasect;

        /* The sector number were cluster 2 (the first one) is
         * for TSK_FS_INFO_TYPE_FAT_32, it will be the same as firstdatasect, but for TSK_FS_INFO_TYPE_FAT_12 & 16
         * it will be the first sector after the Root directory  */
        TSK_DADDR_T firstclustsect;

        /* size of data area in clusters, starting at firstdatasect */
        TSK_DADDR_T clustcnt;

        TSK_DADDR_T lastclust;

        /* sector where the root directory is located */
        TSK_DADDR_T rootsect;

        uint32_t dentry_cnt_se; /* max number of dentries per sector */
        uint32_t dentry_cnt_cl; /* max number of dentries per cluster */

        uint16_t ssize;         /* size of sectors in bytes */
        uint16_t ssize_sh;      /* power of 2 for size of sectors */
        uint8_t csize;          /* size of clusters in sectors */
        uint8_t numfat;         /* number of fat tables */
        uint32_t sectperfat;    /* sectors per fat table */
        uint16_t numroot;       /* number of 32-byte dentries in root dir */
        uint32_t mask;          /* the mask to use for the sectors */

        tsk_lock_t dir_lock;    //< Lock that protects inum2par.
        void *inum2par;         //< Maps subfolder metadata address to parent folder metadata addresses.

		/* RJCTODO: Comment */
		char boot_sector_buffer[FAT_BOOT_SECTOR_SIZE];
        int using_backup_boot_sector;

		/* RJCTODO: Comment */
        uint8_t(*is_clust_alloc)(TSK_FS_INFO* fs, TSK_DADDR_T clust);

	} FATFS_INFO;

	/**
	 * \internal
	 * Open part of a disk image as an FAT file system. 
	 *
	 * @param a_img_info Disk image to analyze
	 * @param a_offset Byte offset where FAT file system starts
	 * @param a_ftype Specific type of FAT file system
	 * @param a_test NOT USED
	 * @returns NULL on error or if data is not a FAT file system
	 */
	extern TSK_FS_INFO 
    *fatfs_open(TSK_IMG_INFO *a_img_info, TSK_OFF_T a_offset, TSK_FS_TYPE_ENUM a_ftype, uint8_t a_test);

	// RJCTODO: Add comment
    extern int8_t fatfs_is_sectalloc(FATFS_INFO *, TSK_DADDR_T);

    // RJCTODO: Add comment
    extern int8_t fatfs_is_clustalloc(FATFS_INFO * fatfs,
        TSK_DADDR_T clust);

	// RJCTODO: Add comment
    extern uint8_t
    fatfs_block_walk(TSK_FS_INFO * fs, TSK_DADDR_T a_start_blk,
        TSK_DADDR_T a_end_blk, TSK_FS_BLOCK_WALK_FLAG_ENUM a_flags,
        TSK_FS_BLOCK_WALK_CB a_action, void *a_ptr);

	// RJCTODO: Add comment
    // RJCTODO: Needed in fs_dir.c by load_orphan_dir_walk_cb
    extern uint8_t 
    fatfs_dir_buf_add(FATFS_INFO * fatfs, TSK_INUM_T par_inum, TSK_INUM_T dir_inum); 

    /* return 1 on error and 0 on success */
    extern uint8_t
    fatfs_jopen(TSK_FS_INFO * fs, TSK_INUM_T inum);

    /* return 1 on error and 0 on success */
    extern uint8_t
    fatfs_jentry_walk(TSK_FS_INFO * fs, int a_flags,
        TSK_FS_JENTRY_WALK_CB a_action, void *a_ptr);

    /* return 1 on error and 0 on success */
    extern uint8_t
    fatfs_jblk_walk(TSK_FS_INFO * fs, TSK_DADDR_T start, TSK_DADDR_T end,
        int a_flags, TSK_FS_JBLK_WALK_CB a_action, void *a_ptr);

#ifdef __cplusplus
}
#endif

#endif