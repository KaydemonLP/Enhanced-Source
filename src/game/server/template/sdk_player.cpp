//=========== Copyright © 2013 - 2014, rHetorical, All rights reserved. =============
//
// Purpose: A useable player simular to the HL2 Player minus the stuff ties with 
// HL2.
//		
//====================================================================================

#include "cbase.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h" 
#include "predicted_viewmodel.h"
#include "player.h"
#include "simtimer.h"
#include "player_pickup.h"
#include "game.h"
#include "gamerules.h"
#include "trains.h"
#include "in_buttons.h" 
#include "globalstate.h"
#include "KeyValues.h"
#include "eventqueue.h"
#include "engine/IEngineSound.h"
#include "ai_basenpc.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "vphysics/player_controller.h"
#include "datacache/imdlcache.h"
#include "soundenvelope.h"
#include "ai_speech.h"		
#include "sceneentity.h"
#include "hintmessage.h"
#include "items.h"
#include "viewport_panel_names.h"
#include "of_shared_schemas.h"
#include "of_class_parse.h"
#include "sdk_gamerules_sp.h"
#include "sdk_player_shared.h"
#include "of_playeranimstate.h"
#include "bone_setup.h" //animstate implementation
#include "of_campaign_system.h"

#include "bots\bot.h"

// Our Player walk speed value.
ConVar player_walkspeed( "player_walkspeed", "190" );

// Show annotations?
ConVar hud_show_annotations( "hud_show_annotations", "1" );

extern ConVar sv_motd_unload_on_dismissal;

// The delay from when we last got hurt to generate.
// This is already defined in ASW
#ifdef SWARM_DLL
extern ConVar sv_regeneration_wait_time;
#else
ConVar sv_regeneration_wait_time ("sv_regeneration_wait_time", "1.0", FCVAR_REPLICATED );
#endif

// Link us!
// Please dont... :(
LINK_ENTITY_TO_CLASS( player, CSDKPlayer );

IMPLEMENT_SERVERCLASS_ST (CSDKPlayer, DT_SDKPlayer) 
	SendPropBool( SENDINFO( m_bPlayerPickedUpObject ) ),
	SendPropInt( SENDINFO( m_iShotsFired ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClassNumber ) ),
	SendPropBool( SENDINFO( m_bReady ) ),
	SendPropDataTable( SENDINFO_DT( m_PlayerShared ), &REFERENCE_SEND_TABLE( DT_SDKPlayerShared ) ),
	SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 1 ), 11, SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
END_SEND_TABLE()

