// The 8x8 pattern "pixelate" / "hatch" rectangle blitters, moved verbatim out
// of vobject_blitters.cpp so they can be built and fuzzed in isolation (see
// test/pixelate_fuzz.cpp) without dragging the whole video layer in. These
// functions touch nothing from that file beyond the ClippingRect global, so
// only the few headers they actually use are included here rather than the
// heavy vobject_blitters.h.

#include <stdlib.h>			// __min / __max
#include "types.h"			// SGPRect and the integer typedefs
#include "DEBUG.H"			// Assert
#include "WCheck.h"			// CHECKF

extern SGPRect ClippingRect;	// defined in vobject_blitters.cpp

/**********************************************************************************************
	Blt16BPPBufferPixelateRectWithColor

		Given an 8x8 pattern and a color, pixelates an area by repeatedly "applying the color" to pixels whereever there
		is a non-zero value in the pattern.

		KM:	Added Nov. 23, 1998
		This is all the code that I moved from Blt16BPPBufferPixelateRect().
		This function now takes a color field (which previously was
		always black.	The 3rd assembler line in this function:

				mov		ax, usColor				// color of pixel

		used to be:

				xor	eax, eax					// color of pixel (black or 0)

	This was the only internal modification I made other than adding the usColor argument.

*********************************************************************************************/
BOOLEAN Blt16BPPBufferPixelateRectWithColor(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area, UINT8 Pattern[8][8], UINT16 usColor )
{
	INT32	width, height;
	UINT32 LineSkip;
	UINT16 *DestPtr;
	INT32	iLeft, iTop, iRight, iBottom;

	// Assertions
	Assert( pBuffer != NULL );
	Assert( Pattern != NULL );

	iLeft=__max(ClippingRect.iLeft, area->iLeft);
	iTop=__max(ClippingRect.iTop, area->iTop);
	iRight=__min(ClippingRect.iRight-1, area->iRight);
	iBottom=__min(ClippingRect.iBottom-1, area->iBottom);

	DestPtr=(pBuffer+(iTop*(uiDestPitchBYTES/2))+iLeft);
	width=iRight-iLeft+1;
	height=iBottom-iTop+1;
	LineSkip=(uiDestPitchBYTES-(width*2));

	CHECKF(width >=1);
	CHECKF(height >=1);

	__asm {
		mov		esi, Pattern				// Pointer to pixel pattern
		mov		edi, DestPtr				// Pointer to top left of rect area
		mov		ax, usColor				// color of pixel
		xor		ebx, ebx						// pattern column index
		xor		edx, edx						// pattern row index


BlitNewLine:
		mov		ecx, width

BlitLine:
		cmp	byte ptr [esi+ebx], 0		// Pattern is UINT8[8][8]
		je	BlitLine2

		mov		[edi], ax

BlitLine2:
		add		edi, 2
		inc		ebx
		and		ebx, 07H
		or		ebx, edx
		dec		ecx
		jnz		BlitLine

		add		edi, LineSkip
		xor		ebx, ebx
		add		edx, 08H
		and		edx, 38H
		dec		height
		jnz		BlitNewLine
	}

	return(TRUE);
}

//KM:	Modified Nov. 23, 1998
//Original prototype (this function) didn't have a color field.	I've added the color field to
//Blt16BPPBufferPixelateRectWithColor(), moved the previous implementation of this function there, and added
//the modification to allow a specific color.
BOOLEAN Blt16BPPBufferPixelateRect(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area, UINT8 Pattern[8][8] )
{
	return Blt16BPPBufferPixelateRectWithColor( pBuffer, uiDestPitchBYTES, area, Pattern, 0 );
}

/**********************************************************************************************
	Blt16BPPBufferHatchRect

		A wrapper for Blt16BPPBufferPixelateRect(), it automatically sends a hatch pattern to it
		of the specified color

*********************************************************************************************/
BOOLEAN Blt16BPPBufferHatchRectWithColor(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area, UINT16 usColor )
{
	UINT8 Pattern[8][8] =
	{
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1
	};
	return Blt16BPPBufferPixelateRectWithColor( pBuffer, uiDestPitchBYTES, area, Pattern, usColor );
}

//Uses black hatch color
BOOLEAN Blt16BPPBufferHatchRect(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area )
{
	UINT8 Pattern[8][8] =
	{
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1,
		1,0,1,0,1,0,1,0,
		0,1,0,1,0,1,0,1
	};
	return Blt16BPPBufferPixelateRectWithColor( pBuffer, uiDestPitchBYTES, area, Pattern, 0 );
}

BOOLEAN Blt16BPPBufferLooseHatchRectWithColor(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area, UINT16 usColor )
{
	UINT8 Pattern[8][8] =
	{
		1,0,0,0,1,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,1,0,0,0,1,0,
		0,0,0,0,0,0,0,0,
		1,0,0,0,1,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,1,0,0,0,1,0,
		0,0,0,0,0,0,0,0,
	};
	return Blt16BPPBufferPixelateRectWithColor( pBuffer, uiDestPitchBYTES, area, Pattern, usColor );
}

BOOLEAN Blt16BPPBufferLooseHatchRect(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area )
{
	UINT8 Pattern[8][8] =
	{
		1,0,0,0,1,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,1,0,0,0,1,0,
		0,0,0,0,0,0,0,0,
		1,0,0,0,1,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,1,0,0,0,1,0,
		0,0,0,0,0,0,0,0,
	};
	return Blt16BPPBufferPixelateRectWithColor( pBuffer, uiDestPitchBYTES, area, Pattern, 0 );
}
