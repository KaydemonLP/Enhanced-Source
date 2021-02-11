//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "sdk_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CSDKWeaponInfo;
}

CSDKWeaponInfo::CSDKWeaponInfo()
{
}


void CSDKWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );
	
	//  Ammo
	m_iReserveAmmo = pKeyValuesData->GetInt( "reserve_ammo" ); // Max Reserve Ammo
	
	// Fire Things
	m_flSpread			= pKeyValuesData->GetFloat( "spread" );		// How much of an angle boolets/projectiles can deviate from the crosshair
	m_flFireRate		= pKeyValuesData->GetFloat( "fire_rate" ); // Time between 2 shots
	m_iProjectileType	= pKeyValuesData->GetInt( "projectile_type" ); // What kind of projectile it shoots, 0 is always Hitscan Bullet
	// TODO: Create Translation Table
	/* TODO: Required projectiles: 	0 - Bullet
									1 - Dynamite
									2 - Projectile Bullet / Laser 
									*/

	// Damage/OnHit things
	m_iDamage = pKeyValuesData->GetInt( "damage", 10 ); // 10 damage cuz fuck you

	// Timers
	// TODO: For now just use what tf2 uses, mess around with timings later
	m_flDrawTime = pKeyValuesData->GetFloat( "draw_time", 0.67f );	// Time it takes for a weapon to activate after being drawn
	
	m_bReloadSingly 	= pKeyValuesData->GetBool ( "reload_singly" );	// Weather or not a weapon reloads its whole clip or one by one
	m_flReloadStartTime = pKeyValuesData->GetFloat( "reload_start" ); 	// Time it takes for the initial reload, only relevant to Reload Singly weapons
	m_flReloadTime 		= pKeyValuesData->GetFloat( "reload" );			// Time it takes to reload

	// Visuals
	if( pKeyValuesData->GetBool( "single_handed" ) )		// Weapon is held with one hand when idle ( may use both when reloading )
		m_iHandHolding = OF_HANDLING_SINGLE;                // 
	else if( pKeyValuesData->GetBool( "akimbo_handed" ) )	// Weapon uses both hands to hold 2 different items
		m_iHandHolding = OF_HANDLING_AKIMBO;                // 
	else													// Weapon is held with both hands at all times
		m_iHandHolding = OF_HANDLING_TWO_HANDED;            // 
}


