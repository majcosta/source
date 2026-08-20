#include "builddefines.h"
#include "types.h"
#include <windows.h>
#include "renderworld.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "DEBUG.H"
#include "WCheck.h"
#include "shading.h"


#define	Z_STRIP_DELTA_Y  ( Z_SUBLAYERS * 10 )

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncClip

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. As it is blitting, it checks the Z value of the ZBuffer, and if the
	pixel's Z level is below that of the current pixel, it is written on, and the Z value is
	updated to the current value,	for any non-transparent pixels. The Z-buffer is 16 bit, and
	must be the same dimensions (including Pitch) as the destination.

**********************************************************************************************/
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion)
{
	UINT16 *p16BPPPalette;
	UINT32 uiOffset;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
  ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip=__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip=__min(max(ClipX2, (iTempX+(INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip=__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip=__min(__max(ClipY2, (iTempY+(INT32)usHeight)) - ClipY2, (INT32)usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return(TRUE);

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	p16BPPPalette = hSrcVObject->pShadeCurrent;
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	if(hSrcVObject->ppZStripInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo=hSrcVObject->ppZStripInfo[usIndex];
	if(pZInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel=(UINT16)((INT16)usZValue+((INT16)pZInfo->bInitialZChange*Z_STRIP_DELTA_Y));
	// set to odd number of pixels for first column

	if(LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols=(LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols=20-(usZStartCols%20);
	}
	else if(LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols=(UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols=20;

	usZColsToGo=usZStartCols;

	pZArray=pZInfo->pbZChange;

	if(LeftSkip >= pZInfo->ubFirstZStripWidth)
	{
		// Index into array after doing left clipping
		usZStartIndex=1 + ((LeftSkip-pZInfo->ubFirstZStripWidth)/20);

		//calculates the Z-value after left-side clipping
		if(usZStartIndex)
		{
			for(usCount=0; usCount < usZStartIndex; usCount++)
			{
				switch(pZArray[usCount])
				{
					case -1:	usZStartLevel-=Z_STRIP_DELTA_Y;
										break;
					case 0:		//no change
										break;
					case 1:		usZStartLevel+=Z_STRIP_DELTA_Y;
										break;
				}
			}
		}
	}
	else
		usZStartIndex=0;

	usZLevel=usZStartLevel;
	usZIndex=usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


// Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop


// Start of line loop

// Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
		mov		eax, LeftSkip					// after we have skipped enough left-side pixels
		mov		LSCount, eax					// LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup					// check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2								// if equal, skip whole, and start blit with new run
		jb		LSSkip1								// if less, skip whole thing

		add		esi, LSCount					// skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1							// *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx							// skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx							// skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup					// if equal, skip whole, and start blit with new run
		jb		LSTrans1							// if less, skip whole thing

		sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent				// *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx					// skip whole run, continue skipping
		jmp		LeftSkipLoop

//-------------------------------------------------
// setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0							// Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

//--------------------------------
// blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]								// check z-level of pixel
		cmp		ax, usZLevel
		jae		BlitNTL2

		mov		ax, usZLevel						// update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]								// copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitNTL5								// dir = 0 no change
		js		BlitNTL4								// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount									// decrement pixel length to blit
		jz		RightSkipLoop						// done blitting the visible line

		dec		ecx
		jnz		BlitNTL1								// continue current run

		jmp		BlitDispatch						// done current run, go for another


//----------------------------
// skipping transparent pixels

BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2									// move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitTrans5							// dir = 0 no change
		js		BlitTrans4							// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount									// decrement the pixels to blit
		jz		RightSkipLoop						// done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

//---------------------------------------------
// Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

// reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
	}

	return(TRUE);
}


/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncClipSaveZBurnsThrough

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. As it is blitting, it checks the Z value of the ZBuffer, and if the
	pixel's Z level is below that of the current pixel, it is written on, and the Z value is
	updated to the current value,	for any non-transparent pixels. The Z-buffer is 16 bit, and
	must be the same dimensions (including Pitch) as the destination.

**********************************************************************************************/
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, INT16 usZStripIndex )
{
	UINT16 *p16BPPPalette;
	UINT32 uiOffset;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
  ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip=__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip=__min(max(ClipX2, (iTempX+(INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip=__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip=__min(__max(ClipY2, (iTempY+(INT32)usHeight)) - ClipY2, (INT32)usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return(TRUE);

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	p16BPPPalette = hSrcVObject->pShadeCurrent;
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	if(hSrcVObject->ppZStripInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo=hSrcVObject->ppZStripInfo[ usZStripIndex ];
	if(pZInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel=(UINT16)((INT16)usZValue+((INT16)pZInfo->bInitialZChange*Z_STRIP_DELTA_Y));
	// set to odd number of pixels for first column

	if(LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols=(LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols=20-(usZStartCols%20);
	}
	else if(LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols=(UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols=20;

	usZColsToGo=usZStartCols;

	pZArray=pZInfo->pbZChange;

	if(LeftSkip >= pZInfo->ubFirstZStripWidth)
	{
		// Index into array after doing left clipping
		usZStartIndex=1 + ((LeftSkip-pZInfo->ubFirstZStripWidth)/20);

		//calculates the Z-value after left-side clipping
		if(usZStartIndex)
		{
			for(usCount=0; usCount < usZStartIndex; usCount++)
			{
				switch(pZArray[usCount])
				{
					case -1:	usZStartLevel-=Z_STRIP_DELTA_Y;
										break;
					case 0:		//no change
										break;
					case 1:		usZStartLevel+=Z_STRIP_DELTA_Y;
										break;
				}
			}
		}
	}
	else
		usZStartIndex=0;

	usZLevel=usZStartLevel;
	usZIndex=usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


// Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop


// Start of line loop

// Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
		mov		eax, LeftSkip					// after we have skipped enough left-side pixels
		mov		LSCount, eax					// LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup					// check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2								// if equal, skip whole, and start blit with new run
		jb		LSSkip1								// if less, skip whole thing

		add		esi, LSCount					// skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1							// *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx							// skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx							// skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup					// if equal, skip whole, and start blit with new run
		jb		LSTrans1							// if less, skip whole thing

		sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent				// *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx					// skip whole run, continue skipping
		jmp		LeftSkipLoop

//-------------------------------------------------
// setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0							// Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

//--------------------------------
// blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]								// check z-level of pixel
		cmp		ax, usZLevel
		ja		BlitNTL2

		mov		ax, usZLevel						// update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]								// copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitNTL5								// dir = 0 no change
		js		BlitNTL4								// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount									// decrement pixel length to blit
		jz		RightSkipLoop						// done blitting the visible line

		dec		ecx
		jnz		BlitNTL1								// continue current run

		jmp		BlitDispatch						// done current run, go for another


//----------------------------
// skipping transparent pixels

BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2									// move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitTrans5							// dir = 0 no change
		js		BlitTrans4							// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount									// decrement the pixels to blit
		jz		RightSkipLoop						// done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

//---------------------------------------------
// Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

// reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
	}

	return(TRUE);
}


/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncObscureClip

	Blits an image into the destination buffer, using an ETRLE brush as a source, and a 16-bit
	buffer as a destination. As it is blitting, it checks the Z value of the ZBuffer, and if the
	pixel's Z level is below that of the current pixel, it is written on, and the Z value is
	updated to the current value,	for any non-transparent pixels. The Z-buffer is 16 bit, and
	must be the same dimensions (including Pitch) as the destination.

	//ATE: This blitter makes the values that are =< z value pixellate rather than not
	// render at all

**********************************************************************************************/
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncObscureClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion)
{
	UINT16 *p16BPPPalette;
	UINT32 uiOffset, uiLineFlag;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
  ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;


	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip=__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip=__min(max(ClipX2, (iTempX+(INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip=__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip=__min(__max(ClipY2, (iTempY+(INT32)usHeight)) - ClipY2, (INT32)usHeight);

	uiLineFlag=(iTempY&1);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return(TRUE);

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	p16BPPPalette = hSrcVObject->pShadeCurrent;
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	if(hSrcVObject->ppZStripInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo=hSrcVObject->ppZStripInfo[usIndex];
	if(pZInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel=(UINT16)((INT16)usZValue+((INT16)pZInfo->bInitialZChange*Z_STRIP_DELTA_Y));
	// set to odd number of pixels for first column

	if(LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols=(LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols=20-(usZStartCols%20);
	}
	else if(LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols=(UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols=20;

	usZColsToGo=usZStartCols;

	pZArray=pZInfo->pbZChange;

	if(LeftSkip >= pZInfo->ubFirstZStripWidth)
	{
		// Index into array after doing left clipping
		usZStartIndex=1 + ((LeftSkip-pZInfo->ubFirstZStripWidth)/20);

		//calculates the Z-value after left-side clipping
		if(usZStartIndex)
		{
			for(usCount=0; usCount < usZStartIndex; usCount++)
			{
				switch(pZArray[usCount])
				{
					case -1:	usZStartLevel-=Z_STRIP_DELTA_Y;
										break;
					case 0:		//no change
										break;
					case 1:		usZStartLevel+=Z_STRIP_DELTA_Y;
										break;
				}
			}
		}
	}
	else
		usZStartIndex=0;

	usZLevel=usZStartLevel;
	usZIndex=usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


// Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:

		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop


// Start of line loop

// Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
		mov		eax, LeftSkip					// after we have skipped enough left-side pixels
		mov		LSCount, eax					// LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup					// check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2								// if equal, skip whole, and start blit with new run
		jb		LSSkip1								// if less, skip whole thing

		add		esi, LSCount					// skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1							// *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx							// skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx							// skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup					// if equal, skip whole, and start blit with new run
		jb		LSTrans1							// if less, skip whole thing

		sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent				// *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx					// skip whole run, continue skipping
		jmp		LeftSkipLoop

//-------------------------------------------------
// setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0							// Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

//--------------------------------
// blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]								// check z-level of pixel
		cmp		ax, usZLevel
		jae		BlitPixellate1
		jmp   BlitPixel1

BlitPixellate1:

		// OK, DO PIXELLATE SCHEME HERE!
		test	uiLineFlag, 1
		jz		BlitSkip1

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitPixel1

BlitSkip1:
		test	edi, 2
		jnz		BlitNTL2

BlitPixel1:

		mov		ax, usZLevel						// update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]								// copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitNTL5								// dir = 0 no change
		js		BlitNTL4								// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount									// decrement pixel length to blit
		jz		RightSkipLoop						// done blitting the visible line

		dec		ecx
		jnz		BlitNTL1								// continue current run

		jmp		BlitDispatch						// done current run, go for another


//----------------------------
// skipping transparent pixels

BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2									// move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitTrans5							// dir = 0 no change
		js		BlitTrans4							// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount									// decrement the pixels to blit
		jz		RightSkipLoop						// done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

//---------------------------------------------
// Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

// reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
	}

	return(TRUE);
}


// Blitter Specs
// 1 ) 8 to 16 bpp
// 2 ) strip z-blitter
// 3 ) clipped
// 4 ) trans shadow - if value is 254, makes a shadow
//
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, INT16 sZIndex, UINT16 *p16BPPPalette, BOOLEAN fIgnoreShadows) 
{
	UINT32 uiOffset, uiLineFlag;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
  ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip=__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip=__min(max(ClipX2, (iTempX+(INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip=__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip=__min(__max(ClipY2, (iTempY+(INT32)usHeight)) - ClipY2, (INT32)usHeight);

	uiLineFlag=(iTempY&1);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return(TRUE);

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	if(hSrcVObject->ppZStripInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo=hSrcVObject->ppZStripInfo[sZIndex];
	if(pZInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel=(UINT16)((INT16)usZValue+((INT16)pZInfo->bInitialZChange*Z_SUBLAYERS*10));

	if(LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols=(LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols=20-(usZStartCols%20);
	}
	else if(LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols=(UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols=20;

	// set to odd number of pixels for first column
	usZColsToGo=usZStartCols;

	pZArray=pZInfo->pbZChange;

	if(LeftSkip >= usZColsToGo)
	{
		// Index into array after doing left clipping
		usZStartIndex=1 + ((LeftSkip-pZInfo->ubFirstZStripWidth)/20);

		//calculates the Z-value after left-side clipping
		if(usZStartIndex)
		{
			for(usCount=0; usCount < usZStartIndex; usCount++)
			{
				switch(pZArray[usCount])
				{
					case -1:	usZStartLevel-=Z_SUBLAYERS;
										break;
					case 0:		//no change
										break;
					case 1:		usZStartLevel+=Z_SUBLAYERS;
										break;
				}
			}
		}
	}
	else
		usZStartIndex=0;

	usZLevel=usZStartLevel;
	usZIndex=usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


// Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:

		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop


// Start of line loop

// Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
		mov		eax, LeftSkip					// after we have skipped enough left-side pixels
		mov		LSCount, eax					// LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup					// check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2								// if equal, skip whole, and start blit with new run
		jb		LSSkip1								// if less, skip whole thing

		add		esi, LSCount					// skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1							// *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx							// skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx							// skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup					// if equal, skip whole, and start blit with new run
		jb		LSTrans1							// if less, skip whole thing

		sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax

		mov		Unblitted, 0
		jmp		BlitTransparent				// *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx					// skip whole run, continue skipping
		jmp		LeftSkipLoop

//-------------------------------------------------
// setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0							// Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

//--------------------------------
// blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]								// check z-level of pixel
		cmp		ax, usZLevel
		//jae		BlitPixellate1 // Original comparison
		ja		BlitPixellate1 // Due to lobot layers having the same z-level, the comparison would pixelate merc's layers against each other. Now we pixelate ONLY if the comparison is above.
		jmp		BlitPixel1

BlitPixellate1:

		// OK, DO PIXELLATE SCHEME HERE!
		test	uiLineFlag, 1
		jz		BlitSkip1

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitPixel1

BlitSkip1:
		test	edi, 2
		jnz		BlitNTL2

BlitPixel1:

		mov		ax, usZLevel						// update z-level of pixel
		mov		[ebx], ax

		// Check for shadow...
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL66

		mov		al, fIgnoreShadows
		cmp		al, 0
		jne		BlitNTL2

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL66:

		mov		ax, [edx+eax*2]					// Copy pixel
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitNTL5								// dir = 0 no change
		js		BlitNTL4								// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_SUBLAYERS

BlitNTL5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount									// decrement pixel length to blit
		jz		RightSkipLoop						// done blitting the visible line

		dec		ecx
		jnz		BlitNTL1								// continue current run

		jmp		BlitDispatch						// done current run, go for another


//----------------------------
// skipping transparent pixels

BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2									// move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitTrans5							// dir = 0 no change
		js		BlitTrans4							// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_SUBLAYERS

BlitTrans5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount									// decrement the pixels to blit
		jz		RightSkipLoop						// done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

//---------------------------------------------
// Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

// reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
	}

	return(TRUE);
}


// Blitter Specs
// 1 ) 8 to 16 bpp
// 2 ) strip z-blitter
// 3 ) clipped
// 4 ) trans shadow - if value is 254, makes a shadow
//
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClipAlpha(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, HVOBJECT hAlphaVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, INT16 sZIndex, UINT16 *p16BPPPalette, BOOLEAN fIgnoreShadows)
{
	UINT32 uiOffset, uiLineFlag;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr, *AlphaPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert(hSrcVObject != NULL);
	Assert(hAlphaVObject != NULL);
	Assert(pBuffer != NULL);

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
	usHeight = (UINT32)pTrav->usHeight;
	usWidth = (UINT32)pTrav->usWidth;
	uiOffset = pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if (clipregion == NULL)
	{
		ClipX1 = ClippingRect.iLeft;
		ClipY1 = ClippingRect.iTop;
		ClipX2 = ClippingRect.iRight;
		ClipY2 = ClippingRect.iBottom;
	}
	else
	{
		ClipX1 = clipregion->iLeft;
		ClipY1 = clipregion->iTop;
		ClipX2 = clipregion->iRight;
		ClipY2 = clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

	uiLineFlag = (iTempY & 1);

	// calculate the remaining rows and columns to blit
	BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
	BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

	// check if whole thing is clipped
	if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
		return(TRUE);

	SrcPtr = (UINT8 *)hSrcVObject->pPixData + uiOffset;
	AlphaPtr = (UINT8 *)hAlphaVObject->pPixData + (hAlphaVObject->pETRLEObject[usIndex]).uiDataOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
	LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

	if (hSrcVObject->ppZStripInfo == NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo = hSrcVObject->ppZStripInfo[sZIndex];
	if (pZInfo == NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel = (UINT16)((INT16)usZValue + ((INT16)pZInfo->bInitialZChange*Z_SUBLAYERS * 10));

	if (LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols = (LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols = 20 - (usZStartCols % 20);
	}
	else if (LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols = (UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols = 20;

	// set to odd number of pixels for first column
	usZColsToGo = usZStartCols;

	pZArray = pZInfo->pbZChange;

	if (LeftSkip >= usZColsToGo)
	{
		// Index into array after doing left clipping
		usZStartIndex = 1 + ((LeftSkip - pZInfo->ubFirstZStripWidth) / 20);

		//calculates the Z-value after left-side clipping
		if (usZStartIndex)
		{
			for (usCount = 0; usCount < usZStartIndex; usCount++)
			{
				switch (pZArray[usCount])
				{
				case -1:	usZStartLevel -= Z_SUBLAYERS;
					break;
				case 0:		//no change
					break;
				case 1:		usZStartLevel += Z_SUBLAYERS;
					break;
				}
			}
		}
	}
	else
		usZStartIndex = 0;

	usZLevel = usZStartLevel;
	usZIndex = usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


		// Skips the number of lines clipped at the top
		TopSkipLoop :

		mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		TopSkipLoop
			jz		TSEndLine

			add		esi, ecx

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			jmp		TopSkipLoop

			TSEndLine :

		xor		uiLineFlag, 1
			dec		TopSkip
			jnz		TopSkipLoop


			// Start of line loop

			// Skips the pixels hanging outside the left-side boundry
		LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
			mov		eax, LeftSkip					// after we have skipped enough left-side pixels
			mov		LSCount, eax					// LSCount counts how many pixels skipped so far
			or eax, eax
			jz		BlitLineSetup					// check for nothing to skip

			LeftSkipLoop :

		mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		LSTrans

			cmp		ecx, LSCount
			je		LSSkip2								// if equal, skip whole, and start blit with new run
			jb		LSSkip1								// if less, skip whole thing

			add		esi, LSCount					// skip partial run, jump into normal loop for rest

			push	esi
			mov		esi, AlphaPtr
			add		esi, LSCount
			mov		AlphaPtr, esi
			pop		esi

			sub		ecx, LSCount
			mov		eax, BlitLength
			mov		LSCount, eax
			mov		Unblitted, 0
			jmp		BlitNTL1							// *** jumps into non-transparent blit loop

			LSSkip2 :
		add		esi, ecx							// skip whole run, and start blit with new run

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			jmp		BlitLineSetup


			LSSkip1 :
		add		esi, ecx							// skip whole run, continue skipping

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			sub		LSCount, ecx
			jmp		LeftSkipLoop


			LSTrans :
		and		ecx, 07fH
			cmp		ecx, LSCount
			je		BlitLineSetup					// if equal, skip whole, and start blit with new run
			jb		LSTrans1							// if less, skip whole thing

			sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
			mov		eax, BlitLength
			mov		LSCount, eax

			mov		Unblitted, 0
			jmp		BlitTransparent				// *** jumps into transparent blit loop


			LSTrans1 :
		sub		LSCount, ecx					// skip whole run, continue skipping
			jmp		LeftSkipLoop

			//-------------------------------------------------
			// setup for beginning of line

			BlitLineSetup :
		mov		eax, BlitLength
			mov		LSCount, eax
			mov		Unblitted, 0

			BlitDispatch :

			cmp		LSCount, 0							// Check to see if we're done blitting
			je		RightSkipLoop

			mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		BlitTransparent
			jz		RSLoop2

			//--------------------------------
			// blitting non-transparent pixels

			and		ecx, 07fH

			BlitNTL1 :
		mov		ax, [ebx]								// check z-level of pixel
			cmp		ax, usZLevel
			jae		BlitPixellate1
			jmp		BlitPixel1

			BlitPixellate1 :

		// OK, DO PIXELLATE SCHEME HERE!
		test	uiLineFlag, 1
			jz		BlitSkip1

			test	edi, 2
			jz		BlitNTL2
			jmp		BlitPixel1

			BlitSkip1 :
		test	edi, 2
			jnz		BlitNTL2

			BlitPixel1 :

		mov		ax, usZLevel						// update z-level of pixel
			mov[ebx], ax

			// Check for shadow...
			xor		eax, eax
			mov		al, [esi]
			cmp		al, 254
			jne		BlitNTL66

			mov		al, fIgnoreShadows
			cmp		al, 0
			jne		BlitNTL2

			mov		ax, [edi]
			mov		ax, ShadeTable[eax * 2]
			mov[edi], ax
			jmp		BlitNTL2

			BlitNTL66 :

		mov		ax, [edx + eax * 2]					// Copy pixel

			push	edx
			push	ecx
			push	ebx
			push	esi
			mov		esi, AlphaPtr
			xor		ebx, ebx
			mov		bl, [esi]
			pop		esi
			push	ebx
			push[edi]
			push	eax
			call    blendWithAlpha
			add     esp, 12
			pop		ebx
			pop		ecx
			pop		edx

			mov[edi], ax

			BlitNTL2 :
		inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			add		edi, 2
			add		ebx, 2

			dec		usZColsToGo
			jnz		BlitNTL6

			// update the z-level according to the z-table

			push	edx
			mov		edx, pZArray						// get pointer to array
			xor		eax, eax
			mov		ax, usZIndex						// pick up the current array index
			add		edx, eax
			inc		eax											// increment it
			mov		usZIndex, ax						// store incremented value

			mov		al, [edx]								// get direction instruction
			mov		dx, usZLevel						// get current z-level

			or al, al
			jz		BlitNTL5								// dir = 0 no change
			js		BlitNTL4								// dir < 0 z-level down
																		// dir > 0 z-level up (default)
			add		dx, Z_SUBLAYERS
			jmp		BlitNTL5

			BlitNTL4 :
		sub		dx, Z_SUBLAYERS

			BlitNTL5 :
		mov		usZLevel, dx						// store the now-modified z-level
			mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
			pop		edx

			BlitNTL6 :
		dec		LSCount									// decrement pixel length to blit
			jz		RightSkipLoop						// done blitting the visible line

			dec		ecx
			jnz		BlitNTL1								// continue current run

			jmp		BlitDispatch						// done current run, go for another


	//----------------------------
	// skipping transparent pixels

		BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

			BlitTrans2 :

		add		edi, 2									// move up the destination pointer
			add		ebx, 2

			dec		usZColsToGo
			jnz		BlitTrans1

			// update the z-level according to the z-table

			push	edx
			mov		edx, pZArray						// get pointer to array
			xor		eax, eax
			mov		ax, usZIndex						// pick up the current array index
			add		edx, eax
			inc		eax											// increment it
			mov		usZIndex, ax						// store incremented value

			mov		al, [edx]								// get direction instruction
			mov		dx, usZLevel						// get current z-level

			or al, al
			jz		BlitTrans5							// dir = 0 no change
			js		BlitTrans4							// dir < 0 z-level down
																		// dir > 0 z-level up (default)
			add		dx, Z_SUBLAYERS
			jmp		BlitTrans5

			BlitTrans4 :
		sub		dx, Z_SUBLAYERS

			BlitTrans5 :
		mov		usZLevel, dx						// store the now-modified z-level
			mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
			pop		edx

			BlitTrans1 :

		dec		LSCount									// decrement the pixels to blit
			jz		RightSkipLoop						// done the line

			dec		ecx
			jnz		BlitTrans2

			jmp		BlitDispatch

			//---------------------------------------------
			// Scans the ETRLE until it finds an EOL marker

			RightSkipLoop :


	RSLoop1:
		mov		al, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or al, al
			jnz		RSLoop1

			RSLoop2 :

		xor		uiLineFlag, 1
			dec		BlitHeight
			jz		BlitDone
			add		edi, LineSkip
			add		ebx, LineSkip

			// reset all the z-level stuff for a new line

			mov		ax, usZStartLevel
			mov		usZLevel, ax
			mov		ax, usZStartIndex
			mov		usZIndex, ax
			mov		ax, usZStartCols
			mov		usZColsToGo, ax


			jmp		LeftSkipSetup


			BlitDone :
	}

	return(TRUE);
}



// Blitter Specs
// 1 ) 8 to 16 bpp
// 2 ) strip z-blitter
// 3 ) clipped
// 4 ) trans shadow - if value is 254, makes a shadow
//
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, INT16 sZIndex, UINT16 *p16BPPPalette, BOOLEAN fIgnoreShadows) 
{
	UINT32 uiOffset;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
  ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if(clipregion==NULL)
	{
		ClipX1=ClippingRect.iLeft;
		ClipY1=ClippingRect.iTop;
		ClipX2=ClippingRect.iRight;
		ClipY2=ClippingRect.iBottom;
	}
	else
	{
		ClipX1=clipregion->iLeft;
		ClipY1=clipregion->iTop;
		ClipX2=clipregion->iRight;
		ClipY2=clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip=__min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip=__min(max(ClipX2, (iTempX+(INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip=__min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip=__min(__max(ClipY2, (iTempY+(INT32)usHeight)) - ClipY2, (INT32)usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength=((INT32)usWidth-LeftSkip-RightSkip);
	BlitHeight=((INT32)usHeight-TopSkip-BottomSkip);

	// check if whole thing is clipped
	if((LeftSkip >=(INT32)usWidth) || (RightSkip >=(INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if((TopSkip >=(INT32)usHeight) || (BottomSkip >=(INT32)usHeight))
		return(TRUE);

	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY+TopSkip)) + ((iTempX+LeftSkip)*2);
	LineSkip=(uiDestPitchBYTES-(BlitLength*2));

	if(hSrcVObject->ppZStripInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo=hSrcVObject->ppZStripInfo[sZIndex];
	if(pZInfo==NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel=(UINT16)((INT16)usZValue+((INT16)pZInfo->bInitialZChange*Z_SUBLAYERS*10));

	if(LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols=(LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols=20-(usZStartCols%20);
	}
	else if(LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols=(UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols=20;

	// set to odd number of pixels for first column
	usZColsToGo=usZStartCols;

	pZArray=pZInfo->pbZChange;

	if(LeftSkip >= usZColsToGo)
	{
		// Index into array after doing left clipping
		usZStartIndex=1 + ((LeftSkip-pZInfo->ubFirstZStripWidth)/20);

		//calculates the Z-value after left-side clipping
		if(usZStartIndex)
		{
			for(usCount=0; usCount < usZStartIndex; usCount++)
			{
				switch(pZArray[usCount])
				{
					case -1:	usZStartLevel-=Z_SUBLAYERS;
										break;
					case 0:		//no change
										break;
					case 1:		usZStartLevel+=Z_SUBLAYERS;
										break;
				}
			}
		}
	}
	else
		usZStartIndex=0;

	usZLevel=usZStartLevel;
	usZIndex=usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


// Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop


// Start of line loop

// Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
		mov		eax, LeftSkip					// after we have skipped enough left-side pixels
		mov		LSCount, eax					// LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup					// check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2								// if equal, skip whole, and start blit with new run
		jb		LSSkip1								// if less, skip whole thing

		add		esi, LSCount					// skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1							// *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx							// skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx							// skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup					// if equal, skip whole, and start blit with new run
		jb		LSTrans1							// if less, skip whole thing

		sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax

		mov		Unblitted, 0
		jmp		BlitTransparent				// *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx					// skip whole run, continue skipping
		jmp		LeftSkipLoop

//-------------------------------------------------
// setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0							// Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

//--------------------------------
// blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]								// check z-level of pixel
		cmp		ax, usZLevel
		ja		BlitNTL2

		mov		ax, usZLevel						// update z-level of pixel
		mov		[ebx], ax

		// Check for shadow...
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL66

		mov		al, fIgnoreShadows
		cmp		al, 0
		jne		BlitNTL2

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL66:

		mov		ax, [edx+eax*2]					// Copy pixel
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitNTL5								// dir = 0 no change
		js		BlitNTL4								// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_SUBLAYERS

BlitNTL5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount									// decrement pixel length to blit
		jz		RightSkipLoop						// done blitting the visible line

		dec		ecx
		jnz		BlitNTL1								// continue current run

		jmp		BlitDispatch						// done current run, go for another


//----------------------------
// skipping transparent pixels

BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2									// move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

// update the z-level according to the z-table

		push	edx
		mov		edx, pZArray						// get pointer to array
		xor		eax, eax
		mov		ax, usZIndex						// pick up the current array index
		add		edx, eax
		inc		eax											// increment it
		mov		usZIndex, ax						// store incremented value

		mov		al, [edx]								// get direction instruction
		mov		dx, usZLevel						// get current z-level

		or		al, al
		jz		BlitTrans5							// dir = 0 no change
		js		BlitTrans4							// dir < 0 z-level down
																	// dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_SUBLAYERS

BlitTrans5:
		mov		usZLevel, dx						// store the now-modified z-level
		mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount									// decrement the pixels to blit
		jz		RightSkipLoop						// done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

//---------------------------------------------
// Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

// reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
	}

	return(TRUE);
}


// Blitter Specs
// 1 ) 8 to 16 bpp
// 2 ) strip z-blitter
// 3 ) clipped
// 4 ) trans shadow - if value is 254, makes a shadow
//
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, HVOBJECT hAlphaVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, INT16 sZIndex, UINT16 *p16BPPPalette, BOOLEAN fIgnoreShadows)
{
	UINT32 uiOffset;
	UINT32 usHeight, usWidth, Unblitted;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr, *AlphaPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	INT32	 iTempX, iTempY, LeftSkip, RightSkip, TopSkip, BottomSkip, BlitLength, BlitHeight, LSCount;
	INT32  ClipX1, ClipY1, ClipX2, ClipY2;
	UINT16 usZLevel, usZStartLevel, usZColsToGo, usZStartIndex, usCount, usZIndex, usZStartCols;
	INT8 *pZArray;
	ZStripInfo *pZInfo;

	// Assertions
	Assert(hSrcVObject != NULL);
	Assert(hAlphaVObject != NULL);
	Assert(pBuffer != NULL);

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[usIndex]);
	usHeight = (UINT32)pTrav->usHeight;
	usWidth = (UINT32)pTrav->usWidth;
	uiOffset = pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	if (clipregion == NULL)
	{
		ClipX1 = ClippingRect.iLeft;
		ClipY1 = ClippingRect.iTop;
		ClipX2 = ClippingRect.iRight;
		ClipY2 = ClippingRect.iBottom;
	}
	else
	{
		ClipX1 = clipregion->iLeft;
		ClipY1 = clipregion->iTop;
		ClipX2 = clipregion->iRight;
		ClipY2 = clipregion->iBottom;
	}

	// Calculate rows hanging off each side of the screen
	LeftSkip = __min(ClipX1 - min(ClipX1, iTempX), (INT32)usWidth);
	RightSkip = __min(max(ClipX2, (iTempX + (INT32)usWidth)) - ClipX2, (INT32)usWidth);
	TopSkip = __min(ClipY1 - __min(ClipY1, iTempY), (INT32)usHeight);
	BottomSkip = __min(__max(ClipY2, (iTempY + (INT32)usHeight)) - ClipY2, (INT32)usHeight);

	// calculate the remaining rows and columns to blit
	BlitLength = ((INT32)usWidth - LeftSkip - RightSkip);
	BlitHeight = ((INT32)usHeight - TopSkip - BottomSkip);

	// check if whole thing is clipped
	if ((LeftSkip >= (INT32)usWidth) || (RightSkip >= (INT32)usWidth))
		return(TRUE);

	// check if whole thing is clipped
	if ((TopSkip >= (INT32)usHeight) || (BottomSkip >= (INT32)usHeight))
		return(TRUE);

	SrcPtr = (UINT8 *)hSrcVObject->pPixData + uiOffset;
	AlphaPtr = (UINT8 *)hAlphaVObject->pPixData + (hAlphaVObject->pETRLEObject[usIndex]).uiDataOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*(iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*(iTempY + TopSkip)) + ((iTempX + LeftSkip) * 2);
	LineSkip = (uiDestPitchBYTES - (BlitLength * 2));

	if (hSrcVObject->ppZStripInfo == NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}
	// setup for the z-column blitting stuff
	pZInfo = hSrcVObject->ppZStripInfo[sZIndex];
	if (pZInfo == NULL)
	{
		DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, String("Missing Z-Strip info on multi-Z object"));
		return(FALSE);
	}

	usZStartLevel = (UINT16)((INT16)usZValue + ((INT16)pZInfo->bInitialZChange*Z_SUBLAYERS * 10));

	if (LeftSkip > pZInfo->ubFirstZStripWidth)
	{
		usZStartCols = (LeftSkip - pZInfo->ubFirstZStripWidth);
		usZStartCols = 20 - (usZStartCols % 20);
	}
	else if (LeftSkip < pZInfo->ubFirstZStripWidth)
		usZStartCols = (UINT16)(pZInfo->ubFirstZStripWidth - LeftSkip);
	else
		usZStartCols = 20;

	// set to odd number of pixels for first column
	usZColsToGo = usZStartCols;

	pZArray = pZInfo->pbZChange;

	if (LeftSkip >= usZColsToGo)
	{
		// Index into array after doing left clipping
		usZStartIndex = 1 + ((LeftSkip - pZInfo->ubFirstZStripWidth) / 20);

		//calculates the Z-value after left-side clipping
		if (usZStartIndex)
		{
			for (usCount = 0; usCount < usZStartIndex; usCount++)
			{
				switch (pZArray[usCount])
				{
				case -1:	usZStartLevel -= Z_SUBLAYERS;
					break;
				case 0:		//no change
					break;
				case 1:		usZStartLevel += Z_SUBLAYERS;
					break;
				}
			}
		}
	}
	else
		usZStartIndex = 0;

	usZLevel = usZStartLevel;
	usZIndex = usZStartIndex;

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0							// check for nothing clipped on top
		je		LeftSkipSetup


		// Skips the number of lines clipped at the top
		TopSkipLoop :

		mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		TopSkipLoop
			jz		TSEndLine

			add		esi, ecx

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			jmp		TopSkipLoop

			TSEndLine :
		dec		TopSkip
			jnz		TopSkipLoop


			// Start of line loop

			// Skips the pixels hanging outside the left-side boundry
		LeftSkipSetup:

		mov		Unblitted, 0					// Unblitted counts any pixels left from a run
			mov		eax, LeftSkip					// after we have skipped enough left-side pixels
			mov		LSCount, eax					// LSCount counts how many pixels skipped so far
			or eax, eax
			jz		BlitLineSetup					// check for nothing to skip

			LeftSkipLoop :

		mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		LSTrans

			cmp		ecx, LSCount
			je		LSSkip2								// if equal, skip whole, and start blit with new run
			jb		LSSkip1								// if less, skip whole thing

			add		esi, LSCount					// skip partial run, jump into normal loop for rest

			push	esi
			mov		esi, AlphaPtr
			add		esi, LSCount
			mov		AlphaPtr, esi
			pop		esi

			sub		ecx, LSCount
			mov		eax, BlitLength
			mov		LSCount, eax
			mov		Unblitted, 0
			jmp		BlitNTL1							// *** jumps into non-transparent blit loop

			LSSkip2 :
		add		esi, ecx							// skip whole run, and start blit with new run

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			jmp		BlitLineSetup


			LSSkip1 :
		add		esi, ecx							// skip whole run, continue skipping

			push	esi
			mov		esi, AlphaPtr
			add		esi, ecx
			mov		AlphaPtr, esi
			pop		esi

			sub		LSCount, ecx
			jmp		LeftSkipLoop


			LSTrans :
		and		ecx, 07fH
			cmp		ecx, LSCount
			je		BlitLineSetup					// if equal, skip whole, and start blit with new run
			jb		LSTrans1							// if less, skip whole thing

			sub		ecx, LSCount							// skip partial run, jump into normal loop for rest
			mov		eax, BlitLength
			mov		LSCount, eax

			mov		Unblitted, 0
			jmp		BlitTransparent				// *** jumps into transparent blit loop


			LSTrans1 :
		sub		LSCount, ecx					// skip whole run, continue skipping
			jmp		LeftSkipLoop

			//-------------------------------------------------
			// setup for beginning of line

			BlitLineSetup :
		mov		eax, BlitLength
			mov		LSCount, eax
			mov		Unblitted, 0

			BlitDispatch :

			cmp		LSCount, 0							// Check to see if we're done blitting
			je		RightSkipLoop

			mov		cl, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or cl, cl
			js		BlitTransparent
			jz		RSLoop2

			//--------------------------------
			// blitting non-transparent pixels

			and		ecx, 07fH

			BlitNTL1 :
		mov		ax, [ebx]								// check z-level of pixel
			cmp		ax, usZLevel
			ja		BlitNTL2

			mov		ax, usZLevel						// update z-level of pixel
			mov[ebx], ax

			// Check for shadow...
			xor		eax, eax
			mov		al, [esi]
			cmp		al, 254
			jne		BlitNTL66

			mov		al, fIgnoreShadows
			cmp		al, 0
			jne		BlitNTL2

			mov		ax, [edi]
			mov		ax, ShadeTable[eax * 2]
			mov[edi], ax
			jmp		BlitNTL2

			BlitNTL66 :

		mov		ax, [edx + eax * 2]					// Copy pixel

			push	edx
			push	ecx
			push	ebx
			push	esi
			mov		esi, AlphaPtr
			xor		ebx, ebx
			mov		bl, [esi]
			pop		esi
			push	ebx
			push[edi]
			push	eax
			call    blendWithAlpha
			add     esp, 12
			pop		ebx
			pop		ecx
			pop		edx

			mov[edi], ax

			BlitNTL2 :
		inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			add		edi, 2
			add		ebx, 2

			dec		usZColsToGo
			jnz		BlitNTL6

			// update the z-level according to the z-table

			push	edx
			mov		edx, pZArray						// get pointer to array
			xor		eax, eax
			mov		ax, usZIndex						// pick up the current array index
			add		edx, eax
			inc		eax											// increment it
			mov		usZIndex, ax						// store incremented value

			mov		al, [edx]								// get direction instruction
			mov		dx, usZLevel						// get current z-level

			or al, al
			jz		BlitNTL5								// dir = 0 no change
			js		BlitNTL4								// dir < 0 z-level down
																		// dir > 0 z-level up (default)
			add		dx, Z_SUBLAYERS
			jmp		BlitNTL5

			BlitNTL4 :
		sub		dx, Z_SUBLAYERS

			BlitNTL5 :
		mov		usZLevel, dx						// store the now-modified z-level
			mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
			pop		edx

			BlitNTL6 :
		dec		LSCount									// decrement pixel length to blit
			jz		RightSkipLoop						// done blitting the visible line

			dec		ecx
			jnz		BlitNTL1								// continue current run

			jmp		BlitDispatch						// done current run, go for another


	//----------------------------
	// skipping transparent pixels

		BlitTransparent:									// skip transparent pixels

		and		ecx, 07fH

			BlitTrans2 :

		add		edi, 2									// move up the destination pointer
			add		ebx, 2

			dec		usZColsToGo
			jnz		BlitTrans1

			// update the z-level according to the z-table

			push	edx
			mov		edx, pZArray						// get pointer to array
			xor		eax, eax
			mov		ax, usZIndex						// pick up the current array index
			add		edx, eax
			inc		eax											// increment it
			mov		usZIndex, ax						// store incremented value

			mov		al, [edx]								// get direction instruction
			mov		dx, usZLevel						// get current z-level

			or al, al
			jz		BlitTrans5							// dir = 0 no change
			js		BlitTrans4							// dir < 0 z-level down
																		// dir > 0 z-level up (default)
			add		dx, Z_SUBLAYERS
			jmp		BlitTrans5

			BlitTrans4 :
		sub		dx, Z_SUBLAYERS

			BlitTrans5 :
		mov		usZLevel, dx						// store the now-modified z-level
			mov		usZColsToGo, 20					// reset the next z-level change to 20 cols
			pop		edx

			BlitTrans1 :

		dec		LSCount									// decrement the pixels to blit
			jz		RightSkipLoop						// done the line

			dec		ecx
			jnz		BlitTrans2

			jmp		BlitDispatch

			//---------------------------------------------
			// Scans the ETRLE until it finds an EOL marker

			RightSkipLoop :


	RSLoop1:
		mov		al, [esi]
			inc		esi

			push	esi
			mov		esi, AlphaPtr
			inc		esi
			mov		AlphaPtr, esi
			pop		esi

			or al, al
			jnz		RSLoop1

			RSLoop2 :

		dec		BlitHeight
			jz		BlitDone
			add		edi, LineSkip
			add		ebx, LineSkip

			// reset all the z-level stuff for a new line

			mov		ax, usZStartLevel
			mov		usZLevel, ax
			mov		ax, usZStartIndex
			mov		usZIndex, ax
			mov		ax, usZStartCols
			mov		usZColsToGo, ax


			jmp		LeftSkipSetup


			BlitDone :
	}

	return(TRUE);
}




BOOLEAN Zero8BPPDataTo16BPPBufferTransparent( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex )
{
	UINT32 uiOffset;
	UINT32 usHeight, usWidth;
	UINT8	 *SrcPtr, *DestPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	INT32	 iTempX, iTempY;


	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	// Validations
	CHECKF( iTempX >= 0 );
	CHECKF( iTempY >= 0 );


	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*iTempY) + (iTempX*2);
	LineSkip=(uiDestPitchBYTES-(usWidth*2));

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
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

		mov		[edi], ax

		inc		esi
		add		edi, 2

BlitNTL2:
		clc
		rcr		cl, 1
		jnc		BlitNTL3

		mov		[edi], ax

		mov		[edi+2], ax

		add		esi, 2
		add		edi, 4

BlitNTL3:

		or		cl, cl
		jz		BlitDispatch

		xor		ebx, ebx

BlitNTL4:

		mov		[edi], ax

		mov		[edi+2], ax

		mov		[edi+4], ax

		mov		[edi+6], ax

		add		esi, 4
		add		edi, 8
		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch

BlitTransparent:

		and		ecx, 07fH
//		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		jmp		BlitDispatch


BlitDone:
	}

	return(TRUE);

}


BOOLEAN Blt8BPPDataTo16BPPBufferTransInvZ( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex )
{
	UINT16 *p16BPPPalette;
	UINT32 uiOffset;
	UINT32 usHeight, usWidth;
	UINT8	 *SrcPtr, *DestPtr, *ZPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	INT32	 iTempX, iTempY;


	// Assertions
	Assert( hSrcVObject != NULL );
	Assert( pBuffer != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	// Validations
	CHECKF( iTempX >= 0 );
	CHECKF( iTempY >= 0 );


	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	DestPtr = (UINT8 *)pBuffer + (uiDestPitchBYTES*iTempY) + (iTempX*2);
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*iTempY) + (iTempX*2);
	p16BPPPalette = hSrcVObject->pShadeCurrent;
	LineSkip=(uiDestPitchBYTES-(usWidth*2));

	__asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

//BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jne		BlitNTL5

		//mov		[ebx], ax

		xor		ah, ah
		mov		al, [esi]
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL5:
		inc		esi
		inc		edi
		inc		ebx
		inc		edi
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
//		shl		ecx, 1
		add   ecx, ecx
		add		edi, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
	}

	return(TRUE);

}



BOOLEAN IsTileRedundent( UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex )
{
	UINT16 *p16BPPPalette;
	UINT32 uiOffset;
	UINT32 usHeight, usWidth;
	UINT8	 *SrcPtr, *ZPtr;
	UINT32 LineSkip;
	ETRLEObject *pTrav;
	INT32	 iTempX, iTempY;
	BOOLEAN		fHidden = TRUE;


	// Assertions
	Assert( hSrcVObject != NULL );

	// Get Offsets from Index into structure
	pTrav = &(hSrcVObject->pETRLEObject[ usIndex ] );
	usHeight				= (UINT32)pTrav->usHeight;
	usWidth					= (UINT32)pTrav->usWidth;
	uiOffset				= pTrav->uiDataOffset;

	// Add to start position of dest buffer
	iTempX = iX + pTrav->sOffsetX;
	iTempY = iY + pTrav->sOffsetY;

	// Validations
	CHECKF( iTempX >= 0 );
	CHECKF( iTempY >= 0 );


	SrcPtr= (UINT8 *)hSrcVObject->pPixData + uiOffset;
	ZPtr = (UINT8 *)pZBuffer + (uiDestPitchBYTES*iTempY) + (iTempX*2);
	p16BPPPalette = hSrcVObject->pShadeCurrent;
	LineSkip=(uiDestPitchBYTES-(usWidth*2));

	__asm {

		mov		esi, SrcPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

//BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jle		BlitNTL5


		//    Set false, flag
		mov   fHidden, 0
		jmp		BlitDone


BlitNTL5:
		inc		esi
		inc		ebx
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
//		shl		ecx, 1
		add   ecx, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
	}

	return(fHidden);

}
