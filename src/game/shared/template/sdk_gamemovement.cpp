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

class COFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( COFGameMovement, CGameMovement );
	
	COFGameMovement( void );
	virtual			~COFGameMovement( void );

	virtual void	ProcessMovement( CBasePlayer *pBasePlayer, CMoveData *pMove );
	virtual	int		TryPlayerMove( Vector *pFirstDest, trace_t *pFirstTrace  );

	virtual void	ReduceTimers(void);

	virtual Vector	GetWishVelocity( Vector &forward, Vector &right );

	virtual bool	CheckJumpButton(void);

	virtual void	AddGravity(void);
	virtual void	StartGravity(void);
	virtual void	FinishGravity(void);

	virtual void	CheckParameters(void);

	virtual void	StartDuck(void);
	virtual void	FinishUnDuck(void);

	virtual void	Friction(void);

	virtual float	MaxSlopeAngle(void);
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

	// Reset our groundede time while we're grappled
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

int COFGameMovement::TryPlayerMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	int iBlocker = 0;

	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;		
	
	numbumps  = 4;           // Bump up to four times
	
	iBlocker = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (mv->m_vecVelocity, original_velocity);  // Store original velocity
	VectorCopy (mv->m_vecVelocity, primal_velocity);
	
	allFraction = 0;
	time_left = gpGlobals->frametime;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( mv->m_vecVelocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA( mv->GetAbsOrigin(), time_left, mv->m_vecVelocity, end );

		// See if we can make it from origin to end point.
		if ( g_bMovementOptimizations )
		{
			// If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
			if ( pFirstDest && ( end == *pFirstDest ) )
			{
				pm = *pFirstTrace;
			}
			else
			{
				TracePlayerBBox( mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
			}
		}
		else
		{
			TracePlayerBBox( mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if( pm.allsolid )
		{	
			// entity is trapped in another solid
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{
			if( numbumps > 0 && pm.fraction == 1 )
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				trace_t stuck;
				TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, stuck );
				if( stuck.startsolid || stuck.fraction != 1.0f )
				{
					//Msg( "Player will become stuck!!!\n" );
					VectorCopy( vec3_origin, mv->m_vecVelocity );
					break;
				}
			}

			// actually covered some distance
			mv->SetAbsOrigin( pm.endpos );
			VectorCopy( mv->m_vecVelocity, original_velocity );
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if( pm.fraction == 1 )
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if( pm.plane.normal[2] > 0.7 )
		{
			iBlocker |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if( !pm.plane.normal[2] )
		{
			iBlocker |= 2;		// step / wall

			if( m_pOFPlayer->m_PlayerShared.m_flGrappleTime == -1 &&
				player->GetGroundEntity() == NULL 
				&& pm.m_pEnt->IsWorld()
				&& (iBlocker & 2) )
			{
				m_pOFPlayer->m_PlayerShared.SetGrappledWall( &pm.plane.normal );
				m_pOFPlayer->SetParent(pm.m_pEnt);
				m_pOFPlayer->m_PlayerShared.m_flGrappleTime = gpGlobals->curtime;
			}
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if( numplanes >= MAX_CLIP_PLANES )
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy( pm.plane.normal, planes[numplanes] );
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			player->GetMoveType() == MOVETYPE_WALK &&
			player->GetGroundEntity() == NULL )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - player->m_surfaceFriction) );
				}
			}

			VectorCopy( new_velocity, mv->m_vecVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity( original_velocity, planes[i], mv->m_vecVelocity, 1 );

				for (j=0 ; j<numplanes ; j++)
				{
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv->m_vecVelocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				}
				
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				dir.NormalizeInPlace();
				d = dir.Dot(mv->m_vecVelocity);
				VectorScale (dir, d, mv->m_vecVelocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, mv->m_vecVelocity);
				break;
			}
		}
	}
	/*
	if( iBlocker & 2 )
	{
		mv->m_vecVelocity.Init(0, 0, 300);
		original_velocity.Init(0, 0, 300);
	}
	*/
	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, mv->m_vecVelocity);
	}

	// Check if they slammed into a wall
	float fSlamVol = 0.0f;

	float fLateralStoppingAmount = primal_velocity.Length2D() - mv->m_vecVelocity.Length2D();
	if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f )
	{
		fSlamVol = 1.0f;
	}
	else if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED )
	{
		fSlamVol = 0.85f;
	}

	if ( fSlamVol > 0.0f )
	{
		PlayerRoughLandingEffects( fSlamVol );
	}

	if( m_pOFPlayer->m_PlayerShared.GrappledWall() && gpGlobals->curtime - m_pOFPlayer->m_PlayerShared.m_flGrappleTime > m_pOFPlayer->m_PlayerShared.GetWallJumpTime() )
	{
		m_pOFPlayer->m_PlayerShared.SetGrappledWall( NULL );
	}

	return iBlocker;
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
		mv->m_flMaxSpeed = SLIDE_SPEED;
		float flSpeed = m_pOFPlayer->m_PlayerShared.m_flSprintSpeed;
		m_pOFPlayer->m_PlayerShared.m_flSprintSpeed -= gpGlobals->frametime * (sv_slidespeed.GetFloat() / sv_slidetime.GetFloat());
		if( flSpeed > 0 )
			return m_pOFPlayer->m_PlayerShared.m_vecSlideDir * flSpeed;
	}

	return BaseClass::GetWishVelocity(forward, right);
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

	bool bWasGrounded = !!player->GetGroundEntity();

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
	
	player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );
	
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
	if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) || !bWasGrounded )
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

		Vector vecNormal = (player->EyePosition() + (m_pOFPlayer->m_PlayerShared.m_vecGrappledNormal * 1000.0f)) - player->EyePosition();
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

		m_pOFPlayer->m_PlayerShared.m_flGrappleTime = -1;
		m_pOFPlayer->m_PlayerShared.SetGrappledWall(false);
	}

	FinishGravity();

	CheckV( player->CurrentCommandNumber(), "CheckJump", mv->m_vecVelocity );

	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - startz;
	mv->m_outStepHeight += 0.15f;


	bool bSetDuckJump = (gpGlobals->maxClients == 1); //most games we only set duck jump if the game is single player


	// Set jump time.
	if ( bSetDuckJump )
	{
		player->m_Local.m_nJumpTimeMsecs = GAMEMOVEMENT_JUMP_TIME;
		player->m_Local.m_bInDuckJump = true;
	}

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

	if ( player->m_flWaterJumpTime )
		return;

	if ( m_pOFPlayer->m_PlayerShared.GrappledWall() )
		return;

	if (player->GetGravity())
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

	if( m_pOFPlayer->m_PlayerShared.GrappledWall() )
		ent_gravity = 0;

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

	if ( m_pOFPlayer->m_PlayerShared.GrappledWall() )
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
	if( m_pOFPlayer->m_PlayerShared.GrappledWall() )
	{
		mv->m_flForwardMove = 0;
		mv->m_flUpMove = 0;
		mv->m_flSideMove = 0;

		mv->m_vecVelocity[2] = 0;
	}

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
			QAngle angSlide, angWish;
			VectorAngles(m_pOFPlayer->m_PlayerShared.m_vecSlideDir, angSlide);
			VectorAngles(wishdir, angWish);

			float flTargetAng = angWish.y;

			if( FloatMakePositive( angSlide.y - angWish.y ) > 180 )
			{
				flTargetAng = angWish.y > angSlide.y ? angSlide.y - 360 : angSlide.y + 360;
			}

			if( flTargetAng > angSlide.y )
			{
				angSlide.y = MIN(flTargetAng, angSlide.y + (gpGlobals->frametime * sv_slideturnspeed.GetFloat()));
			}
			else
			{
				angSlide.y = MAX(flTargetAng, angSlide.y - (gpGlobals->frametime * sv_slideturnspeed.GetFloat()));
			}

			if( angSlide.y >= 360 )
				angSlide.y -= 360;

			AngleVectors(angSlide, &m_pOFPlayer->m_PlayerShared.m_vecSlideDir.GetForModify());
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
		trace_t trace;
		TracePlayerBBox(mv->GetAbsOrigin(),
			mv->GetAbsOrigin() - Vector(0,0, player->m_Local.m_flStepSize + DIST_EPSILON),
			PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);

		if( trace.plane.normal[2] < 0.9 )
		{
			Vector vecVelocity = mv->m_vecVelocity;
			Vector vecNormal = trace.plane.normal;
			vecVelocity.z = 0;
			vecNormal.z = 0;

			QAngle angVelocity, angNormal;
			VectorAngles(vecVelocity, angVelocity);
			VectorAngles(vecNormal, angNormal);

			float flSteepness = (1 - trace.plane.normal[2]);
			float flFacingWall = FloatMakePositive(AngleDiff(angVelocity.y, angNormal.y));
			
			// Grind to a halt if you're going upwards
			if( flFacingWall > 90 )
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