//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "basecombatweapon_shared.h"
#include "sdk_weapon_parse.h"

#pragma once

#define CROSSHAIR_CONTRACT_PIXELS_PER_SECOND	7.0f

#if defined( CLIENT_DLL )
#define CBaseSDKCombatWeapon C_BaseSDKCombatWeapon
#endif

class CBasePlayer;
#ifdef GAME_DLL
class CSDKPlayer;
#else
class C_SDKPlayer;
#define CSDKPlayer C_SDKPlayer
#endif
class CBaseCombatCharacter;

class CBaseSDKCombatWeapon : public CBaseCombatWeapon
{
#if !defined( CLIENT_DLL )
#ifndef _XBOX
	DECLARE_DATADESC();
#else
protected:
	DECLARE_DATADESC();
private:
#endif
#endif

	DECLARE_CLASS( CBaseSDKCombatWeapon, CBaseCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseSDKCombatWeapon();
	~CBaseSDKCombatWeapon();

	virtual bool IsPredicted( void ) const { return true; }

	virtual void Precache( void );

	CBasePlayer* GetPlayerOwner() const;
	CSDKPlayer*	 GetSDKPlayerOwner() const;

	virtual bool	WeaponShouldBeLowered( void );

			bool	CanLower();
	virtual bool	Ready( void );
	virtual bool	Lower( void );
	virtual bool	Deploy( void );
	virtual void	DryFire( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponIdle( void );
	virtual void	CheckReload( void );
	virtual void	ItemPostFrame( void );

	virtual bool	Reload( void );
	virtual void	FinishReload( void );

	virtual void	PrimaryAttack( void );						// do "+ATTACK"
	//virtual void	SecondaryAttack( void ) { return; }			// do "+ATTACK2"

	virtual bool	HasSecondaryAmmo( void );
	virtual bool	HasPrimaryAmmo( void );

	virtual int		GetReserveAmmo( void );

	virtual void	RemoveFromClip1( int iAmount );
	virtual void	RemoveFromClip2( int iAmount );

	virtual void	RemoveFromReserveAmmo( int iAmount );

	virtual int		GetMaxReserveAmmo( void );

	virtual float	GetDamage( void );
	virtual float	GetDrawTime( void );
	virtual float	GetFireRate( void );
	virtual float	GetReloadStartTime( void );
	virtual float	GetReloadTime( void );
	virtual bool	ReloadsSingly( void );
	virtual Vector	GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float	GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const	WeaponProficiencyInfo_t *GetProficiencyValues();
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	virtual void	SetPickupTouch( void );
	virtual void	ItemHolsterFrame( void );
	void			SetTouchPickup( bool bForcePickup ) { m_bForcePickup = bForcePickup; }
	//void			WeaponAccuracyPenalty( float flPenAccuracy ){ flPenAccuracy = m_flPenAccuracy; }

	CSDKWeaponInfo const	&GetSDKWpnData() const;
	virtual void FireBullets( const FireBulletsInfo_t &info );

	virtual void			Equip( CBaseCombatCharacter *pOwner );

	virtual void			OnPickedUp( CBaseCombatCharacter *pNewOwner );

#ifdef CLIENT_DLL
	virtual void			PostDataUpdate( DataUpdateType_t updateType );
#endif

	IPhysicsConstraint		*GetConstraint() { return m_pConstraint; }

public:
	CNetworkVar( int, m_iReserveAmmo );

private:
	WEAPON_FILE_INFO_HANDLE	m_hWeaponFileInfo;
	IPhysicsConstraint		*m_pConstraint;

protected:
	CSDKWeaponInfo *m_pWeaponInfo;

	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered
	bool			m_bForcePickup;

	CNetworkVar( int, m_iReloadStage );
};