//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TAKEDAMAGEINFO_H
#define TAKEDAMAGEINFO_H
#ifdef _WIN32
#pragma once
#endif


#include "networkvar.h" // todo: change this when DECLARE_CLASS is moved into a better location.
#include "utlvector.h"

// Used to initialize m_flBaseDamage to something that we know pretty much for sure
// hasn't been modified by a user. 
#define BASEDAMAGE_NOT_SPECIFIED	FLT_MAX

class CBaseEntity;


class CTakeDamageInfo
{
public:
	DECLARE_CLASS_NOBASE( CTakeDamageInfo );

					CTakeDamageInfo();
#ifdef OFFSHORE_DLL
					// Since the default compiler fucks the CUtlVector up, we gotta do it by hand
					// If it was possible to fix the const constructor for CUtlVector i'd do that but alas
					// yeah i know its stupid - Kay
					CTakeDamageInfo( CTakeDamageInfo const& other );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0 );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0 );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
#else
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0 );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0 );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
#endif

	// Inflictor is the weapon or rocket (or player) that is dealing the damage.
	CBaseEntity*	GetInflictor() const;
	void			SetInflictor( CBaseEntity *pInflictor );

	// Weapon is the weapon that did the attack.
	// For hitscan weapons, it'll be the same as the inflictor. For projectile weapons, the projectile 
	// is the inflictor, and this contains the weapon that created the projectile.
	CBaseEntity*	GetWeapon() const;
	void			SetWeapon( CBaseEntity *pWeapon );

	// Attacker is the character who originated the attack (like a player or an AI).
	CBaseEntity*	GetAttacker() const;
	void			SetAttacker( CBaseEntity *pAttacker );

	float			GetDamage() const;
	void			SetDamage( float flDamage );
	float			GetMaxDamage() const;
	void			SetMaxDamage( float flMaxDamage );
	void			ScaleDamage( float flScaleAmount );
	void			AddDamage( float flAddAmount );
	void			SubtractDamage( float flSubtractAmount );

	float			GetBaseDamage() const;
	bool			BaseDamageIsValid() const;

	Vector			GetDamageForce() const;
	void			SetDamageForce( const Vector &damageForce );
	void			ScaleDamageForce( float flScaleAmount );

	Vector			GetDamagePosition() const;
	void			SetDamagePosition( const Vector &damagePosition );

	Vector			GetReportedPosition() const;
	void			SetReportedPosition( const Vector &reportedPosition );

#ifdef OFFSHORE_DLL
	bool			HasDamageType( int iDamageType ) const;
	CUtlVector<int>	*GetDamageTypes() const;
#else
	int				GetDamageType() const;
#endif
	void			SetDamageType( int bitsDamageType );
#ifdef OFFSHORE_DLL
	void			SetDamageType( CUtlVector<int> *hDamageType );
#endif
	void			AddDamageType( int bitsDamageType );
#ifdef OFFSHORE_DLL
	void			AddDamageType( CUtlVector<int> *hDamageType );
#endif
	int				GetDamageCustom( void ) const;
	void			SetDamageCustom( int iDamageCustom );
	int				GetDamageStats( void ) const;
	void			SetDamageStats( int iDamageStats );

	int				GetAmmoType() const;
	void			SetAmmoType( int iAmmoType );
	const char *	GetAmmoName() const;

	float			GetRadius() const;
	void			SetRadius( float fRadius );

#ifdef OFFSHORE_DLL
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0 );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0 );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
#else
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0 );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0 );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
	void			Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
#endif
	void			AdjustPlayerDamageInflictedForSkillLevel();
	void			AdjustPlayerDamageTakenForSkillLevel();

	// Given a damage type (composed of the #defines above), fill out a string with the appropriate text.
	// For designer debug output.
#ifdef OFFSHORE_DLL
	static void		DebugGetDamageTypeString(CUtlVector<int> *DamageType, char *outbuf, int outbuflength);
#else
	static void		DebugGetDamageTypeString(unsigned int DamageType, char *outbuf, int outbuflength );
#endif

//private:
	void			CopyDamageToBaseDamage();

protected:
#ifdef OFFSHORE_DLL
	void			Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType );
#else
	void			Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );
