#include "cbase.h"
#include "sdk_player_shared.h"
#include "of_shared_schemas.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_NETWORK_TABLE_NOBASE( CSDKPlayerShared, DT_SDKPlayerShared )
#if !defined( CLIENT_DLL )
	SendPropBool( SENDINFO( m_bGrappledWall ) ),
	SendPropTime( SENDINFO( m_flGrappleTime ) ),
    SendPropVector( SENDINFO( m_vecGrappledNormal ), -1, SPROP_COORD ),

	SendPropTime( SENDINFO( m_flLastSprint ) ),
	SendPropTime( SENDINFO( m_flLastGrounded ) ),
	SendPropInt ( SENDINFO( m_iJumpCount ) ),

	SendPropBool( SENDINFO( m_bSliding ) ),
	SendPropFloat( SENDINFO( m_flSprintSpeed ) ),
	SendPropVector( SENDINFO( m_vecSlideDir ), -1, SPROP_COORD ),
#else
	RecvPropBool( RECVINFO( m_bGrappledWall ) ),
	RecvPropTime( RECVINFO( m_flGrappleTime ) ),
    RecvPropVector( RECVINFO( m_vecGrappledNormal ) ),

	RecvPropTime( RECVINFO( m_flLastSprint ) ),
	RecvPropTime( RECVINFO( m_flLastGrounded ) ),
	RecvPropInt ( RECVINFO( m_iJumpCount ) ),

	RecvPropBool( RECVINFO( m_bSliding ) ),
	RecvPropFloat( RECVINFO( m_flSprintSpeed ) ),
	RecvPropVector( RECVINFO( m_vecSlideDir ) ),
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

float CSDKPlayerShared::GetWallJumpTime()
{
	return GetClassManager()->m_hClassInfo[m_pOuter->GetClassNumber()].flWallJumpTime;
}

void CSDKPlayerShared::SetGrappledWall(Vector *vecNormal)
{
	if( GetWallJumpTime() && vecNormal )
	{
		QAngle punchAng;

		punchAng.x = random->RandomFloat( -0.5f, 0.5f );
		punchAng.y = random->RandomFloat( -0.5f, 0.5f );
		punchAng.z = 0.0f;
		
		m_pOuter->ViewPunch( punchAng );

		m_vecGrappledNormal = *vecNormal;
		m_bGrappledWall = true;
		m_iJumpCount++;
	}
	else
		m_bGrappledWall = false;
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