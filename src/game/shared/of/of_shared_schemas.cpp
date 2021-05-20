//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "shareddefs.h"
#if GAME_DLL
#include "sdk_player.h"
#else
#include "c_sdk_player.h"
#endif
#include "of_shared_schemas.h"

#include "tier0/memdbgon.h"


KeyValues* gItemsGame;
KeyValues* GetItemsGame()
{
	return gItemsGame;
}

void InitItemsGame()
{
	if( gItemsGame )
	{
		gItemsGame->deleteThis();
	}
	gItemsGame = new KeyValues( "ItemsGame" );
}

void SetupBaseItemsGame()
{
	// Formatting it like the items game itself, you can't stop me >:3
	KeyValues *pTempItemsGame = new KeyValues( "ItemsGame" );
	{
		KeyValues *pCosmetics = new KeyValues( "Cosmetics" );
		{
			KeyValues *pCosmetic = new KeyValues( "0" );
			{
				pCosmetic->SetString( "model",	"model.mdl" );
				pCosmetic->SetString( "region",	"suit" );
				pCosmetic->SetString( "backpack_icon",	"hud_thing" );
				pCosmetic->SetString( "viewmodel",		"sleeves.mdl" );

				KeyValues *pStyles = new KeyValues( "Styles" );
				{
					KeyValues *pStyle = new KeyValues( "1" );
					{
						pStyle->SetString( "model", "alt_model.mdl" );
					}
					pStyles->AddSubKey( pStyle );
				}

				KeyValues *pBodygroups = new KeyValues( "Bodygroups" );
				{
					pBodygroups->SetString( "torso",	"1" );
					pBodygroups->SetString( "rightarm", "1" );
					pBodygroups->SetString( "leftarm",	"1" );
				}

				pCosmetic->AddSubKey( pStyles );
				pCosmetic->AddSubKey( pBodygroups );
			}
			pCosmetics->AddSubKey(pCosmetic);
		}
		pTempItemsGame->AddSubKey( pCosmetics );

		KeyValues *pWeapons = new KeyValues( "Weapons" );
		{
			KeyValues *pWeapon = new KeyValues( "weapon_test" );
			{
				pWeapon->SetString( "weapon_class", "weapon_example" );

				KeyValues *pWeaponAttributes = new KeyValues( "attributes" );
				{
					KeyValues *pDamageMult = new KeyValues( "damage multiplier" );
					{
						pDamageMult->SetString( "value", "50" );
					}
					pWeaponAttributes->AddSubKey( pDamageMult );
				}
				pWeapon->AddSubKey(pWeaponAttributes);

				KeyValues *pWeaponData = new KeyValues( "WeaponData" );
				{
					// Attributes Base.
					pWeaponData->SetString("printname", "This is a Very rad weapon");
					pWeaponData->SetString("Damage", "1");
				}
				pWeapon->AddSubKey( pWeaponData );
			}
			pWeapons->AddSubKey(pWeapon);
		}
		pTempItemsGame->AddSubKey(pWeapons);

		KeyValues *pAttributes = new KeyValues( "Attributes" );
		{
			KeyValues *pAttribute = new KeyValues( "1" );
			{
				pAttribute->SetString("name","damage multiplier");
			}
			pAttributes->AddSubKey(pAttribute);
		}
		pTempItemsGame->AddSubKey(pAttributes);

	}

	// Always attempt this in case the directory doesn't exist
	filesystem->CreateDirHierarchy("scripts/items/", "MOD");

	if( !pTempItemsGame->SaveToFile( filesystem, "scripts/items/items_game.txt", "MOD" ) )
		Error( "COULDN'T SETUP BASE ITEMS GAME!\nCheck if you have a scripts/items/ folder!" );

	pTempItemsGame->deleteThis();
}

