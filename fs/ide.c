/*
 * operations on IDE disk.
 */

#include "fs.h"
#include "lib.h"
#include <mmu.h>

// Overview:
// 	read data from IDE disk. First issue a read request through
// 	disk register and then copy data from disk buffer
// 	(512 bytes, a sector) to destination array.
//
// Parameters:
//	diskno: disk number.
// 	secno: start sector number.
// 	dst: destination for data read from IDE disk.
// 	nsecs: the number of sectors to read.
//
// Post-Condition:
// 	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)
{
	// 0x200: the size of a sector: 512 bytes.
	u_int offset_begin = secno * 0x200;
	u_int offset_end = offset_begin + nsecs * 0x200;
	u_int offset = 0;

	while (offset_begin + offset < offset_end) {
		u_int now = offset_begin + offset;
//		writef("in ...  ide read 0\n");
		if (syscall_write_dev((u_int)(&diskno), (u_int)0x13000010, (u_int)4) < 0) {
            user_panic("1-syscall_write_dev error!!!\n");
        }
//		writef("in   ide read 1\n");
        if (syscall_write_dev((u_int)(&now), (u_int)0x13000000, (u_int)4) < 0) {
            user_panic("2-syscall_write_dev error!!!\n");
        }
//		writef("in   ide read 2\n");
        char zero = 0;
        if (syscall_write_dev((u_int)(&zero), (u_int)0x13000020, (u_int)1) < 0) {
            user_panic("3-syscall_write_dev error!!!\n");
        }
//		writef("in   ide read 3\n");
        int ret = 0;
        if (syscall_read_dev((u_int)(&ret), (u_int)0x13000030, (u_int)4) < 0) {
            user_panic("4-syscall_read_dev error!!!\n");
        }
//		writef("in   ide read 4\n");
        if (ret != 0) {
            if (syscall_read_dev((u_int)(dst + offset), (u_int)0x13004000, (u_int)512) < 0) {
                user_panic("4-syscall_read_dev error!!!\n");
            }
        } else {
            user_panic("5- ret ==0\n");
        }
//		writef("in   ide read 5\n");
        offset = offset + 0x200;
	}
}


