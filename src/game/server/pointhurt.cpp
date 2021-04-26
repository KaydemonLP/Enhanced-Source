//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements hurting point entity
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "gamerules.h"
#include "basecombatcharacter.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int SF_PHURT_START_ON			= 1;

class CPointHurt : public CPointEntity
{
	DECLARE_CLASS( CPointHurt, CPointEntity );

public:
	void	Spawn( void );
	void	Precache( void );
	void	HurtThink( void );

	// Input handlers
	void InputTurnOn(inputdata_t &inputdata);
	void InputTurnOff(inputdata_t &inputdata);
	void InputToggle(inputdata_t &inputdata);
	void InputHurt(inputdata_t &inputdata);
	
	DECLARE_DATADESC();

	int			m_nDamage;
	int			m_bitsDamageType;
#ifdef OFFSHORE_DLL
	CUtlVector<int> m_hDamageType;
#endif
	float		m_flRadius;
	float		m_flDelay;
	string_t	m_strTarget;
	EHANDLE		m_pActivator;
};

BEGIN_DATADESC( CPointHurt )

	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "DamageRadius" ),
	DEFINE_KEYFIELD( m_nDamage, FIELD_INTEGER, "Damage" ),
	DEFINE_KEYFIELD( m_flDelay, FIELD_FLOAT, "DamageDelay" ),
	DEFINE_KEYFIELD( m_bitsDamageType, FIELD_INTEGER, "DamageType" ),
	DEFINE_KEYFIELD( m_strTarget, FIELD_STRING, "DamageTarget" ),
	
	// Function Pointers
	DEFINE_FUNCTION( HurtThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Hurt", InputHurt ),

	DEFINE_FIELD( m_pActivator, FIELD_EHANDLE ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_hurt, CPointHurt );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointHurt::Spawn(void)
{
	SetThink( NULL );
	SetUse( NULL );
		
	m_pActivator = NULL;

	if ( HasSpawnFlags( SF_PHURT_START_ON ) )
	{
		SetThink( &CPointHurt::HurtThink );
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
	
	if ( m_flRadius <= 0.0f )
	{
		m_flRadius = 128.0f;
	}

	if ( m_nDamage <= 0 )
	{
		m_nDamage = 2;
	}

	if ( m_flDelay <= 0 )
	{
		m_flDelay = 0.1f;
	}

#ifdef OFFSHORE_DLL
	// TODO: THIS IS A HORRIBLE HACK
	// Leave it as a legacy option
	//  but DEFINITELY add a proper way to support all damage types - Kay
	for( int i = 0; i < DMG_COUNT; i++ )
	{ 
		if( m_bitsDamageType & ( 1 << i ) )
			m_hDamageType.AddToTail(i);
	}
#endif

	Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointHurt::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointHurt::HurtThink( void )
{
	if ( m_strTarget != NULL_STRING )
	{
		CBaseEntity	*pEnt = NULL;

#ifdef OFFSHORE_DLL
		CTakeDamageInfo info( this, m_pActivator, m_nDamage, &m_hDamageType );
#else
		CTakeDamageInfo info( this, m_pActivator, m_nDamage, m_bitsDamageType );
#endif
		
		while ( ( pEnt = gEntList.FindEntityByName( pEnt, m_strTarget, NULL, m_pActivator ) ) != NULL )
		{
			GuessDamageForce( &info, (pEnt->GetAbsOrigin() - GetAbsOrigin()), pEnt->GetAbsOrigin() );
			pEnt->TakeDamage( info );
		}
	}
	else
	{
#ifdef OFFSHORE_DLL
		RadiusDamage( CTakeDamageInfo( this, this, m_nDamage, &m_hDamageType ), GetAbsOrigin(), m_flRadius, CLASS_NONE, NULL );
#else
		RadiusDamage( CTakeDamageInfo( this, this, m_nDamage, m_bitsDamageType ), GetAbsOrigin(), m_flRadius, CLASS_NONE, NULL );
#endif
	}

	SetNextThink( gpGlobals->curtime + m_flDelay );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for turning on the point hurt.
//-----------------------------------------------------------------------------
void CPointHurt::InputTurnOn( inputdata_t &data )
{
	SetThink( &CPointHurt::HurtThink );

	SetNextThink( gpGlobals->curtime + 0.1f );

	m_pActivator = data.pActivator;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the point hurt.
//-----------------------------------------------------------------------------
void CPointHurt::InputTurnOff( inputdata_t &data )
{
	SetThink( NULL );

	m_pActivator = data.pActivator;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the on/off state of the point hurt.
//-----------------------------------------------------------------------------
void CPointHurt::InputToggle( inputdata_t &data )
{
	m_pActivator = data.pActivator;

	if ( m_pfnThink == (void (CBaseEntity::*)())&CPointHurt::HurtThink )
	{
		SetThink( NULL );
	}
	else
	{
		SetThink( &CPointHurt::HurtThink );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for instantaneously hurting whatever is near us.
//-----------------------------------------------------------------------------
void CPointHurt::InputHurt( inputdata_t &data )
{
	m_pActivator = data.pActivator;

	HurtThink();
}

