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

// Load all schemas on client/server start
void ParseSharedSchemas()
{
	GetClassManager()->ParseSchema();
}

void ParseLevelSchemas()
{
	ParseMapDataSchema();
}

CTFClassManager gClassManager;
CTFClassManager *GetClassManager()
{
	return &gClassManager;
}

CTFClassManager::CTFClassManager() { }

void CTFClassManager::ParseSchema()
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

void CTFClassManager::PurgeSchema()
{
	m_hClassInfo.Purge();
	m_iClassCount = 0;
}

void CTFClassManager::PrecacheClasses()
{
	for( int i = 0; i < m_iClassCount; i++ )
	{
		m_hClassInfo[i].PrecacheClass();
	}
}

void CC_ReloadClasses( void )
{
	GetClassManager()->ParseSchema();
	
	// Precache again in case something new is added
	GetClassManager()->PrecacheClasses();

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
		if( FStrEq( GetClassManager()->m_hClassInfo[i].szClassName, szClassName ) )
			return i;
	}
	
	return -1;
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