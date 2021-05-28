//====== Grimm Peacock =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "of_campaign_system.h"
#include "sdk_gamerules_sp.h"
#include "filesystem.h"
#include "cdll_int.h"
#include "steam/steam_gameserver.h"

#include "sdk_player_shared.h"
#include "viewport_panel_names.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#include "baseviewport.h"
#else
#include "dt_utlvector_send.h"
#endif

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

bool Uint32Compare( uint32 const &lhs, uint32 const &rhs )
{
	return ( lhs < rhs );
}

ConVar _active_campaign_name( "_active_campaign_name", "", FCVAR_REPLICATED );
ConVar _active_campaign_host( "_active_campaign_host", "0", FCVAR_REPLICATED );

COFCampaignSystem g_pCampaign;

COFCampaignSystem *Campaign()
{
	return &g_pCampaign;
};

BEGIN_NETWORK_TABLE_NOBASE( COFCampaignSystem, DT_CampaignSystem )

END_NETWORK_TABLE()

extern void CampaignDevMsg();

inline char *GetCampaignFileLocation( bool bHost = false )
{
#ifdef GAME_DLL
	return UTIL_VarArgs( "saves/%s.txt", _active_campaign_name.GetString() );
#else
	// Outside Client Pulls from saves_external so campaign don't clash
	// TODO: Dedicated servers are set to have the hostID 0, so they will all get pulled onto that
	// Maybe use the server IP for that instead? - Kay
	if( bHost )
		return UTIL_VarArgs( "saves/%s.txt", _active_campaign_name.GetString() );
	else
		return UTIL_VarArgs( "saves_external/%s/%s.txt", _active_campaign_host.GetString(), _active_campaign_name.GetString() );
#endif
}

#ifdef CLIENT_DLL
inline bool LocalPlayerIsHost()
{
	return steamapicontext->SteamUser()->GetSteamID().GetAccountID() == Q_atoui64(_active_campaign_host.GetString());
}
#endif
//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
COFCampaignSystem::COFCampaignSystem() : CAutoGameSystemPerFrame("COFCampaignSystem")
{
//	m_pActiveSession = NULL;
	m_hSession.m_szName[0] = '\0';
	m_hSession.m_iSeed = 0;
	m_hSession.m_iXP = 0;
	m_hSession.m_iStage = 0;

	m_hSession.m_iHost = 0;
}

bool COFCampaignSystem::Init( void )
{
#ifdef CLIENT_DLL
	ListenForGameEvent( "session_data" );
	ListenForGameEvent( "session_player_data" );
	ListenForGameEvent( "session_player_skill_data" );
	ListenForGameEvent( "session_map_data" );
	ListenForGameEvent( "session_map_info_data" );
#endif
	return BaseClass::Init();
}

void COFCampaignSystem::LevelInitPostEntity( void )
{
	if( !m_pActiveSession )
	{
		KeyValues *pData = new KeyValues( "CampaignSession" );
#ifdef CLIENT_DLL
		if( pData->LoadFromFile(filesystem, GetCampaignFileLocation( LocalPlayerIsHost() ), "MOD") )
#else
		if( pData->LoadFromFile(filesystem, GetCampaignFileLocation(), "MOD") )
#endif
		
			LoadSession( pData );

		pData->deleteThis();

		m_pActiveSession = &m_hSession;
	}
#ifdef GAME_DLL
	SaveSession();
	if( OFGameRules()->IsLobby() )
		OFGameRules()->SetupNextMaps();
#else
	// Host doesn't save session clientside
	if( !LocalPlayerIsHost() )
		SaveSession();
#endif
}

void COFCampaignSystem::LevelShutdownPreEntity( void )
{
#ifdef GAME_DLL
	TransmitSessionToClients();
#endif
	ConVarRef nextlevel("nextlevel");
	char szNextLevel[128];
	Q_strncpy( szNextLevel, nextlevel.GetString(), sizeof(szNextLevel) );
	// If we played a map and exited the game, save
	if( m_pActiveSession && szNextLevel[0] != '\0'
#ifdef CLIENT_DLL
		&& !LocalPlayerIsHost()
#endif
		)
		SaveSession();

	ClearSession();
}

