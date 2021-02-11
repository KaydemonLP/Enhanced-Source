//========= Copyright © 2021, KaydemonLP ============//
//
// Purpose: Class data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "of_class_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ClassInfo_t::ClassInfo_t()
{
	szClassName[0] 		= '\0';
	szLocalizedName[0] 	= '\0';

	szRightViewModel[0] = '\0';
	szLeftViewModel[0] 	= '\0';

	szPlayerModel[0] 	= '\0';

	flSpeed = 300.0f;
	flEyeHeight = 72.0f;
	
	flWallJumpTime = 0.0f;

	iWeaponCount = 0;
}

void ClassInfo_t::Parse( KeyValues *pKeyValuesData )
{
	// Localization and names
	Q_strncpy( szClassName, pKeyValuesData->GetString( "classname" ), MAX_WEAPON_STRING ); 
	Q_strncpy( szLocalizedName, pKeyValuesData->GetString( "classname_localized" ), MAX_WEAPON_STRING );

	// Models

	Q_strncpy( szRightViewModel, pKeyValuesData->GetString( "RightArm" ), MAX_WEAPON_STRING );
	Q_strncpy( szLeftViewModel , pKeyValuesData->GetString( "LeftArm"  ), MAX_WEAPON_STRING );
	
	Q_strncpy( szPlayerModel, pKeyValuesData->GetString( "playermodel" ), MAX_WEAPON_STRING );
	
	// Particles and Cosmetic

	flStepSpeed = pKeyValuesData->GetFloat("step_speed", 0.3f) * 1000;

	// Stats

	iMaxHealth = pKeyValuesData->GetInt( "health" );

	flSpeed = pKeyValuesData->GetFloat( "speed" );
	flEyeHeight = pKeyValuesData->GetFloat( "eye_height" );
	
	iJumpCount = pKeyValuesData->GetInt( "jump_count", 1 );

	// Abilities
	flWallJumpTime = pKeyValuesData->GetFloat( "wall_jump_time", 0.0f );
	flSprintMultiplier = pKeyValuesData->GetFloat("sprint_multiplier", 1.0f);

	// Equipment

	iWeaponCount = 0;

	KeyValues *pWeapons = pKeyValuesData->FindKey("Weapons");
	if( pWeapons )
	{
		FOR_EACH_VALUE( pWeapons, kvValue )
		{
			const char *szTempName = kvValue->GetString();
			m_hWeaponNames[iWeaponCount] = new char[strlen(szTempName)];
			Q_strncpy(m_hWeaponNames[iWeaponCount], szTempName, strlen(szTempName) + 1); // For some reason strlen gets 1 less than it should???
			iWeaponCount++;
		}
	}
}

void ClassInfo_t::PrecacheClass()
{
	CBaseEntity::PrecacheModel( szPlayerModel );
	CBaseEntity::PrecacheModel( szRightViewModel );
	CBaseEntity::PrecacheModel( szLeftViewModel );
}