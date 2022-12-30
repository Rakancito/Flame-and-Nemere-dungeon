/*
	Author: R4kan
	Date: 24-02-2022
	Version: 0.1
	Github: https://github.com/Rakancito/
*/

#include "stdafx.h"
#include "Arrador.h"

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

namespace Arrador
{
	int DungeonStepSort[7][7] =
	{
		{1, 2, 3, 4, 5, 6, 7}, 
		{2, 3, 4, 5, 6, 1, 7}, 
		{3, 4, 5, 6, 1, 2, 7}, 
		{4, 5, 6, 1, 2, 3, 7},
		{5, 6, 1, 2, 3, 4, 7},
		{6, 1, 2, 3, 4, 5, 7} 
	};

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
					PIXEL_POSITION posSub = CArrador::instance().GetSubXYZ();
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
					PIXEL_POSITION pos = CArrador::instance().GetXYZ();
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

	EVENTINFO(r_arradorspawn_info)
	{
		CArradorMap*	pMap;
		BYTE	bStep;
		bool	bIsSubStep;
	};
	
	EVENTFUNC(r_arradorspawn_event)
	{
		r_arradorspawn_info* pEventInfo = dynamic_cast<r_arradorspawn_info*>(event->info);

		if (!pEventInfo)
			return 0;

		CArradorMap* pMap = pEventInfo->pMap;
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
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Kill all the mobs."));
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			pMap->SetIsDungeonStep(true);
			regen_load_in_file("data/dungeon/flame_dungeon/fd_a.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);

			pMap->GetDoor1()->Dead();
			pMap->SetDoor1(NULL);

			pMap->GetSubDoor1()->Dead();
			pMap->SetSubDoor1(NULL);

			return 0;
		}

		else if (pEventInfo->bStep == 2)
		{
			if (pEventInfo->bIsSubStep == false)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Kill mobs in the designated area and collect Golden Cog Wheel."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

				regen_load_in_file("data/dungeon/flame_dungeon/fd_b.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				pMap->SetIsDungeonStep(true);
				pMap->SetStoneLevel2(pMap->Spawn((DWORD)STONE_LEVEL2_5, 195, 352, 0));

				pMap->GetDoor2()->Dead();
				pMap->SetDoor2(NULL);

				pMap->GetSubDoor2()->Dead();
				pMap->SetSubDoor2(NULL);

				pEventInfo->bIsSubStep = true;
			}

			if (pEventInfo->bIsSubStep == true)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/flame_dungeon/fd_b.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

			return 0;

		}

		else if (pEventInfo->bStep == 3)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Kill all mobs in the designated area."));
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			regen_load_in_file("data/dungeon/flame_dungeon/fd_c.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
			pMap->SetIsDungeonStep(true);

			pMap->GetDoor3()->Dead();
			pMap->SetDoor3(NULL);

			pMap->GetSubDoor3()->Dead();
			pMap->SetSubDoor3(NULL);

			return 0;

		}

		else if (pEventInfo->bStep == 4)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: In the designated area, among other mobs kill Ignitor."));
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			pMap->SetBossLevel4(pMap->Spawn((DWORD)BOSS_LEVEL4, 470, 175, 0));
			pMap->SetIsDungeonStep(true);

			pMap->GetDoor4()->Dead();
			pMap->SetDoor4(NULL);

			pMap->GetSubDoor4()->Dead();
			pMap->SetSubDoor4(NULL);

			return 0;

		}

		else if (pEventInfo->bStep == 5)
		{
			if (pEventInfo->bIsSubStep == false)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: By killing mobs in the designated area, your group will collect Maat Stones."));
				SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

				regen_load_in_file("data/dungeon/flame_dungeon/fd_e.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				pMap->SetIsDungeonStep(true);

				pMap->SetStoneLevel5_1(pMap->Spawn((DWORD)STONE_LEVEL2_5, 486, 345, 0));
				pMap->SetStoneLevel5_2(pMap->Spawn((DWORD)STONE_LEVEL2_5, 511, 336, 0));
				pMap->SetStoneLevel5_3(pMap->Spawn((DWORD)STONE_LEVEL2_5, 525, 349, 0));
				pMap->SetStoneLevel5_4(pMap->Spawn((DWORD)STONE_LEVEL2_5, 521, 365, 0));
				pMap->SetStoneLevel5_5(pMap->Spawn((DWORD)STONE_LEVEL2_5, 503, 372, 0));
				pMap->SetStoneLevel5_6(pMap->Spawn((DWORD)STONE_LEVEL2_5, 486, 365, 0));
				pMap->SetStoneLevel5_7(pMap->Spawn((DWORD)STONE_LEVEL2_5, 500, 354, 0));

				pMap->GetDoor5()->Dead();
				pMap->SetDoor5(NULL);

				pMap->GetSubDoor5()->Dead();
				pMap->SetSubDoor5(NULL);

				pEventInfo->bIsSubStep = true;
			}

			if (pEventInfo->bIsSubStep == true)
			{
				struct FCountMonsters f;
				pMap->GetMapSectree()->for_each(f);

				if (f.m_cnt_Monsters < 1)
					regen_load_in_file("data/dungeon/flame_dungeon/fd_e.txt", pMap->GetMapIndex(), pMap->GetMapSectree()->m_setting.iBaseX, pMap->GetMapSectree()->m_setting.iBaseY);
				return PASSES_PER_SEC(5);
			}

			return 0;
		}