BEGIN_DATADESC( CSDKPlayer )

	DEFINE_FIELD( m_bPlayerPickedUpObject, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY( m_vecMissPositions, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_nNumMissPositions, FIELD_INTEGER ),

//#ifdef PLAYER_HEALTH_REGEN
	DEFINE_FIELD( m_fTimeLastHurt, FIELD_TIME ),
//#endif

	DEFINE_FIELD( m_angEyeAngles, FIELD_VECTOR )

END_DATADESC()


class CSDKRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CSDKRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( sdk_ragdoll, CSDKRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSDKRagdoll, DT_SDKRagdoll )
	SendPropVector		( SENDINFO( m_vecRagdollOrigin ), -1,  SPROP_COORD ),
	SendPropEHandle		( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex	( SENDINFO( m_nModelIndex ) ),
	SendPropInt			( SENDINFO( m_nForceBone ), 8, 0 ),
	SendPropVector		( SENDINFO( m_vecForce ), -1, SPROP_NOSCALE ),
	SendPropVector		( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT );

static bool BucketSlotCompare( CBucketIndex const &lhs, CBucketIndex const &rhs )
{
	return ( lhs < rhs );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------
// Basics
//-----------------------
CSDKPlayer::CSDKPlayer()
{
	m_PlayerAnimState =  CreatePlayerAnimState( this, this, LEGANIM_9WAY, true );
	UseClientSideAnimation();
	m_angEyeAngles.Init();

	SetViewOffset( Vector(0,0,0) );

	m_nNumMissPositions = 0;

	// Set up the hints.
	m_pHintMessageQueue = new CHintMessageQueue(this);
	m_iDisplayHistoryBits = 0;
	m_bShowHints = true;

	if ( m_pHintMessageQueue )
	{
		m_pHintMessageQueue->Reset();
	}
	
	// We did not fire any shots.
	m_iShotsFired = 0;
	m_iClassNumber = 0;
	m_bReady = false;
	
	m_PlayerShared.m_pOuter = this;
	m_hWeaponSlots.SetLessFunc( BucketSlotCompare );
}

CSDKPlayer::~CSDKPlayer()
{
	delete m_pHintMessageQueue;
	m_pHintMessageQueue = NULL;

	m_flNextMouseoverUpdate = gpGlobals->curtime;
	
	if( m_PlayerAnimState )
		m_PlayerAnimState->Release();
}

void CSDKPlayer::Precache( void )
{
	BaseClass::Precache();

	// Sounds
	PrecacheScriptSound( SOUND_HINT );
	PrecacheScriptSound( SOUND_USE );
	PrecacheScriptSound( SOUND_USE_DENY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();
	/*
	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	// open info panel on client showing MOTD:
	
	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"motd" );		// use this stringtable entry
	data->SetInt( "cmd", TEXTWINDOW_CMD_IMPULSE101 );// exec this command if panel closed
	data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );
	*/

	// Transmit the needed data to clients who just joined
	Campaign()->TransmitSessionToClients();
	ShowViewPortPanel( PANEL_LOBBY, true );
	StartObserverMode( OBS_MODE_ROAMING );
	ChangeTeam( OF_TEAM_UNASSIGNED );

	//data->deleteThis();

	m_bWelcome = true;
}

void CSDKPlayer::Spawn()
{
	// Dying without a player model crashes the client so set it instantly
	SetModel( GetClass(this).szPlayerModel );

	m_hRagdoll = NULL;

	BaseClass::Spawn();

	// Set speed
	SetMaxSpeed( GetClass(this).flSpeed );

	if( m_bWelcome )
	{
		StartObserverMode( OBS_MODE_ROAMING );
		m_bWelcome = false;
	}
	else
	{
		StopObserverMode();

		// Setup flags
		m_takedamage = DAMAGE_YES;
		m_lifeState = LIFE_ALIVE; // Can't be dead, otherwise movement doesn't work right.
		m_flDeathAnimTime = gpGlobals->curtime;
		pl.deadflag = false;

		SetMoveType( MOVETYPE_WALK );
		RemoveEffects( EF_NODRAW | EF_NOSHADOW );
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		SetModel( GetClass(this).szPlayerModel );
	}
	//////////////////////////////////
	// Set stuff after this comment
	//////////////////////////////////

	if( !IsFakeClient() )
		ChangeTeam( OF_TEAM_FUNNY );
	else
		ChangeTeam( OF_TEAM_UNFUNNY );

	// Set health
	m_iMaxHealth = GetClass(this).iMaxHealth;
	m_iHealth = m_iMaxHealth;


	m_Local.m_iHideHUD = 0;
	
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();

#ifdef PLAYER_MOUSEOVER_HINTS
	m_iDisplayHistoryBits &= ~DHM_ROUND_CLEAR;
	SetLastSeenEntity ( NULL );
#endif

	// We did not fire any shots.
	m_iShotsFired = 0;

	GiveDefaultItems();

	GetPlayerProxy();

	// Setup bot stuff
	if ( GetBotController() ) 
	{
		GetBotController()->Spawn();
	}
}

void CSDKPlayer::ForceRespawn()
{
	RemoveAllItems( true );

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;
	
	Spawn();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T  CSDKPlayer::Classify ( void )
{
	return CLASS_PLAYER;
}

void CSDKPlayer::PreThink()
{
	BaseClass::PreThink();

	if ( m_pHintMessageQueue )
		m_pHintMessageQueue->Update();
}

void CSDKPlayer::PostThink()
{
	BaseClass::PostThink();
	
	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles(angles);

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);

	if ( m_flNextMouseoverUpdate < gpGlobals->curtime )
	{
		m_flNextMouseoverUpdate = gpGlobals->curtime + 0.2f;
		if ( m_bShowHints )
		{
			#ifdef PLAYER_MOUSEOVER_HINTS
			UpdateMouseoverHints();
			#endif
		}
	}

#ifdef PLAYER_HEALTH_REGEN
	// Regenerate heath after 3 seconds
	if ( IsAlive() && GetHealth() < GetMaxHealth() )
	{
		// Color to overlay on the screen while the player is taking damage
		color32 hurtScreenOverlay = {64,0,0,64};

		if ( gpGlobals->curtime > m_fTimeLastHurt + sv_regeneration_wait_time.GetFloat() )
		{
			TakeHealth( 1, DMG_GENERIC );
			m_bIsRegenerating = true;

			if ( GetHealth() >= GetMaxHealth() )
			{
				m_bIsRegenerating = false;
			}
		}
		else
		{
			m_bIsRegenerating = false;
			UTIL_ScreenFade( this, hurtScreenOverlay, 1.0f, 0.1f, FFADE_IN|FFADE_PURGE );
		}
	}
#else
	// HACK!: Make it so that when we fall, we show red!
	if ( IsAlive() && GetHealth() < GetMaxHealth() )
	{
		// Color to overlay on the screen while the player is taking damage
		color32 hurtScreenOverlay = {64,0,0,64};

		if ( gpGlobals->curtime > m_fTimeLastHurt + sv_regeneration_wait_time.GetFloat() )
		{
			if ( GetHealth() >= GetMaxHealth() )
			{
				// Nothing at all???
			}
		}
		else
		{
			UTIL_ScreenFade( this, hurtScreenOverlay, 1.0f, 0.1f, FFADE_IN|FFADE_PURGE );
		}
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Makes a splash when the player transitions between water states
//-----------------------------------------------------------------------------
void CSDKPlayer::Splash( void )
{
	CEffectData data;
	data.m_fFlags = 0;
	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = Vector(0,0,1);
	data.m_vAngles = QAngle( 0, 0, 0 );
	
	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	float flSpeed = GetAbsVelocity().Length();
	if ( flSpeed < 300 )
	{
		data.m_flScale = random->RandomFloat( 10, 12 );
		DispatchEffect( "waterripple", data );
	}
	else
	{
		data.m_flScale = random->RandomFloat( 6, 8 );
		DispatchEffect( "watersplash", data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayer::UpdateClientData( void )
{
#ifdef OFFSHORE_DLL
	bool bEqDmgTypes = m_hHUDDamage.Count() == m_hDamageType.Count();
	for( int i = 0; bEqDmgTypes && i < m_hHUDDamage.Count(); i++ )
	{
		if( !m_hDamageType.HasElement(m_hHUDDamage[i]) )
		{
			bEqDmgTypes = false;
			break;
		}
	}
	if ( m_DmgTake || m_DmgSave || bEqDmgTypes )
#else
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
#endif
	{
		// Comes from inside me if not set
		Vector damageOrigin = GetLocalOrigin();
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		damageOrigin = m_DmgOrigin;

		// only send down damage type that have hud art
#ifdef OFFSHORE_DLL
		CUtlVector<int> hShowHudDamage; g_pGameRules->Damage_GetShowOnHud(&hShowHudDamage);
		CUtlVector<int> hVisibleDamageBits;
		FOR_EACH_VEC(hShowHudDamage, i)
		{
			if( m_hDamageType.HasElement(hShowHudDamage[i]) )
				hVisibleDamageBits.AddToTail(hShowHudDamage[i]);
		}
#else
		int iShowHudDamage = g_pGameRules->Damage_GetShowOnHud();
		int visibleDamageBits = m_bitsDamageType & iShowHudDamage;
#endif

		m_DmgTake = clamp( m_DmgTake, 0, 255 );
		m_DmgSave = clamp( m_DmgSave, 0, 255 );

		// If we're poisoned, but it wasn't this frame, don't send the indicator
		// Without this check, any damage that occured to the player while they were
		// recovering from a poison bite would register as poisonous as well and flash
		// the whole screen! -- jdw
#ifdef OFFSHORE_DLL
		if ( hVisibleDamageBits.HasElement(DMG_POISON) )
#else
		if ( visibleDamageBits & DMG_POISON )
#endif
		{
			float flLastPoisonedDelta = gpGlobals->curtime - m_tbdPrev;
			if ( flLastPoisonedDelta > 0.1f )
			{
#ifdef OFFSHORE_DLL
				hVisibleDamageBits.FindAndRemove(DMG_POISON);
#else
				visibleDamageBits &= ~DMG_POISON;
#endif
			}
		}

#ifdef OFFSHORE_DLL
		char szVisibleDamageBits[256];
		FOR_EACH_VEC(hVisibleDamageBits, i)
		{
			if( !i )
				UTIL_VarArgs("d%", hVisibleDamageBits[i]);

			UTIL_VarArgs("s% d%", szVisibleDamageBits, hVisibleDamageBits[i]);
		}
#endif

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( m_DmgSave );
			WRITE_BYTE( m_DmgTake );
#ifdef OFFSHORE_DLL
			WRITE_STRING( szVisibleDamageBits );
#else
			WRITE_LONG( visibleDamageBits );
#endif
			WRITE_FLOAT( damageOrigin.x );	//BUG: Should be fixed point (to hud) not floats
			WRITE_FLOAT( damageOrigin.y );	//BUG: However, the HUD does _not_ implement bitfield messages (yet)
			WRITE_FLOAT( damageOrigin.z );	//BUG: We use WRITE_VEC3COORD for everything else
		MessageEnd();
	
		m_DmgTake = 0;
		m_DmgSave = 0;
#ifdef OFFSHORE_DLL
		m_hHUDDamage = m_hDamageType;
#else
		m_bitsHUDDamage = m_bitsDamageType;
#endif	
		// Clear off non-time-based damage indicators
#ifdef OFFSHORE_DLL
		CUtlVector<int> hTimeBasedDamage;
		g_pGameRules->Damage_GetTimeBased(&hTimeBasedDamage);
		CUtlVector<int> hRes;
		FOR_EACH_VEC(hTimeBasedDamage, i)
		{
			if( m_hDamageType.HasElement(hTimeBasedDamage[i]) )
				hRes.AddToTail(hTimeBasedDamage[i]);
		}

		m_hDamageType = hRes;
#else
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= iTimeBasedDamage;
#endif
	}

	BaseClass::UpdateClientData();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Viewmodel and weapon stuff!
//-----------------------
void CSDKPlayer::CreateViewModel( int index )
{
	BaseClass::CreateViewModel( index );

	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CSDKPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	return bRet;
}

bool CSDKPlayer::Weapon_Detach( CBaseCombatWeapon *pWeapon )
{
	RemoveWeaponFromSlots( pWeapon );
	bool bRet = BaseClass::Weapon_Detach( pWeapon );

	for( unsigned short i = 0; i < m_hWeaponSlots.Count(); i++ )
		DevMsg( "%s\n", m_hWeaponSlots.Element(i)->GetSchemaName() );

	return bRet;
}

void CSDKPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	if( GetWeaponInSlot( pWeapon->GetSlot(), pWeapon->GetPosition() ) )
	{
		Warning("WARNING: MULTIPLE WEAPONS IN THE SAME SLOT!");
	}
	else
		m_hWeaponSlots.Insert( CBucketIndex(pWeapon->GetSlot(), pWeapon->GetPosition()), pWeapon );

	BaseClass::Weapon_Equip( pWeapon );

	CBaseSDKCombatWeapon *pSDKWeapon = dynamic_cast<CBaseSDKCombatWeapon *>(pWeapon);

	if( pSDKWeapon )
	{
		pSDKWeapon->m_iReserveAmmo = pSDKWeapon->GetMaxReserveAmmo();
	}
}

extern int	gEvilImpulse101;
//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CSDKPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	{
		return false;
	}

	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if( Weapon_OwnsThisType( pWeapon->GetSchemaName(), pWeapon->GetSubType()) ) 
	{
		//Only remove the weapon if we attained ammo from it
		if ( Weapon_EquipAmmoOnly( pWeapon ) == false )
			return false;

		// Only remove me if I have no ammo left
		// Can't just check HasAnyAmmo because if I don't use clips, I want to be removed, 
		if ( pWeapon->UsesClipsForAmmo1() && pWeapon->HasPrimaryAmmo() )
			return false;

		UTIL_Remove( pWeapon );
		return false;
	}
	// -------------------------
	// Otherwise take the weapon
	// -------------------------
	else 
	{
		//Make sure we're not trying to take a new weapon type we already have
		if ( Weapon_SlotOccupied( pWeapon ) )
		{
			CBaseCombatWeapon *pActiveWeapon = Weapon_GetSlot( WEAPON_PRIMARY_SLOT );

			if ( pActiveWeapon != NULL && pActiveWeapon->HasAnyAmmo() == false && Weapon_CanSwitchTo( pWeapon ) )
			{
				Weapon_Equip( pWeapon );
				return true;
			}

			//Attempt to take ammo if this is the gun we're holding already
			if ( Weapon_OwnsThisType( pWeapon->GetSchemaName(), pWeapon->GetSubType() ) )
			{
				Weapon_EquipAmmoOnly( pWeapon );
			}

			return false;
		}
	}

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	return true;
}

CBaseCombatWeapon *CSDKPlayer::GetWeaponInSlot( int iSlot, int iPos )
{
	unsigned short iElement = m_hWeaponSlots.Find(CBucketIndex(iSlot, iPos));

	if( iElement == m_hWeaponSlots.InvalidIndex() || iElement >= m_hWeaponSlots.Count() )
		return NULL;

	return m_hWeaponSlots.Element(iElement);
}

void CSDKPlayer::RemoveWeaponFromSlots( CBaseCombatWeapon *pWeapon )
{
	unsigned short iIndex = m_hWeaponSlots.Find( CBucketIndex(pWeapon->GetSlot(), pWeapon->GetPosition()) );
	m_hWeaponSlots.RemoveAt( iIndex );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Walking
//-----------------------
void CSDKPlayer::StartWalking( void )
{
	m_fIsWalking = true;
}

void CSDKPlayer::StopWalking( void )
{
	m_fIsWalking = false;
}

void CSDKPlayer::HandleSpeedChanges( void )
{
	bool bIsWalking = IsWalking();
	bool bWantWalking;
	
	bWantWalking = true;
	
	if( bIsWalking != bWantWalking )
	{
		if ( bWantWalking )
		{
			StartWalking();
		}
		else
		{
			StopWalking();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Use + Pickup
//-----------------------
bool CSDKPlayer::CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit )
{
	// reep: Ported from the base since in the base this is HL2 exclusive. Yeah, don't you LOVE base code? 

	//Must be valid
	if ( pObject == NULL )
		return false;

	//Must move with physics
	if ( pObject->GetMoveType() != MOVETYPE_VPHYSICS )
		return false;

	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pObject->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );

	//Must have a physics object
	if (!count)
		return false;

	float objectMass = 0;
	bool checkEnable = false;
	for ( int i = 0; i < count; i++ )
	{
		objectMass += pList[i]->GetMass();
		if ( !pList[i]->IsMoveable() )
		{
			checkEnable = true;
		}
		if ( pList[i]->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP )
			return false;
		if ( pList[i]->IsHinged() )
			return false;
	}

	//Must be under our threshold weight
	if ( massLimit > 0 && objectMass > massLimit )
		return false;
	/*
	// reep: Could not get it to recognize classes. Think this is for the physcannon anyway. 
	if ( checkEnable )
	{
		#include "props.h"
		#include "vphysics/player_controller.h"
		#include "physobj.h"
		// Allow pickup of phys props that are motion enabled on player pickup
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>(pObject);
		CPhysBox *pBox = dynamic_cast<CPhysBox*>(pObject);
		if ( !pProp && !pBox )
			return false;

		if ( pProp && !(pProp->HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON )) )
			return false;

		if ( pBox && !(pBox->HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON )) )
			return false;
	}
	*/

	if ( sizeLimit > 0 )
	{
		const Vector &size = pObject->CollisionProp()->OBBSize();
		if ( size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit )
			return false;
	}

	return true;
}

void CSDKPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	// can't pick up what you're standing on
	if ( GetGroundEntity() == pObject )
	{
		DevMsg("Failed to pickup object: Player is standing on object!\n");
		PlayUseDenySound();
		return;
	}

	if ( bLimitMassAndSize == true )
	{
		if ( CanPickupObject( pObject, PLAYER_MAX_LIFT_MASS, PLAYER_MAX_LIFT_SIZE ) == false )
		{
			DevMsg("Failed to pickup object: Object too heavy!\n");
			PlayUseDenySound();
			return;
		}
	}

	// Can't be picked up if NPCs are on me
	if ( pObject->HasNPCsOnIt() )
		return;

	// Bool is to tell the client that we have an object. This is incase you want to change the crosshair 
	// or something for your project.
	m_bPlayerPickedUpObject = true;

	PlayerPickupObject( this, pObject );

}

void CSDKPlayer::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis )
{
	m_bPlayerPickedUpObject = false;
	BaseClass::ForceDropOfCarriedPhysObjects( pOnlyIfHoldingThis );
}

void CSDKPlayer::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	if ( m_afButtonPressed & IN_USE )
	{
		// Currently using a latched entity?
		if ( ClearUseEntity() )
		{
			if (m_bPlayerPickedUpObject)
			{
				m_bPlayerPickedUpObject = false;
			}
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
		}

		// Tracker 3926:  We can't +USE something if we're climbing a ladder
		if ( GetMoveType() == MOVETYPE_LADDER )
		{
			return;
		}
	}

	if( m_flTimeUseSuspended > gpGlobals->curtime )
	{
		// Something has temporarily stopped us being able to USE things.
		// Obviously, this should be used very carefully.(sjb)
		return;
	}

	CBaseEntity *pUseEntity = FindUseEntity();

	bool usedSomething = false;

	// Found an object
	if ( pUseEntity )
	{
		//!!!UNDONE: traceline here to prevent +USEing buttons through walls			
		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;

		if ( m_afButtonPressed & IN_USE )
		{
			// Robin: Don't play sounds for NPCs, because NPCs will allow respond with speech.
			if ( !pUseEntity->MyNPCPointer() )
			{
				EmitSound( SOUND_USE );
			}
		}

		if ( ( (m_nButtons & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}
	}

	else if ( m_afButtonPressed & IN_USE )
	{
		// Signal that we want to play the deny sound, unless the user is +USEing on a ladder!
		// The sound is emitted in ItemPostFrame, since that occurs after GameMovement::ProcessMove which
		// lets the ladder code unset this flag.
		m_bPlayUseDenySound = true;
	}

	// Debounce the use key
	if ( usedSomething && pUseEntity )
	{
		m_Local.m_nOldButtons |= IN_USE;
		m_afButtonPressed &= ~IN_USE;
	}
}

void CSDKPlayer::ClearUsePickup()
{
	m_bPlayerPickedUpObject = false;
}

void CSDKPlayer::PlayUseDenySound()
{
	m_bPlayUseDenySound = true;
}

void CSDKPlayer::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if ( m_bPlayUseDenySound )
	{
		m_bPlayUseDenySound = false;
		EmitSound( SOUND_USE_DENY );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Damage
//-----------------------
bool CSDKPlayer::PassesDamageFilter( const CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();
	if( pAttacker && pAttacker->MyNPCPointer() && pAttacker->MyNPCPointer()->IsPlayerAlly() )
	{
		return false;
	}

	if( m_hPlayerProxy && !m_hPlayerProxy->PassesDamageFilter( info ) )
	{
		return false;
	}

	return BaseClass::PassesDamageFilter( info );
}

int	CSDKPlayer::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo inputInfoCopy( info );

	// If you shoot yourself, make it hurt but push you less
#ifdef OFFSHORE_DLL
	if ( inputInfoCopy.GetAttacker() == this && inputInfoCopy.GetDamageTypes()->HasElement(DMG_BULLET) )
#else
	if ( inputInfoCopy.GetAttacker() == this && inputInfoCopy.GetDamageType() == DMG_BULLET )
#endif
	{
		inputInfoCopy.ScaleDamage( 5.0f );
		inputInfoCopy.ScaleDamageForce( 0.05f );
	}
	
	// ignore fall damage if instructed to do so by input
#ifdef OFFSHORE_DLL
	if ( info.GetDamageTypes()->HasElement(DMG_FALL) )
#else
	if ( ( info.GetDamageType() & DMG_FALL ) )
#endif
	{
		inputInfoCopy.SetDamage(0.0f);
	}

	int ret = BaseClass::OnTakeDamage( inputInfoCopy );
	m_DmgOrigin = info.GetDamagePosition();

#ifdef PLAYER_HEALTH_REGEN
	if ( GetHealth() < 100 )
	{
		m_fTimeLastHurt = gpGlobals->curtime;
	}
#endif

	// Bot on take damage Event
	if ( GetBotController() ) 
	{
		GetBotController()->OnTakeDamage( inputInfoCopy );
	}

	return ret;
}

int CSDKPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// set damage type sustained
#ifdef OFFSHORE_DLL
	m_hDamageType.AddVectorToTail(*info.GetDamageTypes());
#else
	m_bitsDamageType |= info.GetDamageType();
#endif

	if ( !CBaseCombatCharacter::OnTakeDamage_Alive( info ) )
		return 0;

	CBaseEntity * attacker = info.GetAttacker();

	if ( !attacker )
		return 0;

	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}

	if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK) && 
		( !attacker->IsSolidFlagSet(FSOLID_TRIGGER)) )
	{
		Vector force = vecDir;// * -DamageForce( WorldAlignSize(), info.GetBaseDamage() );
		if ( force.z > 250.0f )
		{
			force.z = 250.0f;
		}
		ApplyAbsVelocityImpulse( force );
	}

	// Burnt
#ifdef OFFSHORE_DLL
	if ( info.GetDamageTypes()->HasElement(DMG_BURN) )
#else
	if ( info.GetDamageType() & DMG_BURN )
#endif
	{
		EmitSound( "Player.BurnPain" );
	}

	// fire global game event

	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("health", MAX(0, m_iHealth) );
		event->SetInt("priority", 5 );	// HLTV event priority, not transmitted

		if ( attacker->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( attacker );
			event->SetInt("attacker", player->GetUserID() ); // hurt by other player
		}
		else
		{
			event->SetInt("attacker", 0 ); // hurt by "world"
		}

		gameeventmanager->FireEvent( event );
	}

	// Insert a combat sound so that nearby NPCs hear battle
	if ( attacker->IsNPC() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512, 0.5, this );
	}

	return 1;
}

void CSDKPlayer::OnDamagedByExplosion( const CTakeDamageInfo &info )
{
	if ( info.GetInflictor() && info.GetInflictor()->ClassMatches( "mortarshell" ) )
	{
		// No ear ringing for mortar
		UTIL_ScreenShake( info.GetInflictor()->GetAbsOrigin(), 4.0, 1.0, 0.5, 1000, SHAKE_START, false );
		return;
	}
	BaseClass::OnDamagedByExplosion( info );
}

#ifndef SWARM_DLL
void CSDKPlayer::FirePlayerProxyOutput( const char *pszOutputName, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if ( GetPlayerProxy() == NULL )
		return;

	GetPlayerProxy()->FireNamedOutput( pszOutputName, variant, pActivator, pCaller );
}
#endif

void CSDKPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	FirePlayerProxyOutput( "PlayerDied", variant_t(), this, this );
	
	// Bot on death event
	if ( GetBotController() ) 
	{
		GetBotController()->OnDeath( info );
	}

	CreateRagdollEntity();
}


void CSDKPlayer::CreateRagdollEntity()
{
	// If we already have a ragdoll, don't make another one.
	CSDKRagdoll *pRagdoll = dynamic_cast< CSDKRagdoll* >( m_hRagdoll.Get() );

	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CSDKRagdoll* >( CreateEntityByName( "sdk_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = Vector(0,0,0);
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Item Gifting
//-----------------------
extern int	gEvilImpulse101;

void CSDKPlayer::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
	case 101:
		{
			if( sv_cheats->GetBool() )
			{
				GiveAllItems();
			}
		}
		break;

	default:
		BaseClass::CheatImpulseCommands( iImpulse );
	}
}

void CSDKPlayer::GiveAllItems( void )
{
	EquipSuit();

	// Give the player everything!
	GiveAmmo(255, "Pistol");
	GiveAmmo(255, "AR2");
	GiveAmmo(5, "AR2AltFire");
	GiveAmmo(255, "SMG1");
	GiveAmmo(255, "Buckshot");
	GiveAmmo(3, "smg1_grenade");
	GiveAmmo(3, "rpg_round");
	GiveAmmo(5, "grenade");
	GiveAmmo(32, "357");
	GiveAmmo(16, "XBowBolt");

#ifdef HL2_EPISODIC
	GiveAmmo(5, "Hopwire");
#endif

	GiveNamedItem("weapon_smg1");
	GiveNamedItem("weapon_frag");
	GiveNamedItem("weapon_crowbar");
	GiveNamedItem("weapon_pistol");
	GiveNamedItem("weapon_ar2");
	GiveNamedItem("weapon_shotgun");
	GiveNamedItem("weapon_physcannon");

	GiveNamedItem("weapon_bugbait");

	GiveNamedItem("weapon_rpg");
	GiveNamedItem("weapon_357");
	GiveNamedItem("weapon_crossbow");
}

void CSDKPlayer::ClearAllWeapons( void )
{
	for( int i = 0; i < m_hMyWeapons.Count(); i++ )
	{
		Weapon_Detach( m_hMyWeapons[i] );
		UTIL_Remove( m_hMyWeapons[i] );
	}
}

void CSDKPlayer::GiveDefaultItems( void )
{
	// If you want the player to always start with something, give it
	// to them here.

	int iWeaponCount = GetClass(this).iWeaponCount;

	for( int i = 0; i < iWeaponCount; i++ )
	{
		GiveNamedItem( GetClass(this).m_hWeaponNames[i] );
	}
}

CBaseEntity *CSDKPlayer::GiveNamedItem( const char *szName, int iSubType, bool removeIfNotCarried )
{
	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(szName, iSubType) )
		return NULL;

	const char *szWeaponClass = szName;
	bool bUsesSchema = false;

	KeyValues *pSchemaWeapon = ItemSchema()->GetWeapon(szName);
	if( pSchemaWeapon && pSchemaWeapon->GetString("weapon_class") )
	{
		szWeaponClass = pSchemaWeapon->GetString("weapon_class");
		bUsesSchema = true;
	}

	EHANDLE pent;

	pent = CreateEntityByName(szWeaponClass);
	if ( pent == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	pent->SetLocalOrigin( GetLocalOrigin() );
	pent->AddSpawnFlags( SF_NORESPAWN );

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( (CBaseEntity*)pent );
	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );
		pWeapon->SetupSchemaItem( szName );
	}

	DispatchSpawn( pent );

	if ( pent != NULL && !(pent->IsMarkedForDeletion()) ) 
	{
		pent->Touch( this );
	}

	return pent;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------
// Hint Systems (OBSOLETE)
//-----------------------
void CSDKPlayer::HintMessage( const char *pMessage, bool bDisplayIfDead, bool bOverrideClientSettings, bool bQuiet )
{
	if (!hud_show_annotations.GetBool())
		return;

	if ( !bDisplayIfDead && !IsAlive() || !IsNetClient() || !m_pHintMessageQueue )
		return;

	//Are we gonna play a sound?
	if(!bQuiet)
	{
		EmitSound(SOUND_HINT);
	}

	if ( bOverrideClientSettings || m_bShowHints )
		m_pHintMessageQueue->AddMessage( pMessage );
}

// All other mouse over hints.
#ifdef PLAYER_MOUSEOVER_HINTS
void CSDKPlayer::UpdateMouseoverHints()
{
	// Don't show if we are DEAD!
	if ( !IsAlive())
		return;

	if(PlayerHasObject())
		return;

	// keep writing.....
}
#endif

void CSDKPlayer::OnGroundChanged(CBaseEntity *oldGround, CBaseEntity *newGround)
{
	BaseClass::OnGroundChanged(oldGround, newGround);
	
	if( oldGround && !newGround )
	{
		m_PlayerShared.m_flLastGrounded = gpGlobals->curtime;
	}
	else
		m_PlayerShared.m_iJumpCount = GetClass(this).iJumpCount;

	m_PlayerShared.SetGrappledWall(NULL, NULL);
	m_PlayerShared.m_flGrappleTime = -1;
}

CLogicPlayerProxy *CSDKPlayer::GetPlayerProxy( void )
{
	CLogicPlayerProxy *pProxy = dynamic_cast< CLogicPlayerProxy* > ( m_hPlayerProxy.Get() );

	if ( pProxy == NULL )
	{
		pProxy = (CLogicPlayerProxy*)gEntList.FindEntityByClassname(NULL, "logic_playerproxy" );

		if ( pProxy == NULL )
			return NULL;

		pProxy->m_hPlayer = this;
		m_hPlayerProxy = pProxy;
	}

	return pProxy;
}

bool CSDKPlayer::ClientCommand(const CCommand &args)
{
	const char *cmd = args[0];

	if( FStrEq( cmd, "joinclass" ) )
	{
		int iDesiredClass = GetClassIndexByName( args[1] );

		if( iDesiredClass == -1 )
			iDesiredClass = atoi(args[1]);

		if( iDesiredClass >= ClassManager()->m_iClassCount || iDesiredClass == m_iClassNumber )
			return false;

		if( !ClassManager()->m_hClassInfo[iDesiredClass].bPlayable )
			return false;

		m_iClassNumber = iDesiredClass;
		
		session_player_data_t *pPlayerData = Campaign()->GetSessionPlayerData( GetSteamID() );
		if( pPlayerData )
			Q_strncpy( pPlayerData->m_szClass, GetClass(this).szClassName, sizeof(pPlayerData->m_szClass) );
		// ForceRespawn();

		return true;
	}
	else if( FStrEq( cmd, "get_class_info" ) )
	{
		DevMsg( "%s\n", GetClass(this).szClassName );
		return true;
	}
	else if( FStrEq( cmd, "request_ready" ) )
	{
		m_bReady = true;
		return true;
	}
	else if( FStrEq( cmd, "request_unready" ) )
	{
		m_bReady = false;
		return true;
	}
	else if( FStrEq( cmd, "request_nextmap" ) )
	{
		if( !OFGameRules()->RoundEnded() && !OFGameRules()->IsLobby() )
			return false;

		if( args.ArgC() < 2 )
			return false;

		int iMap = atoi(args[1]);

		if( iMap >= OFGameRules()->m_hNextMaps.Count() )
			return false;

		if( Campaign()->m_pActiveSession->m_iHost == GetSteamID() )
		{
			ConVarRef nextlevel( "nextlevel" );
			nextlevel.SetValue( OFGameRules()->m_hNextMaps[iMap].szMapname );
			engine->ServerCommand(UTIL_VarArgs("say \"Next Map:%s\"", OFGameRules()->m_hNextMaps[iMap].szMapname));
		}
		return true;
	}
	else
		return false;
		//return ClientCommand( args );
}

void CSDKPlayer::UpdateSpeed()
{
	float flSpeed = GetClass(this).flSpeed;

	flSpeed *= m_PlayerShared.GetSpeedMultiplier();

	SetMaxSpeed( flSpeed );
}

//================================================================================
//================================================================================
void CSDKPlayer::SetBotController( IBot * pBot )
{
    if ( m_pBotController ) {
        delete m_pBotController;
        m_pBotController = NULL;
    }

    m_pBotController = pBot;
}

//================================================================================
//================================================================================
void CSDKPlayer::SetUpBot()
{
    CreateSenses();
    SetBotController( new CBot( this ) );
}

//================================================================================
//================================================================================
void CSDKPlayer::CreateSenses()
{
    m_pSenses = new CAI_Senses;
    m_pSenses->SetOuter( this );
}

//================================================================================
//================================================================================
void CSDKPlayer::SetDistLook( float flDistLook )
{
    if ( GetSenses() ) {
        GetSenses()->SetDistLook( flDistLook );
    }
}

//================================================================================
//================================================================================
int CSDKPlayer::GetSoundInterests()
{
    return SOUND_DANGER | SOUND_COMBAT | SOUND_PLAYER | SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE;
}

//================================================================================
//================================================================================
int CSDKPlayer::GetSoundPriority( CSound *pSound )
{
    if ( pSound->IsSoundType( SOUND_COMBAT ) )
	{
        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_DANGER ) )
	{
        if ( pSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER | SOUND_CONTEXT_EXPLOSION ) )
		{
            return SOUND_PRIORITY_HIGHEST;
        }
        else if ( pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE | SOUND_BULLET_IMPACT ) )
		{
            return SOUND_PRIORITY_VERY_HIGH;
        }

        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE ) )
	{
        return SOUND_PRIORITY_VERY_LOW;
    }

    return SOUND_PRIORITY_NORMAL;
}

