/*
 * tkImgGIF.c --
 *
 *	A photo image file handler for GIF files. Reads 87a and 89a GIF
 *	files. At present, there only is a file write function. GIF images
 *	may be read using the -data option of the photo image.  The data may be
 *	given as a binary string in a Tcl_Obj or by representing
 *	the data as BASE64 encoded ascii.  Derived from the giftoppm code
 *	found in the pbmplus package and tkImgFmtPPM.c in the tk4.0b2
 *	distribution.
 *
 * Copyright (c) Reed Wade (wade@cs.utk.edu), University of Tennessee
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1997 Australian National University
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * This file also contains code from the giftoppm program, which is
 * copyrighted as follows:
 *
 * +--------------------------------------------------------------------+
 * | Copyright 1990, David Koblas.					|
 * |   Permission to use, copy, modify, and distribute this software	|
 * |   and its documentation for any purpose and without fee is hereby	|
 * |   granted, provided that the above copyright notice appear in all	|
 * |   copies and that both that copyright notice and this permission	|
 * |   notice appear in supporting documentation.  This software is	|
 * |   provided "as is" without express or implied warranty.		|
 * +-------------------------------------------------------------------+
 *
 * RCS: @(#) Id
 */

/*
 * GIF's are represented as data in base64 format.
 * base64 strings consist of 4 6-bit characters -> 3 8 bit bytes.
 * A-Z, a-z, 0-9, + and / represent the 64 values (in order).
 * '=' is a trailing padding char when the un-encoded data is not a
 * multiple of 3 bytes.  We'll ignore white space when encountered.
 * Any other invalid character is treated as an EOF
 */

#define GIF_SPECIAL	(256)
#define GIF_PAD		(GIF_SPECIAL+1)
#define GIF_SPACE	(GIF_SPECIAL+2)
#define GIF_BAD		(GIF_SPECIAL+3)
#define GIF_DONE	(GIF_SPECIAL+4)

/*
 * structure to "mimic" FILE for Mread, so we can look like fread.
 * The decoder state keeps track of which byte we are about to read,
 * or EOF.
 */

typedef struct mFile {
    unsigned char *data;	/* mmencoded source string */
    int c;			/* bits left over from previous character */
    int state;			/* decoder state (0-4 or GIF_DONE) */
} MFile;

#include "tkInt.h"
#include "tkPort.h"

/*
 * Non-ASCII encoding support:
 * Most data in a GIF image is binary and is treated as such.  However,
 * a few key bits are stashed in ASCII.  If we try to compare those pieces
 * to the char they represent, it will fail on any non-ASCII (eg, EBCDIC)
 * system.  To accomodate these systems, we test against the numeric value
 * of the ASCII characters instead of the characters themselves.  This is
 * encoding independant.
 */

static CONST char GIF87a[] = {			/* ASCII GIF87a */
    0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x00
};
static CONST char GIF89a[] = {			/* ASCII GIF89a */
    0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x00
};
#  define GIF_TERMINATOR 0x3b			/* ASCII ; */
#  define GIF_EXTENSION  0x21			/* ASCII ! */
#  define GIF_START	 0x2c			/* ASCII , */

/*
 * 			 HACK ALERT!!  HACK ALERT!!  HACK ALERT!!
 * This code is hard-wired for reading from files.  In order to read
 * from a data stream, we'll trick fread so we can reuse the same code.
 * 0==from file; 1==from base64 encoded data; 2==from binary data
 */

