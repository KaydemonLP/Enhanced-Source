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

		if( pData->LoadFromFile( filesystem, UTIL_VarArgs("saves/%s.txt", _active_campaign_name.GetString() )), "MOD" )
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
	if( steamapicontext->SteamUser()->GetSteamID().GetAccountID() != Q_atoui64( _active_campaign_host.GetString() ) )
		SaveSession();
#endif
}

void COFCampaignSystem::LevelShutdownPreEntity( void )
{
	// If we played a map and exited the game, save
	if( m_hSession.m_hMapHistory.Count() 
		&& ( !OFGameRules() 
		|| ( OFGameRules() && !OFGameRules()->IsLobby() )))
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
	SaveSession();

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
			Q_strncpy( hPlayer.m_szClass, kvSessionData->GetString( "Class", "" ), sizeof(hPlayer.m_szClass) );

			KeyValues *kvSkillSet = kvPlayer->FindKey( "SkillSet" );
			if( kvSkillSet )
			{
				FOR_EACH_VALUE(kvSkillSet, kvValue)
				{
					hPlayer.m_hSkills.Insert(kvValue->GetName(), kvValue->GetString());
				}
			}

			m_hSession.m_hPlayerData.Insert( Q_atoi64(kvPlayers->GetName()), hPlayer );
		}
	}

	KeyValues *kvMapHistory = kvSessionData->FindKey( "MapHistory" );
	if( kvMapHistory )
	{
		FOR_EACH_SUBKEY( kvMapHistory, kvMap )
		{
			session_map_history_t hMap;
			FOR_EACH_VALUE( kvMap, kvValue )
			{
				if( FStrEq( kvValue->GetName(), "name" ) )
					Q_strncpy( hMap.m_szName, kvValue->GetString(), sizeof(hMap.m_szName) );
				else
				{
					hMap.m_hMapData.Insert(kvValue->GetName(), kvValue->GetString());
				}
			}
			m_hSession.m_hMapHistory.Insert( Q_atoui64( kvMap->GetName() ), hMap );
		}
	}

	m_pActiveSession = &m_hSession;

	return true;
}

bool COFCampaignSystem::SaveSession( void )
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
		KeyValues *kvPlayer = new KeyValues(UTIL_VarArgs("%d", m_hSession.m_hPlayerData[i].m_iSteamID));
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
		KeyValues *kvMap = new KeyValues( UTIL_VarArgs("%d", m_hSession.m_hMapHistory[i].m_iOrder) );
		kvMap->SetString( "Name", m_hSession.m_hMapHistory[i].m_szName );
		for( unsigned int y = 0;  y < m_hSession.m_hMapHistory[i].m_hMapData.Count(); y++ )
		{
			kvMap->SetString( m_hSession.m_hMapHistory[i].m_hMapData.GetElementName(y), m_hSession.m_hMapHistory[i].m_hMapData[y].Get() );
		}
		kvMapHistory->AddSubKey(kvMap);
	}
	kvSession->AddSubKey( kvMapHistory );

	// Always attempt this in case the directory doesn't exist
	filesystem->CreateDirHierarchy("saves/", "MOD");

	bool bSuccess = kvSession->SaveToFile(filesystem, UTIL_VarArgs("saves/%s.txt", m_hSession.m_szName), "MOD");

	kvSession->deleteThis();

	return bSuccess;
}

session_player_data_t *COFCampaignSystem::GetSessionPlayerData( uint32 iSteamID )
{
	if( m_hSession.m_hPlayerData.Find(iSteamID) == m_hSession.m_hPlayerData.InvalidIndex() )
		return NULL;

	return &m_hSession.m_hPlayerData.Element(iSteamID);
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CON_COMMAND( campaign_test, "" )
{
	DevMsg("Count: %u\n", Campaign()->m_hSession.m_hPlayerData.Count());
	for( unsigned int i = 0; i < Campaign()->m_hSession.m_hPlayerData.Count(); i++ )
	{
		DevMsg( "%u\n", Campaign()->m_hSession.m_hPlayerData[i].m_iSteamID );
	}
}

CON_COMMAND( campaign_end, "" )
{
	OFGameRules()->RoundWin();
}
#else
CON_COMMAND( client_campaign_test, "" )
{
	Campaign()->SaveSession();
}
#endif