void COFCampaignSystem::ClearSession( void )
{
	m_hSession.m_szName[0] = '\0';
	m_hSession.m_iSeed = 0;
	m_hSession.m_iXP = 0;
	m_hSession.m_iStage = 0;

	m_hSession.m_iHost = 0;

	m_hSession.m_hPlayerData.Purge();
	m_hSession.m_hMapHistory.Purge();

	m_pActiveSession = NULL;
}

bool COFCampaignSystem::CreateSession( CCommand args, uint64 iSteamID )
{
	Q_strncpy( m_hSession.m_szName, args[0], 128 );
	m_hSession.m_iHost = iSteamID;
	m_hSession.m_iSeed = random->RandomInt( 0, INT_MAX );

	m_pActiveSession = &m_hSession;
	SaveSession(true);

	return true;
}

bool COFCampaignSystem::LoadSession( KeyValues *kvSessionData )
{
	ClearSession();
	Q_strncpy( m_hSession.m_szName, kvSessionData->GetString("Name", "error"), 128 );
	m_hSession.m_iHost = Q_atoui64( kvSessionData->GetString("Host") );
	m_hSession.m_iSeed = kvSessionData->GetInt("Seed");

	KeyValues *kvCampaign = kvSessionData->FindKey("Campaign");
	if( kvCampaign )
	{
		m_hSession.m_iStage = kvCampaign->GetInt("Stage");
		m_hSession.m_iXP = kvCampaign->GetInt("XP");
	}

	KeyValues *kvPlayers = kvSessionData->FindKey( "Players" );
	if( kvPlayers )
	{
		FOR_EACH_SUBKEY( kvPlayers, kvPlayer )
		{
			session_player_data_t hPlayer;
			hPlayer.m_iSteamID = Q_atoi64(kvPlayer->GetName());
			Q_strncpy( hPlayer.m_szClass, kvPlayer->GetString( "Class", "" ), sizeof(hPlayer.m_szClass) );

			KeyValues *kvSkillSet = kvPlayer->FindKey( "SkillSet" );
			if( kvSkillSet )
			{
				FOR_EACH_VALUE(kvSkillSet, kvValue)
				{
					hPlayer.m_hSkills.Insert(kvValue->GetName(), kvValue->GetString());
				}
			}

			m_hSession.m_hPlayerData.InsertOrReplace( hPlayer.m_iSteamID, hPlayer );
		}
	}

	KeyValues *kvMapHistory = kvSessionData->FindKey( "MapHistory" );
	if( kvMapHistory )
	{
		FOR_EACH_SUBKEY( kvMapHistory, kvMap )
		{
			session_map_history_t hMap;
			hMap.m_iOrder = (uint32)Q_atoui64(kvMap->GetName());
			FOR_EACH_VALUE( kvMap, kvValue )
			{
				if( FStrEq( kvValue->GetName(), "name" ) )
					Q_strncpy( hMap.m_szName, kvValue->GetString(), sizeof(hMap.m_szName) );
				else
				{
					hMap.m_hMapData.Insert(kvValue->GetName(), kvValue->GetString());
				}
			}
			m_hSession.m_hMapHistory.Insert( hMap.m_iOrder, hMap );
		}
	}

	m_pActiveSession = &m_hSession;

	return true;
}

