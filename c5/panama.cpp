// panama.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "panama.h"
#include "misc.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

template <class B>
void Panama<B>::Reset()
{
	memset(m_state, 0, m_state.SizeInBytes());
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	m_state[17] = HasSSSE3();
#endif
}

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE

#pragma warning(disable: 4731)	// frame pointer register 'ebp' modified by inline assembly code

void Panama_SSE2_Pull(size_t count, word32 *state, word32 *z, const word32 *y)
{
#ifdef __GNUC__
	__asm__ __volatile__
	(
		".intel_syntax noprefix;"
	AS1(	push	ebx)
#else
	AS2(	mov		ecx, count)
	AS2(	mov		esi, state)
	AS2(	mov		edi, z)
	AS2(	mov		edx, y)
#endif
	AS2(	shl		ecx, 5)
	ASJ(	jz,		5, f)
	AS2(	mov		ebx, [esi+4*17])
	AS2(	add		ecx, ebx)

	AS1(	push	ebp)
	AS1(	push	ecx)

	AS2(	movdqa	xmm0, [esi+0*16])
	AS2(	movdqa	xmm1, [esi+1*16])
	AS2(	movdqa	xmm2, [esi+2*16])
	AS2(	movdqa	xmm3, [esi+3*16])
	AS2(	mov		eax, [esi+4*16])

	ASL(4)
	// gamma and pi
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	AS2(	test	ebx, 1)
	ASJ(	jnz,	6, f)
#endif
	AS2(	movdqa	xmm6, xmm2)
	AS2(	movss	xmm6, xmm3)
	ASS(	pshufd	xmm5, xmm6, 0, 3, 2, 1)
	AS2(	movd	xmm6, eax)
	AS2(	movdqa	xmm7, xmm3)
	AS2(	movss	xmm7, xmm6)
	ASS(	pshufd	xmm6, xmm7, 0, 3, 2, 1)
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	ASJ(	jmp,	7, f)
	ASL(6)
	AS2(	movdqa	xmm5, xmm3)
	AS3(	palignr	xmm5, xmm2, 4)
	AS2(	movd	xmm6, eax)
	AS3(	palignr	xmm6, xmm3, 4)
	ASL(7)
#endif

	AS2(	movd	ecx, xmm2)
	AS1(	not		ecx)
	AS2(	movd	ebp, xmm3)
	AS2(	or		ecx, ebp)
	AS2(	xor		eax, ecx)

#define SSE2_Index(i) ASM_MOD(((i)*13+16), 17)

#define pi(i)	\
	AS2(	movd	ecx, xmm7)\
	AS2(	rol		ecx, ASM_MOD((ASM_MOD(5*i,17)*(ASM_MOD(5*i,17)+1)/2), 32))\
	AS2(	mov		[esi+SSE2_Index(ASM_MOD(5*(i), 17))*4], ecx)

#define pi4(x, y, z, a, b, c, d)	\
	AS2(	pcmpeqb	xmm7, xmm7)\
	AS2(	pxor	xmm7, x)\
	AS2(	por		xmm7, y)\
	AS2(	pxor	xmm7, z)\
	pi(a)\
	ASS(	pshuflw	xmm7, xmm7, 1, 0, 3, 2)\
	pi(b)\
	AS2(	punpckhqdq	xmm7, xmm7)\
	pi(c)\
	ASS(	pshuflw	xmm7, xmm7, 1, 0, 3, 2)\
	pi(d)

	pi4(xmm1, xmm2, xmm3, 1, 5, 9, 13)
	pi4(xmm0, xmm1, xmm2, 2, 6, 10, 14)
	pi4(xmm6, xmm0, xmm1, 3, 7, 11, 15)
	pi4(xmm5, xmm6, xmm0, 4, 8, 12, 16)

	// output keystream and update buffer here to hide partial memory stalls between pi and theta
	AS2(	movdqa	xmm4, xmm3)
	AS2(	punpcklqdq	xmm3, xmm2)		// 1 5 2 6
	AS2(	punpckhdq	xmm4, xmm2)		// 9 10 13 14
	AS2(	movdqa	xmm2, xmm1)
	AS2(	punpcklqdq	xmm1, xmm0)		// 3 7 4 8
	AS2(	punpckhdq	xmm2, xmm0)		// 11 12 15 16

	// keystream
	AS2(	test	edi, edi)
	ASJ(	jz,		0, f)
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm2)
	AS2(	punpckhqdq	xmm6, xmm2)
	AS2(	test	edx, 0xf)
	ASJ(	jnz,	2, f)
	AS2(	test	edx, edx)
	ASJ(	jz,		1, f)
	AS2(	pxor	xmm4, [edx])
	AS2(	pxor	xmm6, [edx+16])
	AS2(	add		edx, 32)
	ASJ(	jmp,	1, f)
	ASL(2)
	AS2(	movdqu	xmm0, [edx])
	AS2(	movdqu	xmm2, [edx+16])
	AS2(	pxor	xmm4, xmm0)
	AS2(	pxor	xmm6, xmm2)
	AS2(	add		edx, 32)
	ASL(1)
	AS2(	test	edi, 0xf)
	ASJ(	jnz,	3, f)
	AS2(	movdqa	[edi], xmm4)
	AS2(	movdqa	[edi+16], xmm6)
	AS2(	add		edi, 32)
	ASJ(	jmp,	0, f)
	ASL(3)
	AS2(	movdqu	[edi], xmm4)
	AS2(	movdqu	[edi+16], xmm6)
	AS2(	add		edi, 32)
	ASL(0)

	// buffer update
	AS2(	lea		ecx, [ebx + 32])
	AS2(	and		ecx, 31*32)
	AS2(	lea		ebp, [ebx + (32-24)*32])
	AS2(	and		ebp, 31*32)

	AS2(	movdqa	xmm0, [esi+20*4+ecx+0*8])
	AS2(	pxor	xmm3, xmm0)
	ASS(	pshufd	xmm0, xmm0, 2, 3, 0, 1)
	AS2(	movdqa	[esi+20*4+ecx+0*8], xmm3)
	AS2(	pxor	xmm0, [esi+20*4+ebp+2*8])
	AS2(	movdqa	[esi+20*4+ebp+2*8], xmm0)

	AS2(	movdqa	xmm4, [esi+20*4+ecx+2*8])
	AS2(	pxor	xmm1, xmm4)
	AS2(	movdqa	[esi+20*4+ecx+2*8], xmm1)
	AS2(	pxor	xmm4, [esi+20*4+ebp+0*8])
	AS2(	movdqa	[esi+20*4+ebp+0*8], xmm4)

	// theta
	AS2(	movdqa	xmm3, [esi+3*16])
	AS2(	movdqa	xmm2, [esi+2*16])
	AS2(	movdqa	xmm1, [esi+1*16])
	AS2(	movdqa	xmm0, [esi+0*16])

#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	AS2(	test	ebx, 1)
	ASJ(	jnz,	8, f)
#endif
	AS2(	movd	xmm6, eax)
	AS2(	movdqa	xmm7, xmm3)
	AS2(	movss	xmm7, xmm6)
	AS2(	movdqa	xmm6, xmm2)
	AS2(	movss	xmm6, xmm3)
	AS2(	movdqa	xmm5, xmm1)
	AS2(	movss	xmm5, xmm2)
	AS2(	movdqa	xmm4, xmm0)
	AS2(	movss	xmm4, xmm1)
	ASS(	pshufd	xmm7, xmm7, 0, 3, 2, 1)
	ASS(	pshufd	xmm6, xmm6, 0, 3, 2, 1)
	ASS(	pshufd	xmm5, xmm5, 0, 3, 2, 1)
	ASS(	pshufd	xmm4, xmm4, 0, 3, 2, 1)
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	ASJ(	jmp,	9, f)
	ASL(8)
	AS2(	movd	xmm7, eax)
	AS3(	palignr	xmm7, xmm3, 4)
	AS2(	movq	xmm6, xmm3)
	AS3(	palignr	xmm6, xmm2, 4)
	AS2(	movq	xmm5, xmm2)
	AS3(	palignr	xmm5, xmm1, 4)
	AS2(	movq	xmm4, xmm1)
	AS3(	palignr	xmm4, xmm0, 4)
	ASL(9)
#endif

	AS2(	xor		eax, 1)
	AS2(	movd	ecx, xmm0)
	AS2(	xor		eax, ecx)
	AS2(	movd	ecx, xmm3)
	AS2(	xor		eax, ecx)

	AS2(	pxor	xmm3, xmm2)
	AS2(	pxor	xmm2, xmm1)
	AS2(	pxor	xmm1, xmm0)
	AS2(	pxor	xmm0, xmm7)
	AS2(	pxor	xmm3, xmm7)
	AS2(	pxor	xmm2, xmm6)
	AS2(	pxor	xmm1, xmm5)
	AS2(	pxor	xmm0, xmm4)

	// sigma
	AS2(	lea		ecx, [ebx + (32-4)*32])
	AS2(	and		ecx, 31*32)
	AS2(	lea		ebp, [ebx + 16*32])
	AS2(	and		ebp, 31*32)

	AS2(	movdqa	xmm4, [esi+20*4+ecx+0*16])
	AS2(	movdqa	xmm5, [esi+20*4+ebp+0*16])
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm5)
	AS2(	punpckhqdq	xmm6, xmm5)
	AS2(	pxor	xmm3, xmm4)
	AS2(	pxor	xmm2, xmm6)

	AS2(	movdqa	xmm4, [esi+20*4+ecx+1*16])
	AS2(	movdqa	xmm5, [esi+20*4+ebp+1*16])
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm5)
	AS2(	punpckhqdq	xmm6, xmm5)
	AS2(	pxor	xmm1, xmm4)
	AS2(	pxor	xmm0, xmm6)

	// loop
	AS2(	add		ebx, 32)
	AS2(	cmp		ebx, [esp])
	ASJ(	jne,	4, b)

	// save state
	AS2(	mov		ebp, [esp+4])
	AS2(	add		esp, 8)
	AS2(	mov		[esi+4*17], ebx)
	AS2(	mov		[esi+4*16], eax)
	AS2(	movdqa	[esi+3*16], xmm3)
	AS2(	movdqa	[esi+2*16], xmm2)
	AS2(	movdqa	[esi+1*16], xmm1)
	AS2(	movdqa	[esi+0*16], xmm0)
	ASL(5)

