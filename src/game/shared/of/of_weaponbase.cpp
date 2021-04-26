//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sdk_player_shared.h"
#include "of_weaponbase.h"
#include "basecombatweapon_shared.h"
#include "in_buttons.h"
#include "sdk_weapon_parse.h"
#include "weapon_parse.h"
#include "physics_saverestore.h"
#include "of_shared_schemas.h"
#include "datacache/imdlcache.h"
#include "activitylist.h"

#if defined( CLIENT_DLL )

	#include "c_baseentity.h"
	#include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	//#include "c_sdk_player_sp.h"
	#include "hud_crosshair.h"

#else
	#include "baseentity.h"
	//#include "sdk_player_sp.h"
	#include "vphysics/constraints.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( of_weaponbase, CBaseSDKCombatWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseSDKCombatWeapon , DT_BaseSDKCombatWeapon )

BEGIN_NETWORK_TABLE( CBaseSDKCombatWeapon , DT_BaseSDKCombatWeapon )
#if !defined( CLIENT_DLL )
	SendPropInt( SENDINFO( m_iReserveAmmo ) ),
	SendPropInt( SENDINFO( m_iReloadStage ) ),
//	SendPropInt( SENDINFO( m_bReflectViewModelAnimations ), 1, SPROP_UNSIGNED ),
#else
//	RecvPropInt( RECVINFO( m_bReflectViewModelAnimations ) ),
	RecvPropInt( RECVINFO( m_iReserveAmmo ) ),
	RecvPropInt( RECVINFO( m_iReloadStage ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
	ConVar cl_crosshaircolor( "cl_crosshaircolor", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	ConVar cl_dynamiccrosshair( "cl_dynamiccrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	ConVar cl_scalecrosshair( "cl_scalecrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	ConVar cl_crosshairscale( "cl_crosshairscale", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	ConVar cl_crosshairalpha( "cl_crosshairalpha", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
	ConVar cl_crosshairusealpha( "cl_crosshairusealpha", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
#endif

#if !defined( CLIENT_DLL )

#include "globalstate.h"

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseSDKCombatWeapon )

	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_FIELD( m_bLowered,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),

END_DATADESC()

#endif

BEGIN_PREDICTION_DATA( CBaseSDKCombatWeapon )
END_PREDICTION_DATA()

ConVar sk_auto_reload_time( "sk_auto_reload_time", "3", FCVAR_REPLICATED );

CBaseSDKCombatWeapon::CBaseSDKCombatWeapon()
{
#if !defined( CLIENT_DLL )
	m_pConstraint = NULL;
	OnBaseCombatWeaponCreated( this );
#endif

	m_bForcePickup = true;

	m_iReloadStage = OF_RELOAD_STAGE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseSDKCombatWeapon::~CBaseSDKCombatWeapon( void )
{
#if !defined( CLIENT_DLL )
	//Remove our constraint, if we have one
	if ( m_pConstraint != NULL )
	{
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
	OnBaseCombatWeaponDestroyed( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpoise: Basically used to load the proper weapon info
//				It's a bit hacky, but not all that bad
//				If it causes problems, make this a copy of the basecombatweapon precache
//				But with GetClassname() replaced with GetSchemaName()
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::Precache( void )
{
	// CLIENT DOES THE PRECACHE FUNCTIONS IN UPDATE CLIENT DATA
	// BECAUSE THE SCHEMA NAME DOESN'T GET NETWORKED AT THIS POINT!
#ifdef GAME_DLL
	BaseClass::Precache();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::SetPickupTouch( void )
{
#if !defined( CLIENT_DLL )

	if( !m_bForcePickup )
	{	
		return;
	}

	SetTouch( &CBaseCombatWeapon::DefaultTouch );

	if ( gpGlobals->maxClients > 1 )
	{
		if ( GetSpawnFlags() & SF_NORESPAWN )
		{
			SetThink( &CBaseEntity::SUB_Remove );
			SetNextThink( gpGlobals->curtime + 30.0f );
		}
	}

	BaseClass::SetPickupTouch();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Become a child of the owner (MOVETYPE_FOLLOW)
//			disables collisions, touch functions, thinking
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	// Attach the weapon to an owner
	SetAbsVelocity( vec3_origin );
	RemoveSolidFlags( FSOLID_TRIGGER );
	FollowEntity( pOwner );
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	// Break any constraint I might have to the world.
	RemoveEffects( EF_ITEM_BLINK );

#if !defined( CLIENT_DLL )

	if ( m_pConstraint != NULL )
	{
		RemoveSpawnFlags( SF_WEAPON_START_CONSTRAINED );
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
#endif


	m_flNextPrimaryAttack		= gpGlobals->curtime;
	m_flNextSecondaryAttack		= gpGlobals->curtime;
	SetTouch( NULL );
	SetThink( NULL );
#if !defined( CLIENT_DLL )
	VPhysicsDestroyObject();
#endif

	if ( pOwner->IsPlayer() )
	{
		SetModel( GetViewModel() );
	}
	else
	{
		// Make the weapon ready as soon as any NPC picks it up.
		m_flNextPrimaryAttack = gpGlobals->curtime;
		m_flNextSecondaryAttack = gpGlobals->curtime;
		SetModel( GetWorldModel() );
	}

}

//#ifndef SWARM_DLL // Use base instead.
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
#if !defined( CLIENT_DLL )
	RemoveEffects( EF_ITEM_BLINK );

	if( pNewOwner->IsPlayer() )
	{
		m_OnPlayerPickup.FireOutput(pNewOwner, this);

		// Play the pickup sound for 1st-person observers
		CRecipientFilter filter;
		for ( int i=1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *player = UTIL_PlayerByIndex(i);
			if ( player && !player->IsAlive() && player->GetObserverMode() == OBS_MODE_IN_EYE )
			{
				filter.AddRecipient( player );
			}
		}
		if ( filter.GetRecipientCount() )
		{

#ifndef WEAPON_QUIET_PICKUP
			// Quiet the pickup sound for Vectronic.
			CBaseEntity::EmitSound( filter, pNewOwner->entindex(), "Player.PickupWeapon" );
#endif
		}

		// Robin: We don't want to delete weapons the player has picked up, so 
		// clear the name of the weapon. This prevents wildcards that are meant 
		// to find NPCs finding weapons dropped by the NPCs as well.
		SetName( NULL_STRING );
	}
	else
	{
		m_OnNPCPickup.FireOutput(pNewOwner, this);
	}

#ifdef HL2MP
	HL2MPRules()->RemoveLevelDesignerPlacedObject( this );
#endif

	// Someone picked me up, so make it so that I can't be removed.
	SetRemoveable( false );
#endif
}
//#endif
#ifdef CLIENT_DLL
void CBaseSDKCombatWeapon::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if( updateType == DATA_UPDATE_CREATED && !IsClientCreated() )
	{
		BaseClass::Precache();
	}
}
#endif
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Just load the clip with no animations
		FinishReload();
		m_flHolsterTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::CanLower()
{
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Drops the weapon into a lowered pose
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::Lower( void )
{
	//Don't bother if we don't have the animation
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Brings the weapon up to the ready position
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::Ready( void )
{
	//Don't bother if we don't have the animation
	if( SelectWeightedSequence( ACT_VM_LOWERED_TO_IDLE ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = false;	
	m_flRaiseTime = gpGlobals->curtime + 0.5f;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::Deploy( void )
{

#ifndef CLIENT_DLL
	CBasePlayer *pFakePlayer = GetPlayerOwner();
	CSDKPlayer *pPlayer = ToSDKPlayer(pFakePlayer);

	if ( pPlayer )
	{
		pPlayer->m_iShotsFired = 0;
	}
#endif

	m_bLowered = false;

	bool bRet = BaseClass::Deploy();

	if( bRet )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( pOwner )
		{
			pOwner->SetNextAttack( gpGlobals->curtime + GetDrawTime() );
		}

		// Can't shoot again until we've finished deploying
		m_flNextPrimaryAttack = gpGlobals->curtime + GetDrawTime();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetDrawTime();
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::Reload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if( !pOwner )
		return false;

	// If I don't have any spare ammo, I can't reload
	if ( !GetReserveAmmo() )
		return false;

	bool bReload = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary	= MIN(GetMaxClip1() - m_iClip1, GetReserveAmmo());
		if ( primary != 0 )
		{
			bReload = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = MIN(GetMaxClip2() - m_iClip2, GetReserveAmmo/*2*/());
		if ( secondary != 0 )
		{
			bReload = true;
		}
	}

	if ( !bReload )
		return false;

#ifdef CLIENT_DLL
	// Play reload
	WeaponSound( RELOAD );
#endif
	int iActivity = ACT_VM_RELOAD;
	
	int g_ReloadSinglyActTable[] = {
		ACT_VM_RELOAD_START,// OF_RELOAD_STAGE_NONE
		ACT_VM_RELOAD,// OF_RELOAD_STAGE_START
		ACT_VM_RELOAD,// OF_RELOAD_STAGE_LOOP
		ACT_VM_RELOAD_END// OF_RELOAD_STAGE_END
	};

	if( ReloadsSingly() )
		iActivity = g_ReloadSinglyActTable[m_iReloadStage];

	DevMsg( "Playing VM activity: %s\n", ActivityList_NameForIndex(iActivity) );

	SendWeaponAnim( iActivity );

	// Play the player's reload animation
	if ( pOwner->IsPlayer() )
	{
		( ( CBasePlayer * )pOwner)->SetAnimation( PLAYER_RELOAD );
	}

	MDLCACHE_CRITICAL_SECTION();
	float flAttackEndTime = gpGlobals->curtime + GetReloadTime();
	if( ReloadsSingly() && m_iReloadStage == OF_RELOAD_STAGE_NONE )
	{
		flAttackEndTime = gpGlobals->curtime + GetReloadStartTime();
	}

	pOwner->SetNextAttack( flAttackEndTime );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = flAttackEndTime;

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::PrimaryAttack( void )
{
	// Empty For now
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::HasPrimaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo1() )
	{
		if ( m_iClip1 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	if ( GetReserveAmmo() > 0 ) 
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::HasSecondaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo2() )
	{
		if ( m_iClip2 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	if ( GetReserveAmmo() > 0 ) 
		return true;

	return false;
}

int	CBaseSDKCombatWeapon::GetReserveAmmo( void )
{
	return m_iReserveAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: Remove a certain amount of ammo from the Clip
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::RemoveFromClip1( int iAmount )
{
	m_iClip1 -= iAmount;
}

//-----------------------------------------------------------------------------
// Purpose: Likewise but for Clip2
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::RemoveFromClip2( int iAmount )
{
	m_iClip2 -= iAmount;
}


void CBaseSDKCombatWeapon::RemoveFromReserveAmmo( int iAmount )
{
	m_iReserveAmmo -= iAmount;
}

int	CBaseSDKCombatWeapon::GetMaxReserveAmmo( void )
{
	return GetSDKWpnData().m_iReserveAmmo;
}

//====================================================================================
// WEAPON RELOAD TYPES
//====================================================================================
void CBaseSDKCombatWeapon::CheckReload( void )
{
	if ( ReloadsSingly() )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
		{
			if( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && m_iClip1 > 0 )
			{
				m_bInReload = false;
				return;
			}

			if( m_iReloadStage == OF_RELOAD_STAGE_NONE )
			{
				m_iReloadStage++;
				Reload();
				return;
			}

			// If out of ammo end reload
			if( !GetReserveAmmo() )
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if( m_iClip1 < GetMaxClip1() )
			{
				// Add them to the clip
				m_iClip1 += 1;
				RemoveFromReserveAmmo( 1 );

				m_iReloadStage = OF_RELOAD_STAGE_LOOP;

				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			// If I use primary clips, reload primary
			if ( UsesClipsForAmmo1() )
			{
				int primary	= MIN( GetMaxClip1() - m_iClip1, GetReserveAmmo() );	
				m_iClip1 += primary;
				RemoveFromReserveAmmo( primary );
			}

			// If I use secondary clips, reload secondary
			if ( UsesClipsForAmmo2() )
			{
				int secondary = MIN( GetMaxClip2() - m_iClip2, GetReserveAmmo/*2*/() );
				m_iClip2 += secondary;
				RemoveFromReserveAmmo/*2*/( secondary );
			}

			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::FinishReload( void )
{
	if ( ReloadsSingly() )
	{
		// In case there's no end anim, don't just skip over the reload anim
		m_iReloadStage = OF_RELOAD_STAGE_NONE;

		if( GetPlayerOwner() && GetPlayerOwner()->GetActiveWeapon() == this )
		{
			float flBackupIdle = GetWeaponIdleTime();
			SendWeaponAnim( ACT_VM_RELOAD_END );
			SetWeaponIdleTime( gpGlobals->curtime + MAX(SequenceDuration(), flBackupIdle) );
		}
	}

	m_bInReload = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::ItemPostFrame()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if( !pOwner )
		return;

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		if (UsesSecondaryAmmo() && GetReserveAmmo/*2*/() )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if( UsesClipsForAmmo2() )
			{
				// reload clip2 if empty
				if( m_iClip2 < 1 )
				{
					RemoveFromReserveAmmo( 1 );
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	
	if( !bFired && ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && GetReserveAmmo() )) )
		{
			HandleFireOnEmpty();
		}
		else if( pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false )
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if( ( pOwner->m_afButtonPressed & IN_ATTACK ) || ( pOwner->m_afButtonReleased & IN_ATTACK2 ) )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if( (pOwner->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() && !m_bInReload && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if( !( (pOwner->m_nButtons & IN_ATTACK) || ( pOwner->m_nButtons & IN_ATTACK2 ) || ( pOwner->m_nButtons & IN_RELOAD )) )
	{
		// no fire buttons down or reloading
		if ( ( m_bInReload == false ) && !ReloadOrSwitchWeapons() )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get my data in the file weapon info array
//-----------------------------------------------------------------------------
const CSDKWeaponInfo &CBaseSDKCombatWeapon::GetSDKWpnData( void ) const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CSDKWeaponInfo *pSDKInfo;

	#ifdef _DEBUG
		pSDKInfo = dynamic_cast< const CSDKWeaponInfo* >( pWeaponInfo );
		Assert( pSDKInfo );
	#else
		pSDKInfo = static_cast< const CSDKWeaponInfo* >( pWeaponInfo );
	#endif

	return *pSDKInfo;

//	return *GetFileWeaponInfoFromHandle( m_hWeaponFileInfo );
}

CBasePlayer* CBaseSDKCombatWeapon::GetPlayerOwner() const
{
	return dynamic_cast< CBasePlayer* >( GetOwner() );
}

CSDKPlayer* CBaseSDKCombatWeapon::GetSDKPlayerOwner() const
{
	return dynamic_cast< CSDKPlayer* >( GetOwner() );
}

//For some reason, ASW has a problem calling our WeaponData
void CBaseSDKCombatWeapon::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;

	modinfo.m_flPlayerDamage = GetSDKWpnData().m_iDamage;

	BaseClass::FireBullets( modinfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
		m_flHolsterTime = gpGlobals->curtime;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseSDKCombatWeapon::WeaponShouldBeLowered( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CBaseSDKCombatWeapon::WeaponIdle( void )
{
	//See if we should idle high or low
	if ( WeaponShouldBeLowered() )
	{
#if !defined( CLIENT_DLL )
		CSDKPlayer *pPlayer = dynamic_cast<CSDKPlayer*>(GetOwner());

		if( pPlayer )
		{
		//	pPlayer->Weapon_Lower();
		}
#endif

		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED 
			 && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( m_flRaiseTime < gpGlobals->curtime && GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}

float CBaseSDKCombatWeapon::GetDamage( void )
{
	return (float)GetSDKWpnData().m_iDamage;
}

float CBaseSDKCombatWeapon::GetDrawTime( void )
{
	return GetSDKWpnData().m_flDrawTime;
}

float CBaseSDKCombatWeapon::GetFireRate( void )
{
	return GetSDKWpnData().m_flFireRate;
}

float CBaseSDKCombatWeapon::GetReloadStartTime( void )
{
	return GetSDKWpnData().m_flReloadStartTime >= 0 ? GetSDKWpnData().m_flReloadStartTime : SequenceDuration();
}

float CBaseSDKCombatWeapon::GetReloadTime( void )
{
	return GetSDKWpnData().m_flReloadTime >= 0 ? GetSDKWpnData().m_flReloadTime : SequenceDuration();
}

bool CBaseSDKCombatWeapon::ReloadsSingly( void )
{
	return GetSDKWpnData().m_bReloadSingly;
}

float	g_lateralBob;
float	g_verticalBob;

#if defined( CLIENT_DLL ) && ( !defined( HL2MP ) && !defined( PORTAL ) )

//-----------------------------------------------------------------------------
Vector CBaseSDKCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	return BaseClass::GetBulletSpread( proficiency );
}

//-----------------------------------------------------------------------------
float CBaseSDKCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	return BaseClass::GetSpreadBias( proficiency );
}
//-----------------------------------------------------------------------------

const WeaponProficiencyInfo_t *CBaseSDKCombatWeapon::GetProficiencyValues()
{
	return NULL;
}

#else


//-----------------------------------------------------------------------------
Vector CBaseSDKCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	Vector baseSpread = BaseClass::GetBulletSpread( proficiency );

	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	float flModifier = (pProficiencyValues)[ proficiency ].spreadscale;
	return ( baseSpread * flModifier );
}

//-----------------------------------------------------------------------------
float CBaseSDKCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	return (pProficiencyValues)[ proficiency ].bias;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseSDKCombatWeapon::GetProficiencyValues()
{
	return GetDefaultProficiencyValues();
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseSDKCombatWeapon::GetDefaultProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}

#endif

