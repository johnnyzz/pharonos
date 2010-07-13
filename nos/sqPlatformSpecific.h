#ifndef __SQ_PLATFORM_SPECIFIC_H__
#define __SQ_PLATFORM_SPECIFIC_H__

#define initialHeapSize 40*1024*1024

#include "multiboot.h"

// Video Mode Information
typedef struct DisplayInfo {
	int width, height, depth, address, bytesPerScanLine;
} DisplayInfo;


typedef struct Computer {
	DisplayInfo videoInfo;
	multiboot_info_t *mbi;
	void *image;
	unsigned long snapshotStartAddress, snapshotEndAddress;
} Computer;

sqInt sqMain(void *image);

void enable_paging();

void mark(int col);

#define warnPrintf printf

/* undefine clock macros (these are implemented as functions) */

#undef ioMSecs
#undef ioMicroMSecs
#undef ioLowResMSecs

#undef sqAllocateMemory
#undef sqGrowMemoryBy
#undef sqShrinkMemoryBy
#undef sqMemoryExtraBytesLeft

#include "sqMemoryAccess.h"

typedef unsigned int squeakFileOffsetType;

#undef	sqFilenameFromString
#undef	sqFilenameFromStringOpen
#define sqFilenameFromStringOpen sqFilenameFromString

#undef dispatchFunctionPointer
#undef dispatchFunctionPointerOnin

#undef	sqFTruncate

// File things

#undef sqImageFile
#undef sqImageFileClose
#undef sqImageFileOpen
#undef sqImageFilePosition
#undef sqImageFileRead
#undef sqImageFileSeek
#undef sqImageFileWrite
#undef sqImageFileStartLocation

#define sqImageFile			     			 MemoryFile*
#define sqImageFileClose(f)                  NULL
#define sqImageFileOpen(fileName, mode)      sqImageCopyMemoryBlock()
#define sqImageFilePosition(f)               f->offset
#define sqImageFileSeek(f, pos)              f->offset=pos
#define sqImageFileStartLocation(fileRef, fileName, size)  0
// #define sqImageFileRead(ptr, sz, count, f)	// see sqPlatformSpecific.c
#define sqImageFileWrite(ptr, sz, count, f)  sqMemoryFileWrite(ptr, sz, count, *f)

typedef struct {
        char            *file;
        unsigned long   offset;
        int             sessionID;
        int             writable;
        int             fileSize;
        int             lastOp;  /* 0 = uncommitted, 1 = read, 2 = write */
} SQFile;

typedef struct {
        unsigned long   start;
        unsigned long   offset;
        unsigned long   length;
} MemoryFile;

#ifndef __GNUC__
# if HAVE_ALLOCA_H
#   include <alloca.h>
# else
#   ifdef _AIX
#     pragma alloca
#   else
#     ifndef alloca /* predefined by HP cc +Olibcalls */
        char *alloca();
#     endif
#   endif
# endif
#endif

#endif  /* __SQ_PLATFORM_SPECIFIC_H__ */
