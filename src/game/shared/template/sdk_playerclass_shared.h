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
#endif

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_SDKPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_SDKPlayerShared );

#endif

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

	float	GetWallJumpTime();
	void 	SetGrappledWall( Vector *vecNormal );
	bool 	GrappledWall()	{ return m_bGrappledWall; }

	float	GetSpeedMultiplier();
	
	bool	Sprinting();

	void	SetSliding( bool bSlide, bool bForce = false );
	bool	IsSliding();
public:
	CSDKPlayer *m_pOuter;
	CNetworkVar( float, m_flGrappleTime );
	CNetworkVector( m_vecGrappledNormal );

	CNetworkVar( float, m_flLastSprint );
	CNetworkVar( float, m_flLastGrounded );
	CNetworkVar( int, m_iJumpCount );

	CNetworkVar( bool, m_bSliding );
	CNetworkVar( float, m_flSprintSpeed );
	CNetworkVector( m_vecSlideDir );
private:
	CNetworkVar( bool, m_bGrappledWall );
};

#endif