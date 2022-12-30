#After this:

#include "DragonLair.h"

#Add


#ifdef ENABLE_GENERAL_DUNGEON
#include "Nemere.h"
#include "Arrador.h"
#endif

#Search this:

	if (true == IsMonster() && 2493 == GetMobTable().dwVnum)
	{
		if (NULL != pkKiller && NULL != pkKiller->GetGuild())
		{
			CDragonLairManager::instance().OnDragonDead( this, pkKiller->GetGuild()->GetID() );
		}
		else
		{
			sys_err("DragonLair: Dragon killed by nobody");
		}
	}
	
#Add after:

#ifdef ENABLE_GENERAL_DUNGEON
	if ((IsStone()) || (IsMonster()))
	{
		if (pkKiller && pkKiller->IsPC())
		{
			if (Nemere::CNemere::instance().IsNemereMap(pkKiller->GetMapIndex()))
				Nemere::CNemere::instance().OnKill(this, pkKiller);
			else if (Arrador::CArrador::instance().IsArradorMap(pkKiller->GetMapIndex()))
				Arrador::CArrador::instance().OnKill(this, pkKiller);
		}
	}
#endif
	