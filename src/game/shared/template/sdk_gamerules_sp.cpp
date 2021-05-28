#include "cbase.h"
#include "sdk_gamerules_sp.h"
#include "ammodef.h"
#include "of_shared_schemas.h"
#include "of_campaign_system.h"
#include "viewport_panel_names.h"
#include "filesystem.h"

#ifdef GAME_DLL
	#include "voice_gamemgr.h"
	#include "sdk_player.h"
	#include "team.h"
#else
	#include "c_sdk_player.h"
#endif


// =====Convars=============

ConVar mm_max_players( "mm_max_players", "4", FCVAR_REPLICATED | FCVAR_CHEAT, "Max players for matchmaking system" );

// ===Ammo/Damage===

//Pistol
ConVar	sk_plr_dmg_pistol			( "sk_plr_dmg_pistol","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_pistol			( "sk_npc_dmg_pistol","0", FCVAR_REPLICATED);
ConVar	sk_max_pistol				( "sk_max_pistol","0", FCVAR_REPLICATED);

//SMG
ConVar	sk_plr_dmg_smg				( "sk_plr_dmg_smg","0", FCVAR_REPLICATED );
ConVar	sk_npc_dmg_smg				( "sk_npc_dmg_smg","0", FCVAR_REPLICATED);
ConVar	sk_max_smg					( "sk_max_smg","0", FCVAR_REPLICATED);

//Shotgun
ConVar	sk_plr_dmg_shotgun			( "sk_plr_dmg_shotgun","0", FCVAR_REPLICATED);	
ConVar	sk_npc_dmg_shotgun			( "sk_npc_dmg_shotgun","0", FCVAR_REPLICATED);
ConVar	sk_max_shotgun				( "sk_max_shotgun","0", FCVAR_REPLICATED);
ConVar	sk_plr_num_shotgun_pellets	( "sk_plr_num_shotgun_pellets","7", FCVAR_REPLICATED);


//===============================================================================
// Purpoise: AutoGameSystems, Gamrules and other similar classes aren't entities
//			 as such, they don't get networked by default
//			 This entity serves as a medium to network vars of those
//===============================================================================
LINK_ENTITY_TO_CLASS( of_gamerules, COFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( OFGameRulesProxy, DT_OFGameRulesProxy )

#ifdef CLIENT_DLL
	void COFGameRulesProxy::OnPreDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnPreDataChanged( updateType );
		// Reroute data changed calls to the non-entity gamerules 
//		COFCampaignSystem *pCampaign = Campaign();
		CSDKGameRules *pRules = OFGameRules();
//		Assert( pCampaign );
		Assert( pRules );
//		pCampaign->OnPreDataChanged( updateType );
		pRules->OnPreDataChanged( updateType );
	}
	
	void COFGameRulesProxy::OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );
		// Reroute data changed calls to the non-entity gamerules 
//		COFCampaignSystem *pCampaign = Campaign();
		CSDKGameRules *pRules = OFGameRules();
//		Assert( pCampaign );
		Assert( pRules );
