#include "cbase.h"
#include "gamemovement.h"
#include "in_buttons.h"
#include "sdk_player_shared.h"
#include "of_shared_schemas.h"
#include "coordsize.h"

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Interpolate Euler angles using quaternions to avoid singularities
// Input  : start - 
//			end - 
//			output - 
//			frac - 
//-----------------------------------------------------------------------------
void InterpolateAngles(const QAngle& start, const QAngle& end, QAngle& output, float frac)
{
	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion(start, src);
	AngleQuaternion(end, dest);

	Quaternion result;

	// Slerp
	QuaternionSlerp(src, dest, frac, result);

	// Convert to euler
	QuaternionAngles(result, output);
}
#endif

Vector RotateVectorTowardsVector( Vector vecVector, Vector vecTarget, float flAngle )
{
	Vector vecUnit = CrossProduct(vecVector, vecTarget);

	return (cos(flAngle) * vecVector) + (sin(flAngle)*CrossProduct(vecUnit, vecVector)) + ((1 - cos(flAngle))*DotProduct(vecUnit, vecVector)*vecUnit);
}

class COFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( COFGameMovement, CGameMovement );
	
	COFGameMovement( void );
	virtual			~COFGameMovement( void );

	virtual void	ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove );
	virtual void	PlayerMove( void );
	virtual	int		TryPlayerMove(Vector *pFirstDest = NULL, trace_t *pFirstTrace = NULL);

	virtual void	ReduceTimers( void );

	virtual Vector	GetWishVelocity( Vector &forward, Vector &right );

	virtual void	WallMove( void );
	virtual bool	StayOnWall( void );

	virtual bool	CheckJumpButton( void );

	virtual void	AddGravity( void );
	virtual void	StartGravity( void );
	virtual void	FinishGravity( void );

	virtual void	CheckParameters( void );

	virtual void	StartDuck( void );
	virtual void	FinishUnDuck( void );

	virtual void	Friction( void );

	virtual float	MaxSlopeAngle( void );

	virtual bool	SlideIsSteep( bool *bDownwards = NULL, float *flSteepness = NULL, float *flAngle = NULL );
private:
	CSDKPlayer *m_pOFPlayer;
};

extern bool g_bMovementOptimizations;
extern ConVar sv_gravity;
extern ConVar sv_jumpheight;
extern ConVar sv_bounce;
extern void DiffPrint(bool bServer, int nCommandNumber, char const *fmt, ...);

void _CheckV(int tick, char const *ctx, const Vector &vel)
{
	DiffPrint(CBaseEntity::IsServer(), tick, "%20.20s %f %f %f", ctx, vel.x, vel.y, vel.z);
}

#define CheckV( tick, ctx, vel ) _CheckV( tick, ctx, vel );
#define	MAX_CLIP_PLANES		5

#define MAX_SLIDE_TIME 0.1f
#define SLIDE_SPEED sv_slidespeed.GetFloat()

