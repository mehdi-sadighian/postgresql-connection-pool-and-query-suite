#pragma once
#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <pqxx/pqxx>
#include <condition_variable>
#include <queue>
#include <mutex>

using namespace std;
using namespace pqxx;

struct potgresql_row
{
	string column[10];
};

typedef  vector<potgresql_row*> postgresql_result;

class pgsql_connection_pool {

public:
	string db_name;
	string db_user;
	string db_password;
	string db_host;
	string db_port = "5432";

	const int max_selected_columns = 10; //postgresql_row size

	//max connections
	int max_connections = 20;


	//functions
	bool connect();
	bool custom_query(string& query);
	postgresql_result select(string& query);
	bool create_database(string dbname);

private:
	string pgsql_database_info;
	//create mutex for concurrent Read/Write access to conList
	mutex pgsql_pool_mutex;
	//condition variable to notify threads waiting that we have one connection free
	condition_variable pgsql_pool_condition;
	//queue to store pgsql connections
	queue<std::shared_ptr<pqxx::lazyconnection>> conList;

	//functions:
	shared_ptr<pqxx::lazyconnection> pgsql_connection_pickup();
	void hand_back_connection(std::shared_ptr<pqxx::lazyconnection> conn_);
	bool setup_pgsql_connection_pool(string database_info);




};//odinpool_database