#include "of_class_parse.h"

#pragma once

#define OF_CLASS_COUNT ClassManager()->m_iClassCount

class KeyValues;
class ClassInfo_t;

extern KeyValues* GetWeaponFromSchema( const char *szName );
extern KeyValues* GetCosmetic( int iID );

extern void ParseSharedSchemas( void );
extern void ParseLevelSchemas( void );
extern void ParseMapDataSchema( void );

extern KeyValues* GetItemsGame();
extern KeyValues* GetLevelItemsGame();

class COFClassManager
{
public:
	COFClassManager();

	void ParseSchema();
	void PurgeSchema();
	void PrecacheClasses();
public:
	CUtlVector<ClassInfo_t> m_hClassInfo;

	int m_iClassCount;
};

extern int GetClassIndexByName( const char *szClassName );

extern COFClassManager *ClassManager();
extern KeyValues* GetMapData();

class COFItemSchema
{
public:
	COFItemSchema();
	void PurgeSchema();
	void PurgeLevelItems();
	
	void AddWeapon( const char *szWeaponName );
	void AddLevelWeapon( const char *szWeaponName );
	KeyValues *GetWeapon( unsigned  int iID );
	KeyValues *GetWeapon( const char *szWeaponName );

	void AddAttribute( int iID, const char *szName );
	int GetAttributeID( char *szName );
	int GetAttributeID( const char *szName );

	int GetWeaponID( const char *szWeaponName );
	const char *GetWeaponName( unsigned int iID );
	
	int GetWeaponCount( void ){ return m_hWeaponNames.Count();};
public:
	bool m_bWeaponWasModified;
	
private:
	CUtlDict<int, unsigned short> m_hWeaponNames;
	CUtlDict<int, unsigned short> m_hLevelWeaponNames;

	CUtlDict<int, unsigned short> m_hAttributeList;
};

extern COFItemSchema *ItemSchema();
extern void InitItemSchema();