void ParseItemsGame( void )
{	
	InitItemsGame();
	
	// This is moreso for other Games that plan on using this system
	// basically just setups the base keyvalues
	// and an example for each one for better use
	if( !filesystem->FileExists( "scripts/items/items_game.txt" ) )
		SetupBaseItemsGame();

	GetItemsGame()->LoadFromFile( filesystem, "scripts/items/items_game.txt" );
	
	KeyValues *pCosmetics = GetItemsGame()->FindKey( "Cosmetics" );
	if( pCosmetics )
	{
		int i = 0;
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			i++;
		}
		GetItemsGame()->SetInt( "cosmetic_count", i );
	}

	KeyValues *pWeapons = GetItemsGame()->FindKey( "Weapons" );
	if( pWeapons )
	{
		FOR_EACH_SUBKEY( pWeapons, kvSubKey )
		{
			// No duplicates in case they're already defined in shareddefs
			// Commented out since Offshore doesn't use Shareddefs! 
			// If your game does however, uncomment it! - Kay
			if( ItemSchema()->GetWeaponID( kvSubKey->GetName() ) == -1 )
				ItemSchema()->AddWeapon( kvSubKey->GetName() );
		}
	}
	
	KeyValues *pAttributes = GetItemsGame()->FindKey( "Attributes" );
	if( pAttributes )
	{
		FOR_EACH_SUBKEY( pAttributes, kvSubKey )
		{
			ItemSchema()->AddAttribute( 
				atoi(kvSubKey->GetName()),
				kvSubKey->GetString( "name", "error_no_name_defined" ) );
		}
	}	
}

KeyValues* gLevelItemsGame;
KeyValues* GetLevelItemsGame()
{
	return gLevelItemsGame;
}

void InitLevelItemsGame()
{
	if( gLevelItemsGame )
	{
		// Remove all weapons and reset the cosmetic count
		if( ItemSchema() )
			ItemSchema()->PurgeLevelItems();
		gLevelItemsGame->deleteThis();
	}
	gLevelItemsGame = new KeyValues( "LevelItemsGame" );
}

void ParseLevelItemsGame( void )
{	
	InitLevelItemsGame();
	
	char szMapItems[MAX_PATH];
	char szMapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	UTIL_VarArgs("maps/%s",STRING(gpGlobals->mapname))
#endif
	, szMapname, sizeof( szMapname ) );
	Q_snprintf( szMapItems, sizeof( szMapItems ), "%s_items_game.txt", szMapname );
	if( !filesystem->FileExists( szMapItems , "GAME" ) )
	{
		DevMsg( "%s not present, not parsing\n", szMapItems );
		// We set this to null so certain functions bail out faster
		gLevelItemsGame = NULL;
		return;
	}
#if CLIENT_DLL
	DevMsg("Client %s\n", szMapItems);
#else
	DevMsg("Server %s\n", szMapItems);
#endif
	GetLevelItemsGame()->LoadFromFile( filesystem, szMapItems, "GAME" );
	
	KeyValues *pCosmetics = GetLevelItemsGame()->FindKey("Cosmetics");
	if( pCosmetics )
	{
		int i = 0;
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			i++;
		}
		GetLevelItemsGame()->SetInt("cosmetic_count", i);
		GetItemsGame()->SetInt( "cosmetic_count", GetItemsGame()->GetInt( "cosmetic_count") + i );
	}

	KeyValues *pWeapons = GetLevelItemsGame()->FindKey("Weapons");
	if( pWeapons )
	{
		FOR_EACH_SUBKEY( pWeapons, kvSubKey )
		{
			ItemSchema()->AddLevelWeapon( kvSubKey->GetName() );
		}
	}	
	
}

void ReloadItemsSchema()
{
	ItemSchema()->PurgeSchema();
	InitItemsGame();
	ParseItemsGame();
	ParseLevelItemsGame();
#ifdef CLIENT_DLL
	engine->ExecuteClientCmd( "schema_reload_items_game_server" );
#endif
}

static ConCommand schema_reload_items_game( 
#ifdef CLIENT_DLL
"schema_reload_items_game",
#else
"schema_reload_items_game_server",
#endif
 ReloadItemsSchema, "Reloads the items game.", FCVAR_NONE );

KeyValues* GetCosmetic( int iID )
{
	if( GetLevelItemsGame() )
	{
		KeyValues *pCosmetics = GetLevelItemsGame()->FindKey("Cosmetics");
		if( pCosmetics )
		{
			KeyValues *pCosmetic = pCosmetics->FindKey( UTIL_VarArgs( "%d", iID ) );
			if( pCosmetic )
				return pCosmetic;
		}
	}

	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( !pCosmetics )
		return NULL;

	KeyValues *pCosmetic = pCosmetics->FindKey( UTIL_VarArgs( "%d", iID ) );
	if( !pCosmetic )
		return NULL;
	
	return pCosmetic;
}

