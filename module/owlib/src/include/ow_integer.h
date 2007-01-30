/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_INTEGER_H
#define OW_INTEGER_H

/* Routines to play with byte <-> integer */
static inline uint8_t UT_uint8(BYTE * p)
{
	return (uint8_t) p[0];
}
static inline uint16_t UT_uint16(BYTE * p)
{
	return (((uint16_t) p[1]) << 8) | ((uint16_t) p[0]);
}
static inline uint32_t UT_uint24(BYTE * p)
{
	return (((uint32_t) p[2]) << 16) | (((uint32_t) p[1]) << 8) |
		((uint32_t) p[0]);
}
static inline uint32_t UT_uint32(BYTE * p)
{
	return (((uint32_t) p[3]) << 24) | (((uint32_t) p[2]) << 16) |
		(((uint32_t) p[1]) << 8) | ((uint32_t) p[0]);
}
static inline int8_t UT_int8(BYTE * p)
{
	return (int8_t) p[0];
}
static inline int16_t UT_int16(BYTE * p)
{
	return (((int16_t) p[1]) << 8) | ((uint16_t) p[0]);
}
static inline int32_t UT_int24(BYTE * p)
{
	return (((int32_t) p[2]) << 16) | (((uint32_t) p[1]) << 8) |
		((uint32_t) p[0]);
}
static inline int32_t UT_int32(BYTE * p)
{
	return (((int32_t) p[3]) << 24) | (((uint32_t) p[2]) << 16) |
		(((uint32_t) p[1]) << 8) | ((uint32_t) p[0]);
}

#endif							/* OW_INTEGER_H */
