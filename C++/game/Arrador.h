#include "../../common/service.h"


#include "../../common/length.h"
#include "../../common/item_length.h"
#include "../../common/tables.h"
#include "guild.h"
#include "char_manager.h"
#include "sectree_manager.h"


namespace Arrador
{

	enum eArradorConfig
	{
		DROP_PERCENT_ITEM_VNUMS = 10,

		STATUE_NPC = 20385,

		DOOR = 20387,
		DOOR2 = 20388,

		STONE_LEVEL2_5 = 20386,
		STONE_LEVEL6 = 8057,
		BOSS_LEVEL4 = 6051,

		ITEM_VNUM1 = 30329,
		ITEM_VNUM2 = 30330,

		MAP_INDEX = 214,
		SUBMAP_INDEX = 62,

		BOSS_VNUM = 6091,
		TIME_LIMIT_DUNGEON = 600,

		RANGE_MIN_VNUM_MONSTERS = 6000,
		RANGE_MAX_VNUM_MONSTERS = 6099,
	};

	extern int DungeonStepSort[7][7];

	class CArradorMap
	{
		public:
			CArradorMap(long lMapIndex, bool bHard);
			~CArradorMap();

			void Destroy();

			void OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar);
			void OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller);
			void EndDungeonWarp();
			void Start();

			LPCHARACTER Spawn(DWORD dwVnum, int iX, int iY, int iDir, bool bSpawnMotion = false);

			/*
				Get and Set
			*/

			void	SetDungeonSortStep(LPCHARACTER pkChar);

			void	SetDungeonStep(BYTE bStep);
			BYTE	GetDungeonStep()	{return dungeon_step;};

			void	SetDungeonSubStep(BYTE bSubStep) {dungeon_substep = bSubStep;};
			BYTE	GetDungeonSubStep()	{return dungeon_substep;};

			void	SetIsDungeonStep(bool bIsStep) {dungeon_Isbstep = bIsStep;};
			bool	GetIsDungeonStep()	{return dungeon_Isbstep;};

			void	SetDungeonLevel(bool bLevel)	{dungeon_level = bLevel;};
			bool	GetDungeonLevel()	{return dungeon_level;};

			void	SetParty(LPPARTY pParty)	{pPartyReg = pParty;};
			LPPARTY & GetParty()			{return pPartyReg;};

			void 	SetMapSectree(LPSECTREE_MAP pkSectree) {pkSectreeMap = pkSectree;};
			LPSECTREE_MAP & GetMapSectree()		{return pkSectreeMap;};

			void	SetMapIndex(long lMapIndex)	{map_index=lMapIndex;};
			long	GetMapIndex()			{return map_index;};

			void	SetKillMetin(int metin)	{metin_dead=metin;};
			int	GetKillMetin()			{return metin_dead;};

			void	SetStatueNPC(LPCHARACTER pkStatueNPC)	{pkStatueNPC=pkStatueNPC;};
			LPCHARACTER	GetStatueNPC()			{return pkStatueNPC;};

			void	SetStoneLevel2(LPCHARACTER pkStatueNPC)	{pkStoneLevel2=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel2()			{return pkStoneLevel2;};

			void	SetBossLevel4(LPCHARACTER pkBoss)	{pkBossLevel4=pkBoss;};
			LPCHARACTER	GetBossLevel4()			{return pkBossLevel4;};

			void	SetSortDungeon(int iSort)	{iSortDungeon=iSort;};
			int	GetSortDungeon()			{return iSortDungeon;};

			void	SetStoneLevel5_1(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_1=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_1()			{return pkStoneLevel5_1;};

			void	SetStoneLevel5_2(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_2=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_2()			{return pkStoneLevel5_2;};

			void	SetStoneLevel5_3(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_3=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_3()			{return pkStoneLevel5_3;};

			void	SetStoneLevel5_4(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_4=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_4()			{return pkStoneLevel5_4;};

			void	SetStoneLevel5_5(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_5=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_5()			{return pkStoneLevel5_5;};

			void	SetStoneLevel5_6(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_6=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_6()			{return pkStoneLevel5_6;};

			void	SetStoneLevel5_7(LPCHARACTER pkStatueNPC)	{pkStoneLevel5_7=pkStatueNPC;};
			LPCHARACTER	GetStoneLevel5_7()			{return pkStoneLevel5_7;};

			void	SetDoor1(LPCHARACTER pkDoor)	{pkDoor1=pkDoor;};
			LPCHARACTER	GetDoor1()			{return pkDoor1;};

			void	SetDoor2(LPCHARACTER pkDoor)	{pkDoor2=pkDoor;};
			LPCHARACTER	GetDoor2()			{return pkDoor2;};

			void	SetDoor3(LPCHARACTER pkDoor)	{pkDoor3=pkDoor;};
			LPCHARACTER	GetDoor3()			{return pkDoor3;};

			void	SetDoor4(LPCHARACTER pkDoor)	{pkDoor4=pkDoor;};
			LPCHARACTER	GetDoor4()			{return pkDoor4;};

			void	SetDoor5(LPCHARACTER pkDoor)	{pkDoor5=pkDoor;};
			LPCHARACTER	GetDoor5()			{return pkDoor5;};

			void	SetDoor6(LPCHARACTER pkDoor)	{pkDoor6=pkDoor;};
			LPCHARACTER	GetDoor6()			{return pkDoor6;};

			void	SetSubDoor1(LPCHARACTER pkDoor)	{pkSubDoor1=pkDoor;};
			LPCHARACTER	GetSubDoor1()			{return pkSubDoor1;};

			void	SetSubDoor2(LPCHARACTER pkDoor)	{pkSubDoor2=pkDoor;};
			LPCHARACTER	GetSubDoor2()			{return pkSubDoor2;};

			void	SetSubDoor3(LPCHARACTER pkDoor)	{pkSubDoor3=pkDoor;};
			LPCHARACTER	GetSubDoor3()			{return pkSubDoor3;};

			void	SetSubDoor4(LPCHARACTER pkDoor)	{pkSubDoor4=pkDoor;};
			LPCHARACTER	GetSubDoor4()			{return pkSubDoor4;};

			void	SetSubDoor5(LPCHARACTER pkDoor)	{pkSubDoor5=pkDoor;};
			LPCHARACTER	GetSubDoor5()			{return pkSubDoor5;};

			void	SetSubDoor6(LPCHARACTER pkDoor)	{pkSubDoor6=pkDoor;};
			LPCHARACTER	GetSubDoor6()			{return pkSubDoor6;};

		private:
			long map_index;
			int metin_dead, iSortDungeon;
			LPSECTREE_MAP	pkSectreeMap;
			LPPARTY pPartyReg;
			BYTE	dungeon_step, dungeon_substep;
			bool	dungeon_level, dungeon_Isbstep;

			LPCHARACTER pkStatueNPC, pkStoneLevel2, pkBossLevel4;
			LPCHARACTER pkStoneLevel5_1, pkStoneLevel5_2, pkStoneLevel5_3, pkStoneLevel5_4, pkStoneLevel5_5, pkStoneLevel5_6, pkStoneLevel5_7;

			LPCHARACTER pkDoor1, pkDoor2, pkDoor3, pkDoor4, pkDoor5, pkDoor6;
			LPCHARACTER pkSubDoor1, pkSubDoor2, pkSubDoor3, pkSubDoor4, pkSubDoor5, pkSubDoor6;

		protected:
			LPEVENT	e_SpawnEvent, e_LimitEvent;


	};

	class CArrador : public singleton<CArrador>
	{
		public:
			void Initialize();
			void Destroy();

			void Remove(long lMapIndex);
			bool Access(LPCHARACTER pChar, bool bHard);

			bool IsArradorMap(long lMapIndex);
			void OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar);
			void OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller);

			void SetDungeonStep(LPCHARACTER pkChar);
			int GetDungeonStep(LPCHARACTER pkChar);

			//Set
			void	SetSubXYZ(long lX, long lY, long lZ)	{lSubMapPos.x = lX, lSubMapPos.y = lY, lSubMapPos.z = lZ;}
			void	SetXYZ(long lX, long lY, long lZ)	{lMapCenterPos.x = lX, lMapCenterPos.y = lY, lMapCenterPos.z = lZ;}
			
			//Get
			const PIXEL_POSITION &	GetSubXYZ() const	{return lSubMapPos;}
			const PIXEL_POSITION &	GetXYZ() const	{return lMapCenterPos;}

		private:
			std::map<long, CArradorMap*> m_dwRegGroups;
			PIXEL_POSITION lMapCenterPos, lSubMapPos;
	};
};
