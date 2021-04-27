#ifndef OF_WEAPONBASE_MELEE_H
#define OF_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "of_weaponbase.h"

#ifdef CLIENT_DLL
#define COFWeaponBaseMelee C_OFWeaponBaseMelee
#endif

//=========================================================
// Melee
//=========================================================
class COFWeaponBaseMelee : public CBaseSDKCombatWeapon
{
public:
	DECLARE_CLASS( COFWeaponBaseMelee, CBaseSDKCombatWeapon );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
//	DECLARE_PREDICTABLE();

	COFWeaponBaseMelee( void );

	virtual void	ItemBusyFrame( void );
	virtual void	ItemPostFrame( void );
	virtual void	ProcessHitboxTick( void );

	virtual void	PrimaryAttack( void );
	//virtual void	SecondaryAttack( void );

	virtual bool	HasAmmo( void ){ return true; }
	virtual bool	HasAnyAmmo( void ){ return true ; }

protected:
	hitsphere_attack_t m_hCurrentAttackData;
#ifdef CLIENT_DLL
	CUtlVector<unsigned short> m_hHitSurfaces;
#else
	CUtlVector<int> m_hHitEntities;
#endif
	CNetworkVar( int, m_iAttackStage );
	CNetworkVar( int, m_iWeight );

	CNetworkVar( float, m_flNextFrame );
	
};

#endif