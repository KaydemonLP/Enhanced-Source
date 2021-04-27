//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "sdk_weapon_parse.h"
#include "ammodef.h"
#include "sdk_shareddefs.h"
#include "in_buttons.h"
#include "activitylist.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CSDKWeaponInfo;
}

CSDKWeaponInfo::CSDKWeaponInfo()
{
	m_iReserveAmmo = 0;
	
	// Fire Things
	m_flSpread = 0;
	m_flFireRate = 0;
	m_iBulletsPerShot = 1;
	m_iProjectileType = OF_PROJECTILE_BULLET;
	
	// Damage/OnHit things
	m_iDamage = 10;
	
	// Timers
	m_flDrawTime = 0.67f;

	m_bReloadSingly = false;
	m_flReloadStartTime = -1;
	m_flReloadTime = -1;

	// Visuals
	m_iHandHolding = OF_HANDLING_TWO_HANDED;
	m_szAnimExtension[0] = '\0';
}

void CSDKWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );
	
	//  Ammo
	m_iReserveAmmo = pKeyValuesData->GetInt( "reserve_ammo" ); // Max Reserve Ammo
	
	// Fire Things
	m_flSpread			= pKeyValuesData->GetFloat( "spread" );		// How much of an angle boolets/projectiles can deviate from the crosshair
	m_flFireRate		= pKeyValuesData->GetFloat( "fire_rate" ); // Time between 2 shots
	m_iBulletsPerShot	= pKeyValuesData->GetFloat( "bullets_per_shot" );
	// What kind of projectile it shoots, 0 is always Hitscan Bullet
	m_iProjectileType = UTIL_StringFieldToInt( pKeyValuesData->GetString("projectile_type"), g_szProjectileTypes, OF_PROJECTILE_COUNT );

	// Damage/OnHit things
	m_iDamage = pKeyValuesData->GetInt( "damage", 10 ); // 10 damage cuz fuck you

	// Timers
	// TODO: For now just use what tf2 uses, mess around with timings later
	m_flDrawTime = pKeyValuesData->GetFloat( "draw_time", 0.67f );	// Time it takes for a weapon to activate after being drawn
	
	m_bReloadSingly 	= pKeyValuesData->GetBool ( "reload_singly" );	// Wether or not a weapon reloads its whole clip or one by one
	m_flReloadStartTime = pKeyValuesData->GetFloat( "reload_start", -1 ); 	// Time it takes for the initial reload, only relevant to Reload Singly weapons
	m_flReloadTime 		= pKeyValuesData->GetFloat( "reload", -1 );			// Time it takes to reload

	// Visuals
	if( pKeyValuesData->GetBool( "single_handed" ) )		// Weapon is held with one hand when idle ( may use both when reloading )
		m_iHandHolding = OF_HANDLING_SINGLE;                // 
	else if( pKeyValuesData->GetBool( "akimbo_handed" ) )	// Weapon uses both hands to hold 2 different items
		m_iHandHolding = OF_HANDLING_AKIMBO;                // 
	else													// Weapon is held with both hands at all times
		m_iHandHolding = OF_HANDLING_TWO_HANDED;            // 

	Q_strncpy( m_szAnimExtension, pKeyValuesData->GetString( "anim_extension", "pistol" ), sizeof(m_szAnimExtension) );


	// Hit sphere data specific for Melees
	// However saved for all weapon types in case needed

	// Clean up in case of re-parsing
	m_hHitBallAttack.Purge();
	m_hBaseMeleeAttacks.Purge();
	m_hSpecialMeleeAttacks.Purge();

	int i = 0;
	KeyValues *pSphereAttackData = pKeyValuesData->FindKey( UTIL_VarArgs("SphereAttackData%d", i) );
	while( pSphereAttackData )
	{
		hitsphere_attack_t hSphereAttackData;
		FOR_EACH_TRUE_SUBKEY( pSphereAttackData, kvFrame )
		{
			hitsphere_attack_t::frames_data_t hFrame;
			hFrame.flDuration = kvFrame->GetFloat( "duration", 0 );
			hSphereAttackData.flTotalDuration += hFrame.flDuration;

			// We don't require frames to have spheres in case we want blank periods between frames
			KeyValues *pSpheres = kvFrame->FindKey( "Spheres" );
			if( pSpheres )
			{
				FOR_EACH_VALUE( pSpheres, kvValue )
				{
					CCommand vecParser;
					vecParser.Tokenize( kvValue->GetName() );
					if( vecParser.ArgC() != 3 )
						continue;

					hFrame.pos.AddToTail(Vector( atof(vecParser[0]), atof(vecParser[1]), atof(vecParser[2]) ));
					hFrame.radius.AddToTail( atof(kvValue->GetString()) );
				}
			}

			hSphereAttackData.AddToTail( hFrame );
		}

		// Only add if we actually have frame data
		if( hSphereAttackData.hFrameData.Count() )
			m_hHitBallAttack.AddToTail( hSphereAttackData );
		else
			Warning( "%s: Sphere data defined but no frames loaded!\n", szWeaponName );

		i++;
		pSphereAttackData = pKeyValuesData->FindKey( UTIL_VarArgs("SphereAttackData%d", i) );
	}

	i = 0;
	KeyValues *pMeleeAttacks = pKeyValuesData->FindKey( UTIL_VarArgs("MeleeAttack%d", i) );
	while( pMeleeAttacks )
	{
		melee_attack_t hMeleeAttack;
		hMeleeAttack.m_iAttackData = pMeleeAttacks->GetInt( "AttackData", 0 );

		CCommand inputParser;
		inputParser.Tokenize( pMeleeAttacks->GetString( "Input" ) );

		for( int y = 0; y < inputParser.ArgC(); y++ )
		{
			hMeleeAttack.m_iInput |= ( 1 << UTIL_StringFieldToInt( inputParser[y], g_szInputNames, IN_BUTTON_COUNT ) );
		}

		hMeleeAttack.m_iVMAct = ActivityList_IndexForName( pMeleeAttacks->GetString( "ViewmodelAct", '\0' ) );
		hMeleeAttack.m_iTPAct = (PlayerAnimEvent_t)UTIL_StringFieldToInt(pMeleeAttacks->GetString("ThirdPersonAct", '\0'), g_szAnimEventName, PLAYERANIMEVENT_COUNT);
		hMeleeAttack.m_iWeight = pMeleeAttacks->GetInt("Weight", 0);

		// Only add it if we have the appropriate data
		if( hMeleeAttack.m_iAttackData < m_hHitBallAttack.Count() )
		{
			if( inputParser.ArgC() == 1 )
				m_hBaseMeleeAttacks.AddToTail( hMeleeAttack );
			else
				m_hSpecialMeleeAttacks.AddToTail( hMeleeAttack );
		}
		else
			Warning( "%s: Melee Attack defined but no valid attack data selected!\n", szWeaponName );
		i++;
		pMeleeAttacks = pKeyValuesData->FindKey( UTIL_VarArgs("MeleeAttack%d", i) );
	}
}