//================================================================================
//================================================================================
bool CSDKPlayer::QueryHearSound( CSound *pSound )
{
    CBaseEntity *pOwner = pSound->m_hOwner.Get();

    if ( pOwner == this )
        return false;

    if ( pSound->IsSoundType( SOUND_PLAYER ) && !pOwner )
	{
        return false;
    }

    if ( pSound->IsSoundType( SOUND_CONTEXT_ALLIES_ONLY ) ) 
	{
        if ( Classify() != CLASS_PLAYER_ALLY && Classify() != CLASS_PLAYER_ALLY_VITAL ) 
		{
            return false;
        }
    }

    if ( pOwner ) 
	{
        // Solo escuchemos sonidos provocados por nuestros aliados si son de combate.
        if ( TheGameRules->PlayerRelationship( this, pOwner ) == GR_ALLY ) {
            if ( pSound->IsSoundType( SOUND_COMBAT ) && !pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                return true;
            }

            return false;
        }
    }

    if ( ShouldIgnoreSound( pSound ) )
	{
        return false;
    }

    return true;
}

//================================================================================
//================================================================================
bool CSDKPlayer::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFear )
{
    if ( bOnlyHateOrFear )
	{
        if ( OFGameRules()->PlayerRelationship( this, pEntity ) == GR_NOTTEAMMATE )
            return true;

        Disposition_t disposition = IRelationType( pEntity );
        return (disposition == D_HT || disposition == D_FR);
    }

    return true;
}

