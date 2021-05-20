//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Game rules for Scratch
//
//=============================================================================

#ifndef SDK_GAMERULES_H
#define SDK_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "teamplay_gamerules.h"

#ifdef CLIENT_DLL
	#define CSDKGameRules C_SDKGameRules
	#define COFGameRulesProxy C_OFGameRulesProxy
#endif

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_OFGameRules );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_OFGameRules );
#endif

class COFGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( COFGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
	virtual void Activate();
	void InputWinRound( inputdata_t &inputdata );
	void InputLoseRound( inputdata_t &inputdata );
#else
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif
public:
#ifdef GAME_DLL
	bool m_bIsLobby;
#endif
};

struct map_difficulty_t
{
	char szMapname[128];
};

class CSDKGameRules : public CTeamplayRules
{
	DECLARE_CLASS( CSDKGameRules, CTeamplayRules );

public:
	CSDKGameRules();
	bool				IsMultiplayer( void );
	void				PlayerThink( CBasePlayer *pPlayer );
	virtual bool		ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool		IsLobby( void ){ return m_bIsLobby; }
#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.

	virtual void		Activate( void );
	virtual bool		ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual void		ClientDisconnected( edict_t *pClient );
	virtual void		Precache( void );
	virtual const char *GetGameDescription( void ) { return "Offshore"; }
	virtual void		CreateStandardEntities( void );

	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );  // handles the user commands;  returns TRUE if command handled properly
//	virtual void PlayerThink( CBasePlayer *pPlayer ) {}

//	virtual void PlayerSpawn( CBasePlayer *pPlayer );

	virtual float		FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual void		InitDefaultAIRelationships( void );
	virtual const char*	AIClassText(int classType);

	virtual void		Think( void );
	virtual void		StartRound( void );
	virtual void		RoundWin( void );
	virtual void		RoundLose( void );
	virtual void		SetupNextMaps( void );
	virtual bool		RoundEnded( void ){ return m_bRoundEnded; }
#endif // CLIENT_DLL

public:
	CUtlVector<CCopyableUtlVector<map_difficulty_t>> m_hMapList;
	CUtlVector<map_difficulty_t> m_hNextMaps;
	CNetworkVar( bool, m_bRoundStarted );
	CNetworkVar( bool, m_bRoundEnded );
private:
	CNetworkVar( bool, m_bIsLobby );
	friend class COFGameRulesProxy;
	// misc
//	virtual void CreateStandardEntities( void );	
};

//-----------------------------------------------------------------------------
// Gets us at the SDK game rules
//-----------------------------------------------------------------------------
inline CSDKGameRules* OFGameRules()
{
	return static_cast<CSDKGameRules*>(g_pGameRules);
}

#endif // rh_GAMERULES_H