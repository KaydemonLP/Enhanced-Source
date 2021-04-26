//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_OF_SHARED_H
#define FX_OF_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// This runs on both the client and the server.
// On the server, it only does the damage calculations.
// On the client, it does all the effects.
void FX_FireBullets( 
	int	iPlayerIndex,
	const Vector &vOrigin,
	const QAngle &vAngles,
	int	iMode,
	int iSeed,
	int iDamage,
	int iBulletsPerShot,
	float flSpread,
	float flRange
	);

#endif // FX_OF_SHARED_H