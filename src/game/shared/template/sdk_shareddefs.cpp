//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sdk_shareddefs.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *g_szProjectileTypes[] =
{
	"projectile_bullet",
	"projectile_dynamite",
	"projectile_laser"
};

const char *g_szInputNames[] =
{
	"IN_ATTACK",
	"IN_JUMP",
	"IN_DUCK",
	"IN_FORWARD",
	"IN_BACK",
	"IN_USE",
	"IN_CANCEL",
	"IN_LEFT",
	"IN_RIGHT",
	"IN_MOVELEFT",
	"IN_MOVERIGHT",
	"IN_ATTACK2",
	"IN_RUN",
	"IN_RELOAD",
	"IN_ALT1",
	"IN_ALT2",
	"IN_SCORE",
	"IN_SPEED",
	"IN_WALK",
	"IN_ZOOM",
	"IN_WEAPON1",
	"IN_WEAPON2",
	"IN_BULLRUSH",
	"IN_GRENADE1",
	"IN_GRENADE2",
	"IN_LOOKSPIN"
};

const char *g_szAnimEventName[] =
{
	"PLAYERANIMEVENT_FIRE_GUN_PRIMARY",
	"PLAYERANIMEVENT_FIRE_GUN_SECONDARY",
	"PLAYERANIMEVENT_THROW_GRENADE",
	"PLAYERANIMEVENT_JUMP",
	"PLAYERANIMEVENT_RELOAD"
};