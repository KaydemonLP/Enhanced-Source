//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef SDK_PLAYER_SHARED_H
#define SDK_PLAYER_SHARED_H

#pragma once


// Shared header file for players
#if defined( CLIENT_DLL )
#define CSDKPlayerShared C_SDKPlayerShared
#define CSDKPlayer C_SDKPlayer
#define walldata_t C_walldata_t
#endif

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_PlayerWallData );
	EXTERN_RECV_TABLE( DT_SDKPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_PlayerWallData );
	EXTERN_SEND_TABLE( DT_SDKPlayerShared );

#endif

class walldata_t
{
public:
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE(walldata_t);

	walldata_t();

public:
	CNetworkVector( m_vecGrapplePos );
	CNetworkVector( m_vecGrappledNormal );
	CNetworkHandle( CBaseEntity, m_hWallEnt );
};

class CSDKPlayerShared
{
public:
	friend class CSDKPlayer;
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#else
	DECLARE_DATADESC();
#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CSDKPlayerShared );

	CSDKPlayerShared();
	~CSDKPlayerShared();

	void	OnHitWall( trace_t *trTrace );

	float	GetWallJumpTime();
	void 	SetGrappledWall( Vector *vecNormal, Vector *vecPos, CBaseEntity *pEnt = NULL );
	bool 	GrappledWall()	{ return m_bGrappledWall; }

	float	GetSpeedMultiplier();
	
	bool	Sprinting();

	void	SetSliding( bool bSlide, bool bForce = false );
	bool	IsSliding();
public:
	CSDKPlayer *m_pOuter;
	CNetworkVar( float, m_flGrappleTime );
	CNetworkVar( float, m_flWallClimbTime );
	CNetworkVar( float, m_flWallRunTime );
	walldata_t m_WallData;

	CNetworkVar( float, m_flLastSprint );
	CNetworkVar( float, m_flLastGrounded );
	CNetworkVar( int, m_iJumpCount );

	CNetworkVar( float, m_flSprintSpeed );
	CNetworkVector( m_vecSlideDir );
private:
	CNetworkVar( bool, m_bGrappledWall );
	CNetworkVar( bool, m_bSliding );
};

#endif