typedef struct ThreadSpecificData {
    int fromData;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The format record for the GIF file format:
 */

static int	FileMatchGIF _ANSI_ARGS_((Tcl_Channel chan, CONST char *fileName,
		    Tcl_Obj *format, int *widthPtr, int *heightPtr,
		    Tcl_Interp *interp));
static int	FileReadGIF  _ANSI_ARGS_((Tcl_Interp *interp,
		    Tcl_Channel chan, CONST char *fileName, Tcl_Obj *format,
		    Tk_PhotoHandle imageHandle, int destX, int destY,
		    int width, int height, int srcX, int srcY));
static int	StringMatchGIF _ANSI_ARGS_(( Tcl_Obj *dataObj,
		    Tcl_Obj *format, int *widthPtr, int *heightPtr,
		    Tcl_Interp *interp));
static int	StringReadGIF _ANSI_ARGS_((Tcl_Interp *interp, Tcl_Obj *dataObj,
		    Tcl_Obj *format, Tk_PhotoHandle imageHandle,
		    int destX, int destY, int width, int height,
		    int srcX, int srcY));
static int 	FileWriteGIF _ANSI_ARGS_((Tcl_Interp *interp,  
		    CONST char *filename, Tcl_Obj *format,
		    Tk_PhotoImageBlock *blockPtr));
static int	CommonWriteGIF _ANSI_ARGS_((Tcl_Interp *interp,
		    Tcl_Channel handle, Tcl_Obj *format,
		    Tk_PhotoImageBlock *blockPtr));

Tk_PhotoImageFormat tkImgFmtGIF = {
    "gif",		/* name */
    FileMatchGIF,	/* fileMatchProc */
    StringMatchGIF,	/* stringMatchProc */
    FileReadGIF,	/* fileReadProc */
    StringReadGIF,	/* stringReadProc */
    FileWriteGIF,	/* fileWriteProc */
    NULL,		/* stringWriteProc */
};

#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define MAXCOLORMAPSIZE		256
#define CM_RED			0
#define CM_GREEN		1
#define CM_BLUE			2
#define CM_ALPHA		3
#define MAX_LWZ_BITS		12
#define LM_to_uint(a,b)		(((b)<<8)|(a))
#define ReadOK(file,buffer,len)	(Fread(buffer, len, 1, file) != 0)

/*
 * Prototypes for local procedures defined in this file:
 */

static int		DoExtension _ANSI_ARGS_((Tcl_Channel chan, int label,
			    int *transparent));
static int		GetCode _ANSI_ARGS_((Tcl_Channel chan, int code_size,
			    int flag));
static int		GetDataBlock _ANSI_ARGS_((Tcl_Channel chan,
			    unsigned char *buf));
static int		ReadColorMap _ANSI_ARGS_((Tcl_Channel chan, int number,
			    unsigned char buffer[MAXCOLORMAPSIZE][4]));
static int		ReadGIFHeader _ANSI_ARGS_((Tcl_Channel chan,
			    int *widthPtr, int *heightPtr));
static int		ReadImage _ANSI_ARGS_((Tcl_Interp *interp,
			    char *imagePtr, Tcl_Channel chan,
			    int len, int rows,
			    unsigned char cmap[MAXCOLORMAPSIZE][4],
			    int width, int height, int srcX, int srcY,
			    int interlace, int transparent));

/*
 * these are for the BASE64 image reader code only
 */

static int		Fread _ANSI_ARGS_((unsigned char *dst, size_t size,
			    size_t count, Tcl_Channel chan));
static int		Mread _ANSI_ARGS_((unsigned char *dst, size_t size,
			    size_t count, MFile *handle));
static int		Mgetc _ANSI_ARGS_((MFile *handle));
static int		char64 _ANSI_ARGS_((int c));
static void		mInit _ANSI_ARGS_((unsigned char *string,
			    MFile *handle));


/*
 *----------------------------------------------------------------------
 *
 * FileMatchGIF --
 *
 *	This procedure is invoked by the photo image type to see if
 *	a file contains image data in GIF format.
 *
 * Results:
 *	The return value is 1 if the first characters in file f look
 *	like GIF data, and 0 otherwise.
 *
 * Side effects:
 *	The access position in f may change.
 *
 *----------------------------------------------------------------------
 */

static int
FileMatchGIF(chan, fileName, format, widthPtr, heightPtr, interp)
    Tcl_Channel chan;		/* The image file, open for reading. */
    CONST char *fileName;	/* The name of the image file. */
    Tcl_Obj *format;		/* User-specified format object, or NULL. */
    int *widthPtr, *heightPtr;	/* The dimensions of the image are
				 * returned here if the file is a valid
				 * raw GIF file. */
    Tcl_Interp *interp;		/* not used */
{
	return ReadGIFHeader(chan, widthPtr, heightPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FileReadGIF --
 *
 *	This procedure is called by the photo image type to read
 *	GIF format data from a file and write it into a given
 *	photo image.
 *
 * Results:
 *	A standard TCL completion code.  If TCL_ERROR is returned
 *	then an error message is left in the interp's result.
 *
 * Side effects:
 *	The access position in file f is changed, and new data is
 *	added to the image given by imageHandle.
 *
 *----------------------------------------------------------------------
 */

static int
FileReadGIF(interp, chan, fileName, format, imageHandle, destX, destY,
	width, height, srcX, srcY)
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    Tcl_Channel chan;		/* The image file, open for reading. */
    CONST char *fileName;	/* The name of the image file. */
    Tcl_Obj *format;		/* User-specified format object, or NULL. */
    Tk_PhotoHandle imageHandle;	/* The photo image to write into. */
    int destX, destY;		/* Coordinates of top-left pixel in
				 * photo image to be written to. */
    int width, height;		/* Dimensions of block of photo image to
				 * be written to. */
    int srcX, srcY;		/* Coordinates of top-left pixel to be used
				 * in image being read. */
{
    int fileWidth, fileHeight;
    int nBytes, index = 0, argc = 0, i;
    Tcl_Obj **objv;
    Tk_PhotoImageBlock block;
    unsigned char buf[100];
    unsigned char *trashBuffer = NULL;
    int bitPixel;
    unsigned char colorMap[MAXCOLORMAPSIZE][4];
    int transparent = -1;
    static CONST char *optionStrings[] = {
	"-index",	NULL
    };

    if (format && Tcl_ListObjGetElements(interp, format,
	    &argc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 1; i < argc; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], optionStrings, "option name", 0,
		&nBytes) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i == (argc-1)) {
	    Tcl_AppendResult(interp, "no value given for \"",
		    Tcl_GetStringFromObj(objv[i], NULL),
		    "\" option", (char *) NULL);
	    return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[++i], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (!ReadGIFHeader(chan, &fileWidth, &fileHeight)) {
    	Tcl_AppendResult(interp, "couldn't read GIF header from file \"",
		fileName, "\"", NULL);
	return TCL_ERROR;
    }
    if ((fileWidth <= 0) || (fileHeight <= 0)) {
	Tcl_AppendResult(interp, "GIF image file \"", fileName,
		"\" has dimension(s) <= 0", (char *) NULL);
	return TCL_ERROR;
    }

    if (Fread(buf, 1, 3, chan) != 3) {
	return TCL_OK;
    }
    bitPixel = 2<<(buf[0]&0x07);

    if (BitSet(buf[0], LOCALCOLORMAP)) {	/* Global Colormap */
	if (!ReadColorMap(chan, bitPixel, colorMap)) {
	    Tcl_AppendResult(interp, "error reading color map",
		    (char *) NULL);
	    return TCL_ERROR;
	}
    }

    if ((srcX + width) > fileWidth) {
	width = fileWidth - srcX;
    }
    if ((srcY + height) > fileHeight) {
	height = fileHeight - srcY;
    }
    if ((width <= 0) || (height <= 0)
	    || (srcX >= fileWidth) || (srcY >= fileHeight)) {
	return TCL_OK;
    }

    Tk_PhotoExpand(imageHandle, destX + width, destY + height);

    block.width = width;
    block.height = height;
    block.pixelSize = 4;
    block.pitch = block.pixelSize * block.width;
    block.offset[0] = 0;
    block.offset[1] = 1;
    block.offset[2] = 2;
    block.offset[3] = 3;
    block.pixelPtr = NULL;

    while (1) {
	if (Fread(buf, 1, 1, chan) != 1) {
	    /*
	     * Premature end of image.  We should really notify
	     * the user, but for now just show garbage.
	     */

	    break;
	}

	if (buf[0] == GIF_TERMINATOR) {
	    /*
	     * GIF terminator.
	     */

	    Tcl_AppendResult(interp,"no image data for this index",
		    (char *) NULL);
	    goto error;
	}

	if (buf[0] == GIF_EXTENSION) {
	    /*
	     * This is a GIF extension.
	     */

	    if (Fread(buf, 1, 1, chan) != 1) {
		Tcl_SetResult(interp,
			"error reading extension function code in GIF image",
			TCL_STATIC);
		goto error;
	    }
	    if (DoExtension(chan, buf[0], &transparent) < 0) {
		Tcl_SetResult(interp, "error reading extension in GIF image",
			TCL_STATIC);
		goto error;
	    }
	    continue;
	}

	if (buf[0] != GIF_START) {
	    /*
	     * Not a valid start character; ignore it.
	     */
	    continue;
	}

	if (Fread(buf, 1, 9, chan) != 9) {
	    Tcl_SetResult(interp,
		    "couldn't read left/top/width/height in GIF image",
		    TCL_STATIC);
	    goto error;
	}

	fileWidth = LM_to_uint(buf[4],buf[5]);
	fileHeight = LM_to_uint(buf[6],buf[7]);

	bitPixel = 1<<((buf[8]&0x07)+1);

	if (index--) {
	    /*
	     * This is not the image we want to read: skip it.
	     */
	    if (BitSet(buf[8], LOCALCOLORMAP)) {
		if (!ReadColorMap(chan, bitPixel, colorMap)) {
		    Tcl_AppendResult(interp,
			    "error reading color map", (char *) NULL);
		    goto error;
		}
	    }

	    /*
	     * If we've not yet allocated a trash buffer, do so now.
	     */
	    if (trashBuffer == NULL) {
		nBytes = fileWidth * fileHeight * 3;
		trashBuffer =
		    (unsigned char *) ckalloc((unsigned int) nBytes);
	    }

	    /*
	     * Slurp!  Process the data for this image and stuff it in
	     * a trash buffer.
	     *
	     * Yes, it might be more efficient here to *not* store the
	     * data (we're just going to throw it away later).
	     * However, I elected to implement it this way for good
	     * reasons.  First, I wanted to avoid duplicating the
	     * (fairly complex) LWZ decoder in ReadImage.  Fine, you
	     * say, why didn't you just modify it to allow the use of
	     * a NULL specifier for the output buffer?  I tried that,
	     * but it negatively impacted the performance of what I
	     * think will be the common case: reading the first image
	     * in the file.  Rather than marginally improve the speed
	     * of the less frequent case, I chose to maintain high
	     * performance for the common case.
	     */
	    if (ReadImage(interp, (char *) trashBuffer, chan, fileWidth,
		    fileHeight, colorMap, 0, 0, 0, 0, 0, -1) != TCL_OK) {
		goto error;
	    }
	    continue;
	}

	if (BitSet(buf[8], LOCALCOLORMAP)) {
	    if (!ReadColorMap(chan, bitPixel, colorMap)) {
		    Tcl_AppendResult(interp, "error reading color map", 
			    (char *) NULL);
		    goto error;
	    }
	}

	index = LM_to_uint(buf[0],buf[1]);
	srcX -= index;
	if (srcX<0) {
	    destX -= srcX; width += srcX;
	    srcX = 0;
	}

	if (width > fileWidth) {
	    width = fileWidth;
	}

	index = LM_to_uint(buf[2],buf[3]);
	srcY -= index;
	if (index > srcY) {
	    destY -= srcY; height += srcY;
	    srcY = 0;
	}
	if (height > fileHeight) {
	    height = fileHeight;
	}

	if ((width <= 0) || (height <= 0)) {
	    block.pixelPtr = 0;
	    goto noerror;
	}

	block.width = width;
	block.height = height;
	block.pixelSize = (transparent>=0) ? 4 : 3;
	block.offset[3] = (transparent>=0) ? 3 : 0;
	block.pitch = block.pixelSize * fileWidth;
	nBytes = block.pitch * fileHeight;
	block.pixelPtr = (unsigned char *) ckalloc((unsigned) nBytes);

	if (ReadImage(interp, (char *) block.pixelPtr, chan, fileWidth,
		fileHeight, colorMap, fileWidth, fileHeight, srcX, srcY,
		BitSet(buf[8], INTERLACE), transparent) != TCL_OK) {
	    goto error;
	}
	break;
    }

    Tk_PhotoPutBlock(imageHandle, &block, destX, destY, width, height,
	    TK_PHOTO_COMPOSITE_SET);

    noerror:
    /*
     * If a trash buffer has been allocated, free it now.
     */
    if (trashBuffer != NULL) {
	ckfree((char *)trashBuffer);
    }
    if (block.pixelPtr) {
	ckfree((char *) block.pixelPtr);
    }
    Tcl_AppendResult(interp, tkImgFmtGIF.name, (char *) NULL);
    return TCL_OK;

    error:
    /*
     * If a trash buffer has been allocated, free it now.
     */
    if (trashBuffer != NULL) {
	ckfree((char *)trashBuffer);
    }
    if (block.pixelPtr) {
	ckfree((char *) block.pixelPtr);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * StringMatchGIF --
 *
 *  This procedure is invoked by the photo image type to see if
 *  an object contains image data in GIF format.
 *
 * Results:
 *  The return value is 1 if the first characters in the data are
 *  like GIF data, and 0 otherwise.
 *
 * Side effects:
 *  the size of the image is placed in widthPre and heightPtr.
 *
 *----------------------------------------------------------------------
 */

static int
StringMatchGIF(dataObj, format, widthPtr, heightPtr, interp)
    Tcl_Obj *dataObj;		/* the object containing the image data */
    Tcl_Obj *format;		/* the image format object, or NULL */
    int *widthPtr;		/* where to put the string width */
    int *heightPtr;		/* where to put the string height */
    Tcl_Interp *interp;		/* not used */
{
    unsigned char *data, header[10];
    int got, length;
    MFile handle;

    data = Tcl_GetByteArrayFromObj(dataObj, &length);

    /*
     * Header is a minimum of 10 bytes.
     */
    if (length < 10) {
	return 0;
    }

    /*
     * Check whether the data is Base64 encoded.
     */

    if ((strncmp(GIF87a, (char *) data, 6) != 0) && 
	    (strncmp(GIF89a, (char *) data, 6) != 0)) {
	/*
	 * Try interpreting the data as Base64 encoded
	 */
	mInit((unsigned char *) data, &handle);
	got = Mread(header, 10, 1, &handle);
	if (got != 10
		|| ((strncmp(GIF87a, (char *) header, 6) != 0)
		&& (strncmp(GIF89a, (char *) header, 6) != 0))) {
	    return 0;
	}
    } else {
	memcpy((VOID *) header, (VOID *) data, 10);
    }
    *widthPtr = LM_to_uint(header[6],header[7]);
    *heightPtr = LM_to_uint(header[8],header[9]);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * StringReadGif -- --
 *
 *	This procedure is called by the photo image type to read
 *	GIF format data from an object, optionally base64 encoded, 
 *	and give it to the photo image.
 *
 * Results:
 *	A standard TCL completion code.  If TCL_ERROR is returned
 *	then an error message is left in the interp's result.
 *
 * Side effects:
 *	new data is added to the image given by imageHandle.  This
 *	procedure calls FileReadGif by redefining the operation of
 *	fprintf temporarily.
 *
 *----------------------------------------------------------------------
 */

static int
StringReadGIF(interp, dataObj, format, imageHandle,
	destX, destY, width, height, srcX, srcY)
    Tcl_Interp *interp;		/* interpreter for reporting errors in */
    Tcl_Obj *dataObj;		/* object containing the image */
    Tcl_Obj *format;		/* format object, or NULL */
    Tk_PhotoHandle imageHandle;	/* the image to write this data into */
    int destX, destY;		/* The rectangular region of the  */
    int width, height;		/*   image to copy */
    int srcX, srcY;
{
    int result;
    MFile handle;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_Channel dataSrc;
    char *data;

    /*
     * Check whether the data is Base64 encoded
     */
    data = (char *) Tcl_GetByteArrayFromObj(dataObj, NULL);
    if ((strncmp(GIF87a, data, 6) != 0) && (strncmp(GIF89a, data, 6) != 0)) {
	mInit((unsigned char *)data, &handle);
	tsdPtr->fromData = 1;
	dataSrc = (Tcl_Channel) &handle;
    } else {
	tsdPtr->fromData = 2;
	mInit((unsigned char *)data, &handle);
	dataSrc = (Tcl_Channel) &handle;
    }
    result = FileReadGIF(interp, dataSrc, "inline data",
	    format, imageHandle, destX, destY, width, height, srcX, srcY);
    tsdPtr->fromData = 0;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ReadGIFHeader --
 *
 *	This procedure reads the GIF header from the beginning of a
 *	GIF file and returns the dimensions of the image.
 *
 * Results:
 *	The return value is 1 if file "f" appears to start with
 *	a valid GIF header, 0 otherwise.  If the header is valid,
 *	then *widthPtr and *heightPtr are modified to hold the
 *	dimensions of the image.
 *
 * Side effects:
 *	The access position in f advances.
 *
 *----------------------------------------------------------------------
 */

static int
ReadGIFHeader(chan, widthPtr, heightPtr)
    Tcl_Channel chan;		/* Image file to read the header from */
    int *widthPtr, *heightPtr;	/* The dimensions of the image are
				 * returned here. */
{
    unsigned char buf[7];

    if ((Fread(buf, 1, 6, chan) != 6)
	    || ((strncmp(GIF87a, (char *) buf, 6) != 0)
	    && (strncmp(GIF89a, (char *) buf, 6) != 0))) {
	return 0;
    }

    if (Fread(buf, 1, 4, chan) != 4) {
	return 0;
    }

    *widthPtr = LM_to_uint(buf[0],buf[1]);
    *heightPtr = LM_to_uint(buf[2],buf[3]);
    return 1;
}

/*
 *-----------------------------------------------------------------
 * The code below is copied from the giftoppm program and modified
 * just slightly.
 *-----------------------------------------------------------------
 */

static int
ReadColorMap(chan, number, buffer)
    Tcl_Channel chan;
    int number;
    unsigned char buffer[MAXCOLORMAPSIZE][4];
{
    int i;
    unsigned char rgb[3];

    for (i = 0; i < number; ++i) {
	if (! ReadOK(chan, rgb, sizeof(rgb))) {
	    return 0;
	}

	if (buffer) {
	    buffer[i][CM_RED] = rgb[0] ;
	    buffer[i][CM_GREEN] = rgb[1] ;
	    buffer[i][CM_BLUE] = rgb[2] ;
	    buffer[i][CM_ALPHA] = 255 ;
	}
    }
    return 1;
}



static int
DoExtension(chan, label, transparent)
    Tcl_Channel chan;
    int label;
    int *transparent;
{
    static unsigned char buf[256];
    int count;

    switch (label) {
    case 0x01:		/* Plain Text Extension */
	break;

    case 0xff:		/* Application Extension */
	break;

    case 0xfe:		/* Comment Extension */
	do {
	    count = GetDataBlock(chan, (unsigned char*) buf);
	} while (count > 0);
	return count;

    case 0xf9:		/* Graphic Control Extension */
	count = GetDataBlock(chan, (unsigned char*) buf);
	if (count < 0) {
	    return 1;
	}
	if ((buf[0] & 0x1) != 0) {
	    *transparent = buf[3];
	}

	do {
	    count = GetDataBlock(chan, (unsigned char*) buf);
	} while (count > 0);
	return count;
    }

    do {
	count = GetDataBlock(chan, (unsigned char*) buf);
    } while (count > 0);
    return count;
}

static int
GetDataBlock(chan, buf)
    Tcl_Channel chan;
    unsigned char *buf;
{
    unsigned char count;

    if (! ReadOK(chan, &count,1)) {
	return -1;
    }

    if ((count != 0) && (! ReadOK(chan, buf, count))) {
	return -1;
    }

    return count;
}



/*
 *----------------------------------------------------------------------
 *
 * ReadImage --
 *
 *	Process a GIF image from a given source, with a given height,
 *	width, transparency, etc.
 *
 *	This code is based on the code found in the ImageMagick GIF decoder,
 *	which is (c) 2000 ImageMagick Studio.
 *
 *	Some thoughts on our implementation:
 *	It sure would be nice if ReadImage didn't take 11 parameters!  I think
 *	that if we were smarter, we could avoid doing that.
 *
 *	Possible further optimizations:  we could pull the GetCode function
 *	directly into ReadImage, which would improve our speed.
 *
 * Results:
 *	Processes a GIF image and loads the pixel data into a memory array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ReadImage(interp, imagePtr, chan, len, rows, cmap,
	width, height, srcX, srcY, interlace, transparent)
    Tcl_Interp *interp;
    char *imagePtr;
    Tcl_Channel chan;
    int len, rows;
    unsigned char cmap[MAXCOLORMAPSIZE][4];
    int width, height;
    int srcX, srcY;
    int interlace;
    int transparent;
{
    unsigned char initialCodeSize;
    int v;
    int xpos = 0, ypos = 0, pass = 0, i;
    register char *pixelPtr;
    CONST static int interlaceStep[] = { 8, 8, 4, 2 };
    CONST static int interlaceStart[] = { 0, 4, 2, 1 };
    unsigned short prefix[(1 << MAX_LWZ_BITS)];
    unsigned char  append[(1 << MAX_LWZ_BITS)];
    unsigned char  stack[(1 << MAX_LWZ_BITS)*2];
    register unsigned char *top;
    int codeSize, clearCode, inCode, endCode, oldCode, maxCode;
    int code, firstCode;

    /*
     *  Initialize the decoder
     */
    if (! ReadOK(chan, &initialCodeSize, 1))  {
	Tcl_AppendResult(interp, "error reading GIF image: ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    if (transparent != -1) {
	cmap[transparent][CM_RED] = 0;
	cmap[transparent][CM_GREEN] = 0;
	cmap[transparent][CM_BLUE] = 0;
	cmap[transparent][CM_ALPHA] = 0;
    }

    pixelPtr = imagePtr;

    /*
     * Initialize the decoder.
     *
     * Set values for "special" numbers:
     * clear code	reset the decoder
     * end code		stop decoding
     * code size	size of the next code to retrieve
     * max code		next available table position
     */
    clearCode = 1 << (int) initialCodeSize;
    endCode = clearCode + 1;
    codeSize = (int) initialCodeSize + 1;
    maxCode = clearCode + 2;
    oldCode = -1;
    firstCode = -1;
    
    memset((void *)prefix, 0, (1 << MAX_LWZ_BITS) * sizeof(short));
    memset((void *)append, 0, (1 << MAX_LWZ_BITS) * sizeof(char));
    for (i = 0; i < clearCode; i++) {
	append[i] = i;
    }
    top = stack;

    GetCode(chan, 0, 1);

    /*
     * Read until we finish the image
     */
    for (i = 0, ypos = 0; i < rows; i++) {
	for (xpos = 0; xpos < len; ) {

	    if (top == stack) {
		/*
		 * Bummer -- our stack is empty.  Now we have to work!
		 */
		code = GetCode(chan, codeSize, 0);
		if (code < 0) {
		    return TCL_OK;
		}

		if (code > maxCode || code == endCode) {
		    /*
		     * If we're doing things right, we should never
		     * receive a code that is greater than our current
		     * maximum code.  If we do, bail, because our decoder
		     * does not yet have that code set up.
		     *
		     * If the code is the magic endCode value, quit.
		     */
		    return TCL_OK;
		}

		if (code == clearCode) {
		    /*
		     * Reset the decoder.
		     */
		    codeSize = initialCodeSize + 1;
		    maxCode = clearCode + 2;
		    oldCode = -1;
		    continue;
		}
		
		if (oldCode == -1) {
		    /*
		     * Last pass reset the decoder, so the first code we
		     * see must be a singleton.  Seed the stack with it,
		     * and set up the old/first code pointers for
		     * insertion into the string table.  We can't just
		     * roll this into the clearCode test above, because
		     * at that point we have not yet read the next code.
		     */
		    *top++ = append[code];
		    oldCode = code;
		    firstCode = code;
		    continue;
		}
		
		inCode = code;

		if (code == maxCode) {
		    /*
		     * maxCode is always one bigger than our highest assigned
		     * code.  If the code we see is equal to maxCode, then
		     * we are about to add a new string to the table. ???
		     */
		    *top++ = firstCode;
		    code = oldCode;
		}

		while (code > clearCode) {
		    /*
		     * Populate the stack by tracing the string in the
		     * string table from its tail to its head
		     */
		    *top++ = append[code];
		    code = prefix[code];
		}
		firstCode = append[code];

		/*
		 * If there's no more room in our string table, quit.
		 * Otherwise, add a new string to the table
		 */
		if (maxCode >= (1 << MAX_LWZ_BITS)) {
		    return TCL_OK;
		}

		/*
		 * Push the head of the string onto the stack.
		 */
		*top++ = firstCode;

		/*
		 * Add a new string to the string table
		 */
		prefix[maxCode] = oldCode;
		append[maxCode] = firstCode;
		maxCode++;

		/*
		 * maxCode tells us the maximum code value we can accept.
		 * If we see that we need more bits to represent it than
		 * we are requesting from the unpacker, we need to increase
		 * the number we ask for.
		 */
		if ((maxCode >= (1 << codeSize))
			&& (maxCode < (1<<MAX_LWZ_BITS))) {
		    codeSize++;
		}
		oldCode = inCode;
	    }

	    /*
	     * Pop the next color index off the stack.
	     */
	    v = *(--top);
	    if (v < 0) {
		return TCL_OK;
	    }

	    /* 
	     * If pixelPtr is null, we're skipping this image (presumably
	     * there are more in the file and we will be called to read 
	     * one of them later)
	     */
	    *pixelPtr++ = cmap[v][CM_RED];
	    *pixelPtr++ = cmap[v][CM_GREEN];
	    *pixelPtr++ = cmap[v][CM_BLUE];
	    if (transparent >= 0) {
		*pixelPtr++ = cmap[v][CM_ALPHA];
	    }
	    xpos++;

	}

	/*
	 * If interlacing, the next ypos is not just +1
	 */
	if (interlace) {
	    ypos += interlaceStep[pass];
	    while (ypos >= height) {
		pass++;
		if (pass > 3) {
		    return TCL_OK;
		}
		ypos = interlaceStart[pass];
	    }
	} else {
	    ypos++;
	}
	pixelPtr = imagePtr + (ypos) * len * ((transparent>=0)?4:3);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * GetCode --
 *
 *	Extract the next compression code from the file.  In GIF's, the
 *	compression codes are between 3 and 12 bits long and are then
 *	packed into 8 bit bytes, left to right, for example:
 *		bbbaaaaa
 *		dcccccbb
 *		eeeedddd
 *		...
 *	We use a byte buffer read from the file and a sliding window
 *	to unpack the bytes.  Thanks to ImageMagick for the sliding window
 *	idea.
 *	args:  chan	    the channel to read from
 *	       code_size    size of the code to extract
 *	       flag	    boolean indicating whether the extractor
 *			    should be reset or not
 *
 * Results:
 *	code		    the next compression code
 *
 * Side effects:
 *	May consume more input from chan.
 *
 *----------------------------------------------------------------------
 */

static int
GetCode(chan, code_size, flag)
    Tcl_Channel chan;
    int code_size;
    int flag;
{
    static unsigned char buf[280];
    static int bytes = 0, done;
    static unsigned char *c;

    static unsigned int window;
    static int bitsInWindow = 0;
    int ret;
    
    if (flag) {
	/*
	 * Initialize the decoder.
	 */
	bitsInWindow = 0;
	bytes = 0;
	window = 0;
	done = 0;
	c = NULL;
	return 0;
    }

    while (bitsInWindow < code_size) {
	/*
	 * Not enough bits in our window to cover the request.
	 */
	if (done) {
	    return -1;
	}
	if (bytes == 0) {
	    /*
	     * Not enough bytes in our buffer to add to the window.
	     */
	    bytes = GetDataBlock(chan, buf);
	    c = buf;
	    if (bytes <= 0) {
		done = 1;
		break;
	    }
	}
	/*
	 * Tack another byte onto the window, see if that's enough.
	 */
	window += (*c) << bitsInWindow;
	c++;
	bitsInWindow += 8;
	bytes--;
    }


    /*
     * The next code will always be the last code_size bits of the window.
     */
    ret = window & ((1 << code_size) - 1);
    
    /*
     * Shift data in the window to put the next code at the end.
     */
    window >>= code_size;
    bitsInWindow -= code_size;
    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * Minit -- --
 *
 *  This procedure initializes a base64 decoder handle
 *
 * Results:
 *  none
 *
 * Side effects:
 *  the base64 handle is initialized
 *
 *----------------------------------------------------------------------
 */

static void
mInit(string, handle)
   unsigned char *string;	/* string containing initial mmencoded data */
   MFile *handle;		/* mmdecode "file" handle */
{
   handle->data = string;
   handle->state = 0;
   handle->c = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Mread --
 *
 *	This procedure is invoked by the GIF file reader as a 
 *	temporary replacement for "fread", to get GIF data out
 *	of a string (using Mgetc).
 *
 * Results:
 *	The return value is the number of characters "read"
 *
 * Side effects:
 *	The base64 handle will change state.
 *
 *----------------------------------------------------------------------
 */

static int
Mread(dst, chunkSize, numChunks, handle)  
   unsigned char *dst;	/* where to put the result */
   size_t chunkSize;	/* size of each transfer */
   size_t numChunks;	/* number of chunks */
   MFile *handle;	/* mmdecode "file" handle */
{
   register int i, c;
   int count = chunkSize * numChunks;

   for(i=0; i<count && (c=Mgetc(handle)) != GIF_DONE; i++) {
	*dst++ = c;
   }
   return i;
}

/*
 * get the next decoded character from an mmencode handle
 * This causes at least 1 character to be "read" from the encoded string
 */

/*
 *----------------------------------------------------------------------
 *
 * Mgetc --
 *
 *  This procedure decodes and returns the next byte from a base64
 *  encoded string.
 *
 * Results:
 *  The next byte (or GIF_DONE) is returned.
 *
 * Side effects:
 *  The base64 handle will change state.
 *
 *----------------------------------------------------------------------
 */

static int
Mgetc(handle)
   MFile *handle;		/* Handle containing decoder data and state */
{
    int c;
    int result = 0;		/* Initialization needed only to prevent
				 * gcc compiler warning. */

    if (handle->state == GIF_DONE) {
	return GIF_DONE;
    }

    do {
	c = char64(*handle->data);
	handle->data++;
    } while (c == GIF_SPACE);

    if (c>GIF_SPECIAL) {
	handle->state = GIF_DONE;
	return handle->c;
    }

    switch (handle->state++) {
    case 0:
	handle->c = c<<2;
	result = Mgetc(handle);
	break;
    case 1:
	result = handle->c | (c>>4);
	handle->c = (c&0xF)<<4;
	break;
    case 2:
	result = handle->c | (c>>2);
	handle->c = (c&0x3) << 6;
	break;
    case 3:
	result = handle->c | c;
	handle->state = 0;
	break;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * char64 --
 *
 *	This procedure converts a base64 ascii character into its binary
 *	equivalent.  This code is a slightly modified version of the
 *	char64 proc in N. Borenstein's metamail decoder.
 *
 * Results:
 *	The binary value, or an error code.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

static int
char64(c)
int c;
{
    switch(c) {
    case 'A': return 0;  case 'B': return 1;  case 'C': return 2;
    case 'D': return 3;  case 'E': return 4;  case 'F': return 5;
    case 'G': return 6;  case 'H': return 7;  case 'I': return 8;
    case 'J': return 9;  case 'K': return 10; case 'L': return 11;
    case 'M': return 12; case 'N': return 13; case 'O': return 14;
    case 'P': return 15; case 'Q': return 16; case 'R': return 17;
    case 'S': return 18; case 'T': return 19; case 'U': return 20;
    case 'V': return 21; case 'W': return 22; case 'X': return 23;
    case 'Y': return 24; case 'Z': return 25; case 'a': return 26;
    case 'b': return 27; case 'c': return 28; case 'd': return 29;
    case 'e': return 30; case 'f': return 31; case 'g': return 32;
    case 'h': return 33; case 'i': return 34; case 'j': return 35;
    case 'k': return 36; case 'l': return 37; case 'm': return 38;
    case 'n': return 39; case 'o': return 40; case 'p': return 41;
    case 'q': return 42; case 'r': return 43; case 's': return 44;
    case 't': return 45; case 'u': return 46; case 'v': return 47;
    case 'w': return 48; case 'x': return 49; case 'y': return 50;
    case 'z': return 51; case '0': return 52; case '1': return 53;
    case '2': return 54; case '3': return 55; case '4': return 56;
    case '5': return 57; case '6': return 58; case '7': return 59;
    case '8': return 60; case '9': return 61; case '+': return 62;
    case '/': return 63;

    case ' ': case '\t': case '\n': case '\r': case '\f':
	return GIF_SPACE;
    case '=':
	return GIF_PAD;
    case '\0':
	return GIF_DONE;
    default:
	return GIF_BAD;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fread --
 *
 *  This procedure calls either fread or Mread to read data
 *  from a file or a base64 encoded string.
 *
 * Results: - same as fread
 *
 *----------------------------------------------------------------------
 */

static int
Fread(dst, hunk, count, chan)
    unsigned char *dst;		/* where to put the result */
    size_t hunk,count;		/* how many */
    Tcl_Channel chan;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    MFile *handle;

    switch (tsdPtr->fromData) {
    case 1:
	return Mread(dst, hunk, count, (MFile *) chan);
    case 2:
	handle = (MFile *) chan;
	memcpy((VOID *)dst, (VOID *) handle->data, (size_t) (hunk * count));
	handle->data += hunk * count;
	return (int)(hunk * count);
    default:
	return Tcl_Read(chan, (char *) dst, (int) (hunk * count));
    }
}


/*
 * ChanWriteGIF - writes a image in GIF format.
 *-------------------------------------------------------------------------
 * Author:			Lolo
 *				Engeneering Projects Area 
 *				Department of Mining 
 *				University of Oviedo
 * e-mail			zz11425958@zeus.etsimo.uniovi.es
 *				lolo@pcsig22.etsimo.uniovi.es
 * Date:			Fri September 20 1996
 *
 * Modified for transparency handling (gif89a) and miGIF compression
 * by Jan Nijtmans <j.nijtmans@chello.nl>
 *
 *----------------------------------------------------------------------
 * FileWriteGIF-
 *
 *    This procedure is called by the photo image type to write
 *    GIF format data from a photo image into a given file 
 *
 * Results:
 *	A standard TCL completion code.  If TCL_ERROR is returned
 *	then an error message is left in interp->result.
 *
 *----------------------------------------------------------------------
 */

 /*
  *  Types, defines and variables needed to write and compress a GIF.
  */

typedef int (* ifunptr) _ANSI_ARGS_((void));	

#define LSB(a)			((unsigned char) (((short)(a)) & 0x00FF))
#define MSB(a)			((unsigned char) (((short)(a)) >> 8))

#define GIFBITS 12
#define HSIZE  5003		/* 80% occupancy */

static int ssize;
static int csize;
static int rsize;
static unsigned char *pixelo;
static int pixelSize;
static int pixelPitch;
static int greenOffset;
static int blueOffset;
static int alphaOffset;
static int num;
static unsigned char mapa[MAXCOLORMAPSIZE][3];

/*
 *	Definition of new functions to write GIFs
 */

static int color _ANSI_ARGS_((int red,int green, int blue,
		unsigned char mapa[MAXCOLORMAPSIZE][3]));
static void compress _ANSI_ARGS_((int init_bits, Tcl_Channel handle,
		ifunptr readValue));
static int nuevo _ANSI_ARGS_((int red, int green ,int blue,
		unsigned char mapa[MAXCOLORMAPSIZE][3]));
static void savemap _ANSI_ARGS_((Tk_PhotoImageBlock *blockPtr,
		unsigned char mapa[MAXCOLORMAPSIZE][3]));
static int ReadValue _ANSI_ARGS_((void));

static int
FileWriteGIF(interp, filename, format, blockPtr)
    Tcl_Interp *interp;		/* Interpreter to use for reporting errors. */
    CONST char	*filename;
    Tcl_Obj	*format;
    Tk_PhotoImageBlock *blockPtr;
{
    Tcl_Channel chan = NULL;
    int result;

    chan = Tcl_OpenFileChannel(interp, (char *) filename, "w", 0644);
    if (!chan) {
	return TCL_ERROR;
    }
    if (Tcl_SetChannelOption(interp, chan, "-translation", "binary") != TCL_OK) {
	Tcl_Close(NULL, chan);
	return TCL_ERROR;
    }

    result = CommonWriteGIF(interp, chan, format, blockPtr);
    if (Tcl_Close(interp, chan) == TCL_ERROR) {
	return TCL_ERROR;
    }
    return result;
}

#define Mputc(c,handle) Tcl_Write(handle,(char *) &c,1)

static int
CommonWriteGIF(interp, handle, format, blockPtr)
    Tcl_Interp *interp;
    Tcl_Channel handle;
    Tcl_Obj *format;
    Tk_PhotoImageBlock *blockPtr;
{
    int  resolution;

    long  width,height,x;
    unsigned char c;
    unsigned int top,left;

    top = 0;
    left = 0;

    pixelSize = blockPtr->pixelSize;
    greenOffset = blockPtr->offset[1]-blockPtr->offset[0];
    blueOffset = blockPtr->offset[2]-blockPtr->offset[0];
    alphaOffset = blockPtr->offset[0];
    if (alphaOffset < blockPtr->offset[2]) {
	alphaOffset = blockPtr->offset[2];
    }
    if (++alphaOffset < pixelSize) {
	alphaOffset -= blockPtr->offset[0];
    } else {
	alphaOffset = 0;
    }

    Tcl_Write(handle, (char *) (alphaOffset ? GIF89a : GIF87a), 6);

    for (x=0 ; x<MAXCOLORMAPSIZE ; x++) {
	mapa[x][CM_RED] = 255;
	mapa[x][CM_GREEN] = 255;
	mapa[x][CM_BLUE] = 255;
    }


    width = blockPtr->width;
    height = blockPtr->height;
    pixelo = blockPtr->pixelPtr + blockPtr->offset[0];
    pixelPitch = blockPtr->pitch;
    savemap(blockPtr,mapa);
    if (num >= MAXCOLORMAPSIZE) {
	Tcl_AppendResult(interp, "too many colors", (char *) NULL);
	return TCL_ERROR;
    }
    if (num<2) {
	num = 2;
    }
    c = LSB(width);
    Mputc(c,handle);
    c = MSB(width);
    Mputc(c,handle);
    c = LSB(height);
    Mputc(c,handle);
    c = MSB(height);
    Mputc(c,handle);

    resolution = 0;
    while (num >> resolution) {
	resolution++;
    }
    c = 111 + resolution * 17;
    Mputc(c,handle);

    num = 1 << resolution;

    /*
     * background color
     */

    c = 0;
    Mputc(c,handle);

    /*
     * zero for future expansion.
     */

    Mputc(c,handle);

    for (x=0 ; x<num ; x++) {
	c = mapa[x][CM_RED];
	Mputc(c,handle);
	c = mapa[x][CM_GREEN];
	Mputc(c,handle);
	c = mapa[x][CM_BLUE];
	Mputc(c,handle);
    }

    /*
     * Write out extension for transparent colour index, if necessary.
     */

    if (alphaOffset) {
	c = GIF_EXTENSION;
	Mputc(c, handle);
	Tcl_Write(handle, "\371\4\1\0\0\0", 7);
    }

    c = GIF_START;
    Mputc(c,handle);
    c = LSB(top);
    Mputc(c,handle);
    c = MSB(top);
    Mputc(c,handle);
    c = LSB(left);
    Mputc(c,handle);
    c = MSB(left);
    Mputc(c,handle);

    c = LSB(width);
    Mputc(c,handle);
    c = MSB(width);
    Mputc(c,handle);

    c = LSB(height);
    Mputc(c,handle);
    c = MSB(height);
    Mputc(c,handle);

    c = 0;
    Mputc(c,handle);
    c = resolution;
    Mputc(c,handle);

    ssize = rsize = blockPtr->width;
    csize = blockPtr->height;
    compress(resolution+1, handle, ReadValue);

    c = 0; 
    Mputc(c,handle);
    c = GIF_TERMINATOR;
    Mputc(c,handle);

    return TCL_OK;	
}

static int
color(red, green, blue, mapa)
    int red;
    int green;
    int blue;
    unsigned char mapa[MAXCOLORMAPSIZE][3];
{
    int x;
    for (x=(alphaOffset != 0) ; x<=MAXCOLORMAPSIZE ; x++) {
	if ((mapa[x][CM_RED] == red) && (mapa[x][CM_GREEN] == green) &&
		(mapa[x][CM_BLUE] == blue)) {
	    return x;
	}
    }
    return -1;
}


static int
nuevo(red, green, blue, mapa)
    int red,green,blue;
    unsigned char mapa[MAXCOLORMAPSIZE][3];
{
    int x = (alphaOffset != 0);
    for (; x<=num ; x++) {
	if ((mapa[x][CM_RED] == red) && (mapa[x][CM_GREEN] == green) &&
		(mapa[x][CM_BLUE] == blue)) {
	    return 0;
	}
    }
    return 1;
}

static void
savemap(blockPtr,mapa)
    Tk_PhotoImageBlock *blockPtr;
    unsigned char mapa[MAXCOLORMAPSIZE][3];
{
    unsigned char *colores;
    int x,y;
    unsigned char red,green,blue;

    if (alphaOffset) {
	num = 0;
	mapa[0][CM_RED] = 0xd9;
	mapa[0][CM_GREEN] = 0xd9;
	mapa[0][CM_BLUE] = 0xd9;
    } else {
	num = -1;
    }

    for(y=0 ; y<blockPtr->height ; y++) {
	colores = blockPtr->pixelPtr + blockPtr->offset[0]
		+ y * blockPtr->pitch;
	for(x=0 ; x<blockPtr->width ; x++) {
	    if (!alphaOffset || (colores[alphaOffset] != 0)) {
		red = colores[0];
		green = colores[greenOffset];
		blue = colores[blueOffset];
		if (nuevo(red,green,blue,mapa)) {
		    num++;
		    if (num >= MAXCOLORMAPSIZE) {
			return;
		    }
		    mapa[num][CM_RED] = red;
		    mapa[num][CM_GREEN] = green;
		    mapa[num][CM_BLUE] = blue;
		}
	    }
	    colores += pixelSize;
	}
    }
    return;
}

static int
ReadValue()
{
    unsigned int col;

    if (csize == 0) {
	return EOF;
    }
    if (alphaOffset && (pixelo[alphaOffset] == 0)) {
	col = 0;
    } else {
	col = color(pixelo[0], pixelo[greenOffset], pixelo[blueOffset], mapa);
    }
    pixelo += pixelSize;
    if (--ssize <= 0) {
	ssize = rsize;
	csize--;
	pixelo += pixelPitch - (rsize * pixelSize);
    }

    return col;
}



/*
 *-----------------------------------------------------------------------
 *
 * miGIF Compression - mouse and ivo's GIF-compatible compression
 *
 *		-run length encoding compression routines-
 *
 * Copyright (C) 1998 Hutchison Avenue Software Corporation
 *		 http://www.hasc.com
 *		 info@hasc.com
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation.  This software is
 * provided "AS IS." The Hutchison Avenue Software Corporation
 * disclaims all warranties, either express or implied, including but
 * not limited to implied warranties of merchantability and fitness
 * for a particular purpose, with respect to this code and
 * accompanying documentation.
 * 
 * The miGIF compression routines do not, strictly speaking, generate
 * files conforming to the GIF spec, since the image data is not
 * LZW-compressed (this is the point: in order to avoid transgression
 * of the Unisys patent on the LZW algorithm.)  However, miGIF
 * generates data streams that any reasonably sane LZW decompresser
 * will decompress to what we want.
 *
 * miGIF compression uses run length encoding. It compresses
 * horizontal runs of pixels of the same color. This type of
 * compression gives good results on images with many runs, for
 * example images with lines, text and solid shapes on a solid-colored
 * background. It gives little or no compression on images with few
 * runs, for example digital or scanned photos.
 *
 *				 der Mouse
 *			mouse@rodents.montreal.qc.ca
 *	      7D C8 61 52 5D E7 2D 39  4E F1 31 3E E8 B3 27 4B
 *
 *			       ivo@hasc.com
 *
 * The Graphics Interchange Format(c) is the Copyright property of
 * CompuServe Incorporated.  GIF(sm) is a Service Mark property of
 * CompuServe Incorporated.
 *
 *-----------------------------------------------------------------------
 */

static int rl_pixel;
static int rl_basecode;
static int rl_count;
static int rl_table_pixel;
static int rl_table_max;
static int just_cleared;
static int out_bits;
static int out_bits_init;
static int out_count;
static int out_bump;
static int out_bump_init;
static int out_clear;
static int out_clear_init;
static int max_ocodes;
static int code_clear;
static int code_eof;
static unsigned int obuf;
static int obits;
static Tcl_Channel ofile;
static unsigned char oblock[256];
static int oblen;

/*
 * Used only when debugging GIF compression code
 */
/* #define MIGIF_DEBUGGING_ENVARS */

#ifdef MIGIF_DEBUGGING_ENVARS

static int verbose_set = 0;
static int verbose;
#define MIGIF_VERBOSE (verbose_set?verbose:set_verbose())
#define DEBUGMSG(printf_args) if (MIGIF_VERBOSE) { printf printf_args; }

static int
set_verbose(void)
{
    verbose = !!getenv("MIGIF_VERBOSE");
    verbose_set = 1;
    return verbose;
}

static CONST char *
binformat(v, nbits)
    unsigned int v;
    int nbits;
{
    static char bufs[8][64];
    static int bhand = 0;
    unsigned int bit;
    int bno;
    char *bp;

    bhand--;
    if (bhand < 0) {
	bhand = (sizeof(bufs) / sizeof(bufs[0])) - 1;
    }
    bp = &bufs[bhand][0];
    for (bno=nbits-1,bit=((unsigned int)1)<<bno ; bno>=0 ; bno--,bit>>=1) {
	*bp++ = (v & bit) ? '1' : '0';
	if (((bno&3) == 0) && (bno != 0)) {
	    *bp++ = '.';
	}
    }
    *bp = '\0';
    return &bufs[bhand][0];
}

#else

#define MIGIF_VERBOSE 0
#define DEBUGMSG(printf_args) /* do nothing */

#endif

static void
write_block()
{
    int i;
    unsigned char c;

    if (MIGIF_VERBOSE) {
	printf("write_block %d:", oblen);
	for (i=0 ; i<oblen ; i++) {
	    printf(" %02x", oblock[i]);
	}
	printf("\n");
    }
    c = oblen;
    Tcl_Write(ofile, (char *) &c, 1);
    Tcl_Write(ofile, (char *) &oblock[0], oblen);
    oblen = 0;
}

static void
block_out(c)
    unsigned char c;
{
    DEBUGMSG(("block_out %s\n", binformat(c, 8)));
    oblock[oblen++] = c;
    if (oblen >= 255) {
	write_block();
    }
}

static void
block_flush()
{
    DEBUGMSG(("block_flush\n"));
    if (oblen > 0) {
	write_block();
    }
}

static void
output(val)
    int val;
{
    DEBUGMSG(("output %s [%s %d %d]\n", binformat(val, out_bits),
	    binformat(obuf, obits), obits, out_bits));
    obuf |= val << obits;
    obits += out_bits;
    while (obits >= 8) {
	block_out(obuf&0xff);
	obuf >>= 8;
	obits -= 8;
    }
    DEBUGMSG(("output leaving [%s %d]\n", binformat(obuf, obits), obits));
}

static void
output_flush()
{
    DEBUGMSG(("output_flush\n"));
    if (obits > 0) {
	block_out(obuf);
    }
    block_flush();
}

static void
did_clear()
{
    DEBUGMSG(("did_clear\n"));
    out_bits = out_bits_init;
    out_bump = out_bump_init;
    out_clear = out_clear_init;
    out_count = 0;
    rl_table_max = 0;
    just_cleared = 1;
}

static void
output_plain(c)
    int c;
{
    DEBUGMSG(("output_plain %s\n", binformat(c, out_bits)));
    just_cleared = 0;
    output(c);
    out_count++;
    if (out_count >= out_bump) {
	out_bits++;
	out_bump += 1 << (out_bits - 1);
    }
    if (out_count >= out_clear) {
	output(code_clear);
	did_clear();
    }
}

static unsigned int
isqrt(x)
    unsigned int x;
{
    unsigned int r;
    unsigned int v;

    if (x < 2) {
	return x;
    }
    for (v=x,r=1 ; v ; v>>=2,r<<=1);
    while (1) {
	v = ((x / r) + r) / 2;
	if (v==r || v==r+1) {
	    return r;
	}
	r = v;
    }
}

static unsigned int
compute_triangle_count(count, nrepcodes)
    unsigned int count;
    unsigned int nrepcodes;
{
    unsigned int perrep;
    unsigned int cost;

    cost = 0;
    perrep = (nrepcodes * (nrepcodes+1)) / 2;
    while (count >= perrep) {
	cost += nrepcodes;
	count -= perrep;
    }
    if (count > 0) {
	unsigned int n;
	n = isqrt(count);
	while (n*(n+1) >= 2*count) {
	    n--;
	}
	while (n*(n+1) < 2*count) {
	    n++;
	}
	cost += n;
    }
    return cost;
}

static void
max_out_clear()
{
    out_clear = max_ocodes;
}

static void
reset_out_clear()
{
    out_clear = out_clear_init;
    if (out_count >= out_clear) {
	output(code_clear);
	did_clear();
    }
}

static void
rl_flush_fromclear(count)
    int count;
{
    int n;

    DEBUGMSG(("rl_flush_fromclear %d\n", count));
    max_out_clear();
    rl_table_pixel = rl_pixel;
    n = 1;
    while (count > 0) {
	if (n == 1) {
	    rl_table_max = 1;
	    output_plain(rl_pixel);
	    count--;
	} else if (count >= n) {
	    rl_table_max = n;
	    output_plain(rl_basecode+n-2);
	    count -= n;
	} else if (count == 1) {
	    rl_table_max++;
	    output_plain(rl_pixel);
	    count = 0;
	} else {
	    rl_table_max++;
	    output_plain(rl_basecode+count-2);
	    count = 0;
	}
	if (out_count == 0) {
	    n = 1;
	} else {
	    n++;
	}
    }
    reset_out_clear();
    DEBUGMSG(("rl_flush_fromclear leaving table_max=%d\n", rl_table_max));
}

static void
rl_flush_clearorrep(count)
    int count;
{
    int withclr;

    DEBUGMSG(("rl_flush_clearorrep %d\n", count));
    withclr = 1 + compute_triangle_count(count, max_ocodes);
    if (withclr < count) {
	output(code_clear);
	did_clear();
	rl_flush_fromclear(count);
    } else {
	for (; count>0 ; count--) {
	    output_plain(rl_pixel);
	}
    }
}

static void
rl_flush_withtable(count)
    int count;
{
    int repmax;
    int repleft;
    int leftover;

    DEBUGMSG(("rl_flush_withtable %d\n", count));
    repmax = count / rl_table_max;
    leftover = count % rl_table_max;
    repleft = (leftover ? 1 : 0);
    if (out_count+repmax+repleft > max_ocodes) {
	repmax = max_ocodes - out_count;
	leftover = count - (repmax * rl_table_max);
	repleft = 1 + compute_triangle_count(leftover, max_ocodes);
    }
    DEBUGMSG(("rl_flush_withtable repmax=%d leftover=%d repleft=%d\n",
	    repmax, leftover, repleft));
    if (1+(int)compute_triangle_count(count, max_ocodes) < repmax+repleft) {
	output(code_clear);
	did_clear();
	rl_flush_fromclear(count);
	return;
    }
    max_out_clear();
    for (; repmax>0 ; repmax--) {
	output_plain(rl_basecode + rl_table_max - 2);
    }
    if (leftover) {
	if (just_cleared) {
	    rl_flush_fromclear(leftover);
	} else if (leftover == 1) {
	    output_plain(rl_pixel);
	} else {
	    output_plain(rl_basecode + leftover - 2);
	}
    }
    reset_out_clear();
}

static void
rl_flush()
{
    DEBUGMSG(("rl_flush [ %d %d\n", rl_count, rl_pixel));
    if (rl_count == 1) {
	output_plain(rl_pixel);
	rl_count = 0;
	DEBUGMSG(("rl_flush ]\n"));
	return;
    }
    if (just_cleared) {
	rl_flush_fromclear(rl_count);
    } else if ((rl_table_max < 2) || (rl_table_pixel != rl_pixel)) {
	rl_flush_clearorrep(rl_count);
    } else {
	rl_flush_withtable(rl_count);
    }
    DEBUGMSG(("rl_flush ]\n"));
    rl_count = 0;
}


static void
compress(init_bits, handle, readValue)
    int init_bits;
    Tcl_Channel handle;
    ifunptr readValue;
{
    int c;

    ofile = handle;
    obuf = 0;
    obits = 0;
    oblen = 0;
    code_clear = 1 << (init_bits - 1);
    code_eof = code_clear + 1;
    rl_basecode = code_eof + 1;
    out_bump_init = (1 << (init_bits - 1)) - 1;
    /*
     * For images with a lot of runs, making out_clear_init larger
     * will give better compression.
     */
    out_clear_init = (init_bits <= 3) ? 9 : (out_bump_init-1);
#ifdef MIGIF_DEBUGGING_ENVARS
    {
	const char *ocienv;
	ocienv = getenv("MIGIF_OUT_CLEAR_INIT");
	if (ocienv) {
	    out_clear_init = atoi(ocienv);
	    DEBUGMSG(("[overriding out_clear_init to %d]\n", out_clear_init));
	}
    }
#endif
    out_bits_init = init_bits;
    max_ocodes = (1 << GIFBITS) - ((1 << (out_bits_init - 1)) + 3);
    did_clear();
    output(code_clear);
    rl_count = 0;
    while (1) {
	c = readValue();
	if ((rl_count > 0) && (c != rl_pixel)) {
	    rl_flush();
	}
	if (c == EOF) {
	    break;
	}
	if (rl_pixel == c) {
	    rl_count++;
	} else {
	    rl_pixel = c;
	    rl_count = 1;
	}
    }
    output(code_eof);
    output_flush();
}

/*
 *-----------------------------------------------------------------------
 *
 * End of miGIF section  - See copyright notice at start of section.
 *
 *-----------------------------------------------------------------------
 */
