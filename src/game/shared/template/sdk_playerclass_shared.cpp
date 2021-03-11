#include "cbase.h"
#include "sdk_player_shared.h"
#include "of_shared_schemas.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

		if( m_flGrappleTime == -1 )
			m_flGrappleTime = gpGlobals->curtime;
	}
}

float CSDKPlayerShared::GetWallJumpTime()
{
	return GetClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flWallJumpTime;
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
		// Always allow for jumps after grapple
		// m_iJumpCount++;
	}
	else
		m_bGrappledWall = false;

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
		flSpeed *= GetClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flSprintMultiplier;


	return flSpeed;
}

bool CSDKPlayerShared::Sprinting()
{
	// Don't sprint if ya can't sprint
	if( GetClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flSprintMultiplier == 1.0f )
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