#endif
	Vector			m_vecDamageForce;
	Vector			m_vecDamagePosition;
	Vector			m_vecReportedPosition;	// Position players are told damage is coming from
	EHANDLE			m_hInflictor;
	EHANDLE			m_hAttacker;
	EHANDLE			m_hWeapon;
	float			m_flDamage;
	float			m_flMaxDamage;
	float			m_flBaseDamage;			// The damage amount before skill leve adjustments are made. Used to get uniform damage forces.
#ifdef OFFSHORE_DLL
	CUtlVector<int>	m_hDamageType;			// Offshore uses CUtlVector in order to have more than 32 possible damage types
#else
	int				m_bitsDamageType;
#endif
	int				m_iDamageCustom;
	int				m_iDamageStats;
	int				m_iAmmoType;			// AmmoType of the weapon used to cause this damage, if any
	float			m_flRadius;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: Multi damage. Used to collect multiple damages in the same frame (i.e. shotgun pellets)
//-----------------------------------------------------------------------------
class CMultiDamage : public CTakeDamageInfo
{
	DECLARE_CLASS( CMultiDamage, CTakeDamageInfo );
public:
	CMultiDamage();

	bool			IsClear( void ) { return (m_hTarget == NULL); }
	CBaseEntity		*GetTarget() const;
	void			SetTarget( CBaseEntity *pTarget );

#ifdef OFFSHORE_DLL
	void			Init( CBaseEntity *pTarget, CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, CUtlVector<int> *hDamageType, int iKillType );
#else
	void			Init( CBaseEntity *pTarget, CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );
#endif
protected:
	EHANDLE			m_hTarget;

	DECLARE_SIMPLE_DATADESC();
};

extern CMultiDamage g_MultiDamage;

// Multidamage accessors
void ClearMultiDamage( void );
void ApplyMultiDamage( void );
void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );

