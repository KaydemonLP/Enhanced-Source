#include "cbase.h"
#include "sdk_player_shared.h"
#include "of_shared_schemas.h"
#include "in_buttons.h"
#include "of_weaponbase.h"
#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "basetempentity.h"
#else
#include "c_te_effect_dispatch.h"
#include "c_basetempentity.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

#ifdef CLIENT_DLL
#define CBaseTempEntity C_BaseTempEntity
#define CTEPlayerAnimEvent C_TEPlayerAnimEvent
#endif

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
	}
#else
	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_SDKPlayer *pPlayer = dynamic_cast< C_SDKPlayer* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}
#endif
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_NETWORKCLASS_ALIASED( TEPlayerAnimEvent, DT_TEPlayerAnimEvent )

BEGIN_NETWORK_TABLE_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
#else
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );
	
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
};
#endif

BEGIN_NETWORK_TABLE_NOBASE( walldata_t, DT_PlayerWallData )
#if !defined( CLIENT_DLL )
	SendPropVector( SENDINFO( m_vecGrappledNormal ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecGrapplePos ), -1, SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hWallEnt ) ),
#else
	RecvPropVector( RECVINFO( m_vecGrappledNormal ) ),
	RecvPropVector( RECVINFO( m_vecGrapplePos ) ),
	RecvPropEHandle( RECVINFO( m_hWallEnt ) ),
#endif
END_NETWORK_TABLE()

walldata_t::walldata_t()
{
	m_vecGrapplePos = vec3_origin;
	m_vecGrappledNormal = vec3_origin;
	m_hWallEnt = NULL;
}

BEGIN_NETWORK_TABLE_NOBASE( CSDKPlayerShared, DT_SDKPlayerShared )
#if !defined( CLIENT_DLL )
	SendPropBool( SENDINFO( m_bGrappledWall ) ),
	SendPropTime( SENDINFO( m_flGrappleTime ) ),
	
	SendPropTime( SENDINFO( m_flWallClimbTime ) ),
	SendPropTime( SENDINFO( m_flWallRunTime ) ),

	SendPropTime( SENDINFO( m_flLastSprint ) ),
	SendPropTime( SENDINFO( m_flLastGrounded ) ),
	SendPropInt ( SENDINFO( m_iJumpCount ) ),

	SendPropBool( SENDINFO( m_bSliding ) ),
	SendPropFloat( SENDINFO( m_flSprintSpeed ) ),
	SendPropVector( SENDINFO( m_vecSlideDir ), -1, SPROP_COORD ),
	SendPropDataTable( SENDINFO_DT( m_WallData ), &REFERENCE_SEND_TABLE( DT_PlayerWallData ) ),
