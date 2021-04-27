//========= Grimm Peacock ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "of_fx_shared.h"
#ifdef CLIENT_DLL
#include "c_sdk_player.h"
#else
#include "sdk_player.h"
#include "ai_basenpc.h"
#include "te_effect_dispatch.h"
#endif
#include "sdk_player_shared.h"
#include "in_buttons.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "of_weaponbase_melee.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
do this sometime maybe idk
#if !defined( CLIENT_DLL )		
	#define NetworkPropInt( x )	SendPropInt( SENDINFO(x) )
	#define NetworkPropIntEx( x, y ) SendPropInt( SENDINFO(x), y )
#else
	#define NetworkPropInt( x ) RecvPropInt( RECVINFO(x) )
	#define NetworkPropIntEx( x, y ) RecvPropInt( RECVINFO(x), y )
#endif							
*/
IMPLEMENT_NETWORKCLASS_ALIASED( OFWeaponBaseMelee, DT_OFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( COFWeaponBaseMelee, DT_OFWeaponBaseMelee )
#if !defined( CLIENT_DLL )		
	SendPropInt( SENDINFO( m_iAttackStage ) ),
	SendPropInt( SENDINFO( m_iWeight ) ),
	SendPropFloat( SENDINFO( m_flNextFrame ) )
#else
	RecvPropInt( RECVINFO( m_iAttackStage ) ),
	RecvPropInt( RECVINFO( m_iWeight ) ),
	RecvPropFloat( RECVINFO( m_flNextFrame ) )
#endif		
END_NETWORK_TABLE()

//=========================================================
//	>> COFWeaponBaseMelee
//=========================================================
BEGIN_DATADESC( COFWeaponBaseMelee )
END_DATADESC()

LINK_ENTITY_TO_CLASS( of_weaponbase_melee, COFWeaponBaseMelee );

COFWeaponBaseMelee::COFWeaponBaseMelee()
{
	m_iAttackStage = 0;
	m_iWeight = 0;
	m_flNextFrame = -1;
}

void COFWeaponBaseMelee::ItemBusyFrame( void )
{
	// Don't process frames if we're not in an attack
	if( m_flNextFrame == -1 )
		return BaseClass::ItemBusyFrame();

	int iProcessedEvents = 0;
	// This is here in the rare case that we can do more than 1 Frame per Tick
	while( m_flNextFrame <= gpGlobals->curtime )
	{
		// This only happens on the second loop
		// Which means that we have progressed past a second stage
		if( iProcessedEvents )
		{
			// Go back
			iProcessedEvents--;
			// Process the last Tick
			ProcessHitboxTick();
			// Return to the present
			iProcessedEvents++;
		}

		// We don't check if the duration is 0, since even if it is, there can only be a limited amount of frames, which we will run out of
		iProcessedEvents++;
		m_iAttackStage++;

		if( m_hCurrentAttackData.hFrameData.Count() > m_iAttackStage )
			m_flNextFrame += m_hCurrentAttackData[m_iAttackStage].flDuration;
		else
			break;
	}

	// We're out of frames, stop our hitbox processing
	if( m_iAttackStage >= m_hCurrentAttackData.hFrameData.Count() )
	{
		m_flNextFrame = -1;
		m_iAttackStage = 0;

		return BaseClass::ItemBusyFrame();
	}

	// Process the current hitbox frame
	ProcessHitboxTick();
	/*
	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        WeaponSound(SINGLE, m_flNextPrimaryAttack);
        m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
        iBulletsToFire++;
    }*/
}

void COFWeaponBaseMelee::ItemPostFrame( void )
{
	CSDKPlayer *pPlayer = GetSDKPlayerOwner();
	if( !pPlayer )
		return;

	FOR_EACH_VEC( GetSDKWpnData().m_hBaseMeleeAttacks, i )
	{
		if( pPlayer->m_nButtons & GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iInput )
		{
			m_hCurrentAttackData = GetSDKWpnData().m_hHitBallAttack[GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iAttackData];
#ifdef GAME_DLL
			m_iWeight = GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iWeight;
			SendWeaponAnim( GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iVMAct );
			pPlayer->DoAnimationEvent( GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iTPAct );

			m_flNextPrimaryAttack = gpGlobals->curtime + m_hCurrentAttackData.flTotalDuration;

			// Restart our Frame Data
			m_iAttackStage = 0;
			// Start immediately
			m_flNextFrame = gpGlobals->curtime + m_hCurrentAttackData[m_iAttackStage].flDuration;

			// Next time iten post frame will be called
			pPlayer->m_flNextAttack = m_flNextPrimaryAttack;
			m_hHitEntities.Purge();
#else
			m_hHitSurfaces.Purge();
#endif
			// Call ItemBusyFrame here so there wont be a 1 tick delay?
			return;
		}
	}

	if( !ReloadOrSwitchWeapons() )
	{
		WeaponIdle();
	}
}

void COFWeaponBaseMelee::PrimaryAttack( void )
{
	// Handled elsewhere
}

void COFWeaponBaseMelee::ProcessHitboxTick( void )
{
	CBasePlayer *pPlayer = GetPlayerOwner();

	if( !pPlayer )
		return;

	int iSphereCount = m_hCurrentAttackData[m_iAttackStage].radius.Count();
	for( int i = 0; i < iSphereCount; i++ )
	{
		Vector forward, right, up;
		AngleVectors( pPlayer->EyeAngles(), &forward, &right, &up );

		Vector vecOrigin = pPlayer->EyePosition() +
			( m_hCurrentAttackData[m_iAttackStage].pos[i].x * forward ) +
			( m_hCurrentAttackData[m_iAttackStage].pos[i].y * right ) +
			( m_hCurrentAttackData[m_iAttackStage].pos[i].z * up );

		float flRadius = m_hCurrentAttackData[m_iAttackStage].radius[i];
#ifdef GAME_DLL
		NDebugOverlay::Sphere( vecOrigin, flRadius, 0, 255, 0, false, gpGlobals->frametime * 2 );
#endif

#ifdef CLIENT_DLL
		CTraceFilterSkipTwoEntities filter;
		filter.SetPassEntity( this );
		filter.SetPassEntity2( pPlayer );

		Vector vecNormal = vecOrigin - pPlayer->EyePosition();
		float flLenght = vecNormal.Length() + flRadius;
		VectorNormalize(vecNormal);

		trace_t tr;
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + (vecNormal*flLenght),
			MASK_ALL, &filter, &tr );

		if( !m_hHitSurfaces.HasElement( tr.worldSurfaceIndex ) )
		{
			CUtlVector<int> hDamageType;
			hDamageType.AddToTail( DMG_BULLET );

			UTIL_ImpactTrace( &tr, &hDamageType );
			m_hHitSurfaces.AddToTail(tr.worldSurfaceIndex);
		}
#else
		CBaseEntity *pEntities[64];
		int iCount = UTIL_EntitiesInSphere( pEntities, 64, vecOrigin, flRadius, MASK_ALL );

		for( int iEntNum = 0; iEntNum < iCount; iEntNum++ )
		{
			CBaseEntity *pEnt = pEntities[iEntNum];
			if( !pEnt )
				continue;

			// Don't hit the same entity twice
			if( m_hHitEntities.HasElement( pEnt->entindex() )
				|| pEnt == this		// Don't hit ourselves
				|| pEnt == pPlayer // Don't hit our player
				)
				continue;
			
			CTraceFilterSkipTwoEntities filter;
			filter.SetPassEntity( this );
			filter.SetPassEntity2( pPlayer );

			trace_t tr;
			UTIL_TraceLine( pPlayer->EyePosition(), pEnt->GetAbsOrigin(),
				MASK_ALL, &filter, &tr );

			Vector vecForward;
			pPlayer->EyeVectors(&vecForward);

			CTakeDamageInfo info( pPlayer, pPlayer, this, GetDamage(), GetDamageTypes()/*, iCustomDamage*/ );
			CalculateMeleeDamageForce( &info, vecForward, pEnt->GetAbsOrigin(), 1.0f / GetDamage() * 80.0f );
			pEnt->DispatchTraceAttack(info, vecForward, &tr);
			ApplyMultiDamage();

			m_hHitEntities.AddToTail( pEnt->entindex() );

			DevMsg( "%s\n", pEnt->GetClassname() );
		}
#endif
	}
}