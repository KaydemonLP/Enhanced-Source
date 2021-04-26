#include "cbase.h"
#include "fx_impact.h"
#include "tier0/vprof.h"

// There is no base implementation of the generic impact effect. This has been copied from HL2.
void ImpactCallback( const CEffectData &data )
{
	VPROF_BUDGET( "ImpactCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iHitbox;
#ifdef OFFSHORE_DLL
	CUtlVector<int> hDamageType;
#else
	int iDamageType;
#endif
	short nSurfaceProp;
#ifdef OFFSHORE_DLL
	C_BaseEntity* pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, &hDamageType, iHitbox );
#else
	C_BaseEntity* pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );
#endif
	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound
#ifdef OFFSHORE_DLL
	if ( Impact( vecOrigin, vecStart, iMaterial, &hDamageType, iHitbox, pEntity, tr ) )
#else
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
#endif	
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1.0 );
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

#ifndef SWARM_DLL
DECLARE_CLIENT_EFFECT( "Impact", ImpactCallback );
#else
DECLARE_CLIENT_EFFECT( Impact, ImpactCallback );
#endif



