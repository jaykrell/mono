#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned UINT, UINT32;

#ifdef _MSC_VER
typedef unsigned __int64 UINT64;
typedef __int64 INT64;
#define I64 "I64"
#define DLLEXPORT
#else
typedef unsigned long long UINT64;
typedef long long INT64;
#define I64 "ll"
#define DLLEXPORT __declspec(dllexport) // for debugging
#define _strtoui64 strtoull
#endif

#if _MSC_VER

UINT64 _rotl64(UINT64 value, int count);
#pragma intrinsic(_rotl64)

UINT64 _rotr64(UINT64 value, int count);
#pragma intrinsic(_rotr64)

UINT32 _rotl(UINT32 value, int count);
#pragma intrinsic(_rotl)

UINT32 _rotr(UINT32 value, int count);
#pragma intrinsic(_rotr)

DLLEXPORT UINT64 intrinsic_rotate_left_64(UINT64 value, int count) { return _rotl64(value, count); }
DLLEXPORT UINT64 intrinsic_rotate_right_64(UINT64 value, int count) { return _rotr64(value, count); }
DLLEXPORT UINT32 intrinsic_rotate_left_32(UINT32 value, int count) { return _rotl(value, count); }
DLLEXPORT UINT32 intrinsic_rotate_right_32(UINT32 value, int count) { return _rotr(value, count); }

#define MSC_ASSERT(x) assert(x)

#else
#define MSC_ASSERT(x)
#endif

#define ROTATE(type, name, direction, other_direction)		\
DLLEXPORT type name(type value, int count)					\
{															\
	type const result = (value direction count) | (value other_direction (sizeof(type) * 8 - count));	\
	MSC_ASSERT(result == intrinsic_##name(value, count));	\
	return result;											\
}

ROTATE(UINT32, rotate_left_32, <<, >>)
ROTATE(UINT32, rotate_right_32, >>, <<)
ROTATE(UINT64, rotate_left_64, <<, >>)
ROTATE(UINT64, rotate_right_64, >>, <<)

void test(UINT64 value, int count)
{
	printf("%X rol32 %d = %X\n", (int)value, count, rotate_left_32((UINT32)value, count));
	printf("%X ror32 %d = %X\n", (int)value, count, rotate_right_32((UINT32)value, count));
	printf("%" I64 "X rol64 %d = %" I64 "X\n", value, count, rotate_left_64(value, count));
	printf("%" I64 "X ror64 %d = %" I64 "X\n", value, count, rotate_right_64(value, count));
}

int main(int argc, char** argv)
{
	if (argc == 3)
	{
		test(_strtoui64(argv[1], 0, 0), _strtoui64(argv[2], 0, 0));
		return 0;
	}
	for (int count = 1; count <= 16; ++count)
	{
		test(-1, count);
	}
	for (UINT64 value = 0; value <= 16; ++value)
	{
		for (int count = 1; count <= 16; ++count)
		{
			test(value ? value : -1, count);
		}
	}
	return 0;
}
