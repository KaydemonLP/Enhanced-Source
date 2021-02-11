#include "of_class_parse.h"

#pragma once

#define OF_CLASS_COUNT GetClassManager()->m_iClassCount

class KeyValues;
class ClassInfo_t;

extern void ParseSharedSchemas( void );
extern void ParseLevelSchemas( void );
extern void ParseMapDataSchema( void );

class CTFClassManager
{
public:
	CTFClassManager();

	void ParseSchema();
	void PurgeSchema();
	void PrecacheClasses();
public:
	CUtlVector<ClassInfo_t> m_hClassInfo;

	int m_iClassCount;
};

extern int GetClassIndexByName( const char *szClassName );

extern CTFClassManager *GetClassManager();
extern KeyValues* GetMapData();