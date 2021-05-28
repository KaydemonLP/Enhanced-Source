#ifndef OF_LOBBY_PANEL_H
#define OF_LOBBY_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include "vgui_avatarimage.h"

#include "GameEventListener.h"

class COFLobbyPlayerStatus : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( COFLobbyPlayerStatus, vgui::EditablePanel );

	COFLobbyPlayerStatus( vgui::Panel *parent, const char *name );
	void ApplySchemeSettings( vgui::IScheme *pScheme );
	void SetPlayer( C_SDKPlayer *pPlayer );
private:
	CAvatarImagePanel	*m_pAvatar;
	vgui::ImagePanel	*m_pClassImage;
	vgui::Label			*m_pNameLabel;

	EHANDLE				m_pPlayer;
};

class COFLobbyClassChoice : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( COFLobbyClassChoice, vgui::EditablePanel );

	COFLobbyClassChoice( vgui::Panel *parent, const char *name );
	void ApplySchemeSettings( vgui::IScheme *pScheme );

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );
	virtual void OnCommand( const char *command );
public:
	CUtlVector<vgui::RadioButton*>	m_pClasses;

	int m_iChosenClass;
};

class COFLobbyMapChoice : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( COFLobbyMapChoice, vgui::EditablePanel );

	COFLobbyMapChoice( vgui::Panel *parent, const char *name );
	void ApplySchemeSettings( vgui::IScheme *pScheme );

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );
	virtual void OnCommand( const char *command );
public:
	vgui::RadioButton	*m_pMaps[3];
	char m_szMapNames[3][128];
	int m_iMapCount;

	vgui::Button		*m_pOK;

	int m_iChosenMap;
};
//-----------------------------------------------------------------------------
// Purpose: Spectator UI
//-----------------------------------------------------------------------------
class COFLobby : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( COFLobby, vgui::Frame );

public:
	COFLobby( IViewPort *pViewPort );
	virtual ~COFLobby();

	virtual const char *GetName( void ) { return PANEL_LOBBY; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent(parent); }

	virtual void Think( void );
	virtual void Update( void ){};
	virtual bool NeedsUpdate( void ){ return false; };
	virtual bool HasInputElements( void ){ return true; };
	virtual bool CanReplace( const char *panelName ) const { return true; } 
	virtual bool CanBeReopened( void ) const { return true; }

	virtual void ShowPanel( bool state ); // activate VGUI Frame
	virtual void OnCommand( const char *command );

	virtual bool WantsBackgroundBlurred( void ){ return false; };

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );

	virtual void FireGameEvent( IGameEvent *event );

protected:	
	// vgui overrides
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

protected:
	// vgui overrides
	IViewPort		*m_pViewPort;
	COFLobbyPlayerStatus *m_pPlayerStatus;
	// If Floppa Had a choice
	COFLobbyClassChoice	*m_pClassChoice;
	// If Scrungo Had a choice
	COFLobbyMapChoice	*m_pMapChoice;
};
#endif // OF_LOBBY_PANEL_H