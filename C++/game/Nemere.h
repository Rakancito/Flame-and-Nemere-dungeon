#include "../../common/service.h"


#include "../../common/length.h"
#include "../../common/item_length.h"
#include "../../common/tables.h"
#include "guild.h"
#include "char_manager.h"
#include "sectree_manager.h"


namespace Nemere
{

	enum eNemereConfig
	{
		MAP_INDEX = 220,
		SUBMAP_INDEX = 61,

		DROP_PERCENT_ITEM_VNUMS = 50,

		BOSS_VNUM = 6191,
		TIME_LIMIT_DUNGEON = 600,

		NPC_LEVEL1 = 20397,
		LEVEL2_REALKEY_VNUM = 30331,

		STONE_LEVEL5_VNUM = 20398,
		LEVEL5_REALKEY_VNUM = 30332,

		LEVEL6_STONE_VNUM = 8058,

		LEVEL7_BOSSMOB_VNUM = 6151,

		LEVEL8_REALKEY_VNUM = 30333,

		LEVEL9_STONE_VNUM = 20399,

		RANGE_MIN_VNUM_MONSTERS = 6100,
		RANGE_MAX_VNUM_MONSTERS = 6199,
	};

	class CNemereMap
	{
		public:
			CNemereMap(long lMapIndex, bool bHard);
			~CNemereMap();

			void Destroy();

			void OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar);
			void OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller);
			void EndDungeonWarp();
			void Start();

			LPCHARACTER Spawn(DWORD dwVnum, int iX, int iY, int iDir, bool bSpawnMotion = false);

			/*
				Get and Set
			*/

			void	SetItemUse(LPITEM pkItem);

			void	SetDungeonStep(BYTE bStep);
			BYTE	GetDungeonStep()	{return dungeon_step;};

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

			void	SetNPCLevel1(LPCHARACTER pkNPC) {pkNPCLevel1 = pkNPC;};
			LPCHARACTER	GetNPCLevel1()	{return pkNPCLevel1;};

			void	SetLevel5Stone1(LPCHARACTER pkStone) {pkLevel5Stone1 = pkStone;};
			LPCHARACTER	GetLevel5Stone1() {return pkLevel5Stone1;}

			void	SetLevel5Stone2(LPCHARACTER pkStone) {pkLevel5Stone2 = pkStone;};
			LPCHARACTER	GetLevel5Stone2() {return pkLevel5Stone2;}

			void	SetLevel5Stone3(LPCHARACTER pkStone) {pkLevel5Stone3 = pkStone;};
			LPCHARACTER	GetLevel5Stone3() {return pkLevel5Stone3;}

			void	SetLevel5Stone4(LPCHARACTER pkStone) {pkLevel5Stone4 = pkStone;};
			LPCHARACTER	GetLevel5Stone4() {return pkLevel5Stone4;}

			void	SetLevel5Stone5(LPCHARACTER pkStone) {pkLevel5Stone5 = pkStone;};
			LPCHARACTER	GetLevel5Stone5() {return pkLevel5Stone5;}

			void	SetStoneLevel6(LPCHARACTER pkStone) {pkStoneLevel6 = pkStone;};
			LPCHARACTER	GetStoneLevel6() {return pkStoneLevel6;}


		protected:
			long map_index;
			int metin_dead;
			LPSECTREE_MAP	pkSectreeMap;
			LPPARTY pPartyReg;
			BYTE	dungeon_step;
			bool	dungeon_level;

			LPCHARACTER pkNPCLevel1, pkStoneLevel6;
			LPCHARACTER pkLevel5Stone1, pkLevel5Stone2, pkLevel5Stone3, pkLevel5Stone4, pkLevel5Stone5;

		protected:
			LPEVENT	e_SpawnEvent, e_LimitEvent;

	};

	class CNemere : public singleton<CNemere>
	{
		public:
			void Initialize();
			void Destroy();

			void Remove(long lMapIndex);
			bool Access(LPCHARACTER pChar, bool bHard);

			bool IsNemereMap(long lMapIndex);
			void OnKill(LPCHARACTER pkMonster, LPCHARACTER pKiller);

			void SetItemUse(LPCHARACTER pkChar, LPITEM pkItem);
			void SetDungeonStep(LPCHARACTER pkChar);
			int GetDungeonStep(LPCHARACTER pkChar);
			void OnKillPilar(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkPilar);

			//Set
			void	SetSubXYZ(long lX, long lY, long lZ)	{lSubMapPos.x = lX, lSubMapPos.y = lY, lSubMapPos.z = lZ;}
			void	SetXYZ(long lX, long lY, long lZ)	{lMapCenterPos.x = lX, lMapCenterPos.y = lY, lMapCenterPos.z = lZ;}
			
			//Get
			const PIXEL_POSITION &	GetSubXYZ() const	{return lSubMapPos;}
			const PIXEL_POSITION &	GetXYZ() const	{return lMapCenterPos;}

		private:
			std::map<long, CNemereMap*> m_dwRegGroups;
			PIXEL_POSITION lMapCenterPos, lSubMapPos;
	};
};
