#include "types.h"
#include "himage.h"

void FindIndecies(SGPPaletteEntry *pSrcPalette, SGPPaletteEntry *pMapPalette, UINT8 *pTable)
{
UINT16 usCurIndex, usCurDelta, usCurCount;
UINT32 *pSavedPtr;

__asm {

// Assumes:
//	ESI = Pointer to source palette (shaded values)
//	EDI = Pointer to original palette (palette we'll end up using!)
//	EBX = Pointer to array of indecies

		mov		esi, pSrcPalette
		mov		edi, pMapPalette
		mov		ebx, pTable

		mov		BYTE PTR [ebx],0						; Index 0 is always 0, for trans col
		inc		ebx

		add		esi,4												; Goto next color entry
		add		edi,4
		mov		pSavedPtr, edi							; Save pointer to original pal

		mov		usCurCount, 255							; We'll check cols 1-255

DoNextIndex:

		mov		edi, pSavedPtr							; Restore saved ptr
		mov		usCurIndex, 256							; Set found index & delta to some
		mov		usCurDelta, 0ffffh					; val so we get at least 1 col.

		mov		ecx,255											; Check cols 1-255 of orig pal
		push	ebx
		xor		bx,bx

NextColor:
		xor		ah,ah						; Calc delta between shaded color
		mov		al,[edi]										; and a color in the orig palette.
		mov		bl,[esi]										; Formula:
		sub		ax,bx						;	Delta = abs(red-origred) +
		or		ax,ax												;			abs(green-origgreen) +
		jns		NC1
		neg		ax
NC1:mov		dx,ax						;			abs(blue-origblue)
		xor		ah,ah
		mov		al,[edi+1]
		mov		bl,[esi+1]
		sub		ax,bx
		or		ax,ax												;			abs(green-origgreen) +
		jns		NC2
		neg		ax
NC2:add		dx,ax
		xor		ah,ah
		mov		al,[edi+2]
		mov		bl,[esi+2]
		sub		ax,bx
		or		ax,ax												;			abs(green-origgreen) +
		jns		NC3
		neg		ax
NC3:add		dx,ax

		cmp		dx,usCurDelta								; If delta < old delta
		jae		NotThisCol									;	Save this delta and it's
		mov		ax,256						;	palette index
		mov		[usCurDelta],dx
		sub	ax,cx
		mov		[usCurIndex],ax
NotThisCol:
		add		edi,4												; Try next color in orginal pal
		dec		cx
		jnz	NextColor

		pop		ebx
		mov		ax,usCurIndex								; By now, usCurIndex holds pal index
		mov		[ebx],al										; of closest color in orig pal
		inc		ebx													; so save it, then repeat above
		add		esi,4												; for the other cols in shade pal
		dec		usCurCount
		jnz		DoNextIndex
	}
}