//		pCampaign->OnDataChanged( updateType );
		pRules->OnDataChanged( updateType );
	}

	void RecvProxy_OFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CSDKGameRules *pRules = OFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}
	void RecvProxy_Campaign( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		COFCampaignSystem *pCampaign = Campaign();
		Assert( pCampaign );
		*pOut = pCampaign;
	}	
	
	BEGIN_RECV_TABLE( COFGameRulesProxy, DT_OFGameRulesProxy )
		RecvPropDataTable( "campaign_data", 	0, 0, &REFERENCE_RECV_TABLE( DT_CampaignSystem ), RecvProxy_Campaign ),
		RecvPropDataTable( "of_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_OFGameRules ), RecvProxy_OFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_OFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CSDKGameRules *pRules = OFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	void* SendProxy_Campaign( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		COFCampaignSystem *pCampaign = Campaign();
		Assert( pCampaign );
		pRecipients->SetAllRecipients();
		return pCampaign;
	}

	BEGIN_SEND_TABLE( COFGameRulesProxy, DT_OFGameRulesProxy )
		SendPropDataTable( "campaign_data", 	0, &REFERENCE_SEND_TABLE( DT_CampaignSystem ), 	SendProxy_Campaign ),
		SendPropDataTable( "of_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_OFGameRules ), SendProxy_OFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( COFGameRulesProxy )
//Keyfields
	DEFINE_KEYFIELD( m_bIsLobby , FIELD_BOOLEAN, "is_lobby" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "WinRound", InputWinRound ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LoseRound", InputLoseRound ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void COFGameRulesProxy::Activate()
{
	OFGameRules()->m_bIsLobby = m_bIsLobby;
	OFGameRules()->Activate();

	BaseClass::Activate();
}

void COFGameRulesProxy::InputWinRound( inputdata_t &inputdata )
{
	OFGameRules()->RoundWin();
}

void COFGameRulesProxy::InputLoseRound( inputdata_t &inputdata )
{
	OFGameRules()->RoundLose();
}
#endif

REGISTER_GAMERULES_CLASS( CSDKGameRules );

void InitBodyQue() { }


BEGIN_NETWORK_TABLE_NOBASE( CSDKGameRules, DT_OFGameRules )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bIsLobby ) ),
RecvPropBool( RECVINFO( m_bRoundEnded ) ),
RecvPropBool( RECVINFO( m_bRoundStarted ) ),
#else
SendPropBool( SENDINFO( m_bIsLobby ) ),
SendPropBool( SENDINFO( m_bRoundEnded ) ),
SendPropBool( SENDINFO( m_bRoundStarted ) ),
#endif
END_NETWORK_TABLE()

CSDKGameRules::CSDKGameRules()
{
#ifdef GAME_DLL
	InitTeams();
	m_bRoundStarted = false;
	m_bRoundEnded = false;
#else
	Msg("%s\n", engine->GetLevelNameShort());
	if( FStrEq( engine->GetLevelNameShort(), "lobby" ) )
		m_bIsLobby = true;
#endif
	KeyValues *kvMapDiff = new KeyValues( "MapDifficulty" );
	kvMapDiff->LoadFromFile( filesystem, "scripts/map_data.txt", "MOD" );

	FOR_EACH_VALUE( kvMapDiff, kvValue )
	{
		if( kvValue->GetInt() >= m_hMapList.Count() )
		{
			int iNumToAdd = (kvValue->GetInt() - m_hMapList.Count()) + 1;
			for( int i = 0; i < iNumToAdd; i++ )
			{
				CCopyableUtlVector<map_difficulty_t> hNewVector;
				m_hMapList.AddToTail(hNewVector);
			}
		}

		map_difficulty_t hNewMap;
		Q_strncpy( hNewMap.szMapname, kvValue->GetName(), sizeof(hNewMap.szMapname) );
		m_hMapList[kvValue->GetInt()].AddToTail(hNewMap);
	}
}

bool CSDKGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// The smaller number is always first
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup0 = COLLISION_GROUP_PLAYER;
	}

	if( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup1 = COLLISION_GROUP_PLAYER;
	}

	//If collisionGroup0 is not a player then NPC_ACTOR behaves just like an NPC.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER )
	{
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	//players don't collide against NPC Actors.
	//I could've done this up where I check if collisionGroup0 is NOT a player but I decided to just
	//do what the other checks are doing in this function for consistency sake.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER )
		return false;
		
	// In cases where NPCs are playing a script which causes them to interpenetrate while riding on another entity,
	// such as a train or elevator, you need to disable collisions between the actors so the mover can move them.
	if ( collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED )
		return false;

	return true;
}
// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;
#ifdef OFFSHORE_DLL
		CUtlVector<int> hDamage; hDamage.AddToTail(DMG_BULLET);
		def.AddAmmoType("Pistol",	&hDamage,		TRACER_LINE_AND_WHIZ, "sk_plr_dmg_pistol",	"sk_npc_dmg_pistol", "sk_max_pistol", BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("SMG",		&hDamage,	TRACER_LINE_AND_WHIZ, "sk_plr_dmg_smg", "sk_npc_dmg_smg", "sk_max_smg", BULLET_IMPULSE(200, 1225), 0 );
		hDamage.AddToTail(DMG_BUCKSHOT);
		def.AddAmmoType("Shotgun",	&hDamage,	TRACER_LINE, "sk_plr_dmg_shotgun", "sk_npc_dmg_shotgun", "sk_max_shotgun", BULLET_IMPULSE(400, 1200), 0 );
#else
		def.AddAmmoType("Pistol",	DMG_BULLET,		TRACER_LINE_AND_WHIZ, "sk_plr_dmg_pistol",	"sk_npc_dmg_pistol", "sk_max_pistol", BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("SMG",		DMG_BULLET,	TRACER_LINE_AND_WHIZ, "sk_plr_dmg_smg", "sk_npc_dmg_smg", "sk_max_smg", BULLET_IMPULSE(200, 1225), 0 );
		def.AddAmmoType("Shotgun",	DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE, "sk_plr_dmg_shotgun", "sk_npc_dmg_shotgun", "sk_max_shotgun", BULLET_IMPULSE(400, 1200), 0 );
#endif
	}

	return &def;
}