#else
	RecvPropBool( RECVINFO( m_bGrappledWall ) ),
	RecvPropTime( RECVINFO( m_flGrappleTime ) ),
	
	RecvPropTime( RECVINFO( m_flWallClimbTime ) ),
	RecvPropTime( RECVINFO( m_flWallRunTime ) ),

	RecvPropTime( RECVINFO( m_flLastSprint ) ),
	RecvPropTime( RECVINFO( m_flLastGrounded ) ),
	RecvPropInt ( RECVINFO( m_iJumpCount ) ),

	RecvPropBool( RECVINFO( m_bSliding ) ),
	RecvPropFloat( RECVINFO( m_flSprintSpeed ) ),
	RecvPropVector( RECVINFO( m_vecSlideDir ) ),
	RecvPropDataTable( RECVINFO_DT( m_WallData ), 0, &REFERENCE_RECV_TABLE( DT_PlayerWallData ) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC_NO_BASE( CSDKPlayerShared )
//	DEFINE_FIELD( m_bLowered,			FIELD_BOOLEAN ),
END_DATADESC()

#else

BEGIN_PREDICTION_DATA_NO_BASE(CSDKPlayerShared)
END_PREDICTION_DATA()

#endif

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point" );

extern ConVar sv_slidespeed;

CSDKPlayerShared::CSDKPlayerShared()
{
	m_flGrappleTime = -1;
	m_flLastSprint = -1;
	m_flLastGrounded = -1;

	m_flSprintSpeed = 0;
}

CSDKPlayerShared::~CSDKPlayerShared()
{
	
}

void CSDKPlayerShared::OnHitWall( trace_t *trTrace )
{
	if( 
		m_pOuter->GetGroundEntity() == NULL 
		&& ( GetClass( m_pOuter ).flWallJumpTime > 0
		|| GetClass( m_pOuter ).flWallClimbTime > 0 
		|| GetClass( m_pOuter ).flWallRunTime > 0 )
//		&& trTrace->m_pEnt->IsWorld()
	)
	{
		SetGrappledWall( &trTrace->plane.normal, &trTrace->endpos, trTrace->m_pEnt && !trTrace->m_pEnt->IsWorld() ? trTrace->m_pEnt : NULL );
	}
}

float CSDKPlayerShared::GetWallJumpTime()
{
	return ClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flWallJumpTime;
}

void CSDKPlayerShared::SetGrappledWall( Vector *vecNormal, Vector *vecPos, CBaseEntity *pEnt )
{
	if( vecNormal )
	{
		/*
		QAngle punchAng;

		punchAng.x = random->RandomFloat( -0.5f, 0.5f );
		punchAng.y = random->RandomFloat( -0.5f, 0.5f );
		punchAng.z = 0.0f;
		
		m_pOuter->ViewPunch( punchAng );
		*/
		// We're grappling!
		m_bGrappledWall = true;

		// Should probably make a "OnStartGrapple" function here later if needed
		if( m_flGrappleTime == -1 )
		{
			m_flGrappleTime = gpGlobals->curtime;

			// Always allow for jumps after grapple
			m_iJumpCount++;
		}
		
	}
	else
	{
		m_bGrappledWall = false;
		m_flGrappleTime = -1;
	}

	m_WallData.m_hWallEnt = pEnt;

	if( vecPos )
		m_WallData.m_vecGrapplePos = *vecPos;

	if( vecNormal )
		m_WallData.m_vecGrappledNormal = *vecNormal;
}

float CSDKPlayerShared::GetSpeedMultiplier()
{
	float flSpeed = 1;

	if( Sprinting() )
		flSpeed *= ClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flSprintMultiplier;


	return flSpeed;
}

bool CSDKPlayerShared::Sprinting()
{
	// Don't sprint if ya can't sprint
	if( ClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flSprintMultiplier == 1.0f )
		return false;

	// Can't move fast if we're ducking
	if( m_pOuter->GetFlags() & FL_DUCKING )
		return false;
	
	// SPEEEEN
	return !!(m_pOuter->m_nButtons & IN_SPEED);
}

void CSDKPlayerShared::SetSliding( bool bSliding, bool bForce )
{
	bSliding = !GetClass(m_pOuter).bCanSlide && !bForce ? false : bSliding;

	if( bSliding )
	{
		m_bSliding = true;
		Vector vecVelocity = m_pOuter->GetAbsVelocity();
		vecVelocity[2] = vecVelocity[2] > 0 ? 0 : vecVelocity[2];
		m_flSprintSpeed = MAX(vecVelocity.Length(), sv_slidespeed.GetFloat());

		Vector vecWishDir;
		AngleVectors(m_pOuter->EyeAngles(), &vecWishDir);
		VectorNormalize(vecWishDir);
		m_vecSlideDir = vecWishDir;
	}
	else
	{
		m_bSliding = false;
		m_flSprintSpeed = 0.0f;
		m_vecSlideDir = vec3_origin;
	}
}

bool CSDKPlayerShared::IsSliding()
{
	return m_bSliding;
}

//======================
// Functions shared between Client and Server
//======================

float CSDKPlayer::GetPlayerMaxSpeed()
{
	int iBaseSpeed = BaseClass::GetPlayerMaxSpeed();
	
	return iBaseSpeed;
}

void CSDKPlayer::UpdateStepSound(surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity)
{
	if( m_flStepSoundTime > 0 )
	{
		m_flStepSoundTime -= m_PlayerShared.GetSpeedMultiplier() * (1000.0f) * gpGlobals->frametime;
		if( m_flStepSoundTime < 0 )
		{
			m_flStepSoundTime = 0;
		}
	}

	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKPlayer::SetStepSoundTime(stepsoundtimes_t iStepSoundTime, bool bWalking)
{
	switch( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = GetClass(this).flStepSpeed;
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = 600;
		break;

	default:
		Assert(0);
		break;
	}

	// UNDONE: need defined numbers for run, walk, crouch, crouch run velocities!!!!	
	if ((GetFlags() & FL_DUCKING) || (GetMoveType() == MOVETYPE_LADDER))
	{
		m_flStepSoundTime *= 1.33333333f;
	}
}

// Shared Functions
const QAngle &CSDKPlayer::EyeAngles(void)
{
	if( m_PlayerShared.GrappledWall() && m_PlayerShared.m_WallData.m_hWallEnt )
		return pl.v_angle;

	return BaseClass::EyeAngles();
}

CBaseSDKCombatWeapon* CSDKPlayer::SDKAnim_GetActiveWeapon()
{
	return GetActiveSDKWeapon();
}


bool CSDKPlayer::SDKAnim_CanMove()
{
	return true;
}


void CSDKPlayer::FireBullet( 
						   Vector vecSrc,	// shooting postion
						   const QAngle &shootAngles,  //shooting angle
						   float vecSpread, // spread vector
						   int iDamage, // base damage
						   int iBulletType, // ammo type
						   CBaseEntity *pevAttacker, // shooter
						   bool bDoEffects,	// create impact effect ?
						   float x,	// spread x factor
						   float y	// spread y factor
						   )
{
	float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	if ( !pevAttacker )
		pevAttacker = this;  // the default attacker is ourselves

	// add the spray 
	Vector vecDir = vecDirShooting +
		x * vecSpread * vecRight +
		y * vecSpread * vecUp;

	VectorNormalize( vecDir );

	float flMaxRange = 8000;

	Vector vecEnd = vecSrc + vecDir * flMaxRange; // max bullet range is 10000 units

	trace_t tr; // main enter bullet trace

	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
			return; // we didn't hit anything, stop tracing shoot

	if ( sv_showimpacts.GetBool() )
	{
#ifdef CLIENT_DLL
		// draw red client impact markers
		debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawClientHitboxes( 4, true );
		}
#else
		// draw blue server impact markers
		NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawServerHitboxes( 4, true );
		}
#endif
	}

		//calculate the damage based on the distance the bullet travelled.
		flCurrentDistance += tr.fraction * flMaxRange;

		// damage get weaker of distance
		fCurrentDamage *= pow ( 0.85f, (flCurrentDistance / 500));

		CUtlVector<int> hDamageType;
		hDamageType.AddToTail( DMG_BULLET );
		hDamageType.AddToTail( DMG_NEVERGIB );

		if( bDoEffects )
		{
			// See if the bullet ended up underwater + started out of the water
			if ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) )
			{	
				trace_t waterTrace;
				UTIL_TraceLine( vecSrc, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace );

				if( waterTrace.allsolid != 1 )
				{
					CEffectData	data;
					data.m_vOrigin = waterTrace.endpos;
					data.m_vNormal = waterTrace.plane.normal;
					data.m_flScale = random->RandomFloat( 8, 12 );

					if ( waterTrace.contents & CONTENTS_SLIME )
					{
						data.m_fFlags |= FX_WATER_IN_SLIME;
					}

					DispatchEffect( "gunshotsplash", data );
				}
			}
			else
			{
				//Do Regular hit effects

				// Don't decal nodraw surfaces
				if ( !( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) ) )
				{
//					CBaseEntity *pEntity = tr.m_pEnt;
//					if ( !( !friendlyfire.GetBool() && pEntity && pEntity->IsPlayer() && pEntity->GetTeamNumber() == GetTeamNumber() ) )
					{
						UTIL_ImpactTrace( &tr, &hDamageType );
					}
				}
			}
		} // bDoEffects

		// add damage to entity that we hit

#ifdef GAME_DLL
		ClearMultiDamage();

		CTakeDamageInfo info( pevAttacker, pevAttacker, fCurrentDamage, &hDamageType );
		CalculateBulletDamageForce( &info, iBulletType, vecDir, tr.endpos );
		tr.m_pEnt->DispatchTraceAttack( info, vecDir, &tr );

		TraceAttackToTriggers( info, tr.startpos, tr.endpos, vecDir );

		ApplyMultiDamage();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapon if already owns a weapon of this class
//-----------------------------------------------------------------------------
CBaseCombatWeapon* CSDKPlayer::Weapon_OwnsThisType( const char *pszWeapon, int iSubType ) const
{
	for ( int i = 0; i < MAX_WEAPONS; i++ ) 
	{
		if( m_hMyWeapons[i].Get() && FStrEq( pszWeapon, m_hMyWeapons[i]->GetSchemaName() ) )
		{
			// Make sure it matches the subtype
			if ( m_hMyWeapons[i]->GetSubType() == iSubType )
			{
				return m_hMyWeapons[i];
			}
		}
	}

	return NULL;
}

void CSDKPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
#ifdef GAME_DLL
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
#endif
}