#ifdef __GNUC__
	AS1(	pop		ebx)
	".att_syntax prefix;"
		:
		: "c" (count), "S" (state), "D" (z), "d" (y)
		: "%eax", "memory", "cc"
	);
#endif
}

#endif

template <class B>
void Panama<B>::Iterate(size_t count, const word32 *p, word32 *z, const word32 *y)
{
	word32 bstart = m_state[17];
	word32 *const aPtr = m_state;
	word32 cPtr[17];

#define bPtr ((byte *)(aPtr+20))

// reorder the state for SSE2
// a and c: 4 8 12 16 | 3 7 11 15 | 2 6 10 14 | 1 5 9 13 | 0
//			xmm0		xmm1		xmm2		xmm3		eax
#define a(i) aPtr[((i)*13+16) % 17]		// 13 is inverse of 4 mod 17
#define c(i) cPtr[((i)*13+16) % 17]
// b: 0 4 | 1 5 | 2 6 | 3 7
#define b(i, j) b##i[(j)*2%8 + (j)/4]

// output
#define OA(i) z[i] = ConditionalByteReverse(B::ToEnum(), a(i+9))
#define OX(i) z[i] = y[i] ^ ConditionalByteReverse(B::ToEnum(), a(i+9))
// buffer update
#define US(i) {word32 t=b(0,i); b(0,i)=ConditionalByteReverse(B::ToEnum(), p[i])^t; b(25,(i+6)%8)^=t;}
#define UL(i) {word32 t=b(0,i); b(0,i)=a(i+1)^t; b(25,(i+6)%8)^=t;}
// gamma and pi
#define GP(i) c(5*i%17) = rotlFixed(a(i) ^ (a((i+1)%17) | ~a((i+2)%17)), ((5*i%17)*((5*i%17)+1)/2)%32)
// theta and sigma
#define T(i,x) a(i) = c(i) ^ c((i+1)%17) ^ c((i+4)%17) ^ x
#define TS1S(i) T(i+1, ConditionalByteReverse(B::ToEnum(), p[i]))
#define TS1L(i) T(i+1, b(4,i))
#define TS2(i) T(i+9, b(16,i))

	while (count--)
	{
		if (z)
		{
			if (y)
			{
				OX(0); OX(1); OX(2); OX(3); OX(4); OX(5); OX(6); OX(7);
				y += 8;
			}
			else
			{
				OA(0); OA(1); OA(2); OA(3); OA(4); OA(5); OA(6); OA(7);
			}
			z += 8;
		}

		word32 *const b16 = (word32 *)(bPtr+((bstart+16*32) & 31*32));
		word32 *const b4 = (word32 *)(bPtr+((bstart+(32-4)*32) & 31*32));
       	bstart += 32;
		word32 *const b0 = (word32 *)(bPtr+((bstart) & 31*32));
		word32 *const b25 = (word32 *)(bPtr+((bstart+(32-25)*32) & 31*32));

		if (p)
		{
			US(0); US(1); US(2); US(3); US(4); US(5); US(6); US(7);
		}
		else
		{
			UL(0); UL(1); UL(2); UL(3); UL(4); UL(5); UL(6); UL(7);
		}

		GP(0); 
		GP(1); 
		GP(2); 
		GP(3); 
		GP(4); 
		GP(5); 
		GP(6); 
		GP(7);
		GP(8); 
		GP(9); 
		GP(10); 
		GP(11); 
		GP(12); 
		GP(13); 
		GP(14); 
		GP(15); 
		GP(16);

		T(0,1);

		if (p)
		{
			TS1S(0); TS1S(1); TS1S(2); TS1S(3); TS1S(4); TS1S(5); TS1S(6); TS1S(7);
			p += 8;
		}
		else
		{
			TS1L(0); TS1L(1); TS1L(2); TS1L(3); TS1L(4); TS1L(5); TS1L(6); TS1L(7);
		}

		TS2(0); TS2(1); TS2(2); TS2(3); TS2(4); TS2(5); TS2(6); TS2(7);
	}
	m_state[17] = bstart;
}

