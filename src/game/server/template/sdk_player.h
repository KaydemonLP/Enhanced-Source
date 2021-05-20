#ifndef SDK_PLAYER_H
#define SDK_PLAYER_H
#include "cbase.h"
#include "predicted_viewmodel.h"
#include "ai_speech.h"			// For expresser host
#include "hintmessage.h"
#include "sdk_shareddefs.h"
#include "of_weaponbase.h"
#include "sdk_playerclass_shared.h"
#include "of_playeranimstate.h"

//-----NonOptional Defines-----
// Pretty much values here can be changed.
// Should be self explanitory.
//-----------------------------

// Or pickup limits.
#define PLAYER_MAX_LIFT_MASS 85
#define PLAYER_MAX_LIFT_SIZE 128

// Sounds.
#define SOUND_HINT		"Hint.Display"
#define SOUND_USE_DENY	"Player.UseDeny"
#define SOUND_USE		"Player.Use"

// Definitions for weapon slots
#define	WEAPON_MELEE_SLOT			0
#define	WEAPON_SECONDARY_SLOT		1
#define	WEAPON_PRIMARY_SLOT			2
#define	WEAPON_EXPLOSIVE_SLOT		3
#define	WEAPON_TOOL_SLOT			4

// -----------------------------
// Purpoise: Special class used for Bucket index saving and searching
// -----------------------------
class CBucketIndex
{
public:
	CBucketIndex()
	{
		slot = 0;
		pos = 0;
	}

	CBucketIndex( int iSlot, int iPos )
	{
		slot = iSlot;
		pos = iPos;
	}

	bool operator==(const CBucketIndex& v) const;
	bool operator!=(const CBucketIndex& v) const;
	bool operator<(const CBucketIndex& v) const;
	bool operator<=(const CBucketIndex& v) const;
	bool operator>(const CBucketIndex& v) const;
	bool operator>=(const CBucketIndex& v) const;

	// arithmetic operations
	CBucketIndex&	operator+=(const CBucketIndex &v);
	CBucketIndex&	operator-=(const CBucketIndex &v);
	CBucketIndex&	operator*=(const CBucketIndex &v);
	CBucketIndex&	operator*=(float s);
	CBucketIndex&	operator/=(const CBucketIndex &v);
	CBucketIndex&	operator/=(float s);

	CBucketIndex&	operator=(const CBucketIndex &vOther);

public:
	int slot;
	int pos;
};

inline CBucketIndex& CBucketIndex::operator=(const CBucketIndex &vOther)	
{
	Assert( vOther.IsValid() );
	slot = vOther.slot; pos = vOther.pos;
	return *this; 
}

inline bool CBucketIndex::operator==( const CBucketIndex& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.slot == slot) && (src.pos == pos);
}

inline bool CBucketIndex::operator!=( const CBucketIndex& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.slot != slot) || (src.pos != pos);
}

inline bool CBucketIndex::operator<( const CBucketIndex& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (slot < src.slot) || (slot == src.slot && pos < src.pos);
}

inline bool CBucketIndex::operator<=( const CBucketIndex& src ) const
{
	return !( src < *this );
}

inline bool CBucketIndex::operator>( const CBucketIndex& src ) const
{
	return src < *this;
}

inline bool CBucketIndex::operator>=( const CBucketIndex& src ) const
{
	return !( *this < src );
}

inline CBucketIndex& CBucketIndex::operator+=(const CBucketIndex& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	slot+=v.slot; pos+=v.pos;	
	return *this;
}

inline CBucketIndex& CBucketIndex::operator-=(const CBucketIndex& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	slot-=v.slot; pos-=v.pos;	
	return *this;
}

inline CBucketIndex& CBucketIndex::operator*=(float fl)	
{
	slot *= fl;
	pos *= fl;
	Assert( IsValid() );
	return *this;
}

inline CBucketIndex& CBucketIndex::operator*=(const CBucketIndex& v)	
{ 
	slot *= v.slot;
	pos *= v.pos;
	Assert( IsValid() );
	return *this;
}

inline CBucketIndex& CBucketIndex::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	slot *= oofl;
	pos *= oofl;
	Assert( IsValid() );
	return *this;
}