KeyValues* GetWeaponFromSchema( const char *szName )
{
	if( GetLevelItemsGame() )
	{
		KeyValues *pWeapons = GetLevelItemsGame()->FindKey("Weapons");
		if( pWeapons )
		{
			KeyValues *pWeapon = pWeapons->FindKey( szName );
			if( pWeapon )
				return pWeapon;
		}
	}

	KeyValues *pWeapons = GetItemsGame()->FindKey("Weapons");
	if( !pWeapons )
		return NULL;
	
	KeyValues *pWeapon = pWeapons->FindKey( szName );
	if( !pWeapon )
		return NULL;
	
	return pWeapon;
}

COFClassManager gClassManager;
COFClassManager *ClassManager()
{
	return &gClassManager;
}

COFClassManager::COFClassManager() { }

void COFClassManager::ParseSchema()
{
	// Clear the schema in case its there
	PurgeSchema();

	KeyValues *pClassFiles = new KeyValues("ClassFiles");
	pClassFiles->LoadFromFile( filesystem, "classes/class_manifest.txt" );

	FOR_EACH_SUBKEY( pClassFiles, kvSubKey )
	{
		KeyValues *pClassData = new KeyValues("ClassData");
		pClassData->LoadFromFile( filesystem, kvSubKey->GetString() );
		
		ClassInfo_t hClassInfo;
		hClassInfo.Parse( pClassData );
		
		m_hClassInfo.AddToTail(hClassInfo);
		pClassData->deleteThis();
	}
	
	m_iClassCount = m_hClassInfo.Count();
	
	pClassFiles->deleteThis();
}

void COFClassManager::PurgeSchema()
{
	m_hClassInfo.Purge();
	m_iClassCount = 0;
}

void COFClassManager::PrecacheClasses()
{
	for( int i = 0; i < m_iClassCount; i++ )
	{
		m_hClassInfo[i].PrecacheClass();
	}
}

void CC_ReloadClasses( void )
{
	ClassManager()->ParseSchema();
	
	// Precache again in case something new is added
	ClassManager()->PrecacheClasses();

#ifdef CLIENT_DLL
	engine->ExecuteClientCmd( "schema_reload_classes_server" );
#endif
}

#ifdef CLIENT_DLL
	static ConCommand schema_reload_classes("schema_reload_classes", CC_ReloadClasses, "Reload class schema\n", FCVAR_CHEAT );
#else
	static ConCommand schema_reload_classes_server("schema_reload_classes_server", CC_ReloadClasses, "Reload class schema\n", FCVAR_CHEAT | FCVAR_HIDDEN );
#endif


int GetClassIndexByName( const char *szClassName )
{	
	for( int i = 0; i < OF_CLASS_COUNT; i++ )
	{
		if( FStrEq( ClassManager()->m_hClassInfo[i].szClassName, szClassName ) )
			return i;
	}
	
	return -1;
}

COFItemSchema *gItemSchema;
COFItemSchema *ItemSchema()
{
	return gItemSchema;
}

void InitItemSchema()
{
	gItemSchema = new COFItemSchema();
}

COFItemSchema::COFItemSchema()
{
	m_bWeaponWasModified = false;
}

void COFItemSchema::PurgeSchema()
{
	m_hWeaponNames.Purge();
	m_hAttributeList.Purge();
}

extern void MarkWeaponAsDirty( const char *szName );
extern void RemoveWeaponFromDatabase( const char *szName );

void COFItemSchema::PurgeLevelItems()
{
	if( m_bWeaponWasModified )
	{
		// Reload weapons if one was modified
		KeyValues *pWeapons = GetLevelItemsGame() ? GetLevelItemsGame()->FindKey("Weapons") : NULL;
		if( pWeapons )
		{
			FOR_EACH_SUBKEY( pWeapons, kvSubKey )
			{
				MarkWeaponAsDirty(kvSubKey->GetName());
			}
		}
		m_bWeaponWasModified = false;
	}	
	int iCount = m_hLevelWeaponNames.Count();
	for( int i = 0; i < iCount; i++ )
	{
		m_hWeaponNames.Remove(m_hLevelWeaponNames.GetElementName(i));
		// This currently causes a lot of crashes
		// Lets hope it doesn't clog up memory too much
		//RemoveWeaponFromDatabase(m_hLevelWeaponNames[i]);
	}
	m_hLevelWeaponNames.Purge();
	if( GetItemsGame() && GetLevelItemsGame() )
		GetItemsGame()->SetInt( "cosmetic_count", GetItemsGame()->GetInt( "cosmetic_count") - GetLevelItemsGame()->GetInt( "cosmetic_count") );
}

