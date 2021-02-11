//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once

#include "networkvar.h"

#include "smmod/weapon_parse_custom_weapon.h"

//--------------------------------------------------------------------------------------------------------
class CSDKWeaponInfo : public CustomWeaponInfo
{
public:
	DECLARE_CLASS_GAMEROOT( CSDKWeaponInfo, CustomWeaponInfo );
	
	CSDKWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );


public:
	//  Ammo
	int		m_iReserveAmmo;
	
	// Fire Things
	float	m_flSpread;
	float	m_flFireRate;
	int		m_iProjectileType;
	
	// Damage/OnHit things
	int		m_iDamage;
	
	// Timers
	float	m_flDrawTime;

	bool	m_bReloadSingly;
	float	m_flReloadStartTime;
	float	m_flReloadTime;

	// Visuals
	int		m_iHandHolding;
};