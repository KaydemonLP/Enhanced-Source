//========= Copyright © 2021, KaydemonLP ============//
//
// Purpose: Class data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#pragma once

#include "shareddefs.h"

class IFileSystem;

class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the class' script file. 
// It's cached so we only read each class's script file once.
//-----------------------------------------------------------------------------
class ClassInfo_t
{
public:

	ClassInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData );
	virtual void PrecacheClass();
public:	
// 	SHARED

	// Misc
	bool		bPlayable;	// Is this class selectable as a player?

	// Localization and names
	char		szClassName[MAX_WEAPON_STRING];
	char		szLocalizedName[MAX_WEAPON_STRING];		// Name for showing in HUD, etc.

	// Models
	char		szRightViewModel[MAX_WEAPON_STRING];	// View model of this weapon
	char		szLeftViewModel[MAX_WEAPON_STRING];		// View model of this weapon
	char		szPlayerModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player

	// HUD Stuff
	char		szClassImage[MAX_WEAPON_STRING];

	// Particles and Cosmetic
	float		flStepSpeed;

	// Stats
	int			iMaxHealth;

	float		flSpeed;
	float		flEyeHeight;
	
	int			iJumpCount;
	
	// Abilities
	float		flWallClimbTime;
	float		flWallRunTime;
	float		flWallJumpTime;
	float		flSprintMultiplier;
	bool		bCanSlide;
	bool		bCanWallRun;
	
	// Equipment
	// TODO: Replace weapon names with a weapon dictionary Index
	char		*m_hWeaponNames[MAX_WEAPONS];

	int			iWeaponCount;
};