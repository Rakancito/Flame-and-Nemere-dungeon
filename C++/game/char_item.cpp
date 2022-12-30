#Search

#include "DragonSoul.h"

#Add after


#ifdef ENABLE_GENERAL_DUNGEON
#include "Arrador.h"
#include "Nemere.h"
#endif

#Search

bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	[...]
	
	switch (item->GetType())
	{

		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
	
#Add after	
		
#ifdef ENABLE_GENERAL_DUNGEON
			if (item->GetVnum() == Nemere::LEVEL2_REALKEY_VNUM && Nemere::CNemere::instance().IsNemereMap(GetMapIndex()))
			{
				Nemere::CNemere::instance().SetItemUse(this, item);
				item->SetCount(item->GetCount() - 1);
				return true;
			}

			if (item->GetVnum() == Nemere::LEVEL8_REALKEY_VNUM && Nemere::CNemere::instance().IsNemereMap(GetMapIndex()))
			{
				Nemere::CNemere::instance().SetItemUse(this, item);
				item->SetCount(item->GetCount() - 1);
				return true;
			}
#endif

#Search

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
	
#Add after

#ifdef ENABLE_GENERAL_DUNGEON
		case Arrador::STONE_LEVEL2_5:
			if (Arrador::CArrador::instance().GetDungeonStep(from) == 2)
			{
				if (item->GetVnum() == Arrador::ITEM_VNUM1)
					return true;
				else
					return false;
			}
			else if (Arrador::CArrador::instance().GetDungeonStep(from) == 5)
			{
				if (item->GetVnum() == Arrador::ITEM_VNUM2)
					return true;
				else
					return false;
			}
			break;
		case Nemere::STONE_LEVEL5_VNUM:
			if (Nemere::CNemere::instance().GetDungeonStep(from) == 5)
			{
				if (item->GetVnum() == Nemere::LEVEL5_REALKEY_VNUM)
					return true;
				else
					return false;
			}
#endif

#Search

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;
			
#Add after


#ifdef ENABLE_GENERAL_DUNGEON
		case Arrador::STONE_LEVEL2_5:
			{
				if(from->IsPC())
				{
					if (Arrador::CArrador::instance().IsArradorMap(from->GetMapIndex()))
						Arrador::CArrador::instance().OnKillPilar(item, from, this);
				}
			}
			break;

		case Nemere::STONE_LEVEL5_VNUM:
			{
				if(from->IsPC())
				{
					if (Nemere::CNemere::instance().IsNemereMap(from->GetMapIndex()))
						Nemere::CNemere::instance().OnKillPilar(item, from, this);
				}
			}
			break;

#endif

			