bool COFCampaignSystem::SaveSession( bool bHost )
{
	KeyValues *kvSession = new KeyValues("CampaignSession");
	kvSession->SetString("Name", m_hSession.m_szName);
	kvSession->SetString("Host", UTIL_VarArgs( "%u", m_hSession.m_iHost ) );
	kvSession->SetInt("Seed", m_hSession.m_iSeed);

	KeyValues *kvCampaign = new KeyValues( "Campaign" );
	kvCampaign->SetInt( "Stage", m_hSession.m_hMapHistory.Count() );
	kvCampaign->SetInt( "XP", m_hSession.m_iXP );
	kvSession->AddSubKey( kvCampaign );

	KeyValues *kvPlayers = new KeyValues( "Players" );
	for( unsigned int i = 0;  i < m_hSession.m_hPlayerData.Count(); i++ )
	{
		KeyValues *kvPlayer = new KeyValues(UTIL_VarArgs("%u", m_hSession.m_hPlayerData[i].m_iSteamID));
		kvPlayer->SetString( "Class", m_hSession.m_hPlayerData[i].m_szClass );
		KeyValues *kvSkillSet = new KeyValues( "SkillSet" );
		for( unsigned int y = 0;  y < m_hSession.m_hPlayerData[i].m_hSkills.Count(); y++ )
		{
			kvSkillSet->SetString( m_hSession.m_hPlayerData[i].m_hSkills.GetElementName(y), m_hSession.m_hPlayerData[i].m_hSkills[y].Get());
		}
		kvPlayer->AddSubKey(kvSkillSet);
		kvPlayers->AddSubKey(kvPlayer);
	}
	kvSession->AddSubKey( kvPlayers );

	KeyValues *kvMapHistory = new KeyValues( "MapHistory" );
	for( unsigned int i = 0;  i < m_hSession.m_hMapHistory.Count(); i++ )
	{
		KeyValues *kvMap = new KeyValues( UTIL_VarArgs("%u", m_hSession.m_hMapHistory[i].m_iOrder) );
		kvMap->SetString( "Name", m_hSession.m_hMapHistory[i].m_szName );
		for( unsigned int y = 0;  y < m_hSession.m_hMapHistory[i].m_hMapData.Count(); y++ )
		{
			kvMap->SetString( m_hSession.m_hMapHistory[i].m_hMapData.GetElementName(y), m_hSession.m_hMapHistory[i].m_hMapData[y].Get() );
		}
		kvMapHistory->AddSubKey(kvMap);
	}
	kvSession->AddSubKey( kvMapHistory );

	// Always attempt this in case the directory doesn't exist
#ifdef GAME_DLL
	filesystem->CreateDirHierarchy( "saves/", "MOD" );
#else
	if( bHost )
		filesystem->CreateDirHierarchy("saves/", "MOD");
	else
		filesystem->CreateDirHierarchy( UTIL_VarArgs("saves_external/%s/", _active_campaign_host.GetString() ), "MOD" );
#endif
	bool bSuccess = kvSession->SaveToFile(filesystem, GetCampaignFileLocation(bHost), "MOD");

	kvSession->deleteThis();

	return bSuccess;
}

session_player_data_t *COFCampaignSystem::GetSessionPlayerData( uint32 iSteamID )
{
	int iIndex = m_hSession.m_hPlayerData.Find(iSteamID);
	if( iIndex == m_hSession.m_hPlayerData.InvalidIndex() )
		return NULL;

	return &m_hSession.m_hPlayerData[iIndex];
}

session_map_history_t *COFCampaignSystem::GetSessionMapData( uint32 iOrder )
{
	if( m_hSession.m_hMapHistory.Find(iOrder) == m_hSession.m_hMapHistory.InvalidIndex() )
		return NULL;

	return &m_hSession.m_hMapHistory.Element(iOrder);
}

#ifdef GAME_DLL
void COFCampaignSystem::RegisterPlayer( edict_t *pPlayer )
{
	int iIndex = pPlayer->GetIServerEntity()->GetBaseEntity()->entindex();

	if( !iIndex || !steamapicontext->SteamUtils() )
		return;

	player_info_t pi;
	engine->GetPlayerInfo(iIndex, &pi);

	if ( !pi.friendsID )
		return;

	CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

	RegisterPlayer( steamIDForPlayer.GetAccountID() );
}

void COFCampaignSystem::UnregisterPlayer( edict_t *pPlayer )
{
	int iIndex = pPlayer->GetIServerEntity()->GetBaseEntity()->entindex();

	if( !iIndex || !steamapicontext->SteamUtils() )
		return;

	player_info_t pi;
	if( !engine->GetPlayerInfo(iIndex, &pi) )
		return;

	if ( !pi.friendsID )
		return;

	CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
	UnregisterPlayer( steamIDForPlayer.GetAccountID() );
}

