//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for all animating characters and objects.
//
//=============================================================================//

#include "cbase.h"
#include "of_baseschemaitem.h"
#include "of_shared_schemas.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#else
#include "dt_utlvector_send.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CBaseSchemaEntity )
	DEFINE_FIELD( m_iszSchemaName, FIELD_STRING ),
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( BaseSchemaEntity, DT_BaseSchemaEntity )

BEGIN_NETWORK_TABLE( CBaseSchemaEntity, DT_BaseSchemaEntity )
#ifdef GAME_DLL
	SendPropString( SENDINFO( m_iszSchemaName ) ),
	SendPropUtlVector( 
		SENDINFO_UTLVECTOR( m_Attributes ),
		32, // max elements
		SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( DT_Attribute ) ) )
#else
	RecvPropString( RECVINFO( m_iszSchemaName )),
	RecvPropUtlVector( 
		RECVINFO_UTLVECTOR( m_Attributes ), 
		32,
		RecvPropDataTable( NULL, 0, 0, &REFERENCE_RECV_TABLE( DT_Attribute ) ) )
#endif
END_NETWORK_TABLE()


CBaseSchemaEntity::CBaseSchemaEntity()
{
	m_iszSchemaName.GetForModify()[0] = '\0';
	// Maybe replace with Init()? - Kay
}

CBaseSchemaEntity::~CBaseSchemaEntity()
{
}

void CBaseSchemaEntity::SetupSchemaItem( const char *szName )
{
	SetSchemaName(szName);
	//SetKillIcon(szName);
}

void CBaseSchemaEntity::AddAttribute( CTFAttribute hAttribute )
{
	m_Attributes.AddToTail(hAttribute);
}

void CBaseSchemaEntity::RemoveAttribute(int iID)
{
	FOR_EACH_VEC(m_Attributes, i)
	{
		if( m_Attributes[i].GetID() == iID )
		{
			m_Attributes.Remove(i);
			return;
		}
	}
}

void CBaseSchemaEntity::RemoveAttribute( char *szName )
{
	int iID = ItemSchema()->GetAttributeID( szName );

	// Return if its an invalid Index
	if( iID == -1 )
		return;

	FOR_EACH_VEC(m_Attributes, i)
	{
		if( m_Attributes[i].GetID() == iID )
		{
			m_Attributes.Remove(i);
			return;
		}
	}
}

void CBaseSchemaEntity::RemoveAttribute( const char *szName )
{
	char szFullName[128];

	Q_strncpy(szFullName, szName, 128);
	RemoveAttribute(szFullName);
}

void CBaseSchemaEntity::PurgeAttributes()
{
	m_Attributes.Purge();
}

// Get a pointer to the Attribute
CTFAttribute *CBaseSchemaEntity::GetAttribute( char *szName ) const
{
	int iFindID = ItemSchema()->GetAttributeID(szName);

	// Return nothing if its an invalid Index
	if( iFindID == -1 )
		return NULL;

	FOR_EACH_VEC(m_Attributes, i)
	{
		CTFAttribute *pAttribute = const_cast<CTFAttribute *>(&m_Attributes[i]);
		if( pAttribute->GetID() == iFindID )
			return pAttribute;
	}

	// None Found, return Null
	return NULL;
}

bool CBaseSchemaEntity::GetAttributeValue_Int( char *szName, int &iValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		iValue = pAttribute->GetInt();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_Bool( char *szName, bool &bValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		bValue = pAttribute->GetBool();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_Float( char *szName, float &flValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		flValue = pAttribute->GetFloat();

	return bSuccess;
}

bool CBaseSchemaEntity::GetAttributeValue_String( char *szName, string_t &szValue ) const
{
	CTFAttribute *pAttribute = GetAttribute(szName);
	bool bSuccess = !!pAttribute;
	
	if( bSuccess )
		szValue = pAttribute->GetString();

	return bSuccess;
}