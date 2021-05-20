//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once


// Shared header file for players
#if defined( CLIENT_DLL )
#include "c_sdk_player.h"
#define CSDKPlayer C_SDKPlayer
#else
#include "sdk_player.h"
#endif

#define GetClass(x) ClassManager()->m_hClassInfo[x->GetClassNumber()]