//=========================================================
//=========================================================
bool CSDKGameRules::IsMultiplayer( void )
{
	return true;
}

void CSDKGameRules::PlayerThink( CBasePlayer *pPlayer )
{
}

#ifdef GAME_DLL

void CSDKGameRules::Activate( void )
{
	// Nothing yet
}

//=========================================================
//=========================================================
bool CSDKGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	bool bRet = BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );

	return bRet;
}

//=========================================================
//=========================================================
void CSDKGameRules::ClientDisconnected( edict_t *pClient )
{
	Campaign()->UnregisterPlayer( pClient );
	BaseClass::ClientDisconnected( pClient );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSDKGameRules::Precache(void)
{
	BaseClass::Precache();

	ClassManager()->PrecacheClasses();
}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CSDKGameRules::CreateStandardEntities()
{
	// Creates the player resource
	BaseClass::CreateStandardEntities();

	// Creates the proxy that sends all the data from various systems to clients
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "of_gamerules" );

	if ( !pEnt )
	{
		pEnt = CBaseEntity::Create( "of_gamerules", vec3_origin, vec3_angle );
		pEnt->SetName( AllocPooledString( "of_gamerules" ) );
	}
}

bool CSDKGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CSDKPlayer *pPlayer = ToSDKPlayer( pEdict );
	gameeventmanager->LoadEventsFromFile("resource/ModEvents.res");

	//const char *pcmd = args[0];
	if( pPlayer && pPlayer->ClientCommand( args ) )
	{
		return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}

float CSDKGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	return 0.0f;
}

void CSDKGameRules::Think( void )
{
	// Idk about this honestly
	// At this point i feel like doing Roundbased gamerules might be smarter
	if( !m_bRoundStarted && GetGlobalTeam( OF_TEAM_FUNNY )->GetNumPlayers() )
	{
		int iNumReady = 0;
		for( int i = 0; i < GetGlobalTeam( OF_TEAM_FUNNY )->GetNumPlayers(); i++ )
		{
			CSDKPlayer *pPlayer = static_cast<CSDKPlayer*>(GetGlobalTeam(OF_TEAM_FUNNY)->GetPlayer(i));
			iNumReady += pPlayer->IsReady() * pPlayer->GetClassNumber() != 0;
		}
	
		if( iNumReady == GetGlobalTeam( OF_TEAM_FUNNY )->GetNumPlayers() )
			StartRound();
	}
	BaseClass::Think();
}

void CSDKGameRules::StartRound( void )
{
	if( m_bRoundEnded || IsLobby() )
	{
		ConVarRef nextlevel("nextlevel");
		char szNextLevel[128];
		Q_strncpy(szNextLevel, nextlevel.GetString(), sizeof(szNextLevel));
		if( szNextLevel[0] != '\0' && Q_strncmp(szNextLevel, STRING(gpGlobals->mapname), sizeof(szNextLevel)))
		{
			GoToIntermission();
			//m_bRoundEnded = false;

			// Stop the ready think
			m_bRoundStarted = true;
		}
	}
	else
	{
		for( int i = 0; i < GetGlobalTeam( OF_TEAM_FUNNY )->GetNumPlayers(); i++ )
		{
			CSDKPlayer *pPlayer = static_cast<CSDKPlayer*>(GetGlobalTeam(OF_TEAM_FUNNY)->GetPlayer(i));
			pPlayer->ForceRespawn();
			pPlayer->ShowViewPortPanel( PANEL_LOBBY, false );
			pPlayer->UnReady();
		}
		m_bRoundStarted = true;
	}
}