template <class B>
size_t Weak::PanamaHash<B>::HashMultipleBlocks(const word32 *input, size_t length)
{
	this->Iterate(length / this->BLOCKSIZE, input);
	return length % this->BLOCKSIZE;
}

template <class B>
void Weak::PanamaHash<B>::TruncatedFinal(byte *hash, size_t size)
{
	this->ThrowIfInvalidTruncatedSize(size);

	PadLastBlock(this->BLOCKSIZE, 0x01);
	
	HashEndianCorrectedBlock(this->m_data);

	this->Iterate(32);	// pull

	FixedSizeSecBlock<word32, 8> buf;
	this->Iterate(1, NULL, buf, NULL);

	memcpy(hash, buf, size);

	this->Restart();		// reinit for next use
}

template <class B>
void PanamaCipherPolicy<B>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	assert(length==32);
	memcpy(m_key, key, 32);
}

template <class B>
void PanamaCipherPolicy<B>::CipherResynchronize(byte *keystreamBuffer, const byte *iv)
{
	this->Reset();
	this->Iterate(1, m_key);
	if (iv && IsAligned<word32>(iv))
		this->Iterate(1, (const word32 *)iv);
	else
	{
		FixedSizeSecBlock<word32, 8> buf;
		if (iv)
			memcpy(buf, iv, 32);
		else
			memset(buf, 0, 32);
		this->Iterate(1, buf);
	}

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2())
		Panama_SSE2_Pull(32, this->m_state, NULL, NULL);
	else
#endif
		this->Iterate(32);
}

#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X64
template <class B>
unsigned int PanamaCipherPolicy<B>::GetAlignment() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2())
		return 16;
	else
#endif
		return 1;
}
#endif

template <class B>
void PanamaCipherPolicy<B>::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2())
		Panama_SSE2_Pull(iterationCount, this->m_state, (word32 *)output, (const word32 *)input);
	else
#endif
		this->Iterate(iterationCount, NULL, (word32 *)output, (const word32 *)input);
}

template class Panama<BigEndian>;
template class Panama<LittleEndian>;

template class Weak::PanamaHash<BigEndian>;
template class Weak::PanamaHash<LittleEndian>;

template class PanamaCipherPolicy<BigEndian>;
template class PanamaCipherPolicy<LittleEndian>;

NAMESPACE_END
