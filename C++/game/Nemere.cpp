/*
	Author: R4kan
	Date: 24-02-2022
	Version: 0.1
	Github: https://github.com/Rakancito/
*/

#include "stdafx.h"
#include "Nemere.h"

#include "item.h"
#include "char.h"
#include "utils.h"
#include "party.h"
#include "regen.h"
#include "config.h"
#include "packet.h"
#include "motion.h"
#include "item_manager.h"
#include "guild_manager.h"
#include "guild.h"
#include "start_position.h"
#include "locale_service.h"
#include "char_manager.h"
#include <boost/lexical_cast.hpp> 

namespace Nemere
{
	//Estructura recolectora de miembros de grupo.
	struct FPartyCHCollector
	{
		std::vector <DWORD> pPlayerID;
		FPartyCHCollector()
		{
		}
		void operator () (LPCHARACTER ch)
		{
			pPlayerID.push_back(ch->GetPlayerID());
		}
	};

	struct FExitAndGoTo
	{
		FExitAndGoTo() {};
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pkChar = (LPCHARACTER) ent;
				if (pkChar && pkChar->IsPC())
				{
					PIXEL_POSITION posSub = CNemere::instance().GetSubXYZ();
					if (!posSub.x)
						pkChar->WarpSet(EMPIRE_START_X(pkChar->GetEmpire()), EMPIRE_START_Y(pkChar->GetEmpire()));
					else
						pkChar->WarpSet(posSub.x, posSub.y);
				}
			}
		}
	};

	struct FWarpMap
	{
		FWarpMap(int x, int y, long lMapIndex) : m_x(x), m_y(y), m_lMapIndex(lMapIndex) {};
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pkChar = (LPCHARACTER) ent;
				if (pkChar->IsPC())
				{
					PIXEL_POSITION pos = CNemere::instance().GetXYZ();
					if (!pos.x)
						return;
					else
						pkChar->WarpSet(pos.x+m_x, pos.y+m_y, m_lMapIndex);
				}
			}
		}

		int m_x, m_y;
		long m_lMapIndex;
	};

	struct FCountMonsters
	{
		size_t m_cnt_Monsters;

		FCountMonsters() : m_cnt_Monsters(0) {}

		void operator() (LPENTITY ent)
		{
			if ( ent->IsType(ENTITY_CHARACTER) == true )
			{
				LPCHARACTER lpChar = (LPCHARACTER)ent;

				if ( lpChar->IsMonster() == true && (lpChar->GetMobTable().dwVnum >= RANGE_MIN_VNUM_MONSTERS && lpChar->GetMobTable().dwVnum <= RANGE_MAX_VNUM_MONSTERS))
				{
					m_cnt_Monsters++;
				}
			}
		}
	};

	// DUNGEON_KILL_ALL_BUG_FIX
	struct FPurgeSectree
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
#ifdef NEW_PET_SYSTEM
				if (ch && !ch->IsPC() && !ch->IsPet() && !ch->IsNewPet())
#else
				if (ch && !ch->IsPC() && !ch->IsPet())
#endif
				{
					//if (ch->IsNPC())
					//	M2_DESTROY_CHARACTER(ch);
					//else
					if (ch->IsMonster())
						ch->Dead(); //fix Purge Area
				}
			}
		}
	};

	// DUNGEON_KILL_ALL_BUG_FIX
	struct FPurgeSectreeAll
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
#ifdef NEW_PET_SYSTEM
				if (ch && !ch->IsPC() && !ch->IsPet() && !ch->IsNewPet())
#else
				if (ch && !ch->IsPC() && !ch->IsPet())
