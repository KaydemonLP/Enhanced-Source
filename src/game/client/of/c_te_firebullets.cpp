//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "of_fx_shared.h"
#include "c_sdk_player.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>


class C_TEFireBullets : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFireBullets, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int		m_iPlayer;
	Vector	m_vecOrigin;
	QAngle	m_vecAngles;
	int		m_iMode;
	int		m_iDamage;
	int		m_iBulletsPerShot;
	int		m_iSeed;
	float	m_flSpread;
	float	m_flRange;
};


void C_TEFireBullets::PostDataUpdate( DataUpdateType_t updateType )
{
	// Create the effect.
	
	m_vecAngles.z = 0;	
	
	FX_FireBullets( 
		m_iPlayer+1,
		m_vecOrigin,
		m_vecAngles,
		m_iMode,
		m_iDamage,
		m_iBulletsPerShot,
		m_iSeed,
		m_flSpread,
		m_flRange
		);
}


IMPLEMENT_CLIENTCLASS_EVENT( C_TEFireBullets, DT_TEFireBullets, CTEFireBullets );


BEGIN_RECV_TABLE_NOBASE(C_TEFireBullets, DT_TEFireBullets)
	RecvPropVector( RECVINFO( m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_vecAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_vecAngles[1] ) ),
	RecvPropInt( RECVINFO( m_iMode ) ), 
	RecvPropInt( RECVINFO( m_iDamage ) ), 
	RecvPropInt( RECVINFO( m_iBulletsPerShot ) ), 
	RecvPropInt( RECVINFO( m_iSeed ) ),
	RecvPropInt( RECVINFO( m_iPlayer ) ),
	RecvPropFloat( RECVINFO( m_flSpread ) ),
	RecvPropFloat( RECVINFO( m_flRange ) ),
END_RECV_TABLE()