void COFCampaignSystem::LogCurrentMap( void )
{
	session_map_history_t hMap;
	hMap.m_iOrder = m_hSession.m_hMapHistory.Count();
	Q_strncpy( hMap.m_szName, STRING(gpGlobals->mapname), sizeof(hMap.m_szName) );
	m_hSession.m_hMapHistory.Insert( m_hSession.m_hMapHistory.Count(), hMap );
}
#endif

void COFCampaignSystem::RegisterPlayer( uint64 iSteamID )
{
	session_player_data_t hNewPlayer;
	hNewPlayer.m_iSteamID = iSteamID;

	// Don't register same player twice
	if( m_hSession.m_hPlayerData.Find(iSteamID) != m_hSession.m_hPlayerData.InvalidIndex() )
		return;

	m_hSession.m_hPlayerData.InsertOrReplace(hNewPlayer.m_iSteamID, hNewPlayer);
}

void COFCampaignSystem::UnregisterPlayer( uint64 iSteamID )
{
	m_hSession.m_hPlayerData.Remove( m_hSession.m_hPlayerData.Find(iSteamID) );
}

#ifdef GAME_DLL
void COFCampaignSystem::TransmitSessionToClients( void )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "session_data", true );
	if( event )
	{
		event->SetString( "name", m_hSession.m_szName );
		event->SetInt( "seed", m_hSession.m_iSeed );
		event->SetInt( "xp", m_hSession.m_iXP );
		event->SetInt( "stage", m_hSession.m_iStage );

		event->SetUint64( "host", m_hSession.m_iHost );

		event->SetBool( "clear", true );

		gameeventmanager->FireEvent( event );
	}

	for( unsigned int i = 0;  i < m_hSession.m_hPlayerData.Count(); i++  )
	{
		m_hSession.m_hPlayerData[i].NetworkPlayer(OF_SESSION_DEPTH_ALL);
	}

	for( unsigned int i = 0;  i < m_hSession.m_hMapHistory.Count(); i++  )
	{
		m_hSession.m_hMapHistory[i].NetworkMap(OF_SESSION_DEPTH_ALL);
	}
}

void session_player_data_t::NetworkPlayer( int iDepth )
{
	iDepth--;
	IGameEvent *event = gameeventmanager->CreateEvent( "session_player_data", true );
	if( event )
	{
		event->SetString( "class", m_szClass );
		event->SetUint64( "steamid", m_iSteamID );

		gameeventmanager->FireEvent( event );
		if( iDepth )
		{
			for( unsigned int i = 0; i < m_hSkills.Count(); i++ )
			{
				IGameEvent *eventSkill = gameeventmanager->CreateEvent( "session_player_skill_data", true );
				if( eventSkill )
				{
					eventSkill->SetUint64( "steamid", m_iSteamID );
					eventSkill->SetString( "name", m_hSkills.GetElementName(i) );
					eventSkill->SetString( "value", m_hSkills[i].Get() );

					gameeventmanager->FireEvent( eventSkill );
				}
			}
		}
	}
}

void session_map_history_t::NetworkMap( int iDepth )
{
	iDepth--;
	IGameEvent *event = gameeventmanager->CreateEvent( "session_map_data", true );
	if( event )
	{
		event->SetString( "name", m_szName );
		event->SetInt( "order", m_iOrder );

		gameeventmanager->FireEvent( event );
		if( iDepth )
		{
			for( unsigned int i = 0; i < m_hMapData.Count(); i++ )
			{
				IGameEvent *eventInfo = gameeventmanager->CreateEvent( "session_map_info_data", true );
				if( eventInfo )
				{
					eventInfo->SetInt( "order", m_iOrder );
					eventInfo->SetString( "name", m_hMapData.GetElementName(i) );
					eventInfo->SetString( "value", m_hMapData[i].Get() );

					gameeventmanager->FireEvent( eventInfo );
				}
			}
		}
	}
}
#endif

