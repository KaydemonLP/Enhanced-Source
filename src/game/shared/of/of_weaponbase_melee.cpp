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
	ProcessHitboxTick();
}

void COFWeaponBaseMelee::ItemPostFrame( void )
{
	CSDKPlayer *pPlayer = GetSDKPlayerOwner();
	if( !pPlayer )
		return;

#ifdef GAME_DLL
	FOR_EACH_VEC( GetSDKWpnData().m_hBaseMeleeAttacks, i )
	{
		if( pPlayer->m_nButtons & GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iInput )
		{
			m_iWeight = GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iWeight;
			m_hCurrentAttackData = GetSDKWpnData().m_hHitBallAttack[GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iAttackData];
			SendWeaponAnim( GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iVMAct );
			pPlayer->DoAnimationEvent( GetSDKWpnData().m_hBaseMeleeAttacks[i].m_iTPAct );

			m_flNextPrimaryAttack = gpGlobals->curtime + m_hCurrentAttackData.flTotalDuration;

			// Restart our Frame Data
			m_iAttackStage = -1;
			m_flNextFrame = 0;

			// Next time iten post frame will be called
			pPlayer->m_flNextAttack = m_flNextPrimaryAttack;
			return;
		}
	}

	if( !ReloadOrSwitchWeapons() )
	{
		WeaponIdle();
	}
#endif
}

void COFWeaponBaseMelee::ProcessHitboxTick( void )
{
#ifdef GAME_DLL
	CBasePlayer *pPlayer = GetPlayerOwner();

	if( m_flNextFrame == -1 || !pPlayer || !m_hCurrentAttackData.hFrameData.Count() )
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

		NDebugOverlay::Sphere( vecOrigin, m_hCurrentAttackData[m_iAttackStage].radius[i], 0, 255, 0, false, gpGlobals->frametime * 2 );
	}
#endif
}

void COFWeaponBaseMelee::PrimaryAttack( void )
{

}