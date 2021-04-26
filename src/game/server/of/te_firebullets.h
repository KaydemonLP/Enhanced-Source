//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TE_FIREBULLETS_H
#define TE_FIREBULLETS_H
#ifdef _WIN32
#pragma once
#endif


void TE_FireBullets( 
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

#endif // TE_FIREBULLETS_H