void COFCampaignSystem::FireGameEvent( IGameEvent *event )
{
	char szName[32];
	Q_strncpy( szName, event->GetName(),sizeof(szName) );
	if( FStrEq( szName, "session_data" ) )
	{
		Q_strncpy( m_hSession.m_szName, event->GetString( "name", "error" ), sizeof(m_hSession.m_szName) );
		m_hSession.m_iSeed = event->GetInt("seed");
		m_hSession.m_iXP = event->GetInt("xp");
		m_hSession.m_iStage = event->GetInt("stage");

		m_hSession.m_iHost = event->GetUint64("host");

		if( event->GetBool("clear") )
		{
			m_hSession.m_hMapHistory.Purge();
			m_hSession.m_hPlayerData.Purge();
		}
	}
	else if( FStrEq( szName, "session_player_data" ) )
	{
		session_player_data_t hPlayer;
		hPlayer.m_iSteamID = event->GetUint64( "steamid" );
		Q_strncpy( hPlayer.m_szClass, event->GetString("class"), sizeof(hPlayer.m_szClass) );
		
		m_hSession.m_hPlayerData.InsertOrReplace( hPlayer.m_iSteamID, hPlayer );
	}
	else if( FStrEq( szName, "session_player_skill_data" ) )
	{
		uint32 iSteamID = event->GetUint64("steamid");
		if( m_hSession.m_hPlayerData.Find(iSteamID) == m_hSession.m_hPlayerData.InvalidIndex() )
			return;

		int iElement = m_hSession.m_hPlayerData.Element(iSteamID).m_hSkills.Find(event->GetString("name", ""));
		if( iElement == m_hSession.m_hPlayerData.Element(iSteamID).m_hSkills.InvalidIndex() )
			m_hSession.m_hPlayerData.Element(iSteamID).m_hSkills.Insert(event->GetString("name", ""), event->GetString("value", ""));
		else
			m_hSession.m_hPlayerData.Element(iSteamID).m_hSkills[iElement] = event->GetString("value", "");
	}
	else if( FStrEq( szName, "session_map_data" ) )
	{
		session_map_history_t hMap;
		hMap.m_iOrder = event->GetInt( "order" );
		Q_strncpy(hMap.m_szName, event->GetString("name"), sizeof(hMap.m_szName));
		
		m_hSession.m_hMapHistory.InsertOrReplace( hMap.m_iOrder, hMap );
	}
	else if( FStrEq( szName, "session_map_info_data" ) )
	{
		int iOrder = event->GetUint64("order");
		if( m_hSession.m_hMapHistory.Find(iOrder) == m_hSession.m_hMapHistory.InvalidIndex() )
			return;

		int iElement = m_hSession.m_hMapHistory.Element(iOrder).m_hMapData.Find(event->GetString("name", ""));
		if( iElement == m_hSession.m_hMapHistory.Element(iOrder).m_hMapData.InvalidIndex() )
			m_hSession.m_hMapHistory.Element(iOrder).m_hMapData.Insert(event->GetString("name", ""), event->GetString("value", ""));
		else
			m_hSession.m_hMapHistory.Element(iOrder).m_hMapData[iElement] = event->GetString("value", "");
	}
}

// This function is here for our CNetworkVars.
void COFCampaignSystem::NetworkStateChanged()
{
	// Forward the call to the entity that will send the data.
	COFGameRulesProxy::NotifyNetworkStateChanged();
}

void COFCampaignSystem::NetworkStateChanged( void *pVar )
{
	// Forward the call to the entity that will send the data.
	COFGameRulesProxy::NotifyNetworkStateChanged();
}

#ifdef GAME_DLL
CON_COMMAND( start_campaign, "" )
{
	char szCampaignName[128];
	Q_strncpy( szCampaignName, args[1], sizeof(szCampaignName) );

	uint32 iHost = 0;

	if( !engine->IsDedicatedServer() && steamapicontext )
		iHost = steamapicontext->SteamUser()->GetSteamID().GetAccountID();

	_active_campaign_name.SetValue( args[1] );
	_active_campaign_host.SetValue( UTIL_VarArgs( "%u", iHost ) );

	const char *szArgs[] = { szCampaignName };
	CCommand campaign_args( 1, szArgs );
	Campaign()->CreateSession( campaign_args, iHost );

	engine->ServerCommand( "exec chapter1.cfg" );
}

