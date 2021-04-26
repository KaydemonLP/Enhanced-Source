#ifndef OF_WEAPONBASE_RANGED_H
#define OF_WEAPONBASE_RANGED_H
#ifdef _WIN32
#pragma once
#endif

#include "of_weaponbase.h"

#ifdef CLIENT_DLL
#define COFWeaponBaseRanged C_OFWeaponBaseRanged
#endif

//=========================================================
// Gun base class
//=========================================================
class COFWeaponBaseRanged : public CBaseSDKCombatWeapon
{
public:
	DECLARE_CLASS( COFWeaponBaseRanged, CBaseSDKCombatWeapon );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
//	DECLARE_PREDICTABLE();

	COFWeaponBaseRanged( void );

	virtual bool	Deploy( void );
	virtual void	ItemPostFrame( void );

	virtual void	PrimaryAttack( void );
	//virtual void	SecondaryAttack( void );
	
	virtual void	FireRangedAttack();

	virtual void	FireBullets( const FireBulletsInfo_t &info );

	// utility function
	static void		DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

	virtual float	GetSpread( void );
	virtual int		GetProjectileType( void );

protected:
};

#endif