		else if (pEventInfo->bStep == 6)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Destroy the Purgatory Fire Metin in the designated area."));
			SendNoticeMap(szNotice, pMap->GetMapIndex(), true);

			pMap->Spawn((DWORD)STONE_LEVEL6, 511, 480, 0);
			pMap->SetIsDungeonStep(true);

			pMap->GetDoor6()->Dead();
			pMap->SetDoor6(NULL);

			pMap->GetSubDoor6()->Dead();
			pMap->SetSubDoor6(NULL);

			return 0;
		}

		else if (pEventInfo->bStep == 7)
		{

			FWarpMap f((685*100), (710*100), pMap->GetMapIndex());
			pMap->GetMapSectree()->for_each(f);

			pMap->Spawn((DWORD)BOSS_VNUM, 686, 637, 0);

			pMap->SetIsDungeonStep(true);

			return 0;
		}

		else if (pEventInfo->bStep == 8)
		{
			FPurgeSectreeAll f;
			pMap->GetMapSectree()->for_each(f);

			pMap->EndDungeonWarp();
			return 0;
		}

		return 0;
	}

	EVENTINFO(r_arradorlimit_info)
	{
		CArradorMap*	pMap;
	};
	
	EVENTFUNC(r_arradorlimit_event)
	{
		r_arradorlimit_info* pEventInfo = dynamic_cast<r_arradorlimit_info*>(event->info);
		if (pEventInfo)
		{
			CArradorMap* pMap = pEventInfo->pMap;
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
	CArradorMap::CArradorMap(long lMapIndex, bool bHard)
	{
		SetDungeonStep(0);
		SetMapIndex(lMapIndex);

		SetMapSectree(SECTREE_MANAGER::instance().GetMap(map_index));

		SetIsDungeonStep(false);
		SetDungeonSubStep(0);
		SetDungeonLevel(bHard);

		SetStoneLevel2(NULL);
		SetStoneLevel5_1(NULL);
		SetStoneLevel5_2(NULL);
		SetStoneLevel5_3(NULL);
		SetStoneLevel5_4(NULL);
		SetStoneLevel5_5(NULL);
		SetStoneLevel5_6(NULL);
		SetStoneLevel5_7(NULL);

		Start();
	}

	CArradorMap::~CArradorMap()
	{
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (e_LimitEvent != NULL)
			event_cancel(&e_LimitEvent);
		e_LimitEvent = NULL;
	}

	void CArradorMap::Destroy()
	{
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (e_LimitEvent != NULL)
			event_cancel(&e_LimitEvent);
		e_LimitEvent = NULL;

		SetDungeonStep(0);
		SetDungeonSubStep(0);
		SetIsDungeonStep(false);
		SetMapIndex(NULL);
		SetMapSectree(NULL);

		SetStoneLevel2(NULL);
		SetStoneLevel5_1(NULL);
		SetStoneLevel5_2(NULL);
		SetStoneLevel5_3(NULL);
		SetStoneLevel5_4(NULL);
		SetStoneLevel5_5(NULL);
		SetStoneLevel5_6(NULL);
		SetStoneLevel5_7(NULL);

		SetParty(NULL);

	}

	void CArradorMap::OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar)
	{
		if ((!pkItem) || (!pkChar) || (!pkPilar))
			return;

		if (!GetMapSectree())
			return;

		ITEM_MANAGER::instance().RemoveItem(pkItem);

		if (GetStoneLevel2())
		{
			if (GetStoneLevel2()->GetVID() == pkPilar->GetVID())
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				GetStoneLevel2()->Dead();
				SetStoneLevel2(NULL);

				FPurgeSectree f;
				GetMapSectree()->for_each(f);
				SetIsDungeonStep(false);

				if (e_SpawnEvent != NULL)
					event_cancel(&e_SpawnEvent);
				e_SpawnEvent = NULL;

				return;
			}
		}
		
		if(GetStoneLevel5_1())
		{
			if(GetStoneLevel5_1()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_1()->Dead();
				SetStoneLevel5_1(NULL);
			}
		}
		if(GetStoneLevel5_2())
		{
			if(GetStoneLevel5_2()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_2()->Dead();
				SetStoneLevel5_2(NULL);
			}
		}
		if(GetStoneLevel5_3())
		{
			if(GetStoneLevel5_3()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_3()->Dead();
				SetStoneLevel5_3(NULL);
			}
		}
		if(GetStoneLevel5_4())
		{
			if(GetStoneLevel5_4()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_4()->Dead();
				SetStoneLevel5_4(NULL);
			}
		}
		if(GetStoneLevel5_5())
		{
			if(GetStoneLevel5_5()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_5()->Dead();
				SetStoneLevel5_5(NULL);	
			}
		}	
		if(GetStoneLevel5_6())
		{
			if(GetStoneLevel5_6()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_6()->Dead();
				SetStoneLevel5_6(NULL);
			}
		}
		if(GetStoneLevel5_7())
		{
			if(GetStoneLevel5_7()->GetVID() == pkPilar->GetVID())
			{
				GetStoneLevel5_7()->Dead();
				SetStoneLevel5_7(NULL);
			}
		}
			
		if (!GetStoneLevel5_1() && !GetStoneLevel5_2() && !GetStoneLevel5_3() && !GetStoneLevel5_4() &&
			!GetStoneLevel5_5() && !GetStoneLevel5_6() && !GetStoneLevel5_7()
		)		
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			FPurgeSectree f;
			GetMapSectree()->for_each(f);
			SetIsDungeonStep(false);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;

		}
	}

	void CArradorMap::OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller)
	{		
		BYTE bStep = dungeon_step;

		if (!bStep)
			return;
		
		if (!GetMapSectree())
			return;

		if (!GetIsDungeonStep())
			return;

		if (bStep == 1)
		{
			SetKillMetin(GetKillMetin()+1);
			if (GetKillMetin() == 20)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f;
				GetMapSectree()->for_each(f);
				SetIsDungeonStep(false);
				SetKillMetin(0);	
			}
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
					LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(ITEM_VNUM1);
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
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f;
				GetMapSectree()->for_each(f);
				SetIsDungeonStep(false);
				SetKillMetin(0);	
			}
		}
		else if (bStep == 4)
		{
			if (pkMonster->GetMobTable().dwVnum == (DWORD)BOSS_LEVEL4)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f;
				GetMapSectree()->for_each(f);
				SetIsDungeonStep(false);
				SetBossLevel4(NULL);	
			}
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
					LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(ITEM_VNUM2);
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
			if (pkMonster->GetMobTable().dwVnum == (DWORD)STONE_LEVEL6)
			{
				char szNotice[512];
				snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Return to the center for a new task."));
				SendNoticeMap(szNotice, GetMapIndex(), true);

				FPurgeSectree f;
				GetMapSectree()->for_each(f);
				SetIsDungeonStep(false);	
			}
		}

		if (((bStep == 7) && (pkMonster->GetMobTable().dwVnum == BOSS_VNUM)))
		{
			FPurgeSectree f;
			GetMapSectree()->for_each(f);

			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("All partecipants will be teleport in 60 seconds."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;

			r_arradorspawn_info* pEventInfo = AllocEventInfo<r_arradorspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = 8;
			e_SpawnEvent = event_create(r_arradorspawn_event, pEventInfo, PASSES_PER_SEC(60));

		}
	}

	void CArradorMap::EndDungeonWarp()
	{
		if (GetMapSectree())
		{
			FExitAndGoTo f;
			GetMapSectree()->for_each(f);
		}

		long lMapIndex = GetMapIndex();
		SECTREE_MANAGER::instance().DestroyPrivateMap(GetMapIndex());
		Destroy();
		CArrador::instance().Remove(lMapIndex);
		M2_DELETE(this);

	}

	void CArradorMap::SetDungeonSortStep(LPCHARACTER pkChar)
	{
		if (!pkChar)
			return;

		if (GetIsDungeonStep())
		{
			pkChar->ChatPacket(CHAT_TYPE_INFO, "Razador: You are during a step in this moment.");
			return;
		}

		if (GetDungeonStep() >= 7)
		{
			PIXEL_POSITION pos = CArrador::instance().GetXYZ();
			pkChar->WarpSet(pos.x+685*100, pos.y+710*100, GetMapIndex());
			return;
		}

		SetDungeonStep(DungeonStepSort[GetSortDungeon()][GetDungeonSubStep()]);
		SetDungeonSubStep(GetDungeonSubStep()+1);			
	}

	void CArradorMap::SetDungeonStep(BYTE bStep)
	{		
		dungeon_step = bStep;

		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		e_SpawnEvent = NULL;

		if (bStep == 0)
		{
			r_arradorspawn_info* pEventInfo = AllocEventInfo<r_arradorspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bIsSubStep = false;
			e_SpawnEvent = event_create(r_arradorspawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;
		}
		else if (bStep == 7)
		{
			char szNotice[512];
			snprintf(szNotice, sizeof(szNotice), LC_TEXT("Razador: Awaken the Am-heh Gorge."));
			SendNoticeMap(szNotice, GetMapIndex(), true);

			SetIsDungeonStep(true);

			r_arradorspawn_info* pEventInfo = AllocEventInfo<r_arradorspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = bStep;
			pEventInfo->bIsSubStep = false;
			e_SpawnEvent = event_create(r_arradorspawn_event, pEventInfo, PASSES_PER_SEC(5));

			return;
		}

		r_arradorspawn_info* pEventInfo = AllocEventInfo<r_arradorspawn_info>();
		pEventInfo->pMap = this;
		pEventInfo->bStep = bStep;
		pEventInfo->bIsSubStep = false;
		e_SpawnEvent = event_create(r_arradorspawn_event, pEventInfo, PASSES_PER_SEC(0));
	}

	void CArradorMap::Start()
	{
		if (!GetMapSectree())
			EndDungeonWarp();
		else
		{
			if (e_LimitEvent != NULL)
				event_cancel(&e_LimitEvent);
			e_LimitEvent = NULL;

			r_arradorlimit_info* pEventInfo = AllocEventInfo<r_arradorlimit_info>();
			pEventInfo->pMap = this;
			e_LimitEvent = event_create(r_arradorlimit_event, pEventInfo, PASSES_PER_SEC(600));

			if (e_SpawnEvent != NULL)
				event_cancel(&e_SpawnEvent);
			e_SpawnEvent = NULL;

			r_arradorspawn_info* pEventInfo2 = AllocEventInfo<r_arradorspawn_info>();
			pEventInfo2->pMap = this;
			pEventInfo2->bStep = 0;
			e_SpawnEvent = event_create(r_arradorspawn_event, pEventInfo2, PASSES_PER_SEC(5));
		
			SetKillMetin(0);
			SetDungeonStep(0);

			int iSort = number(0,5);
			SetSortDungeon(iSort);

			SetStatueNPC(Spawn((DWORD)(STATUE_NPC), 350, 360, 0));

			SetDoor1(Spawn((DWORD)(DOOR), 320, 394, 135));
			SetDoor2(Spawn((DWORD)(DOOR), 293, 359, 90));
			SetDoor3(Spawn((DWORD)(DOOR), 333, 321, 210));
			SetDoor4(Spawn((DWORD)(DOOR), 378, 320, 152));
			SetDoor5(Spawn((DWORD)(DOOR), 400, 355, 90));
			SetDoor6(Spawn((DWORD)(DOOR), 394, 401, 223));

			SetSubDoor1(Spawn((DWORD)(DOOR2), 268, 447, 135));
			SetSubDoor2(Spawn((DWORD)(DOOR2), 234, 359, 90));
			SetSubDoor3(Spawn((DWORD)(DOOR2), 300, 264, 210));
			SetSubDoor4(Spawn((DWORD)(DOOR2), 454, 217, 152));
			SetSubDoor5(Spawn((DWORD)(DOOR2), 470, 355, 90));
			SetSubDoor6(Spawn((DWORD)(DOOR2), 467, 469, 239));

		}
	}

	LPCHARACTER CArradorMap::Spawn(DWORD dwVnum, int iX, int iY, int iDir, bool bSpawnMotion)
	{
		if (dwVnum == 0)
			return NULL;

		if (!GetMapSectree())
			return NULL;
	
		LPCHARACTER pkMob = CHARACTER_MANAGER::instance().SpawnMob(dwVnum, GetMapIndex(), GetMapSectree()->m_setting.iBaseX + iX * 100, GetMapSectree()->m_setting.iBaseY + iY * 100, 0, bSpawnMotion, iDir == 0 ? -1 : (iDir - 1) * 45);
		if (pkMob)
			sys_log(0, "<Razador> Spawn: %s (map index: %d). x: %d y: %d", pkMob->GetName(), GetMapIndex(), (GetMapSectree()->m_setting.iBaseX + iX * 100), (GetMapSectree()->m_setting.iBaseY + iY * 100));
		
		return pkMob;
	}

	/*
		Global Part
	*/

	void CArrador::Initialize()
	{
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
		m_dwRegGroups.clear();
	}

	void CArrador::Destroy()
	{
		itertype(m_dwRegGroups) iter = m_dwRegGroups.begin();
		for (; iter != m_dwRegGroups.end(); ++iter)
		{
			CArradorMap* pMap = iter->second;
			SECTREE_MANAGER::instance().DestroyPrivateMap(pMap->GetMapIndex());
			pMap->Destroy();
			M2_DELETE(pMap);
		}
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
		m_dwRegGroups.clear();
	}


	void CArrador::Remove(long lMapIndex)
	{
		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex);

		if (iter != m_dwRegGroups.end())
		{
			m_dwRegGroups.erase(iter);
		}

		return;
	}

	bool CArrador::Access(LPCHARACTER pChar, bool bHard)
	{
		if (!pChar)
			return false;

		long lNormalMapIndex = (long)MAP_INDEX;
		PIXEL_POSITION pos = GetXYZ(), posSub = GetSubXYZ();

		if (!pos.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(Arrador::MAP_INDEX));
			if (pkSectreeMap)
				SetXYZ(pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY, 0);
		}
			
		if (!posSub.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(Arrador::SUBMAP_INDEX));
			if (pkSectreeMap)
				SetSubXYZ(pkSectreeMap->m_setting.iBaseX + 91*100, pkSectreeMap->m_setting.iBaseY + 916*100, 0);
		}

		long lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(lNormalMapIndex);

		if (!lMapIndex)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, "An error ocurred during map creation.");
			return false;
		}

		CArradorMap *pMap;

		if (bHard)
			pMap = M2_NEW CArradorMap(lMapIndex, true);
		else
			pMap = M2_NEW CArradorMap(lMapIndex, false);

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
						pkChr->WarpSet(mPos.x + 342*100, mPos.y + 584*100, pMap->GetMapIndex());
					}
				}
			}
			else
			{
				if(pChar)
				{
					pMap->SetParty(NULL);
					pChar->SaveExitLocation();
					pChar->WarpSet(mPos.x + 342*100, mPos.y + 584*100, pMap->GetMapIndex());
				}
			}
			return true;
		}
	}

	bool CArrador::IsArradorMap(long lMapIndex)
	{
		long lMinIndex = (long)(MAP_INDEX) * 10000, lMaxIndex = (long)(MAP_INDEX) * 10000 + 10000;
		if (((lMapIndex >= lMinIndex) && (lMapIndex <= lMaxIndex)) || (lMapIndex == (long)(MAP_INDEX)))
			return true;
		
		return false;
	}


	void CArrador::OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar)
	{
		if ((!pkItem) || (!pkChar) || (!pkPilar))
			return;
				
		if (pkItem->GetOriginalVnum() != (DWORD)(ITEM_VNUM1) && pkItem->GetOriginalVnum() != (DWORD)(ITEM_VNUM2))
			return;

		long lMapIndex = pkChar->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
		
		CArradorMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		
		if (!pMap)
			return;
				
		pMap->OnKillPilar(pkItem, pkChar, pkPilar);

		return;	
	}

	void CArrador::OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller)
	{
		if ((!pkMonster) || (!pKiller))
			return;

		long lMapIndex = pKiller->GetMapIndex();
		
		if (lMapIndex < 1000)
			return;
	
		CArradorMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (!pMap)
			return;
		
		pMap->OnKill(pkMonster, pKiller);

		return;
	}

	void CArrador::SetDungeonStep(LPCHARACTER pkChar)
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
		
		CArradorMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
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
			pMap->SetDungeonSortStep(pkChar);
		}
	}

	int CArrador::GetDungeonStep(LPCHARACTER pkChar)
	{
		if (!pkChar)
			return 0;

		long lMapIndex = pkChar->GetMapIndex();
		if (lMapIndex < 1000)
			return 0;

		itertype(m_dwRegGroups) iter = m_dwRegGroups.find(lMapIndex), iterEnd = m_dwRegGroups.end();
		if (iter == iterEnd)
			return 0;
		
		CArradorMap* pMap = m_dwRegGroups.find(lMapIndex)->second;
		if (pMap)
		{
			return pMap->GetDungeonStep();
		}
	}
};