void CSDKGameRules::RoundWin( void )
{
	Campaign()->LogCurrentMap();

	// Network the campaign first
	Campaign()->TransmitSessionToClients();

	// Then open the panels for all players
	for( int i = 0; i < GetGlobalTeam( OF_TEAM_FUNNY )->GetNumPlayers(); i++ )
	{
		CSDKPlayer *pPlayer = static_cast<CSDKPlayer*>(GetGlobalTeam(OF_TEAM_FUNNY)->GetPlayer(i));
		pPlayer->ShowViewPortPanel( PANEL_LOBBY, true );
	}

	SetupNextMaps();

	m_bRoundEnded = true;
	m_bRoundStarted = false;
}

void CSDKGameRules::RoundLose( void )
{
}

void CSDKGameRules::SetupNextMaps( void )
{
	// init random system with this seed
	int iSeed = Campaign()->m_pActiveSession->m_iSeed + Campaign()->m_pActiveSession->m_hMapHistory.Count();

	CUtlVector<map_difficulty_t> hMapVote;
	int iCurrentDiff = Campaign()->m_pActiveSession->m_hMapHistory.Count();

	while( iCurrentDiff > m_hMapList.Count() && iCurrentDiff >= 0 )
		iCurrentDiff--;

	if( iCurrentDiff == -1 )
		return;

	while( hMapVote.Count() < 3 && iCurrentDiff >= 0 )
	{
		CCopyableUtlVector<map_difficulty_t> hCurrentDiffPool = m_hMapList[iCurrentDiff];

		while( hCurrentDiffPool.Count() && hMapVote.Count() < 3 )
		{
			RandomSeed( iSeed );
			int iIndex = RandomInt(0, hCurrentDiffPool.Count() - 1);
			map_difficulty_t hMap = hCurrentDiffPool[iIndex];

			// REALLY inneficient but works for now
			// Ensure we don't play the same map twice
			for( uint32 iMapHistory = 0; iMapHistory < Campaign()->m_pActiveSession->m_hMapHistory.Count(); iMapHistory++ )
			{
				if( FStrEq( Campaign()->m_pActiveSession->m_hMapHistory[iMapHistory].m_szName, hMap.szMapname ) )
				{
					hCurrentDiffPool.Remove(iIndex);
					break;
				}
			}
			hMapVote.AddToTail(hMap);
			hCurrentDiffPool.Remove(iIndex);
			iSeed++;
		}
		iCurrentDiff--;
	}
	m_hNextMaps.CopyArray(hMapVote.Base(), hMapVote.Count());

	// Then start the map vote
	IGameEvent *event = gameeventmanager->CreateEvent( "round_win", true );
	if( event )
	{
		for( int i = 0; i < m_hNextMaps.Count(); i++ )
		{
			event->SetString( UTIL_VarArgs("map_choice%d",i), m_hNextMaps[i].szMapname );
		}
		event->SetInt( "map_count", m_hNextMaps.Count() );
		gameeventmanager->FireEvent( event );
	}
}
// This being required here is a bug. It should be in shared\BaseGrenade_shared.cpp
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool	CanPlayerHearPlayer( CBasePlayer* pListener, CBasePlayer* pTalker, bool &bProximity ) { return true; }
};

CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper* g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

void CSDKGameRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i=0;i<NUM_AI_CLASSES;i++)
	{
		for (j=0;j<NUM_AI_CLASSES;j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

		// ------------------------------------------------------------
		//	> CLASS_PLAYER
		// ------------------------------------------------------------
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);			
		CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_NU, 0);			
}

	const char* CSDKGameRules::AIClassText(int classType)
	{
		switch (classType)
		{
			case CLASS_NONE:			return "CLASS_NONE";
			case CLASS_PLAYER:			return "CLASS_PLAYER";
			default:					return "MISSING CLASS in ClassifyText()";
		}
	}
#endif // #ifdef GAME_DLL

#ifdef CLIENT_DLL
void CSDKGameRules::OnPreDataChanged( DataUpdateType_t updateType )
{
}

void CSDKGameRules::OnDataChanged( DataUpdateType_t updateType )
{
}
#endif