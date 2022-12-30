#Search

#include "check_server.h"

#Add after

#ifdef ENABLE_GENERAL_DUNGEON
#include "Nemere.h"
#include "Arrador.h"
#endif

#Search
	quest::CQuestManager quest_manager;
	
#Add before or after

#ifdef ENABLE_GENERAL_DUNGEON
	Nemere::CNemere Nemere_manager;
	Arrador::CArrador Arrador_manager;
#endif

#Search
	// Client PackageCrypt

	//TODO : make it config
	const std::string strPackageCryptInfoDir = "package/";
	if( !desc_manager.LoadClientPackageCryptInfo( strPackageCryptInfoDir.c_str() ) )
	{
		sys_err("Failed to Load ClientPackageCryptInfo File(%s)", strPackageCryptInfoDir.c_str());
	}
	
#Add before

#ifdef ENABLE_GENERAL_DUNGEON
		Nemere_manager.Initialize();
		Arrador_manager.Initialize();
#endif

#Search

	}

	sys_log(0, "<shutdown> Flushing TrafficProfiler...");
	trafficProfiler.Flush();
	
#Add before


#ifdef ENABLE_GENERAL_DUNGEON
		sys_log(0, "<shutdown> Destroying Nemere_manager.");
		Nemere_manager.Destroy();
		sys_log(0, "<shutdown> Destroying Arrador_manager.");
		Arrador_manager.Destroy();
#endif	