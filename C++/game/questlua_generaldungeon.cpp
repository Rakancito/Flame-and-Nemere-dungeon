#include "stdafx.h"
#undef sys_err
#ifndef __WIN32__
	#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
	#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

#include "questlua.h"
#include "questmanager.h"
//#include "DBlueDragon.h"
#include "Nemere.h"
#include "Arrador.h"
//#include "DefanceWave.h"
#include "char.h"
#include "party.h"

namespace quest
{
	/*
	ALUA(BlueDragon_Access)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
		{
			DBlueDragon::CBlueDragon::instance().Access(pkChar, false);
			lua_pushboolean(L, true);
		}
		else
		{
			lua_pushboolean(L, false);
		}
		
		return 2;
	}
	*/
	ALUA(Nemere_Access)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
		{
			Nemere::CNemere::instance().Access(pkChar, false);
			lua_pushboolean(L, true);
		}
		else
		{
			lua_pushboolean(L, false);
		}
		
		return 2;
	}

	ALUA(Arrador_Access)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
		{
			Arrador::CArrador::instance().Access(pkChar, false);
			lua_pushboolean(L, true);
		}
		else
		{
			lua_pushboolean(L, false);
		}
		
		return 2;
	}

	/*
	ALUA(DefanceWave_Access)
	{
		LPCHARACTER pkChar = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChar)
		{
			DefanceWave::CDefanceWave::instance().Access(pkChar, false);
			lua_pushboolean(L, true);
		}
		else
		{
			lua_pushboolean(L, false);
		}
		
		return 2;
	}
	*/

	void RegisterGeneralDungeonFunctionTable()
	{
		luaL_reg functions[] =
		{
			//{"BlueDragonAccess", BlueDragon_Access},
			{"NemereAccess", Nemere_Access},
			{"ArradorAccess", Arrador_Access},
			//{"DefanceWaveAccess", DefanceWave_Access},
			{NULL, NULL}
		};
		
		CQuestManager::instance().AddLuaFunctionTable("GeneralDungeon", functions);
	}

};
