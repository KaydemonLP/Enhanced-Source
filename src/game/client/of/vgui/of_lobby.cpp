//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CHudHealth class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISystem.h>
#include <game/client/iviewport.h>

#include "of_shared_schemas.h"
#include "sdk_player_shared.h"
#include "of_campaign_system.h"
#include "sdk_gamerules_sp.h"

using namespace vgui;

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/RadioButton.h>


#include "of_lobby.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COFLobbyPlayerStatus::COFLobbyPlayerStatus( Panel *parent, const char *name ) : BaseClass( parent, name ) 
{
	m_pAvatar = new CAvatarImagePanel( this, "avatar" );
	m_pClassImage = new ImagePanel( this, "class" );

	m_pNameLabel = new Label( this, "name", "" );

	m_pPlayer = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the color of the top and bottom bars
//-----------------------------------------------------------------------------
void COFLobbyPlayerStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/lobby/PlayerStatus.res");

	if( m_pPlayer )
		SetPlayer( (C_SDKPlayer*)m_pPlayer.Get() );
}


void COFLobbyPlayerStatus::SetPlayer( C_SDKPlayer *pPlayer )
{
	m_pPlayer = pPlayer;
	m_pAvatar->SetPlayer( pPlayer );
	m_pAvatar->ForceFriend( k_ForceFriend_OFF );
	m_pClassImage->SetImage( GetClass(pPlayer).szClassImage );

	m_pNameLabel->SetText( pPlayer->GetPlayerName() );
}

COFLobbyClassChoice::COFLobbyClassChoice( Panel *parent, const char *name ) : BaseClass( parent, name )
{
	// Skip the Undefined class
	for( int i = 1; i < ClassManager()->m_iClassCount; i++ )
	{
		RadioButton *pClassButton = new RadioButton( this, ClassManager()->m_hClassInfo[i].szClassName, ClassManager()->m_hClassInfo[i].szLocalizedName );
		m_pClasses.AddToTail( pClassButton );
	}
}

void COFLobbyClassChoice::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if( C_SDKPlayer::GetLocalSDKPlayer() && C_SDKPlayer::GetLocalSDKPlayer()->GetClassNumber() != 0 )
		m_pClasses[C_SDKPlayer::GetLocalSDKPlayer()->GetClassNumber()-1]->SetSelected(true);
}

void COFLobbyClassChoice::OnRadioButtonChecked( Panel *panel )
{
	RadioButton *pButton = (RadioButton*) panel;
	
	if( FStrEq( pButton->GetCommand()->GetName(), "Command" ) )
		OnCommand( pButton->GetCommand()->GetString( "command", "" ) );
}

void COFLobbyClassChoice::OnCommand( const char *command )
{
	if( !Q_strncmp(command, "joinclass", 9) )
	{
		CCommand args;
		args.Tokenize(command);
		if (args.ArgC() < 2)
			return;

		engine->ClientCmd(VarArgs("%s %s", args[0], args[1]));
	}
}

COFLobbyMapChoice::COFLobbyMapChoice( Panel *parent, const char *name ) : BaseClass( parent, name )
{
	for( int i = 0; i < 3; i++ )
	{
		m_pMaps[i] = new RadioButton( this, VarArgs("Map%d",i+1), "" );
	}

	m_pOK = new Button( this, "Ok", "#PropertyDialog_OK" );

	m_iChosenMap = -1;
	m_iMapCount = 0;
}

void COFLobbyMapChoice::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void COFLobbyMapChoice::OnRadioButtonChecked( Panel *panel )
{
	RadioButton *pButton = (RadioButton*) panel;
	
	if( FStrEq( pButton->GetCommand()->GetName(), "Command" ) )
		OnCommand( pButton->GetCommand()->GetString( "command", "" ) );
}

