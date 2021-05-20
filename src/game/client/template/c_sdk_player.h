//=========== Copyright © 2014, rHetorical, All rights reserved. =============
//
// Purpose: 
//		
//=============================================================================
#ifndef OF_PLAYER_H
#define OF_PLAYER_H
#include "cbase.h"
#include "weapons/c_basesdkcombatweapon.h"
#include "sdk_playerclass_shared.h"
#include "of_playeranimstate.h"

#undef CSDKPlayer


// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_SDKPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_SDKPlayerShared );

#endif

class C_SDKPlayer : public C_BasePlayer, public ISDKPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS( C_SDKPlayer, C_BasePlayer );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void OnDataChanged( DataUpdateType_t type );
	void PostDataUpdate( DataUpdateType_t type  );
	static C_SDKPlayer* GetLocalPlayer( int nSlot = -1 );

	C_SDKPlayer();
	~C_SDKPlayer( void );

	static C_SDKPlayer* GetLocalSDKPlayer();

	void ClientThink( void );

	virtual bool ShouldRegenerateOriginFromCellBits() const
	{
		// C_BasePlayer assumes that we are networking a high-res origin value instead of using a cell
		// (and so returns false here), but this is not by default the case.
		return true; // TODO: send high-precision origin instead?
	}

	const QAngle &GetRenderAngles();

	void UpdateClientSideAnimation();
	virtual	void		BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	// Input handling
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	void			PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd );
	void			PerformClientSideNPCSpeedModifiers( float flFrameTime, CUserCmd *pCmd );

	virtual float	GetPlayerMaxSpeed();
	virtual void	UpdateStepSound(surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity);
	virtual void	SetStepSoundTime(stepsoundtimes_t iStepSoundTime, bool bWalking);

	virtual int 	GetClassNumber() { return m_iClassNumber; }

	// Campaign Stuff
	bool IsReady(){ return m_bReady; }

	// Shared Functions
	virtual const QAngle &EyeAngles();
	virtual uint32 GetSteamID( void );
public:
	// ISDKPlayerAnimState overrides.
	virtual CBaseSDKCombatWeapon* SDKAnim_GetActiveWeapon();
	virtual bool SDKAnim_CanMove();

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	bool ShouldDraw();

	ISDKPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkVar( int, m_iThrowGrenadeCounter );	// used to trigger grenade throw animations.
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	EHANDLE	m_hRagdoll;

	CBaseSDKCombatWeapon *GetActiveSDKWeapon() const;

	C_BaseAnimating *BecomeRagdollOnClient();
	IRagdoll* C_SDKPlayer::GetRepresentativeRagdoll() const;

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

	// Are we looking over a useable object?
//	bool IsCursorOverUseable() { return m_bMouseOverUseable; }
//	bool m_bMouseOverUseable;

	// Do we have an object?
	bool PlayerHasObject() { return m_bPlayerPickedUpObject; }
	bool m_bPlayerPickedUpObject;

	// Are we looking at an enemy?
//	bool IsCursorOverEnemy() { return m_bMouseOverEnemy; }
//	bool m_bMouseOverEnemy;

	EHANDLE				m_hClosestNPC;
	float				m_flSpeedModTime;
	float				m_flExitSpeedMod;

	CSDKPlayerShared 	m_PlayerShared;
private:
	bool	TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir );

	float	m_flSpeedMod;
	QAngle m_angRender;

	CNetworkVar( int, m_iClassNumber );
	CNetworkVar( bool, m_bReady );
};

inline C_SDKPlayer* ToSDKPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;
	
	if ( !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast< C_SDKPlayer* >( pEntity ) != NULL );
	return static_cast< C_SDKPlayer* >( pEntity );
}
#endif