CON_COMMAND( continue_campaign, "" )
{
	char szCampaignName[128];
	Q_strncpy( szCampaignName, args[1], sizeof(szCampaignName) );
	const char *szArgs[] = { szCampaignName };
	CCommand campaign_args( 1, szArgs );

	uint32 iHost = 0;

	if( !engine->IsDedicatedServer() && steamapicontext )
		iHost = steamapicontext->SteamUser()->GetSteamID().GetAccountID();

	_active_campaign_name.SetValue( args[1] );
	_active_campaign_host.SetValue( UTIL_VarArgs( "%u", iHost ) );

	KeyValues *pData = new KeyValues( "CampaignSession" );

	if( !pData->LoadFromFile( filesystem, UTIL_VarArgs("saves/%s.txt", _active_campaign_name.GetString() ), "MOD" ) )
	{
		pData->deleteThis();
		return;
	}
	
	Campaign()->LoadSession(pData);

	pData->deleteThis();
	engine->ServerCommand( "exec chapter1.cfg" );
}
#endif

#ifdef GAME_DLL
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( campaign_test_server, "" )
#else
CON_COMMAND( campaign_test_client, "" )
#endif
{
	CampaignDevMsg();
}

void CampaignDevMsg()
{
	Msg( "saves/%s.txt\n", Campaign()->m_hSession.m_szName );
	
	Msg( "\tName %s\n", Campaign()->m_hSession.m_szName );
	Msg( "\tHost %u\n", Campaign()->m_hSession.m_iHost );
	Msg( "\tSeed %d\n", Campaign()->m_hSession.m_iSeed ); 

	Msg( "\tCampaign\n" );
	Msg( "\t\tStage %d\n", Campaign()->m_hSession.m_hMapHistory.Count() );
	Msg( "\t\tXP %d\n", Campaign()->m_hSession.m_iXP );

	Msg( "\tPlayers\n" );
	for( unsigned int i = 0;  i < Campaign()->m_hSession.m_hPlayerData.Count(); i++ )
	{
		Msg( "\t\t%d\n", Campaign()->m_hSession.m_hPlayerData[i].m_iSteamID);
		Msg( "\t\t\tClass %s\n", Campaign()->m_hSession.m_hPlayerData[i].m_szClass );
		Msg( "\t\t\tSkillSet\n" );
		for( unsigned int y = 0;  y < Campaign()->m_hSession.m_hPlayerData[i].m_hSkills.Count(); y++ )
		{
			Msg( "\t\t\t\t%s %s\n", Campaign()->m_hSession.m_hPlayerData[i].m_hSkills.GetElementName(y), Campaign()->m_hSession.m_hPlayerData[i].m_hSkills[y].Get());
		}
	}

	Msg( "\tMapHistory\n" );
	for( unsigned int i = 0;  i < Campaign()->m_hSession.m_hMapHistory.Count(); i++ )
	{
		Msg( "\t\t%d\n", Campaign()->m_hSession.m_hMapHistory[i].m_iOrder );
		Msg( "\t\t\tName %s\n", Campaign()->m_hSession.m_hMapHistory[i].m_szName );
		for( unsigned int y = 0;  y < Campaign()->m_hSession.m_hMapHistory[i].m_hMapData.Count(); y++ )
		{
			Msg( "\t\t\t%s %s\n", Campaign()->m_hSession.m_hMapHistory[i].m_hMapData.GetElementName(y), Campaign()->m_hSession.m_hMapHistory[i].m_hMapData[y].Get() );
		}
	}
}

#ifdef GAME_DLL
CON_COMMAND( campaign_end, "" )
{
	OFGameRules()->RoundWin();
}

CON_COMMAND( campaign_transmit, "" )
{
	Campaign()->TransmitSessionToClients();
}

CON_COMMAND( campaign_save_server, "" )
{
	Campaign()->SaveSession();
}
#else
CON_COMMAND( campaign_save_client, "" )
{
	Campaign()->SaveSession();
}
#endif