// Overview:
// 	write data to IDE disk.
//
// Parameters:
//	diskno: disk number.
//	secno: start sector number.
// 	src: the source data to write into IDE disk.
//	nsecs: the number of sectors to write.
//
// Post-Condition:
//	If error occurrs during the read of the IDE disk, panic.
//
// Hint: use syscalls to access device registers and buffers
/*** exercise 5.2 ***/
void
ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)
{
	// Your code here
	u_int offset_begin = secno * 0x200;
	u_int offset_end = offset_begin + nsecs * 0x200;
	u_int offset = 0;

	// DO NOT DELETE WRITEF !!!
	writef("diskno: %d\n", diskno);

	while (offset_begin + offset < offset_end) {
		//  copy data from source array to disk buffer.
		// if error occur, then panic.
        u_int now = offset_begin + offset;
		if (syscall_write_dev((u_int)(&diskno), (u_int)0x13000010, (u_int)4) < 0) {
            user_panic("1-syscall_write_dev error!!!\n");
        }
        if (syscall_write_dev((u_int)(&now), (u_int)0x13000000, (u_int)4) < 0) {
            user_panic("2-syscall_write_dev error!!!\n");
        }
        if (syscall_write_dev((u_int)(src + offset), (u_int)0x13004000, (u_int)512) < 0) {
            user_panic("3-syscall_write_dev error!!!\n");
        }
        char one = 1;
        if (syscall_write_dev((u_int)(&one), (u_int)0x13000020, (u_int)1) < 0) {
            user_panic("4-syscall_write_dev error!!!\n");
        }
        int ret = 0;
        if (syscall_read_dev((u_int)(&ret), (u_int)0x13000030, (u_int)4) < 0) {
            user_panic("5-syscall_read_dev error!!!\n");
        }
        if (ret == 0) {
            user_panic("6-write dev error!!!\n");
        }
        offset = offset + 0x200;
	}
}
/*
int raid4_valid(u_int diskno) {
//	writef("hhhhhhhhh 1\n");
    if (syscall_write_dev((u_int)(&diskno), (u_int)0x13000010, (u_int)4) < 0) { //id
        user_panic("1-syscall_write_dev error!!!\n");
    }
ii	writef("hhhhhhhhh 2\n");
	int zero = 0;
    if (syscall_write_dev((u_int)(&zero), (u_int)0x13000000, (u_int)4) < 0) { //offset
        user_panic("2-syscall_write_dev error!!!\n");
    }
	 zero = 0;
//	writef("hhhhhhhhh 3\n");
    if (syscall_write_dev((u_int)(&zero), (u_int)0x13000020, (u_int)1) < 0) {
        user_panic("3-syscall_write_dev error!!!\n");
    }
    int ret;
//	writef("hhhhhhhhh\n");
    if (syscall_read_dev((u_int)(&ret), (u_int)0x13000030, (u_int)4) < 0) {
        user_panic("4-syscall_read_dev error!!!\n");
    }
    if (ret != 0) { //valid
        return 1;
    } else {
        return 0;
    }
}

int raid4_write(u_int blockno, void *src) {
    int i, j;
    u_int p[BY2SECT / 4];
    int temp[10];
    int wrong[10];
    int destroy = 0;
    u_int offset = 0;
    void *now;

    for (i = 0; i < (BY2SECT / 4); ++i) {
        p[i] = 0;
    }

    int secno = blockno * 2;
    for (i = 1; i <= 4; ++i) {
        wrong[i] = raid4_valid(i);
        now = src + offset;
        if (wrong[i] == 0) {
            destroy++;
        } else {
            ide_write((u_int)i, secno, now, 1);
        }
        for (j = 0; j < (BY2SECT / 4); ++j) {
            u_int *num = ((u_int *)now);
            p[j] = p[j] ^ num[j];
        }
        offset = offset + BY2SECT;
    }
    wrong[5] = raid4_valid(5);
    if (wrong[5] == 0) {
        destroy++;
    } else {
        ide_write(5, secno, (void *)p, 1);
    }

    for (i = 0; i < (BY2SECT / 4); ++i) {
        p[i] = 0;
    }

    secno = blockno * 2 + 1;
    for (i = 1; i <= 4; ++i) {
        now = src + offset;
        if (wrong[i] != 0) {
            ide_write((u_int)i, secno, now, 1);
        }
        u_int *num = ((u_int *)now);
        for (j = 0; j < (BY2SECT / 4); ++j) {
            p[j] = p[j] ^ num[j];
        }
        offset = offset + BY2SECT;
    }
    if (wrong[5] != 0) {
        ide_write(5, secno, (void *)p, 1);
    }
    return destroy;
}

int raid4_read(u_int blockno, void *dst) {
    int destroy = 0;
    int wrong[10];
    int wrong_disk_no;
    int i, j;
    for (i = 1; i <= 5; ++i) {
        wrong[i] = raid4_valid(i);
        if (wrong[i] == 0) {
            destroy++;
            wrong_disk_no = i;
        }
    }
    if (destroy > 1) {
        return destroy;
    }
    u_int offset;
    int secno;
    void *now;
    if ((destroy == 1) && (wrong[5]) == 0) {
        offset = 0;
        secno = blockno * 2;
        for (i = 1; i <= 4; ++i) {
            now = dst + offset;
            ide_read(i, secno, now, 1);
            offset = offset + BY2SECT;
        }
        secno = blockno * 2 + 1;
        for (i = 1; i <= 4; ++i) {
            now = dst + offset;
            ide_read(i, secno, now, 1);
            offset = offset + BY2SECT;
        }
        return 1;
    }
    if ((destroy == 1) && (wrong[5] == 1)) {

        return 1;
    }
    // destroy == 0;
    int ret = 0;
    offset = 0;
    u_int p[BY2SECT / 4];
    u_int pca[BY2SECT / 4];
    secno = blockno * 2;
    ide_read(5, secno, (void *)p, 1);
    for (i = 0; i < (BY2SECT / 4); ++i) {
        pca[i] = 0;
    }
    for (i = 1; i <= 4; ++i) {
        now = dst + offset;
        ide_read(i, secno, now, 1);
        u_int *num = ((u_int *)now);
        for (j = 0; j < (BY2SECT / 4); ++j) {
            pca[j] = pca[j] ^ num[j];
        }
        offset = offset + BY2SECT;
    }
    for (j = 0; j < (BY2SECT / 4); ++j) {
        if (p[j] != pca[j]) {
            ret = -1;
            break;
        }
    }
    secno = blockno * 2 + 1;
    ide_read(5, secno, (void *)p, 1);
    for (i = 0; i < (BY2SECT / 4); ++i) {
        pca[i] = 0;
    }
    for (i = 1; i <= 4; ++i) {
        now = dst + offset;
        ide_read(i, secno, now, 1);
        u_int *num = ((u_int *)now);
        for (j = 0; j < (BY2SECT / 4); ++j) {
            pca[j] = pca[j] ^ num[j];
        }
        offset = offset + BY2SECT;
    }
    if (ret != -1) {
        for (j = 0; j < (BY2SECT / 4); ++j) {
            if (p[j] != pca[j]) {
                ret = -1;
                break;
            }
        }
    }
    return ret;
}*/