inline CBucketIndex& CBucketIndex::operator/=(const CBucketIndex& v)	
{ 
	Assert( v.slot != 0.0f && v.pos != 0.0f );
	slot /= v.slot;
	pos /= v.pos;
	Assert( IsValid() );
	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: Used to relay outputs/inputs from the player to the world and viceversa
//-----------------------------------------------------------------------------
class CLogicPlayerProxy : public CLogicalEntity
{
	DECLARE_CLASS( CLogicPlayerProxy, CLogicalEntity );

private:

	DECLARE_DATADESC();

public:

	COutputEvent m_OnFlashlightOn;
	COutputEvent m_OnFlashlightOff;
	COutputEvent m_PlayerHasAmmo;
	COutputEvent m_PlayerHasNoAmmo;
	COutputEvent m_PlayerDied;
	COutputEvent m_PlayerMissedAR2AltFire; // Player fired a combine ball which did not dissolve any enemies. 

	COutputInt m_RequestedPlayerHealth;

	void InputRequestPlayerHealth( inputdata_t &inputdata );
	void InputSetFlashlightSlowDrain( inputdata_t &inputdata );
	void InputSetFlashlightNormalDrain( inputdata_t &inputdata );
	void InputSetPlayerHealth( inputdata_t &inputdata );
	void InputRequestAmmoState( inputdata_t &inputdata );
	void InputLowerWeapon( inputdata_t &inputdata );
	void InputEnableCappedPhysicsDamage( inputdata_t &inputdata );
	void InputDisableCappedPhysicsDamage( inputdata_t &inputdata );
	void InputSetLocatorTargetEntity( inputdata_t &inputdata );

	void Activate ( void );

	bool PassesDamageFilter( const CTakeDamageInfo &info );

	EHANDLE m_hPlayer;
};

class CSDKPlayer : public CBasePlayer, public ISDKPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS( CSDKPlayer, CBasePlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	// Basics
	CSDKPlayer();
	~CSDKPlayer();

	static CSDKPlayer *CreatePlayer( const char *className, edict_t *ed )
	{
		CSDKPlayer::s_PlayerEdict = ed;
		return (CSDKPlayer*)CreateEntityByName( className );
	}

	virtual void Precache();
	virtual void InitialSpawn();
	virtual void Spawn();
	virtual void ForceRespawn();
	virtual void UpdateClientData( void );
	virtual void PostThink();
	virtual void PreThink();
	virtual void Splash( void );
	CLogicPlayerProxy	*GetPlayerProxy( void );

	virtual void SetClassNumber( int iClassNumber ) { m_iClassNumber = iClassNumber; }
	virtual int GetClassNumber() { return m_iClassNumber; }
	
	//==============
	//	Bot Functions
	//

    virtual IBot *GetBotController() 
	{
        return m_pBotController;
    }

    virtual void SetBotController( IBot *pBot );
    virtual void SetUpBot();

    // Senses
    virtual CAI_Senses *GetSenses() 
	{
        return m_pSenses;
    }

    virtual const CAI_Senses *GetSenses() const 
	{
        return m_pSenses;
    }

    virtual void CreateSenses();

    virtual void SetDistLook( float flDistLook );

    virtual int GetSoundInterests();
    virtual int GetSoundPriority( CSound *pSound );

    virtual bool QueryHearSound( CSound *pSound );
    virtual bool QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );

    virtual void OnLooked( int iDistance );
    virtual void OnListened();

    virtual CSound *GetLoudestSoundOfType( int iType );
    virtual bool SoundIsVisible( CSound *pSound );

    virtual CSound *GetBestSound( int validTypes = ALL_SOUNDS );
    virtual CSound *GetBestScent( void );

	// End Bot

#ifndef SWARM_DLL
	void FirePlayerProxyOutput( const char *pszOutputName, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller );
	EHANDLE			m_hPlayerProxy;	// Handle to a player proxy entity for quicker reference
#endif

	virtual bool ClientCommand(const CCommand &args);

	virtual void UpdateSpeed();

	// Use + Pickup
	virtual void PlayerUse( void );
	virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis );
	virtual void ClearUsePickup();
	virtual bool CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit );
	CNetworkVar( bool, m_bPlayerPickedUpObject );
	bool PlayerHasObject() { return m_bPlayerPickedUpObject; }

	// This float is outside PLAYER_MOUSEOVER_HINTS to make things much easier. 
	float m_flNextMouseoverUpdate;

	// Viewmodel + Weapon
	virtual void CreateViewModel( int index );
	virtual bool Weapon_Detach( CBaseCombatWeapon *pWeapon );		// Clear any pointers to the weapon.
	virtual void Weapon_Equip ( CBaseCombatWeapon *pWeapon );
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual CBaseCombatWeapon *GetWeaponInSlot( int iSlot, int iPos );
	virtual void RemoveWeaponFromSlots( CBaseCombatWeapon *pWeapon );
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	//Walking
	virtual void StartWalking( void );
	virtual void StopWalking( void );
	CNetworkVarForDerived( bool, m_fIsWalking );
	virtual bool IsWalking( void ) { return m_fIsWalking; }
	virtual void  HandleSpeedChanges( void );
	bool  m_bPlayUseDenySound;		// Signaled by PlayerUse, but can be unset by HL2 ladder code...

	virtual float	GetPlayerMaxSpeed();
	virtual void	UpdateStepSound(surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity);
	virtual void	SetStepSoundTime(stepsoundtimes_t iStepSoundTime, bool bWalking);

	CBaseSDKCombatWeapon*	GetActiveSDKWeapon() const 
	{ 
		return dynamic_cast<CBaseSDKCombatWeapon *>(GetActiveWeapon()); 
	};

	// Damage
	virtual bool PassesDamageFilter( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void OnDamagedByExplosion( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

#ifdef PLAYER_HEALTH_REGEN
	//Regenerate
	float m_fTimeLastHurt;
	bool  m_bIsRegenerating;		// Is the player currently regaining health
#endif

	// Lag compensate when firing bullets
	//void FireBullets ( const FireBulletsInfo_t &info );

	// CHEAT
	virtual void			CheatImpulseCommands( int iImpulse );
	virtual void			GiveAllItems( void );
	virtual void			ClearAllWeapons( void );
	virtual void			GiveDefaultItems( void );
	virtual CBaseEntity		*GiveNamedItem( const char *szName, int iSubType = 0, bool removeIfNotCarried = true );

	// Hints
	virtual void HintMessage( const char *pMessage, bool bDisplayIfDead, bool bOverrideClientSettings = false, bool bQuiet = false ); // Displays a hint message to the player
	CHintMessageQueue *m_pHintMessageQueue;
	unsigned int m_iDisplayHistoryBits;
	bool m_bShowHints;

	virtual void	OnGroundChanged(CBaseEntity *oldGround, CBaseEntity *newGround);

	// Hint Flags: These are flags to tick when the hint shows. This prevents players feeling that the game think's they are stupid by
	// repeating the same hint over, and over.
	#define DHF_GAME_STARTED		( 1 << 1 )
	#define DHF_GAME_ENDED			( 1 << 2 )
	// etc....

	// You can as many flags as you wish.

#ifdef PLAYER_MOUSEOVER_HINTS
	void UpdateMouseoverHints();
#endif

	Class_T Classify ( void );

	// Campaign Stuff
	bool IsReady(){ return m_bReady; }
	void UnReady(){ m_bReady = false; }

	// Shared Functions
	virtual const QAngle &EyeAngles(); 
	virtual uint32 GetSteamID( void );

public:
	// ISDKPlayerAnimState overrides.
	virtual CBaseSDKCombatWeapon* SDKAnim_GetActiveWeapon();
	virtual bool SDKAnim_CanMove();
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	virtual CBaseCombatWeapon*	Weapon_OwnsThisType( const char *pszWeapon, int iSubType = 0 ) const;  // True if already owns a weapon of this class

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y );
		
		CSDKPlayerShared m_PlayerShared;
private:
		float				m_flTimeUseSuspended;

		Vector				m_vecMissPositions[16];
		int					m_nNumMissPositions;
		
		bool				m_bWelcome;

		CNetworkVar( int, m_iClassNumber );
		CNetworkVar( bool, m_bReady );
		
		CNetworkQAngle( m_angEyeAngles );
		
		void CreateRagdollEntity();

		ISDKPlayerAnimState *m_PlayerAnimState;

		// Tracks our ragdoll entity.
		CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle

		//	KEY, ELEMENT
		CUtlMap< CBucketIndex, CBaseCombatWeapon *> m_hWeaponSlots;

protected:
		virtual void		ItemPostFrame();
		virtual void		PlayUseDenySound();
protected:
		IBot *m_pBotController;
		CAI_Senses *m_pSenses;

};

inline CSDKPlayer *ToSDKPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;
	
	if( !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<CSDKPlayer*>( pEntity ) != 0 );
	return static_cast< CSDKPlayer* >( pEntity );
}

#endif