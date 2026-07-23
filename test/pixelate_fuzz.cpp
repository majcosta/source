// Fuzz harness for the 8x8 pattern pixelate blitter (sgp/vobject_pixelate.cpp).
//
// Blt16BPPBufferPixelateRectWithColor is hand-written 32-bit inline assembly.
// This harness drives it over a fixed, seeded corpus of inputs and folds every
// output buffer into one deterministic digest. Two builds of the function that
// behave identically over the corpus print the same digest; any behavioural
// difference changes it.
//
// Why it exists: to prove the "byte ptr" size annotation added to the pattern
// compare (the byte-ptr commit) changed nothing -- build the harness at the
// commit before it and at it, and compare the digest -- and, when the assembly
// is later rewritten as portable C, to be the golden that rewrite must match.
//
// Build:  configure with -DBUILD_TESTS=ON, then `ninja pixelate_fuzz`.
// Run:    pixelate_fuzz.exe [iterations]   (under Wine; prints one digest line)
//
// The corpus is fully determined by the seed and iteration count below, so the
// digest is reproducible and only the code under test can move it.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "types.h"		// SGPRect and the integer typedefs the blitter uses

// --- the code under test -------------------------------------------------
// Declared here rather than via vobject_blitters.h to keep the harness off the
// video layer. ClippingRect and the assert entry point are the only externals
// vobject_pixelate.cpp needs; the harness supplies them.
BOOLEAN Blt16BPPBufferPixelateRectWithColor(
	UINT16 *pBuffer, UINT32 uiDestPitchBYTES, SGPRect *area,
	UINT8 Pattern[8][8], UINT16 usColor);

SGPRect ClippingRect;

// Assert() in vobject_pixelate.cpp expands to a call to this on failure. A
// failed assertion in the harness is a real bug, so make it loud and fatal.
void _FailMessage(const char *message, unsigned lineNum,
                  const char *functionName, const char *sourceFileName)
{
	std::fprintf(stderr, "assertion failed at %s:%u\n",
	             sourceFileName ? sourceFileName : "?", lineNum);
	std::abort();
}

// --- deterministic RNG (splitmix64) --------------------------------------
static uint64_t g_rngState = 0x1234567890ABCDEFull;

static uint64_t NextRandom()
{
	uint64_t z = (g_rngState += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

// Bounded, still deterministic; range must be >= 1.
static uint32_t NextIn(uint32_t range)
{
	return (uint32_t)(NextRandom() % range);
}

// --- FNV-1a 64 digest ----------------------------------------------------
static uint64_t g_digest = 1469598103934665603ull;

static void DigestByte(uint8_t b)
{
	g_digest = (g_digest ^ b) * 1099511628211ull;
}

// The buffer is a fixed slab. Every rectangle the corpus produces is clipped
// into it (see ClippingRect below), so the blitter never writes outside.
static const int kPitchPxMax = 120;	// pixels per row
static const int kRowsMax    = 120;
static const int kBufferPx   = kPitchPxMax * kRowsMax + kPitchPxMax;

int main(int argc, char **argv)
{
	unsigned long iterations = 200000;
	if (argc > 1)
		iterations = std::strtoul(argv[1], nullptr, 10);

	static UINT16 buffer[kBufferPx];

	for (unsigned long i = 0; i < iterations; ++i)
	{
		// Destination pitch, in pixels then bytes.
		const uint32_t pitchPx = 8 + NextIn(kPitchPxMax - 8 + 1);	// 8..120
		const uint32_t pitchBytes = pitchPx * 2;

		// Clip rectangle: a sub-rectangle of the slab, so clamped writes stay
		// in bounds whatever the (deliberately wilder) area rectangle is.
		const int clipLeft = NextIn(pitchPx);
		const int clipRight = clipLeft + 1 + NextIn(pitchPx - clipLeft);	// clipLeft+1..pitchPx
		const int clipTop = NextIn(kRowsMax);
		const int clipBottom = clipTop + 1 + NextIn(kRowsMax - clipTop);	// clipTop+1..kRowsMax
		ClippingRect.iLeft = clipLeft;
		ClippingRect.iTop = clipTop;
		ClippingRect.iRight = clipRight;
		ClippingRect.iBottom = clipBottom;

		// Area rectangle: overlaps the clip most of the time but is allowed to
		// spill past every edge, exercising the __min/__max clamp and the
		// width/height < 1 early-outs.
		SGPRect area;
		area.iLeft = (int)NextIn(pitchPx + 8) - 4;
		area.iTop = (int)NextIn(kRowsMax + 8) - 4;
		area.iRight = area.iLeft + (int)NextIn(pitchPx + 1);
		area.iBottom = area.iTop + (int)NextIn(kRowsMax + 1);

		// Pattern: random bytes so both branches of the pattern compare fire,
		// with all-zero and all-nonzero forced in periodically.
		UINT8 pattern[8][8];
		const uint32_t special = NextIn(16);
		for (int r = 0; r < 8; ++r)
			for (int c = 0; c < 8; ++c)
			{
				if (special == 0)      pattern[r][c] = 0;
				else if (special == 1) pattern[r][c] = 0xFF;
				else                   pattern[r][c] = (UINT8)NextRandom();
			}

		const UINT16 usColor = (UINT16)NextRandom();

		// Prime the slab with a per-iteration canary so written cells stand out
		// from untouched ones in the digest.
		const UINT16 canary = (UINT16)(0x1234u + i * 0x9Du);
		for (int p = 0; p < kBufferPx; ++p)
			buffer[p] = canary;

		const BOOLEAN result = Blt16BPPBufferPixelateRectWithColor(
			buffer, pitchBytes, &area, pattern, usColor);

		// Fold the return value and the whole slab into the running digest.
		DigestByte((uint8_t)result);
		const uint8_t *raw = (const uint8_t *)buffer;
		for (int b = 0; b < (int)sizeof(buffer); ++b)
			DigestByte(raw[b]);
	}

	std::printf("pixelate_fuzz iters=%lu digest=0x%016llX\n",
	            iterations, (unsigned long long)g_digest);
	return 0;
}
