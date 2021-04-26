//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_ATTRIBUTES_H
#define OF_ATTRIBUTES_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#define CTFAttribute C_TFAttribute
#endif

//=============
// Copied from CTFPlayerShared
// Assuming this is sometimes needed if we have another entity calling for DT_Attribute
// If you're sure we wont ever need it, feel free to remove
//=============

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_Attribute );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_Attribute );
#endif

#define OF_MAX_ATTRIBUTE_LENGHT 64

class CTFAttribute
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CTFAttribute );

	// Mandatory shared
	CTFAttribute();

	// ========================
	// If necessary, could probably move these to only be in the server
	CTFAttribute( int iID, int iValue );
	CTFAttribute( int iID, bool bValue );
	CTFAttribute( int iID, float flValue );
#ifdef GAME_DLL
	// string_t is serverside only
	CTFAttribute( int iID, string_t szValue );
#endif
	CTFAttribute( int iID, const char *szValue );
	CTFAttribute( int iID, char *szValue );
	// ========================

	// Mandatory Shared
	int GetID() { return m_iID; };

	int GetInt( void );
	bool GetBool( void );
	float GetFloat( void );
	string_t GetString( void );
	
private:
	CNetworkVar( int, m_iID );
	CNetworkString( m_iszValue, OF_MAX_ATTRIBUTE_LENGHT );
};

#endif // OF_ATTRIBUTES_H
