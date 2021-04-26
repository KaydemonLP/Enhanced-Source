//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OF_BASESCHEMAITEM_H
#define OF_BASESCHEMAITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "schemas/of_attributes.h"

#ifdef CLIENT_DLL
	#define CBaseSchemaEntity C_BaseSchemaEntity
	
	// Since CBaseAnimating doesn't do this on it's own
	#define CBaseAnimating C_BaseAnimating

	#include "c_baseanimating.h"
#else
	#include "baseanimating.h"
#endif


class CBaseSchemaEntity : public CBaseAnimating
{
public:
	DECLARE_CLASS( CBaseSchemaEntity, CBaseAnimating );

	CBaseSchemaEntity();
	~CBaseSchemaEntity();

	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

public:

	const char *GetSchemaName()
	{
		return m_iszSchemaName.GetForModify()[0] != '\0' ? (char*)m_iszSchemaName.Get() : GetClassname();
	};
	
	void SetSchemaName( char *szSchemaName )
	{
		if( !FClassnameIs( this, szSchemaName ) )
			Q_strncpy( m_iszSchemaName.GetForModify(), szSchemaName, 128);
	};
	
	void SetSchemaName( const char *szSchemaName )
	{
		if( !FClassnameIs( this, szSchemaName ) )
			Q_strncpy( m_iszSchemaName.GetForModify(), szSchemaName, 128);
	};
	
	bool IsSchemaItem()
	{
		return m_iszSchemaName.GetForModify()[0] != '\0';
	};

	virtual void SetupSchemaItem( const char *szName );

	// ========================
	// If necessary, could probably move these to only be in the server
	void AddAttribute( CTFAttribute hAttribute );
	void RemoveAttribute( int iID );
	void RemoveAttribute( char *szName );
	void RemoveAttribute( const char *szName );
	void PurgeAttributes();
	// ========================

	// Mandatory Shared
	CTFAttribute *GetAttribute( char *szName ) const;

	CTFAttribute *GetAttribute( int i )  const{ return i < m_Attributes.Count() ? const_cast<CTFAttribute *>(&m_Attributes[i]) : NULL; }
	int GetAttributeCount( void ){ return m_Attributes.Count(); }

	bool GetAttributeValue_Int( char *szName, int &iValue ) const;
	bool GetAttributeValue_Bool( char *szName, bool &bValue ) const;
	bool GetAttributeValue_Float( char *szName, float &flValue ) const;

	// string_t is const char* on the Client, just a note
	bool GetAttributeValue_String( char *szName, string_t &szValue ) const;

public:

protected:
	CNetworkString(m_iszSchemaName, 128);
	
	CUtlVector< CTFAttribute > m_Attributes;

public:
#ifdef GAME_DLL
	friend class CSDKPlayer;
#else
	friend class C_SDKPlayer;
#endif
	friend class CSDKPlayerShared;
};

#endif // OF_BASESCHEMAITEM_H
