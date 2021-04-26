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
#include "of_weaponbase_ranged.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( OFWeaponBaseRanged, DT_OFWeaponBaseRanged )

BEGIN_NETWORK_TABLE( COFWeaponBaseRanged, DT_OFWeaponBaseRanged )
END_NETWORK_TABLE()

//=========================================================
//	>> COFWeaponBaseRanged
//=========================================================
BEGIN_DATADESC( COFWeaponBaseRanged )
END_DATADESC()

LINK_ENTITY_TO_CLASS( of_weaponbase_ranged, COFWeaponBaseRanged );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COFWeaponBaseRanged::COFWeaponBaseRanged( void )
{

}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void COFWeaponBaseRanged::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if( !pPlayer )
		return;
	
	// Abort here to handle burst and auto fire modes
	if ( ( UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) ) )
		return;

	pPlayer->DoMuzzleFlash();

//	Start Lag comp

	FireRangedAttack();

	//Factor in the view kick
	AddViewKick();

	if( !m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

//	End Lag Comp

	// Register a muzzleflash for the AI
#ifdef GAME_DLL
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
#endif
}

void COFWeaponBaseRanged::FireRangedAttack( void )
{
	CSDKPlayer *pPlayer = GetSDKPlayerOwner();
	switch( GetProjectileType() )
	{
		case OF_PROJECTILE_BULLET:
			FX_FireBullets(pPlayer->entindex(),
				pPlayer->EyePosition(),
				pPlayer->EyeAngles(),
				NULL, // Weapon Mode
				GetPredictionRandomSeed(),
				GetDamage(),
				GetSDKWpnData().m_iBulletsPerShot,
				GetSpread(),
				8192);
			break;
	}

	RemoveFromClip1( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void COFWeaponBaseRanged::FireBullets(const FireBulletsInfo_t &info)
{
	if( CBasePlayer *pPlayer = ToBasePlayer ( GetOwner() ) )
	{
		pPlayer->FireBullets(info);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFWeaponBaseRanged::DoMachineGunKick(CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime)
{
	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	
	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	//Wibble left and right
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	//Wobble up and down
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;

	//Clip this to our desired min/max
#ifdef GAME_DLL
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );
#endif
	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: Reset our shots fired
//-----------------------------------------------------------------------------
bool COFWeaponBaseRanged::Deploy(void)
{
	CBasePlayer *pFakePlayer = GetPlayerOwner();
	CSDKPlayer *pPlayer = ToSDKPlayer(pFakePlayer);
	if( !pPlayer )
		return false;

	pPlayer->m_iShotsFired = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFWeaponBaseRanged::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	BaseClass::ItemPostFrame();
}

float COFWeaponBaseRanged::GetSpread( void )
{
	return GetSDKWpnData().m_flSpread;
}

int COFWeaponBaseRanged::GetProjectileType( void )
{
	return GetSDKWpnData().m_iProjectileType;
}