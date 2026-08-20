#include "builddefines.h"
#include "vobject.h"
#include "DEBUG.H"

// This function will check the video object at SrcX and SrcY for the lack of transparency
// will return true if data found, else false
BOOLEAN CheckVideoObjectScreenCoordinateInData( HVOBJECT hSrcVObject, UINT16 usIndex, INT32 iTestX, INT32 iTestY )
{
	UINT32 uiOffset;
	UINT32 usHeight, usWidth;
	UINT8	*SrcPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	BOOLEAN	fDataFound = FALSE;
	INT32	iTestPos, iStartPos;

	// Assertions
	Assert( hSrcVObject != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;


	// Calculate test position we are looking for!
	// Calculate from 0, 0 at top left!
	iTestPos	= ( ( usHeight - iTestY ) * usWidth ) + iTestX;
	iStartPos	= 0;
	LineSkip	= usWidth;

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;

	__asm {

		mov		esi, SrcPtr
		mov		edi, iStartPos
		xor		eax, eax
		xor		ebx, ebx
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

//BlitNonTransLoop:

		clc
		rcr		cl, 1
		jnc		BlitNTL2

		inc		esi

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1


BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		add		esi, 2

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1


BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

		xor		ebx, ebx

BlitNTL4:

		add		esi, 4

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1

		// Check
		cmp		edi, iTestPos
		je		BlitFound
		add		edi, 1

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
//		shl		ecx, 1
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

// Here check if we have passed!
		cmp		edi, iTestPos
		jge		BlitDone

		dec		usHeight
		jz		BlitDone
//		add		edi, LineSkip
		jmp		BlitDispatch


BlitFound:

		mov		fDataFound, 1

BlitDone:
	}

	return(fDataFound);

}
