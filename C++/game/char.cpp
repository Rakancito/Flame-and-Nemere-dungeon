#Search

#include "skill_power.h"

#Add after

#ifdef ENABLE_GENERAL_DUNGEON
#include "Arrador.h"
#include "Nemere.h"
#endif


#After this:


void CHARACTER::OnClick(LPCHARACTER pkChrCauser)
{
	[...]
	sys_log(0, "OnClick %s[vnum %d ServerUniqueID %d, pid %d] by %s", GetName(), GetRaceNum(), vid, GetPlayerID(), pkChrCauser->GetName());
	
	
#add 

#ifdef ENABLE_GENERAL_DUNGEON
	if (IsNPC() && GetRaceNum() == (WORD)(Arrador::STATUE_NPC) && Arrador::CArrador::instance().IsArradorMap(pkChrCauser->GetMapIndex()))
	{
		Arrador::CArrador::instance().SetDungeonStep(pkChrCauser);
		return;
	}
	if (IsNPC() && GetRaceNum() == (WORD)(Nemere::NPC_LEVEL1) && Nemere::CNemere::instance().IsNemereMap(pkChrCauser->GetMapIndex()))
	{
		Nemere::CNemere::instance().SetDungeonStep(pkChrCauser);
		return;
	}

#endif