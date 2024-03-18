#include <stdio.h>

#ifdef WIN32
#include <WinSock2.h>
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif

#include "sql_mm.h"
#include "iserver.h"

#include "common/interface.h"

#include "mysql/mysql_database.h"
#include "mysql/mysql_client.h"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

SQLPlugin g_SQLPlugin;
IServerGameDLL *server = nullptr;
IVEngineServer *engine = nullptr;
ISQLInterface *g_sqlInterface = nullptr;

// MySQL
IMySQLClient* g_mysqlClient = nullptr;
std::vector<MySQLConnection*> g_vecMysqlConnections;

// SQLite
ISQLiteClient* g_sqliteClient = nullptr;

// Should only be called within the active game loop (i e map should be loaded and active)
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

PLUGIN_EXPOSE(SQLPlugin, g_SQLPlugin);
bool SQLPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	// Required to get the IMetamodListener events
	g_SMAPI->AddListener( this, this );

	META_CONPRINTF( "Starting plugin.\n" );

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &SQLPlugin::Hook_GameFrame, true);

	if (mysql_library_init(0, NULL, NULL))
	{
		snprintf(error, maxlen, "Failed to initialize mysql library\n");
		return false;
	}

	g_mysqlClient = new CMySQLClient();

	return true;
}

bool SQLPlugin::Unload(char *error, size_t maxlen)
{
	mysql_library_end();

	delete g_mysqlClient;
	delete g_sqliteClient;
	delete g_sqlInterface;
	return true;
}

void* SQLPlugin::OnMetamodQuery(const char* iface, int* ret)
{
	if (!strcmp(iface, SQLMM_INTERFACE))
	{
		*ret = META_IFACE_OK;
		return g_sqlInterface;
	}

	*ret = META_IFACE_FAILED;
	return nullptr;
}

void SQLPlugin::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */

	for (auto connection : g_vecMysqlConnections)
	{
		connection->RunFrame();
	}
}

const char *SQLPlugin::GetLicense()
{
	return "GPLv3";
}

const char *SQLPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *SQLPlugin::GetDate()
{
	return __DATE__;
}

const char *SQLPlugin::GetLogTag()
{
	return "SQLMM";
}

const char *SQLPlugin::GetAuthor()
{
	return "Poggu, zer0.k";
}

const char *SQLPlugin::GetDescription()
{
	return "Exposes SQL connectivity";
}

const char *SQLPlugin::GetName()
{
	return "SQLMM";
}

const char *SQLPlugin::GetURL()
{
	return "https://poggu.me";
}

IMySQLClient *SQLInterface::GetMySQLClient()
{
	return g_mysqlClient;
}

ISQLiteClient *SQLInterface::GetSQLiteClient()
{
	return g_sqliteClient;
}