//-----------------------------------------------------------------------------
// Purpose: Utility functions for physics damage force calculation 
//-----------------------------------------------------------------------------
float ImpulseScale( float flTargetMass, float flDesiredSpeed );
void CalculateExplosiveDamageForce( CTakeDamageInfo *info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateBulletDamageForce( CTakeDamageInfo *info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateMeleeDamageForce( CTakeDamageInfo *info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void GuessDamageForce( CTakeDamageInfo *info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale = 1.0 );


// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //

inline CBaseEntity* CTakeDamageInfo::GetInflictor() const
{
	return m_hInflictor;
}


inline void CTakeDamageInfo::SetInflictor( CBaseEntity *pInflictor )
{
	m_hInflictor = pInflictor;
}


inline CBaseEntity* CTakeDamageInfo::GetAttacker() const
{
	return m_hAttacker;
}


inline void CTakeDamageInfo::SetAttacker( CBaseEntity *pAttacker )
{
	m_hAttacker = pAttacker;
}

inline CBaseEntity* CTakeDamageInfo::GetWeapon() const
{
	return m_hWeapon;
}


inline void CTakeDamageInfo::SetWeapon( CBaseEntity *pWeapon )
{
	m_hWeapon = pWeapon;
}


inline float CTakeDamageInfo::GetDamage() const
{
	return m_flDamage;
}

inline void CTakeDamageInfo::SetDamage( float flDamage )
{
	m_flDamage = flDamage;
}

inline float CTakeDamageInfo::GetMaxDamage() const
{
	return m_flMaxDamage;
}

inline void CTakeDamageInfo::SetMaxDamage( float flMaxDamage )
{
	m_flMaxDamage = flMaxDamage;
}

inline void CTakeDamageInfo::ScaleDamage( float flScaleAmount )
{
	m_flDamage *= flScaleAmount;
}

inline void CTakeDamageInfo::AddDamage( float flAddAmount )
{
	m_flDamage += flAddAmount;
}

inline void CTakeDamageInfo::SubtractDamage( float flSubtractAmount )
{
	m_flDamage -= flSubtractAmount;
}

inline float CTakeDamageInfo::GetBaseDamage() const
{
	if( BaseDamageIsValid() )
		return m_flBaseDamage;

	// No one ever specified a base damage, so just return damage.
	return m_flDamage;
}

inline bool CTakeDamageInfo::BaseDamageIsValid() const
{
	return (m_flBaseDamage != BASEDAMAGE_NOT_SPECIFIED);
}

inline Vector CTakeDamageInfo::GetDamageForce() const
{
	return m_vecDamageForce;
}

inline void CTakeDamageInfo::SetDamageForce( const Vector &damageForce )
{
	m_vecDamageForce = damageForce;
}

inline void	CTakeDamageInfo::ScaleDamageForce( float flScaleAmount )
{
	m_vecDamageForce *= flScaleAmount;
}

inline Vector CTakeDamageInfo::GetDamagePosition() const
{
	return m_vecDamagePosition;
}


inline void CTakeDamageInfo::SetDamagePosition( const Vector &damagePosition )
{
	m_vecDamagePosition = damagePosition;
}

inline Vector CTakeDamageInfo::GetReportedPosition() const
{
	return m_vecReportedPosition;
}


inline void CTakeDamageInfo::SetReportedPosition( const Vector &reportedPosition )
{
	m_vecReportedPosition = reportedPosition;
}
#ifdef OFFSHORE_DLL
inline bool CTakeDamageInfo::HasDamageType( int iDamageType ) const
{
	return m_hDamageType.HasElement(iDamageType);
}
inline CUtlVector<int> *CTakeDamageInfo::GetDamageTypes() const
{
	return const_cast<CUtlVector<int> *>(&m_hDamageType);
}
#else
inline int CTakeDamageInfo::GetDamageType() const
{
	return m_bitsDamageType;
}
#endif

#ifdef OFFSHORE_DLL
inline void CTakeDamageInfo::SetDamageType( CUtlVector<int> *hDamageType )
{
	m_hDamageType.Purge();

	if( hDamageType )
	{
		FOR_EACH_VEC(*hDamageType, iType)
		{
			m_hDamageType.AddToTail(hDamageType->Element(iType));
		}
	}
}
#endif


inline void CTakeDamageInfo::SetDamageType( int bitsDamageType )
{
#ifdef OFFSHORE_DLL
	Assert(!"OFFSHORE DOES NOT USE BITS FOR SET DAMAGE TYPE, CHANGE WHICHEVER FUNCTION IS USING THIS");
#else
	m_bitsDamageType = bitsDamageType;
#endif

}

#ifdef OFFSHORE_DLL
inline void	CTakeDamageInfo::AddDamageType( CUtlVector<int> *hDamageType )
{
	FOR_EACH_VEC(*hDamageType, iType)
	{
		m_hDamageType.AddToTail(hDamageType->Element(iType));
	}
}
#endif

inline void	CTakeDamageInfo::AddDamageType( int bitsDamageType )
{
#ifdef OFFSHORE_DLL
	m_hDamageType.AddToTail(bitsDamageType);
#else
	m_bitsDamageType |= bitsDamageType;
#endif
	
}

inline int CTakeDamageInfo::GetDamageCustom() const
{
	return m_iDamageCustom;
}

inline void CTakeDamageInfo::SetDamageCustom( int iDamageCustom )
{
	m_iDamageCustom = iDamageCustom;
}

inline int CTakeDamageInfo::GetDamageStats() const
{
	return m_iDamageCustom;
}

inline void CTakeDamageInfo::SetDamageStats( int iDamageCustom )
{
	m_iDamageCustom = iDamageCustom;
}

inline int CTakeDamageInfo::GetAmmoType() const
{
	return m_iAmmoType;
}

inline void CTakeDamageInfo::SetAmmoType( int iAmmoType )
{
	m_iAmmoType = iAmmoType;
}

inline void CTakeDamageInfo::CopyDamageToBaseDamage()
{ 
	m_flBaseDamage = m_flDamage;
}

inline float CTakeDamageInfo::GetRadius() const
{
	return m_flRadius;
}

inline void CTakeDamageInfo::SetRadius( float flRadius )
{
	m_flRadius = flRadius;
}


// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //
inline CBaseEntity *CMultiDamage::GetTarget() const
{
	return m_hTarget;
}

inline void CMultiDamage::SetTarget( CBaseEntity *pTarget )
{
	m_hTarget = pTarget;
}


#endif // TAKEDAMAGEINFO_H