void COFItemSchema::AddWeapon( const char *szWeaponName )
{
	m_hWeaponNames.Insert(szWeaponName);
}

void COFItemSchema::AddLevelWeapon( const char *szWeaponName )
{
	for( unsigned int i = 0; i < m_hWeaponNames.Count(); i++ )
	{
		// Don't add a name if we already have it but mark the fact that we modified a weapon
		if( FStrEq(szWeaponName, m_hWeaponNames.GetElementName(i)) )
		{
			MarkWeaponAsDirty(szWeaponName);
			m_bWeaponWasModified = true;
			return;
		}
	}
	m_hLevelWeaponNames.Insert( szWeaponName );
	AddWeapon(szWeaponName);
}

void COFItemSchema::AddAttribute( int iID, const char *szName )
{
	m_hAttributeList.Insert( szName, iID );
}

int COFItemSchema::GetAttributeID( char *szName )
{
	int ret = m_hAttributeList.Find(szName);
	return ret == m_hAttributeList.InvalidIndex() ? -1 : m_hAttributeList[ret];
}

int COFItemSchema::GetAttributeID( const char *szName )
{
	char szFullName[128];
	Q_strncpy(szFullName, szName, 128);
	return GetAttributeID( szFullName );
}

KeyValues *COFItemSchema::GetWeapon( unsigned int iID )
{
	if( iID >= m_hWeaponNames.Count() || iID < 0 )
		return NULL;
	
	return GetWeaponFromSchema( m_hWeaponNames.GetElementName(iID) );
}

KeyValues *COFItemSchema::GetWeapon( const char *szWeaponName )
{
	return GetWeaponFromSchema( szWeaponName );
}

int COFItemSchema::GetWeaponID( const char *szWeaponName )
{
	char szName[128];
	Q_strncpy(szName, szWeaponName, sizeof(szName));
	strlwr(szName);
	int iMax = m_hWeaponNames.Count();
	for( int i = 0; i < iMax; i++ )
	{
		if( FStrEq(m_hWeaponNames.GetElementName(i), szWeaponName) )
			return i;
	}

	return -1;
}

const char *COFItemSchema::GetWeaponName( unsigned int iID )
{
	if( iID >= m_hWeaponNames.Count() )
		return NULL;

	return m_hWeaponNames.GetElementName(iID);
}

KeyValues* gMapData;
KeyValues* GetMapData()
{
	return gMapData;
}

void InitMapData()
{
	if( gMapData )
	{
		gMapData->deleteThis();
	}
	gMapData = new KeyValues( "MapData" );
}

void ParseMapDataSchema( void )
{	
	InitMapData();
	
	char szMapData[MAX_PATH];
	char szMapName[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	UTIL_VarArgs("maps/%s",STRING(gpGlobals->mapname))
#endif
	, szMapName, sizeof( szMapName ) );
	Q_snprintf( szMapData, sizeof( szMapData ), "%s_mapdata.txt", szMapName );

	if( !filesystem->FileExists( szMapData , "GAME" ) )	
	{
		DevMsg( "%s not present, not parsing¸\n", szMapData );
		return;
	}
	
	GetMapData()->LoadFromFile( filesystem, szMapData );
	
	FOR_EACH_SUBKEY( GetMapData(), kvSubKey )
	{
		DevMsg( "%s\n", kvSubKey->GetName() );
	}
}

#if 0 // client
void PrecacheUISoundScript( char *szSoundScript )
{
	KeyValues *pSoundScript = GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return;
	
	KeyValues *pWave = pSoundScript->FindKey("rndwave");
	if( pWave )
	{
		bool bSuccess = false;
		for( KeyValues *pSub = pWave->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			bSuccess = true;
			enginesound->PrecacheSound( pSub->GetString(), true, false );
		}

		if( !bSuccess )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
	}
	else
	{
		pWave = pSoundScript->FindKey("wave");
		if( !pWave )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
		
		enginesound->PrecacheSound( pWave->GetString(), true, false );
	}
}
#endif



// Load all schemas on client/server start
// Must be at the end of the file so our inits can be declared above
void ParseSharedSchemas()
{
	ClassManager()->ParseSchema();
	InitItemSchema();
	ParseItemsGame();
}

void ParseLevelSchemas()
{
	ParseMapDataSchema();
	ParseLevelItemsGame();
}