//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENTHANDLE_H
#define DMELEMENTHANDLE_H

#ifdef _WIN32
#pragma once
#endif

#include "utlbufferutil.h"


//-----------------------------------------------------------------------------
// handle to an CDmElement
//-----------------------------------------------------------------------------
#define PERFORM_HANDLE_TYPECHECKING 1
#if PERFORM_HANDLE_TYPECHECKING

// this is here to make sure we're being type-safe about element handles
// otherwise, the compiler lets us cast to bool incorrectly
// the other solution would be to redefine DmElementHandle_t s.t. DMELEMENT_HANDLE_INVALID==0
struct DmElementHandle_t
{
	constexpr DmElementHandle_t() : handle( 0xffffffff ) {}
	explicit constexpr DmElementHandle_t( int h ) : handle( h ) {}
	constexpr bool operator==( const DmElementHandle_t &h ) const { return handle == h.handle; }
	constexpr bool operator!=( const DmElementHandle_t &h ) const { return handle != h.handle; }
	constexpr bool operator<( const DmElementHandle_t &h ) const { return handle < h.handle; }
//	inline operator int() const { return handle; } // if we're okay with implicit int casts, uncomment this method
	int handle;
};
constexpr DmElementHandle_t DMELEMENT_HANDLE_INVALID;

#else // PERFORM_HANDLE_TYPECHECKING

enum DmElementHandle_t : int32
{
	DMELEMENT_HANDLE_INVALID = 0xffffffff
};

#endif // PERFORM_HANDLE_TYPECHECKING



#endif // DMELEMENTHANDLE_H