#endif
				{
					if (ch->IsNPC())
						M2_DESTROY_CHARACTER(ch);
					else
						ch->Dead(); //fix Purge Area
				}
			}
		}
	};
	// END_OF_DUNGEON_KILL_ALL_BUG_FIX

	EVENTINFO(r_nemerespawn_info)
	{
		CNemereMap*	pMap;
		BYTE	bStep;
		BYTE	bSubStep;
	};
	
	EVENTFUNC(r_nemerespawn_event)
	{
		r_nemerespawn_info* pEventInfo = dynamic_cast<r_nemerespawn_info*>(event->info);

		if (!pEventInfo)
			return 0;

		CNemereMap* pMap = pEventInfo->pMap;
		if (!pMap)
			return 0;

		if (!pMap->GetMapSectree())
			return 0;

		if (pEventInfo->bStep == 0)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			return 0;

		}
		else if (pEventInfo->bStep == 1)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 1 msg."));
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			pMap->GetNPCLevel1()->Dead();
			pMap->SetNPCLevel1(NULL);

			regen_load_in_file("data/dungeon/ice_dungeon/zone_1.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
		
			return 0;	
		}

		else if (pEventInfo->bStep == 2)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((422*100), (261*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 2 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
			else if (pEventInfo->bSubStep == 2)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/ice_dungeon/zone_2.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);

				return PASSES_PER_SEC(5);
			}
		}

		else if (pEventInfo->bStep == 3)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((764*100), (264*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 3 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
			else if (pEventInfo->bSubStep == 2)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/ice_dungeon/zone_3.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

		}

		else if (pEventInfo->bStep == 4)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((175*100), (535*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 4 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
			else if (pEventInfo->bSubStep == 2)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/ice_dungeon/zone_4.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

		}

		else if (pEventInfo->bStep == 5)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((422*100), (538*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pMap->SetLevel5Stone1(pMap->Spawn((DWORD)STONE_LEVEL5_VNUM, 449, 488, 0));
				pMap->SetLevel5Stone2(pMap->Spawn((DWORD)STONE_LEVEL5_VNUM, 455, 445, 0));
				pMap->SetLevel5Stone3(pMap->Spawn((DWORD)STONE_LEVEL5_VNUM, 419, 422, 0));
				pMap->SetLevel5Stone4(pMap->Spawn((DWORD)STONE_LEVEL5_VNUM, 382, 444, 0));
				pMap->SetLevel5Stone5(pMap->Spawn((DWORD)STONE_LEVEL5_VNUM, 389, 488, 0));

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 5 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
			else if (pEventInfo->bSubStep == 2)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/ice_dungeon/zone_5.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

		}

		else if (pEventInfo->bStep == 6)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((748*100), (540*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pMap->SetStoneLevel6(pMap->Spawn((DWORD)LEVEL6_STONE_VNUM, 747, 494, 0));;

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 6 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return 0;

			}
		}

		else if (pEventInfo->bStep == 7)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((305*100), (708*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pMap->Spawn((DWORD)LEVEL7_BOSSMOB_VNUM, 302, 678, 0);
				pMap->Spawn((DWORD)LEVEL7_BOSSMOB_VNUM, 281, 657, 0);
				pMap->Spawn((DWORD)LEVEL7_BOSSMOB_VNUM, 303, 635, 0);
				pMap->Spawn((DWORD)LEVEL7_BOSSMOB_VNUM, 324, 656, 0);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 5 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
		}

		else if (pEventInfo->bStep == 8)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((571*100), (701*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 8 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
			else if (pEventInfo->bSubStep == 2)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/ice_dungeon/zone_8.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

		}

		else if (pEventInfo->bStep == 9)
		{
			if (pEventInfo->bSubStep == 0)
			{
				FWarpMap f((851*100), (693*100), pMap->GetMapIndex());
				pMap->GetMapSectree()->for_each(f);

				pMap->Spawn((DWORD)LEVEL9_STONE_VNUM, 849, 660, 0);

				pEventInfo->bSubStep = 1;
				return PASSES_PER_SEC(5);
			}
			else if (pEventInfo->bSubStep == 1)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 9 msg."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);
				pEventInfo->bSubStep = 2;

				return PASSES_PER_SEC(1);

			}
		}

		else if (pEventInfo->bStep == 10)
		{
			FWarpMap f((927*100), (395*100), pMap->GetMapIndex());
			pMap->GetMapSectree()->for_each(f);

			pMap->Spawn((DWORD)BOSS_VNUM, 927, 333, 0);

			return 0;
		}

		else if (pEventInfo->bStep == 11)
		{
			pMap->EndDungeonWarp();
			return 0;
		}

		return 0;
	}

	EVENTINFO(r_nemerelimit_info)
	{
		CNemereMap*	pMap;
	};
	
	EVENTFUNC(r_nemerelimit_event)
	{
		r_nemerelimit_info* pEventInfo = dynamic_cast<r_nemerelimit_info*>(event->info);
		if (pEventInfo)
		{
			CNemereMap* pMap = pEventInfo->pMap;
			if (pMap)
			{
				pMap->EndDungeonWarp();
			}
		}
		
		return 0;
	}

	/*
		Object Part
	*/
	CNemereMap::CNemereMap(long lMapIndex, bool bHard)
	{
		SetDungeonStep(0);
		SetMapIndex(lMapIndex);

		SetMapSectree(SECTREE_MANAGER::instance().GetMap(map_index));

		SetDungeonLevel(bHard);
		Start();
	}

	CNemereMap::~CNemereMap()
	{
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (e_LimitEvent != NULL)
			event_cancel(&e_LimitEvent);
		e_LimitEvent = NULL;
	}

	void CNemereMap::Destroy()
	{
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (e_LimitEvent != NULL)
			event_cancel(&e_LimitEvent);
		e_LimitEvent = NULL;

		SetDungeonStep(0);
		SetMapIndex(0); //Before SetMapIndex(NULL);
		SetMapSectree(NULL);
		SetParty(NULL);

	}

	void CNemereMap::SetItemUse(LPITEM pkItem)
	{
		if (!pkItem)
			return;

		if (pkItem->GetVnum() == LEVEL2_REALKEY_VNUM && GetDungeonStep() == 2)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 2_1 msg."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;
			
			SetDungeonStep(3);
		}
		else if (pkItem->GetVnum() == LEVEL8_REALKEY_VNUM && GetDungeonStep() == 8)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 2_1 msg."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;
			
			SetDungeonStep(9);
		}

	}

	void CNemereMap::OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar)
	{
		if ((!pkItem) || (!pkChar) || (!pkPilar))
			return;

		if (!GetMapSectree())
			return;

		ITEM_MANAGER::instance().RemoveItem(pkItem);
		
		if(GetLevel5Stone1())
		{
			if(GetLevel5Stone1()->GetVID() == pkPilar->GetVID())
			{
				GetLevel5Stone1()->Dead();
				SetLevel5Stone1(NULL);
			}
		}
		if(GetLevel5Stone2())
		{
			if(GetLevel5Stone2()->GetVID() == pkPilar->GetVID())
			{
				GetLevel5Stone2()->Dead();
				SetLevel5Stone2(NULL);
			}
		}
		if(GetLevel5Stone3())
		{
			if(GetLevel5Stone3()->GetVID() == pkPilar->GetVID())
			{
				GetLevel5Stone3()->Dead();
				SetLevel5Stone3(NULL);
			}
		}
		if(GetLevel5Stone4())
		{
			if(GetLevel5Stone4()->GetVID() == pkPilar->GetVID())
			{
				GetLevel5Stone4()->Dead();
				SetLevel5Stone4(NULL);
			}
		}
		if(GetLevel5Stone5())
		{
			if(GetLevel5Stone5()->GetVID() == pkPilar->GetVID())
			{
				GetLevel5Stone5()->Dead();
				SetLevel5Stone5(NULL);	
			}
		}	
			
		if (!GetLevel5Stone1() && !GetLevel5Stone2() && !GetLevel5Stone3() && !GetLevel5Stone4() &&
			!GetLevel5Stone5()
		)		
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Arrador Step 6 msg."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			FPurgeSectree f_purge;
			GetMapSectree()->for_each(f_purge);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;

			SetDungeonStep(6);

		}
	}

	void CNemereMap::OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller)
	{		
		BYTE bStep = dungeon_step;

		if (!bStep)
			return;
		
		if (!GetMapSectree())
			return;

		if (bStep == 1)
		{
			SetKillMetin(GetKillMetin()+1);
			if (GetKillMetin() == 20)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 6 msg."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f_purge;
				GetMapSectree()->for_each(f_purge);
				SetKillMetin(0);
				SetDungeonStep(2);	
			}
			return;
		}
		else if (bStep == 2)
		{
			if (pkMonster->IsMonster())
			{
				if(!pKiller)
					return;

				int iChance = number(1, 100);
				if (iChance < DROP_PERCENT_ITEM_VNUMS)
				{
					LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(LEVEL2_REALKEY_VNUM);
					if (!pkItem)
						return;
			
					PIXEL_POSITION mPos;
					mPos.x = pkMonster->GetX();
					mPos.y = pkMonster->GetY();
			
					pkItem->AddToGround(GetMapIndex(), mPos);
					pkItem->StartDestroyEvent();
					pkItem->SetOwnership(pKiller, 60);
				}
			}
		}
		else if (bStep == 3)
		{
			SetKillMetin(GetKillMetin()+1);
			if (GetKillMetin() == 20)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 6 msg."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f_purge;
				GetMapSectree()->for_each(f_purge);
				SetKillMetin(0);
				SetDungeonStep(4);	
			}
			return;
		}

		else if (bStep == 4)
		{
			SetKillMetin(GetKillMetin()+1);
			if (GetKillMetin() == 20)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 6 msg."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f_purge;
				GetMapSectree()->for_each(f_purge);
				SetKillMetin(0);
				SetDungeonStep(5);	
			}
			return;
		}

		else if (bStep == 5)
		{
			if (pkMonster->IsMonster())
			{
				if(!pKiller)
					return;

				int iChance = number(1, 100);
				if (iChance < DROP_PERCENT_ITEM_VNUMS)
				{
					LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(LEVEL5_REALKEY_VNUM);
					if (!pkItem)
						return;
			
					PIXEL_POSITION mPos;
					mPos.x = pkMonster->GetX();
					mPos.y = pkMonster->GetY();
			
					pkItem->AddToGround(GetMapIndex(), mPos);
					pkItem->StartDestroyEvent();
					pkItem->SetOwnership(pKiller, 60);
				}
			}
		}

		else if (bStep == 6)
		{
			if (pkMonster->GetMobTable().dwVnum == (DWORD)LEVEL6_STONE_VNUM)
			{
				SetDungeonStep(7);
				return;
			}
		}

		else if (bStep == 7)
		{
			if (pkMonster->GetMobTable().dwVnum == (DWORD)LEVEL7_BOSSMOB_VNUM)
			{
				SetKillMetin(GetKillMetin()+1);
				if (GetKillMetin() == 4)
				{
					char szNotice[512];
					snprintf(szNotice, sizeof(szNotice), LC_TEXT("Nemere Step 7 msg."));
					SendNoticeMap(szNotice, GetMapIndex(), true);

					FPurgeSectree f_purge;
					GetMapSectree()->for_each(f_purge);
					SetKillMetin(0);
					SetDungeonStep(8);	
				}
			}
			return;
		}
		else if (bStep == 8)
		{
			if (pkMonster->IsMonster())
			{
				if(!pKiller)
					return;

				int iChance = number(1, 100);
				if (iChance < DROP_PERCENT_ITEM_VNUMS)
				{
					LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(LEVEL8_REALKEY_VNUM);
					if (!pkItem)
						return;
			
					PIXEL_POSITION mPos;
					mPos.x = pkMonster->GetX();
					mPos.y = pkMonster->GetY();
			
					pkItem->AddToGround(GetMapIndex(), mPos);
					pkItem->StartDestroyEvent();
					pkItem->SetOwnership(pKiller, 60);
				}
			}
		}
		else if (bStep == 9)
		{
			if (pkMonster->GetMobTable().dwVnum == (DWORD)LEVEL9_STONE_VNUM)
			{
				SetDungeonStep(10);
				return;
			}
		}
		else if (((bStep == 10) && (pkMonster->GetMobTable().dwVnum == BOSS_VNUM)))
		{
			FPurgeSectree f_purge;
			GetMapSectree()->for_each(f_purge);

			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("All partecipants will be teleport in 60 seconds."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = 11;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(60));

		}
	}

	void CNemereMap::EndDungeonWarp()
	{
		if (GetMapSectree())
		{
			FExitAndGoTo f;
			GetMapSectree()->for_each(f);
		}

		long lMapIndex = GetMapIndex();
		SECTREE_MANAGER::instance().DestroyPrivateMap(GetMapIndex());
		Destroy();
		CNemere::instance().Remove(lMapIndex);
		M2_DELETE(this);

	}

	void CNemereMap::SetDungeonStep(BYTE bStep)
	{		
		dungeon_step = bStep;

		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (!GetMapSectree())
			return;

		if (bStep == 0)
		{
			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;
		}
		else if (bStep == 2)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;
		}
		else if (bStep == 3)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}
		else if (bStep == 4)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}
		else if (bStep == 5)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		else if (bStep == 6)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		else if (bStep == 7)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		else if (bStep == 8)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		else if (bStep == 9)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		else if (bStep == 10)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("The dungeon will be available for %d minutes."), DWORD(TIME_LIMIT_DUNGEON)/60);
			SendNoticeMap(szNotice, GetMapIndex(), true);

			r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bSubStep = 0;
			e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;

		}

		r_nemerespawn_info* pEventInfo = AllocEventInfo<r_nemerespawn_info>();
		pEventInfo->pMap = this;
		pEventInfo->bStep = bStep;
		e_SpawnEvent = event_create(r_nemerespawn_event, pEventInfo, PASSES_PER_SEC(0));

	}

	void CNemereMap::Start()
	{
		if (!GetMapSectree())
			EndDungeonWarp();

		else
		{		
			SetKillMetin(0);
			SetDungeonStep(0);

			SetNPCLevel1(Spawn((DWORD)NPC_LEVEL1, 172, 261, 1));

			if (e_LimitEvent != NULL)
				event_cancel(&e_LimitEvent);
			e_LimitEvent = NULL;

			r_nemerelimit_info* pEventInfo = AllocEventInfo<r_nemerelimit_info>();
			pEventInfo->pMap = this;
			e_LimitEvent = event_create(r_nemerelimit_event, pEventInfo, PASSES_PER_SEC(3600));

		}
	}

	LPCHARACTER CNemereMap::Spawn(DWORD dwVnum, int iX, int iY, int iDir, bool bSpawnMotion)
	{
		if (dwVnum == 0)
			return NULL;

		if (!GetMapSectree())
			return NULL;
	
		LPCHARACTER pkMob = CHARACTER_MANAGER::instance().SpawnMob(dwVnum, GetMapIndex(), GetMapSectree()->m_setting.iBaseX + iX * 100, GetMapSectree()->m_setting.iBaseY + iY * 100, 0, bSpawnMotion, iDir == 0 ? -1 : (iDir - 1) * 45);
		if (pkMob)
			sys_log(0, "<SnakeLair> Spawn: %s (map index: %d). x: %d y: %d", pkMob->GetName(), GetMapIndex(), (GetMapSectree()->m_setting.iBaseX + iX * 100), (GetMapSectree()->m_setting.iBaseY + iY * 100));
		
		return pkMob;
	}

	/*
		Global Part
	*/

	void CNemere::Initialize()
	{
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
		m_dwRegGroups.clear();
	}

	void CNemere::Destroy()
	{
		itertype(m_dwRegGroups) iter = m_dwRegGroups.begin();
		for (; iter != m_dwRegGroups.end(); ++iter)
		{
			CNemereMap* pMap = iter->second;
			SECTREE_MANAGER::instance().DestroyPrivateMap(pMap->GetMapIndex());
			pMap->Destroy();
			M2_DELETE(pMap);
		}
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
		m_dwRegGroups.clear();
	}


	void CNemere::Remove(long lMapIndex)
	{
		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex);

		if (iter != m_dwRegGroups.end())
		{
			m_dwRegGroups.erase(iter);
		}

		return;
	}

	bool CNemere::Access(LPCHARACTER pChar, bool bHard)
	{
		if (!pChar)
			return false;

		long lNormalMapIndex = (long)MAP_INDEX;
		PIXEL_POSITION pos = GetXYZ(), posSub = GetSubXYZ();

		if (!pos.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(Nemere::MAP_INDEX));
			if (pkSectreeMap)
				SetXYZ(pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY, 0);
		}
			
		if (!posSub.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(Nemere::SUBMAP_INDEX));
			if (pkSectreeMap)
				SetSubXYZ(pkSectreeMap->m_setting.iBaseX + 738*100, pkSectreeMap->m_setting.iBaseY + 119*100, 0);
		}

		long lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(lNormalMapIndex);

		if (!lMapIndex)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, "An error ocurred during map creation.");
			return false;
		}

		CNemereMap *pMap;

		if (bHard)
			pMap = M2_NEW CNemereMap(lMapIndex, true);
		else
			pMap = M2_NEW CNemereMap(lMapIndex, false);

		if (pMap)
		{
			m_dwRegGroups.insert(std::make_pair(lMapIndex, pMap));

			PIXEL_POSITION mPos;
			if (!SECTREE_MANAGER::instance().GetRecallPositionByEmpire((int)(MAP_INDEX), 0, mPos))
			{
				pChar->ChatPacket(CHAT_TYPE_INFO, "Sectree Error get recall position.");
				return true;
			}

			pMap->SetDungeonStep(0);

			LPPARTY pParty = pChar->GetParty();

			if (pParty)
			{
				pMap->SetParty(pParty);

				FPartyCHCollector f;
				pChar->GetParty()->ForEachOnMapMember(f, pChar->GetMapIndex());
				std::vector <DWORD>::iterator it;
				for (it = f.pPlayerID.begin(); it != f.pPlayerID.end(); it++)
				{
					LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(*it);
					if(pkChr)
					{
						pkChr->SaveExitLocation();
						pkChr->WarpSet(mPos.x + 171*100, mPos.y + 270*100, pMap->GetMapIndex());
					}
				}
			}
			else
			{
				if(pChar)
				{
					pMap->SetParty(NULL);
					pChar->SaveExitLocation();
					pChar->WarpSet(mPos.x + 171*100, mPos.y + 270*100, pMap->GetMapIndex());
				}
			}
			return true;
		}
	}

	bool CNemere::IsNemereMap(long lMapIndex)
	{
		long lMinIndex = (long)(MAP_INDEX) * 10000, lMaxIndex = (long)(MAP_INDEX) * 10000 + 10000;
		if (((lMapIndex >= lMinIndex) && (lMapIndex <= lMaxIndex)) || (lMapIndex == (long)(MAP_INDEX)))
			return true;
		
		return false;
	}

	void CNemere::OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller)
	{
		if ((!pkMonster) || (!pKiller))
			return;

		long lMapIndex = pKiller->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
	
		CNemereMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (!pMap)
			return;
		
		pMap->OnKill(pkMonster, pKiller);

		return;
	}

	void CNemere::SetItemUse(LPCHARACTER pkChar, LPITEM pkItem)
	{
		if (!pkChar)
			return;

		if (!pkItem)
			return;

		long lMapIndex = pkChar->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
	
		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex), iterEnd = m_dwRegGroups.end();
		if (iter == iterEnd)
			return;
		
		CNemereMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (pMap)
		{
			pMap->SetItemUse(pkItem);
		}
	}

	void CNemere::SetDungeonStep(LPCHARACTER pkChar)
	{
		if (!pkChar)
			return;
		
		LPPARTY pParty = pkChar->GetParty();
		
		if (pParty)
		{
			if(pParty->GetLeaderPID() != pkChar->GetPlayerID())
			{
				pkChar->ChatPacket(CHAT_TYPE_INFO, "Only the party leader can create a new Room.");
				return;
			}
		}

		long lMapIndex = pkChar->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
	
		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex), iterEnd = m_dwRegGroups.end();
		if (iter == iterEnd)
			return;
		
		CNemereMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (pMap)
		{
			if (pParty)
			{
				if (pMap->GetParty())
				{
					if (pMap->GetParty()->GetLeaderPID() != pkChar->GetPlayerID())
					{
						pkChar->ChatPacket(CHAT_TYPE_INFO, "Wow WoW Wow, you are in other Group Register.");
						return;
					}
				}
			}
			pMap->SetDungeonStep(pMap->GetDungeonStep()+1);
		}
	}

	int CNemere::GetDungeonStep(LPCHARACTER pkChar)
	{
		if (!pkChar)
			return 0;

		long lMapIndex = pkChar->GetMapIndex();
		if (lMapIndex < 1000)
			return 0;

		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex), iterEnd = m_dwRegGroups.end();
		if (iter == iterEnd)
			return 0;
		
		CNemereMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (pMap)
		{
			return pMap->GetDungeonStep();
		}
	}

	void CNemere::OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar)
	{
		if ((!pkItem) || (!pkChar) || (!pkPilar))
			return;
				
		if (pkItem->GetOriginalVnum() != (DWORD)(LEVEL5_REALKEY_VNUM))
			return;

		long lMapIndex = pkChar->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
		
		CNemereMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		
		if (!pMap)
			return;
				
		pMap->OnKillPilar(pkItem, pkChar, pkPilar);

		return;	
	}
};
