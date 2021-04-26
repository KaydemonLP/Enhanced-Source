//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_IMPACT_H
#define FX_IMPACT_H
#ifdef _WIN32
#pragma once
#endif

#include "c_te_effect_dispatch.h"
#include "istudiorender.h"

#ifdef OFFSHORE_DLL
// Parse the impact data from the server's data block
C_BaseEntity *ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, Vector *vecShotDir, short &nSurfaceProp, int &iMaterial, CUtlVector<int> *hDamageType, int &iHitbox );

// Get the decal name to use based on an impact with the specified entity, surface material, and damage type
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, CUtlVector<int> *hDamageType );
#else
// Parse the impact data from the server's data block
C_BaseEntity *ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, Vector *vecShotDir, short &nSurfaceProp, int &iMaterial, int &iDamageType, int &iHitbox );

// Get the decal name to use based on an impact with the specified entity, surface material, and damage type
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, int iDamageType );
#endif

// Basic decal handling
// Returns true if it hit something
enum
{
	IMPACT_NODECAL = 0x1,
	IMPACT_REPORT_RAGDOLL_IMPACTS = 0x2,
};

#ifdef OFFSHORE_DLL
bool Impact( Vector &vecOrigin, Vector &vecStart, int iMaterial, CUtlVector<int> *hDamageType, int iHitbox, C_BaseEntity *pEntity, trace_t &tr, int nFlags = 0, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );
#else
bool Impact( Vector &vecOrigin, Vector &vecStart, int iMaterial, int iDamageType, int iHitbox, C_BaseEntity *pEntity, trace_t &tr, int nFlags = 0, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );
#endif
// Flags for PerformCustomEffects
enum
{
	FLAGS_CUSTIOM_EFFECTS_NOFLECKS = 0x1,
};

// Do spiffy things according to the material hit
void PerformCustomEffects( const Vector &vecOrigin, trace_t &tr, const Vector &shotDir, int iMaterial, int iScale, int nFlags = 0 );

// Play the correct impact sound according to the material hit
void PlayImpactSound( C_BaseEntity *pServerEntity, trace_t &tr, Vector &vecServerOrigin, int nServerSurfaceProp );

// This can be used to hook impact sounds and play them at a later time.
// Shotguns do this so it doesn't play 10 identical sounds in the same spot.
typedef void (*ImpactSoundRouteFn)( const char *pSoundName, const Vector &vEndPos );
void SetImpactSoundRoute( ImpactSoundRouteFn fn );

//-----------------------------------------------------------------------------
// Purpose: Enumerator class for ragdolls being affected by bullet forces
//-----------------------------------------------------------------------------
class CRagdollEnumerator : public IPartitionEnumerator
{
public:
	// Forced constructor
#ifdef OFFSHORE_DLL
	CRagdollEnumerator( Ray_t& shot, CUtlVector<int> *hDamageType );
#else
	CRagdollEnumerator( Ray_t& shot, int iDamageType );
#endif

	// Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	bool Hit( void ) const { return m_bHit; }

private:
	Ray_t			m_rayShot;
#ifdef OFFSHORE_DLL
	CUtlVector<int>	m_hDamageType;
#else
	int				m_iDamageType;
#endif
	bool			m_bHit;
};

#endif // FX_IMPACT_H