static COFGameMovement g_GameMovement;
IGameMovement* g_pGameMovement = (IGameMovement*)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(COFGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

ConVar sv_kayotetime    ( "sv_kayotetime"	 , "0.1", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar sv_slidespeed    ( "sv_slidespeed"	 , "600", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar sv_slidetime     ( "sv_slidetime"	 , "1.0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar sv_slidefriction ( "sv_slidefriction" , "0.4", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar sv_slideturnspeed( "sv_slideturnspeed", "180", FCVAR_REPLICATED | FCVAR_NOTIFY );
extern ConVar sv_stopspeed;

COFGameMovement::COFGameMovement( void )
{
	m_pOFPlayer = NULL;
}

COFGameMovement::~COFGameMovement()
{
	m_pOFPlayer = NULL;
}

void COFGameMovement::ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove )
{
	// Verify data.
	Assert(pBasePlayer);
	Assert(pMove);
	if (!pBasePlayer || !pMove)
		return;
	
	m_pOFPlayer = ToSDKPlayer(pBasePlayer);

	// Reset our grounded time while we're grappled
	if( m_pOFPlayer->m_PlayerShared.GrappledWall() )
		m_pOFPlayer->m_PlayerShared.m_flLastGrounded = gpGlobals->curtime;

	if( m_pOFPlayer->m_PlayerShared.Sprinting() )
		m_pOFPlayer->m_PlayerShared.m_flLastSprint = gpGlobals->curtime;

	if( m_pOFPlayer->GetGroundEntity() == NULL															// When we're airborn
		&& gpGlobals->curtime - m_pOFPlayer->m_PlayerShared.m_flLastGrounded > sv_kayotetime.GetFloat() // And our Kayote time ran out
		&& m_pOFPlayer->m_PlayerShared.m_iJumpCount == GetClass(m_pOFPlayer).iJumpCount)				// AND we're at max jumps
		m_pOFPlayer->m_PlayerShared.m_iJumpCount--;														// Remove one jump

	BaseClass::ProcessMovement(pBasePlayer, pMove);
}

void COFGameMovement::PlayerMove( void )
{
	if( m_pOFPlayer->m_PlayerShared.GrappledWall() )
		player->SetMoveType( MOVETYPE_CUSTOM );

	BaseClass::PlayerMove();

	if( player->GetMoveType() == MOVETYPE_CUSTOM )
	{
		// The only Special Movetype for now, later on we'll do checks for other things if needed
		WallMove();
	}
}

int COFGameMovement::TryPlayerMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	return BaseClass::TryPlayerMove(pFirstDest, pFirstTrace);
}

void COFGameMovement::ReduceTimers(void)
{
	float frame_msec = 1000.0f * gpGlobals->frametime;
	int nFrameMsec = (int)frame_msec;

	int nDuckTimeOld = player->m_Local.m_nDuckTimeMsecs;

	BaseClass::ReduceTimers();

	player->m_Local.m_nDuckTimeMsecs = nDuckTimeOld;

	if( player->m_Local.m_nDuckTimeMsecs > 0 )
	{
		// Quick slide duck
		if( m_pOFPlayer->m_PlayerShared.IsSliding() )
			nFrameMsec *= 3;

		player->m_Local.m_nDuckTimeMsecs -= nFrameMsec;
		if( player->m_Local.m_nDuckTimeMsecs < 0 )
		{
			player->m_Local.m_nDuckTimeMsecs = 0;
		}
	}
}

Vector COFGameMovement::GetWishVelocity( Vector &forward, Vector &right )
{
	if( m_pOFPlayer->m_PlayerShared.IsSliding() )
	{
		float flSpeed = m_pOFPlayer->m_PlayerShared.m_flSprintSpeed;
		mv->m_flMaxSpeed = flSpeed;

		bool bDownwards;
		float flSteepness;
		float flFacingWall;
		float flSpeedModifier = 1;
		
		if( SlideIsSteep(&bDownwards, &flSteepness, &flFacingWall) )
		{
			// Don't slow down if we're going down
			if( bDownwards )
				flSpeedModifier = -flSteepness;
			else
				flSpeedModifier += (10.0f * flSteepness * ((flFacingWall - 90) / 90.0f));
		}
		m_pOFPlayer->m_PlayerShared.m_flSprintSpeed -= gpGlobals->frametime * (sv_slidespeed.GetFloat() / sv_slidetime.GetFloat()) * flSpeedModifier;
		if( flSpeed > 0 )
			return m_pOFPlayer->m_PlayerShared.m_vecSlideDir * flSpeed;
	}

	return BaseClass::GetWishVelocity(forward, right);
}

extern ConVar sv_accelerate;

void COFGameMovement::WallMove( void )
{
	bool bUsedWallMove = false;
	bool bEndGrapple = false;

	// Was jump button pressed?
	if( mv->m_nButtons & IN_JUMP )
	{
		// Ignore the rest if we jumped
		bool bJumped = CheckJumpButton();

		bUsedWallMove |= bJumped;
		bEndGrapple |= bJumped;

	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}

	Vector forward, right, up;

	AngleVectors( mv->m_vecViewAngles, &forward, &right, &up );  // Determine movement angles

	Vector vecWishVel = GetWishVelocity( forward, right );

	// We can wall climb and have time
	if( !bUsedWallMove && GetClass(m_pOFPlayer).flWallClimbTime 	// And we're facing the wall directly
		&& vecWishVel.Length() && forward.AngTo(m_pOFPlayer->m_PlayerShared.m_WallData.m_vecGrappledNormal) > 125)
	{
		bUsedWallMove = true;

		// If we have ANY side movement, turn it up
		if( mv->m_vecVelocity[0] || mv->m_vecVelocity[1] )
		{
			mv->m_vecVelocity[2] = 0;
			mv->m_vecVelocity[0] = 0;
			mv->m_vecVelocity[1] = 0;
		}

		Vector vecWishClimb(0, 0, vecWishVel.Length());

		Vector dest;
		// first try just moving to the destination	
		dest[0] = mv->GetAbsOrigin()[0];
		dest[1] = mv->GetAbsOrigin()[1];	
		dest[2] = mv->GetAbsOrigin()[2] + (vecWishClimb[2] * gpGlobals->frametime);

		trace_t pm;
		// first try moving directly to the next spot
		TracePlayerBBox( mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		// If we made it all the way, then copy trace end as new player position.
		mv->m_outWishVel += VectorNormalize(vecWishClimb) * vecWishClimb.Length();

		mv->SetAbsOrigin( pm.endpos );
		m_pOFPlayer->SetAbsOrigin( pm.endpos );

		// Reset grapple time
		if( m_pOFPlayer->m_PlayerShared.m_flGrappleTime )
			m_pOFPlayer->m_PlayerShared.m_flGrappleTime = gpGlobals->curtime;
	}
	else if( !bUsedWallMove && GetClass(m_pOFPlayer).flWallRunTime 	// And we're not facing the wall directly
		&& vecWishVel.Length() )
	{
		bUsedWallMove = true;

		// Don't move down while wall running
		mv->m_vecVelocity[2] *= !!mv->m_vecVelocity[0];

		Vector vecWishRun = RotateVectorTowardsVector(m_pOFPlayer->m_PlayerShared.m_WallData.m_vecGrappledNormal, forward*Vector(1,1,0), M_PI/2);
		VectorNormalize(vecWishRun);

		vecWishRun *= 400;

		Vector dest;
		// first try just moving to the destination	
		dest[0] = mv->GetAbsOrigin()[0] + (vecWishRun[0] * gpGlobals->frametime);
		dest[1] = mv->GetAbsOrigin()[1] + (vecWishRun[1] * gpGlobals->frametime);
		dest[2] = mv->GetAbsOrigin()[2];

		trace_t pm;
		// first try moving directly to the next spot
		TracePlayerBBox( mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		// If we made it all the way, then copy trace end as new player position.
		mv->m_outWishVel += VectorNormalize(vecWishRun) * vecWishRun.Length();

		mv->SetAbsOrigin( pm.endpos );
		m_pOFPlayer->SetAbsOrigin( pm.endpos );

		// Reset grapple time
		if( m_pOFPlayer->m_PlayerShared.m_flGrappleTime )
			m_pOFPlayer->m_PlayerShared.m_flGrappleTime = gpGlobals->curtime;
	}

	bEndGrapple |= (!bUsedWallMove || m_pOFPlayer->m_PlayerShared.GetWallJumpTime()) && gpGlobals->curtime - m_pOFPlayer->m_PlayerShared.m_flGrappleTime > m_pOFPlayer->m_PlayerShared.GetWallJumpTime();

	bEndGrapple |= !StayOnWall();

	if( bEndGrapple )
	{
		m_pOFPlayer->m_PlayerShared.SetGrappledWall( NULL, NULL );
		player->SetMoveType( MOVETYPE_WALK );
		m_pOFPlayer->SetParent(NULL);

		FullWalkMove();
		// Don't go further if our grapple time is over
		return;
	}

	// Not moved via any wall movement methods, so just lower our stored velocity
	// Don't instantly set it to 0 so we can somewhat keep our momentum if we time it right
	if( !bUsedWallMove && mv->m_vecVelocity.Length() > 0 )
	{
		// Fairly quick decel, ~16 per frame
		float accelspeed = 1000 * gpGlobals->frametime;

		for( int i = 0; i < 3 ; i++ )
		{
			if( mv->m_vecVelocity[i] < 0 )
				mv->m_vecVelocity[i] = MIN( 0, mv->m_vecVelocity[i] + accelspeed );
			else
				mv->m_vecVelocity[i] = MAX( 0, mv->m_vecVelocity[i] - accelspeed );
		}

	}
	return;
	/*
		Demo,,,.man..
													  ,##                                                                   
											  #*####%%%%&%,,,/                                                              
											##%%%&%%%%%%@&#*****,,                         *(*         (###%                
										   &&%%%%&@&&&%&&#######(//,%(,**%%####(((////*,,*%%##%%%#% #&&&&&&&%%%             
											&&#%%&&&&&#&&%#####(####%&(%##&####%%%#((((##%%%@@@&&&* /&&&&&&&&&&#%            
											 &%&%&&%&&&##%%########%&&#&&&&&&&&&&&&&%*##((#@@@@@&@@@&&&%&##(%%*@@*          
											  &&&&&&&%%%&%%##########&&&&&&&&&&&&%*,(#&&%%#,,*&&@@@@@@@@%%%%##%%@@*         
											 &&&&&&&&&&&%&&&%%###/@&&&&&&@&&&&@* /*&%%#/#&%#%&&&@@@@@@@&&&&&%%&@@@/%//%      
											&&&&&&&&&&&&&&&&&&*&&&&&&&&&&&&   #%##(#&%#* /#%&&&&&%@@@@@@&&%%%&@@@@@&(@&%(#   
												@@@@@&&&   &&%@&&&&&&&&&#######&&%##*%%&&&&&&&&%&&&%%@@@@&%%@@@@@@@&%###@&&&
												@@@@@&&      &&&&&&&#######%%%%%&&#%&&&&&&&&&&&&&#&&&&%%@@@@@@@@@&%%%&@%%&&&
												@@@@&&       %%&&&&&@#######%%###%&&&@&@@@&&&&&&&&&(&&&&//(/@@@&&%%&&%%%%%  
											   (@@&&&&        *@@&&&&@#############%@@@@@&&&&&&&&&&&&(&&&#((%%&&&@&%%%%%%%  
											  .%%%%%%%(        &&@@&&&&##%####%%%%%##&&&&&&&&&&&&&&//%(@@&&&%%%%&%#//(((#%  
											  #%%%%%%%(      /%&&&&&@@&%##%###%%%%%#(#&&&&&&&&%/(@&#%#%%&@@&&%%%((***##(/#  
											(####%%################&&&&@########%%%%(((&&&&&(%&(/%%@&&&%%%%@@###((/,#/(/(   
								 ((%%%###(/&%%%%%%%%###############%&&@&@###&@@@@@@@@@@@@@@@@@&&&#%&&@&&%%#@%%#(((#######   
			   #/#(/%          #&&%%&&&&&&&&&%%%%%%%%##########%%&&&&&&@@@&&&&@@%%#%%@@@@@@@@@@@&&&%%*@@@@#(((((((#%%%%%%   
	 %#&&%%%%%%&%%%%%%%%%%%(#&&&&&%%%&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&#(#(&&@&@@@@%%#,,,,,,,,,,,,,,,,,&&###((###    
	%%&&&@@@@@@&&&&&&&&&&&@@@@@@&&&&%%&&&&&&&&&&&&&&&((((           @@&&&&@&####%%&&@@%#####%&%*,(,,,,,,,,,,,,,,*&&%%%%,    
	&&@@@@@@@@@@&&&&&&&@&&&&     *&&&&&&&&&&                       @@&@@@&##(#%%&&@&&%%%%%%&&&&&##################&&&&%     
	  &@@(                          %&#                            &@%%%####%%&&@@@&%%%%%%%&&&&&%#################%&&&      
	*/
}

bool COFGameMovement::StayOnWall( void )
{
	Vector forward, right, up;

	trace_t pm;
	TracePlayerBBox(mv->GetAbsOrigin(), mv->GetAbsOrigin() +		// Unlike step size, our calc's don't start at the end of our collision box, so move it out first
		(-1 * m_pOFPlayer->m_PlayerShared.m_WallData.m_vecGrappledNormal * (player->GetStepSize() + GetPlayerMaxs().AsVector2D().DistTo(GetPlayerMins().AsVector2D()))),
		PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);

	if( pm.fraction < 1.0f &&	// must hit something
		!pm.startsolid 			// can't be embedded in a solid
	)
	{
		mv->SetAbsOrigin(pm.endpos);

		// Reset our wall in case we hit another normal
		m_pOFPlayer->m_PlayerShared.OnHitWall(&pm);

		return true;
	}

	return false || pm.DidHit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool COFGameMovement::CheckJumpButton( void )
{
	if( player->pl.deadflag )
	{
		mv->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
		return false;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if( player->m_flWaterJumpTime )
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
			player->m_flWaterJumpTime = 0;
		
		return false;
	}

	// If we are in the water most of the way...
	if ( player->GetWaterLevel() >= 2 )
	{	
		// swimming, not jumping
		SetGroundEntity( NULL );

		if(player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			mv->m_vecVelocity[2] = 100;
		else if (player->GetWaterType() == CONTENTS_SLIME)
			mv->m_vecVelocity[2] = 80;
		
		// play swiming sound
		if ( player->m_flSwimSoundTime <= 0 )
		{
			// Don't play sound again for 1 second
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	bool bUsedKayoteTime = gpGlobals->curtime - m_pOFPlayer->m_PlayerShared.m_flLastGrounded < sv_kayotetime.GetFloat();
	bool bWasGrounded = !!player->GetGroundEntity() || bUsedKayoteTime;
	// No more effect
 	if( m_pOFPlayer->m_PlayerShared.m_iJumpCount <= 0 )
	{
		mv->m_nOldButtons |= IN_JUMP;
		return false;		// in air, so no effect
	}

	// Don't allow jumping when the player is in a stasis field.
#ifndef HL2_EPISODIC
	if ( player->m_Local.m_bSlowMovement )
		return false;
#endif

	if ( mv->m_nOldButtons & IN_JUMP )
		return false;		// don't pogo stick

	// Cannot jump will in the unduck transition.
	if ( player->m_Local.m_bDucking && (  player->GetFlags() & FL_DUCKING ) )
		return false;

	// Still updating the eye position.
	if ( player->m_Local.m_nDuckJumpTimeMsecs > 0 )
		return false;

	// In the air now.
    SetGroundEntity( NULL );

	// Subtract one jump
	m_pOFPlayer->m_PlayerShared.m_iJumpCount--;

	m_pOFPlayer->m_PlayerShared.SetSliding( false );
	
	if( bWasGrounded )
	{
		player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );

		// Jumping instantly removes your kayote time privelages
		m_pOFPlayer->m_PlayerShared.m_flLastGrounded -= sv_kayotetime.GetFloat();
	}
	MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );

	float flGroundFactor = 1.0f;
	if (player->m_pSurfaceData)
	{
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor; 
	}

	float flMul;
	if ( g_bMovementOptimizations )
	{
#if defined(HL2_DLL) || defined(HL2_CLIENT_DLL) || defined(WORLD_USE_HL2_GRAVITY)
		Assert( sv_gravity.GetFloat() == 600.0f );
		flMul = 160.0f;	// approx. 21 units.
#else
		Assert( sv_gravity.GetFloat() == 800.0f );
		flMul = 268.3281572999747f;
#endif
	}
	else
	{
		flMul = sqrt(2 * sv_gravity.GetFloat() * GAMEMOVEMENT_JUMP_HEIGHT);
	}

	// Acclerate upward
	// If we are ducking or we werent grounded, stop our velocity first
	float startz = mv->m_vecVelocity[2];
	if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) || (bWasGrounded && bUsedKayoteTime) )
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		mv->m_vecVelocity[2] = flGroundFactor * flMul;  // 2 * gravity * height
	}
	else
	{
		mv->m_vecVelocity[2] += flGroundFactor * flMul;  // 2 * gravity * height
	}

	// Add a little forward velocity based on your view if you're Grappled onto a wall

	if ( m_pOFPlayer->m_PlayerShared.GrappledWall() )
	{
		//Get the lunge direction
		Vector vecDir;
		Vector vecTemp;

		QAngle angEyes = player->EyeAngles();

		angEyes.x = 0;

		AngleVectors(angEyes, &vecDir, &vecTemp, &vecTemp);

		//Test the lunge direction to make sure player does not touch the floor in the next few frames
		float lungeSpeed = sqrt(2 * sv_gravity.GetFloat() * GAMEMOVEMENT_JUMP_HEIGHT);
		trace_t pm;

		UTIL_TraceLine(
			player->EyePosition(),
			player->EyePosition() + (vecDir * 1000.0f),
			MASK_PLAYERSOLID,
			player,
			COLLISION_GROUP_PLAYER_MOVEMENT,
			&pm);

		if (pm.fraction != 1.0f) //If the tracer touched the ground
		{
			vecDir = pm.endpos - mv->GetAbsOrigin();
		}

		//get the vector from the player origin to the tracer endpos
		QAngle tempAngle;
		VectorAngles(vecDir, tempAngle);

		Vector vecNormal = (player->EyePosition() + (m_pOFPlayer->m_PlayerShared.m_WallData.m_vecGrappledNormal * 1000.0f)) - player->EyePosition();
		VectorNormalize(vecNormal);
		Vector2D vecForward(vecNormal.x, vecNormal.y);
		QAngle angNormal;
		VectorAngles(vecNormal, angNormal);

		vecNormal = pm.endpos - player->EyePosition();
		VectorNormalize(vecNormal);
		Vector2D vecEyes(vecNormal.x, vecNormal.y);

		float flAdjust = DotProduct2D(vecForward, vecEyes);
		if( flAdjust < 0.4f )
			InterpolateAngles(tempAngle, angNormal, tempAngle, 0.4f - flAdjust);

		AngleVectors(tempAngle, &vecDir);

		//Lounge!
		mv->m_vecVelocity += vecDir * lungeSpeed;

		m_pOFPlayer->m_PlayerShared.SetGrappledWall(NULL, NULL);
	}
	else if( !bWasGrounded )
	{
		QAngle angEyes = player->EyeAngles();

		Vector forward, right, up;
		AngleVectors(angEyes, &forward, &right, &up);
		Vector vecWishVel = GetWishVelocity(forward, right);

		// If we're already moving fast in a certain direction
		// allow us to transfer that velocity to our desired direction
		if( mv->m_vecVelocity.AsVector2D().Length() <= 0 &&
			vecWishVel.AsVector2D().Length() < mv->m_vecVelocity.AsVector2D().Length() )
		{
			VectorNormalize(vecWishVel);
			vecWishVel *= mv->m_vecVelocity.AsVector2D().Length();
		}

		mv->m_vecVelocity[0] = vecWishVel[0];
		mv->m_vecVelocity[1] = vecWishVel[1];
	}

	FinishGravity();

	CheckV( player->CurrentCommandNumber(), "CheckJump", mv->m_vecVelocity );

	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - startz;
	mv->m_outStepHeight += 0.15f;

	player->m_Local.m_nJumpTimeMsecs = GAMEMOVEMENT_JUMP_TIME;
	player->m_Local.m_bInDuckJump = true;

#if defined( HL2_DLL )

	if ( xc_uncrouch_on_jump.GetBool() )
	{
		// Uncrouch when jumping
		if ( player->GetToggledDuckState() )
		{
			player->ToggleDuck();
		}
	}

#endif

	// Flag that we jumped.
	mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
	return true;
}

void COFGameMovement::AddGravity( void )
{
	float ent_gravity;

	if( player->m_flWaterJumpTime )
		return;

	if( player->GetGravity() )
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime);
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;
	Vector temp = player->GetBaseVelocity();
	temp[2] = 0;
	player->SetBaseVelocity( temp );
	
	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFGameMovement::StartGravity( void )
{
	float ent_gravity;
	
	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * 0.5 * gpGlobals->frametime );
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;

	Vector temp = player->GetBaseVelocity();
	temp[ 2 ] = 0;
	player->SetBaseVelocity( temp );

	CheckVelocity();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFGameMovement::FinishGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if ( player->GetGravity() )
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime * 0.5);

	CheckVelocity();
}

void COFGameMovement::CheckParameters(void)
{
	Vector wishdir;
	if( m_pOFPlayer->m_PlayerShared.IsSliding() )
	{
		Vector forward, right, up;
		AngleVectors(mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

		forward[2] = 0;
		VectorNormalize(forward);

		right[2] = 0;
		VectorNormalize(right);

		for( int i = 0; i<2; i++ )       // Determine x and y parts of velocity
			wishdir[i] = forward[i] * mv->m_flForwardMove + right[i] * mv->m_flSideMove;

		wishdir[2] = 0;             // Zero out z part of velocity

		VectorNormalize(wishdir);
	}

	BaseClass::CheckParameters();

	if( m_pOFPlayer->m_PlayerShared.IsSliding() )
	{
		if( mv->m_flForwardMove != 0 || mv->m_flSideMove != 0 )
		{
			m_pOFPlayer->m_PlayerShared.m_vecSlideDir = RotateVectorTowardsVector(
				m_pOFPlayer->m_PlayerShared.m_vecSlideDir, wishdir, gpGlobals->frametime * DEG2RAD(sv_slideturnspeed.GetFloat()));
			/*
			QAngle angSlide, angWish;
			VectorAngles(m_pOFPlayer->m_PlayerShared.m_vecSlideDir, angSlide);
			VectorAngles(wishdir, angWish);

			float flTargetAng = angWish.y;

			if( FloatMakePositive( angSlide.y - angWish.y ) > 180 )
				flTargetAng = angWish.y > angSlide.y ? angSlide.y - 360 : angSlide.y + 360;

			if( flTargetAng > angSlide.y )
				angSlide.y = MIN(flTargetAng, angSlide.y + (gpGlobals->frametime * sv_slideturnspeed.GetFloat()));
			else
				angSlide.y = MAX(flTargetAng, angSlide.y - (gpGlobals->frametime * sv_slideturnspeed.GetFloat()));

			if( angSlide.y >= 360 )
				angSlide.y -= 360;

			AngleVectors(angSlide, &m_pOFPlayer->m_PlayerShared.m_vecSlideDir.GetForModify());
			*/
		}
		if (mv->m_vecVelocity.Length() < m_pOFPlayer->GetPlayerMaxSpeed() * 0.33333333f)
			m_pOFPlayer->m_PlayerShared.SetSliding( false );
	}
}

void COFGameMovement::StartDuck( void )
{
	if( gpGlobals->curtime - m_pOFPlayer->m_PlayerShared.m_flLastSprint < MAX_SLIDE_TIME )
	{
		m_pOFPlayer->m_PlayerShared.SetSliding(true);
	}

	BaseClass::StartDuck();
}

void COFGameMovement::FinishUnDuck( void )
{
	BaseClass::FinishUnDuck();

	m_pOFPlayer->m_PlayerShared.SetSliding( false );
}

void COFGameMovement::Friction( void )
{
	int iOldFriction = player->m_surfaceFriction;

	if( m_pOFPlayer->m_PlayerShared.IsSliding() )
	{
		float flSteepness = 0;
		float flFacingWall = 0;
		bool bDownwards = false;

		if( SlideIsSteep( &bDownwards, &flSteepness, &flFacingWall  ) )
		{
			// Grind to a halt if you're going upwards
			if( bDownwards )
				player->m_surfaceFriction *= ( 10.0f * flSteepness * ( (flFacingWall - 90 ) / 90.0f ) );
			// Otherwise speed up
			else
				player->m_surfaceFriction = 0 - flSteepness;
		}
		else
			player->m_surfaceFriction *= sv_slidefriction.GetFloat();
	}

	BaseClass::Friction();

	player->m_surfaceFriction = iOldFriction;
}

float COFGameMovement::MaxSlopeAngle(void)
{
	return BaseClass::MaxSlopeAngle();
}

bool COFGameMovement::SlideIsSteep( bool *bDownwards, float *flSteepness, float *flAngle )
{
	trace_t trace;
	TracePlayerBBox(mv->GetAbsOrigin(),
		mv->GetAbsOrigin() - Vector(0,0, player->m_Local.m_flStepSize + DIST_EPSILON),
		PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);

	if (trace.plane.normal[2] < 0.9)
	{
		if( bDownwards != nullptr )
		{
			Vector vecVelocity = mv->m_vecVelocity;
			Vector vecNormal = trace.plane.normal;
			vecVelocity.z = 0;
			vecNormal.z = 0;

			QAngle angVelocity, angNormal;
			VectorAngles(vecVelocity, angVelocity);
			VectorAngles(vecNormal, angNormal);

			if( flSteepness != nullptr )
				*flSteepness = (1 - trace.plane.normal[2]);
			float flFacingWall = FloatMakePositive(AngleDiff(angVelocity.y, angNormal.y));

			if( flAngle != nullptr )
				*flAngle = flFacingWall;

			if( flFacingWall > 90 )
				*bDownwards = false;
			else
				*bDownwards = true;
		}
		return true;
	}
	else
		return false;
}