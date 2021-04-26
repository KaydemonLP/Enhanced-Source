//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for all animating characters and objects.
//
//=============================================================================//

#include "cbase.h"
#include "schemas/of_attributes.h"
#include "of_shared_schemas.h"

#ifdef CLIENT_DLL
#include "dt_utlvector_recv.h"
#else
#include "dt_utlvector_send.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_NETWORK_TABLE_NOBASE( CTFAttribute, DT_Attribute )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iID ) ),
	RecvPropString( RECVINFO( m_iszValue ) )
#else
	SendPropInt( SENDINFO( m_iID ) ),
	SendPropString( SENDINFO( m_iszValue ) )
#endif
END_NETWORK_TABLE()

// Maybe replace with Init()? - Kay
CTFAttribute::CTFAttribute()
{
	m_iID = -1;
	m_iszValue.GetForModify()[0] = '\0';
}

CTFAttribute::CTFAttribute( int iID, int iValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), UTIL_VarArgs("%d", iValue), OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, bool bValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), UTIL_VarArgs("%d", bValue), OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, float flValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), UTIL_VarArgs("%f", flValue), OF_MAX_ATTRIBUTE_LENGHT);
}
#ifdef GAME_DLL
CTFAttribute::CTFAttribute( int iID, string_t szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), STRING(szValue), OF_MAX_ATTRIBUTE_LENGHT);
}
#endif
CTFAttribute::CTFAttribute( int iID, const char *szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), szValue, OF_MAX_ATTRIBUTE_LENGHT);
}
CTFAttribute::CTFAttribute( int iID, char *szValue )
{
	m_iID = iID; // We'll consider -1 invalid
	Q_strncpy(m_iszValue.GetForModify(), szValue, OF_MAX_ATTRIBUTE_LENGHT);
}

int CTFAttribute::GetInt( void )
{
	return Q_atoi(m_iszValue.Get());
}

bool CTFAttribute::GetBool( void )
{
	return !!Q_atoi(m_iszValue.Get());
}

float CTFAttribute::GetFloat( void )
{
	return Q_atof(m_iszValue.Get());
}

string_t CTFAttribute::GetString( void )
{
	return MAKE_STRING(m_iszValue.Get());
}