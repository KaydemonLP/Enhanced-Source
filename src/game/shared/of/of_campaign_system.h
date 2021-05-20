//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef OF_CAMPAIGN_SYSTEM_H
#define OF_CAMPAIGN_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif
#include "matchmaking/imatchframework.h"
#include "GameEventListener.h"
#include "utlmap.h"

#ifdef CLIENT_DLL
#define COFCampaignSystem C_OFCampaignSystem
#define Csession_player_data_t C_session_player_data_t
#define Csession_map_history_t C_session_map_history_t
#endif

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_CampaignSystem );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_CampaignSystem );
#endif

#define OF_MAX_ATTRIBUTE_LENGHT 64
#ifdef GAME_DLL
#define OF_SESSION_DEPTH_ALL 9999
#endif

struct value_string_t
{
	value_string_t()
	{
		m_szValue[0] = '\0';
	}
	value_string_t( value_string_t const& other )
	{
		Q_strncpy( m_szValue, other.m_szValue, sizeof(m_szValue) );
	}
	value_string_t( char *szValue )
	{
		Q_strncpy( m_szValue, szValue, sizeof(m_szValue) );
	}
	value_string_t( const char *szValue )
	{
		Q_strncpy( m_szValue, szValue, sizeof(m_szValue) );
	}
	char *Get()
	{ return m_szValue; }
public:
	char m_szValue[128];
};

class session_player_data_t
{
public:
	DECLARE_CLASS_NOBASE( session_player_data_t );

	session_player_data_t()
	{
		m_szClass[0] = '\0';
		m_iSteamID = 0;
	};

#ifdef GAME_DLL
	void NetworkPlayer( int iDepth );
#endif

public:
	char m_szClass[64];
	uint32 m_iSteamID;
	CCopyableUtlDict<value_string_t> m_hSkills;
};

class session_map_history_t
{
public:
	DECLARE_CLASS_NOBASE( session_map_history_t );

	session_map_history_t()
	{
		m_szName[0] = '\0';
		m_iOrder = 0;
	};

#ifdef GAME_DLL
	void NetworkMap( int iDepth );
#endif

public:
	char m_szName[128];
	int	m_iOrder;

	CCopyableUtlDict<value_string_t> m_hMapData;
};

extern bool Uint32Compare( uint32 const &lhs, uint32 const &rhs );

class session_t
{
public:
	DECLARE_CLASS_NOBASE(session_t);

	session_t()
	{
		m_szName[0] = '\0';
		m_iHost = 0;
		m_hPlayerData.SetLessFunc( Uint32Compare );
		m_hMapHistory.SetLessFunc( Uint32Compare );
	};
#ifdef GAME_DLL
	void NetworkSession( int iDepth );
#endif
public:
	char m_szName[128];
	int m_iSeed;
	int m_iXP;
	int m_iStage;

	uint32 m_iHost;

	//	KEY, ELEMENT
	CUtlMap< uint32, session_player_data_t > m_hPlayerData;
	CUtlMap< uint32, session_map_history_t > m_hMapHistory;
};

class COFCampaignSystem : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	COFCampaignSystem();
	DECLARE_CLASS( COFCampaignSystem, CAutoGameSystemPerFrame );
	
	// AutoGameSystem
	virtual bool Init();
	/*virtual void PostInit();
	virtual void Shutdown();*/
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();

	virtual bool IsPerFrame( void ) { return true; }

#ifdef CLIENT_DLL
	virtual void Update( float frametime )
	{
	};
#else
	virtual void PreClientUpdate()
	{
	};
#endif

	// Campaign System
	virtual void ClearSession( void );
	virtual bool CreateSession( CCommand args, uint64 iSteamID );
	virtual bool LoadSession( KeyValues *kvSessionData );
	virtual bool SaveSession( void );

	session_map_history_t *GetSessionMapData( uint32 iOrder );
	session_player_data_t *GetSessionPlayerData( uint32 iSteamID );

#ifdef GAME_DLL
	virtual void RegisterPlayer( edict_t *pPlayer );
	virtual void UnregisterPlayer( edict_t *pPlayer );
	virtual void LogCurrentMap( void );
#endif
	virtual void RegisterPlayer( uint64 iSteamID );
	virtual void UnregisterPlayer( uint64 iSteamID );

	// Networking
#ifdef GAME_DLL
	virtual void TransmitSessionToClients();
#endif
	virtual void FireGameEvent( IGameEvent *event );
	
	// This function is here for our CNetworkVars.
	virtual void NetworkStateChanged( void );
	virtual void NetworkStateChanged( void *pVar );

public:
	session_t *m_pActiveSession;

public:
	session_t m_hSession;
};

extern COFCampaignSystem *Campaign();

#endif
