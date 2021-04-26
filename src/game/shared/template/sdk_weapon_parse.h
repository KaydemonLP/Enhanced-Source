//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once

#include "networkvar.h"

#include "smmod/weapon_parse_custom_weapon.h"


struct hitsphere_attack_t
{
	struct frames_data_t
	{
		frames_data_t()
		{
			flDuration = 0.0f;
		}

		frames_data_t( frames_data_t const& other ) : frames_data_t()
		{
			flDuration = other.flDuration;

			radius = other.radius;
			pos = other.pos;
		}
			
		// How long this frame lasts
		float	flDuration;

		// Radius of the hit sphere
		CUtlVector<float>	radius;
		// Position relative to center of mass and look direction
		CUtlVector<Vector>	pos;
	};

	hitsphere_attack_t()
	{
		flTotalDuration = 0;
	}

	hitsphere_attack_t( hitsphere_attack_t const& other ) : hitsphere_attack_t()
	{
		hFrameData = other.hFrameData;
		flTotalDuration = other.flTotalDuration;
	}

	void AddToTail( frames_data_t hData )
	{
		hFrameData.AddToTail(hData);
	}

	// For quick access to the frames
	frames_data_t& operator[]( int i )
	{
		return hFrameData[i];
	};

	const frames_data_t& operator[]( int i ) const
	{
		return hFrameData[i];
	};

	float flTotalDuration;
	CUtlVector<frames_data_t> hFrameData;
};

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
	int		m_iBulletsPerShot;
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
	char	m_szAnimExtension[128];

	// Hit sphere data specific for Melees
	// However saved for all weapon types in case needed

	struct melee_attack_t
	{
		melee_attack_t()
		{
			m_iInput = 0;
			m_iAttackData = 0;
		}

		int					m_iInput;
		int					m_iAttackData;
		int					m_iVMAct;
		PlayerAnimEvent_t	m_iTPAct;
		int					m_iWeight;
	};

	CUtlVector<hitsphere_attack_t> m_hHitBallAttack;
	CUtlVector<melee_attack_t> m_hBaseMeleeAttacks;
	CUtlVector<melee_attack_t> m_hSpecialMeleeAttacks;
};