//================================================================================
//================================================================================
void CSDKPlayer::OnLooked( int iDistance )
{
    if ( GetBotController() )
	{
        GetBotController()->OnLooked( iDistance );
    }
}

//================================================================================
//================================================================================
void CSDKPlayer::OnListened()
{
    if ( GetBotController() ) 
	{
        GetBotController()->OnListened();
    }
}

//================================================================================
//================================================================================
CSound *CSDKPlayer::GetLoudestSoundOfType( int iType )
{
    return CSoundEnt::GetLoudestSoundOfType( iType, EarPosition() );
}

//================================================================================
// Devuelve si podemos ver el origen del sonido
//================================================================================
bool CSDKPlayer::SoundIsVisible( CSound *pSound )
{
    return (FVisible( pSound->GetSoundReactOrigin() ) && IsInFieldOfView( pSound->GetSoundReactOrigin() ));
}

//================================================================================
//================================================================================
CSound* CSDKPlayer::GetBestSound( int validTypes )
{
    CSound *pResult = GetSenses()->GetClosestSound( false, validTypes );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestSound\n" );
    }

    return pResult;
}

//================================================================================
//================================================================================
CSound* CSDKPlayer::GetBestScent()
{
    CSound *pResult = GetSenses()->GetClosestSound( true );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestScent\n" );
    }

    return pResult;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( get_weapon_in_pos, "" )
{
	CSDKPlayer *pPlayer = ToSDKPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer 
		&& args.ArgC() >= 2 )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeaponInSlot(atoi(args[1]), args.ArgC() > 2 ? atoi(args[2]) : 0);

		if( pWeapon )
			Msg( "Weapon in slot %d %d is %s." , atoi(args[1]), args.ArgC() > 2 ? atoi(args[2]) : 0, pWeapon->GetSchemaName() );
	}
}