void COFLobbyMapChoice::OnCommand( const char *command )
{
	if( Campaign()->m_pActiveSession->m_iHost != C_SDKPlayer::GetLocalSDKPlayer()->GetSteamID() )
	{
		return;
	}
	// This Could have been done with a loop but i don't have time - Kay
	if( FStrEq( command, "map1" ) )
	{
		m_iChosenMap = 0;
	}
	else if( FStrEq( command, "map2" ) )
	{
		m_iChosenMap = 1;
	}
	else if( FStrEq( command, "map3" ) )
	{
		m_iChosenMap = 2;
	}
	else if( FStrEq( command, "okay" ) )
	{
		engine->ExecuteClientCmd( VarArgs("request_nextmap %d", m_iChosenMap) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COFLobby::COFLobby( IViewPort *pViewPort ) : BaseClass( NULL, PANEL_LOBBY )
{
	// initialize dialog
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetMoveable( false );
	SetSizeable( false );
	SetProportional( true );

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pPlayerStatus = new COFLobbyPlayerStatus( this, "test" );
	m_pClassChoice = new COFLobbyClassChoice( this, "floppas_choice" );
	m_pMapChoice = new COFLobbyMapChoice( this, "scrungos_choice" );

	ListenForGameEvent( "round_win" );
}

COFLobby::~COFLobby()
{

}

void COFLobby::Reset( void )
{
	m_pClassChoice->InvalidateLayout( true, true );

	if( OFGameRules() && ( OFGameRules()->m_bRoundEnded || OFGameRules()->IsLobby() ) )
	{
		m_pMapChoice->SetVisible( true );

		// Fallback in case we don't porperly network the map vote
		// Client uses the same seed and data so it should be fine
		if( !m_pMapChoice->m_iMapCount )
		{
			// init random system with this seed
			int iSeed = Campaign()->m_pActiveSession->m_iSeed + Campaign()->m_pActiveSession->m_hMapHistory.Count();

			CUtlVector<map_difficulty_t> hMapVote;
			int iCurrentDiff = Campaign()->m_pActiveSession->m_hMapHistory.Count();

			while( iCurrentDiff > OFGameRules()->m_hMapList.Count() && iCurrentDiff >= 0 )
				iCurrentDiff--;

			if( iCurrentDiff == -1 )
				return;

			while( hMapVote.Count() < 3 && iCurrentDiff >= 0 )
			{
				CCopyableUtlVector<map_difficulty_t> hCurrentDiffPool = OFGameRules()->m_hMapList[iCurrentDiff];

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
			OFGameRules()->m_hNextMaps.CopyArray(hMapVote.Base(), hMapVote.Count());
			m_pMapChoice->m_iMapCount = OFGameRules()->m_hNextMaps.Count();

			for( int i = 0; i < m_pMapChoice->m_iMapCount; i++ )
				Q_strncpy(m_pMapChoice->m_szMapNames[i], OFGameRules()->m_hNextMaps[i].szMapname, sizeof(m_pMapChoice->m_szMapNames[i]));
		}
		for( int i = 0; i < m_pMapChoice->m_iMapCount; i++ )
		{
			m_pMapChoice->m_pMaps[i]->SetText(m_pMapChoice->m_szMapNames[i]);
			m_pMapChoice->m_pMaps[i]->SetVisible(true);
		}
	}
	else
		m_pMapChoice->SetVisible( false );

	m_pPlayerStatus->InvalidateLayout( true, true );
}

void COFLobby::Think( void )
{
	if( C_SDKPlayer::GetLocalSDKPlayer() )
		m_pPlayerStatus->SetPlayer( C_SDKPlayer::GetLocalSDKPlayer() );
}
//-----------------------------------------------------------------------------
// Purpose: Sets the color of the top and bottom bars
//-----------------------------------------------------------------------------
void COFLobby::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/lobby.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFLobby::ShowPanel( bool bShow )
{
	if( bShow )
		Reset();

	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		Activate();

		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

void COFLobby::OnRadioButtonChecked( Panel *panel )
{
	RadioButton *pButton = (RadioButton*) panel;
	
	if( FStrEq( pButton->GetCommand()->GetName(), "Command" ) )
		OnCommand( pButton->GetCommand()->GetString("command", "" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COFLobby::OnCommand( const char *command )
{
//	else if( !Q_strcmp( command, "okay" ) )
//	{
//		m_pViewPort->ShowPanel( this, false );
//	}
	if( !Q_strcmp( command, "ready" ) )
	{
		engine->ClientCmd( "request_ready" );
	}
	else if( !Q_strcmp( command, "unready" ) )
	{
		engine->ClientCmd( "request_unready" );
	}

	BaseClass::OnCommand( command );
}

void COFLobby::FireGameEvent( IGameEvent *event )
{
	const char *szName = event->GetName();
	if( FStrEq(szName, "round_win") )
	{
		int iMapCount = event->GetInt( "map_count", 0 );
		m_pMapChoice->m_iMapCount = iMapCount;
		for( int i = 0; i < iMapCount; i++ )
		{
			Q_strncpy( m_pMapChoice->m_szMapNames[i], event->GetString( UTIL_VarArgs("map_choice%d", i), "" ), sizeof(m_pMapChoice->m_szMapNames[i]) );
		}
		OFGameRules()->m_bRoundEnded = true;
		Reset();
	}
}