#include "postgresql_connection_pool.hpp"

shared_ptr<pqxx::lazyconnection> pgsql_connection_pool::pgsql_connection_pickup()
{
	std::unique_lock<std::mutex> lock_(pgsql_pool_mutex);

	//if pool is empty, we have no connection available, so we wait until a connection become available
	while (conList.empty()) {

		pgsql_pool_condition.wait(lock_);//wait for notfiy
		//cout << "HERE" << endl;

	}//while

	//when program reachs this point, we have an available connection in pool

	//get first element (connection) in the queue
	auto conn_ = conList.front();

	//remove first element from queue
	conList.pop();
	return conn_;

}//pgsql_connection_pickup

void pgsql_connection_pool::hand_back_connection(std::shared_ptr<pqxx::lazyconnection> conn_)
{
	std::unique_lock<std::mutex> lock_(pgsql_pool_mutex);

	//hand back connection into pool
	conList.push(conn_);

	// unlock mutex
	lock_.unlock();

	// notify one of thread that is waiting
	pgsql_pool_condition.notify_one();

	return;
}//hand_back_connection


bool pgsql_connection_pool::setup_pgsql_connection_pool(string database_info)
{
	bool can_connect = false;
	int i = 0;

	//check if wen can connect to database
	try
	{
		connection C(database_info);

		if (C.is_open())
		{
			can_connect = true;

			//close database connection
			C.disconnect();
		}
		else {
			cout << "Can't open database" << endl;
			can_connect = false;
		}//if
	}
	catch (const std::exception& e)
	{
		cout << "ERROR2:" << endl;
		cerr << e.what() << std::endl;
		can_connect = false;
	}//catch


	if (can_connect)//we have access to database so we start creating connections for pool
	{
		for (i = 0; i < max_connections; i++)
		{
			//conList.emplace(std::make_shared<pqxx::lazyconnection>());
			try
			{
				conList.emplace(std::make_shared<pqxx::lazyconnection>(database_info));
			}
			catch (const std::exception& e)
			{
				cerr << e.what() << std::endl;
			}

		}//for

	}//if can_connect

	return can_connect;

}//setup_pgsql_connection_pool


bool pgsql_connection_pool::connect()
{
	pgsql_database_info = "dbname= " + db_name;
	pgsql_database_info += " user=" + db_user;
	pgsql_database_info += " password=" + db_password;
	pgsql_database_info += " hostaddr=" + db_host;
	pgsql_database_info += " port=" + db_port;
	pgsql_database_info += " connect_timeout=6";

	return setup_pgsql_connection_pool(pgsql_database_info);
}//connect

bool pgsql_connection_pool::create_database(string dbname)
{
	const char* sql;
	bool success = false;
	string query = "CREATE DATABASE " + dbname + ";";
	sql = query.c_str();

	auto conn = pgsql_connection_pickup();

	//if we dont have nullptr in conn
	if (conn)
	{
		try
		{
			// create a nontransaction from a connection
			pqxx::nontransaction W(reinterpret_cast<pqxx::lazyconnection&>(*conn.get()));

			try {

				/* Execute SQL query */
				W.exec(sql);
				success = true;
			}//try
			catch (const std::exception& e) {
				cout << "CREATE DATABASE ERROR:" << query << endl;
				cerr << e.what() << std::endl;
			}//catch

			if (success)
			{
				try
				{
					W.commit();
				}
				catch (const std::exception& e) {
					cout << "commit ERROR:" << endl;
					cerr << e.what() << std::endl;
					W.abort();
					success = false;
				}
			}
			else
			{
				cout << "exec ERROR:" << endl;
				W.abort();
			}//else

		}//try
		catch (const std::exception& e) {
			cout << "Work Create ERROR:" << endl;
			cerr << e.what() << std::endl;
			//W.abort();
			//exit(0);
		}//catch

			//hand back pgsql_connection
		hand_back_connection(conn);

	}//if conn
	else {
		cout << "Can't open database" << endl;
	}

	return success;
}//create_database

bool pgsql_connection_pool::custom_query(string& query)
{
	const char* sql;
	bool success = false;
	sql = query.c_str();

	auto conn = pgsql_connection_pickup();

	//if we dont have nullptr in conn
	if (conn)
	{
		try
		{
			// create a transaction from a connection
			work W(reinterpret_cast<pqxx::lazyconnection&>(*conn.get()));

			try {

				/* Execute SQL query */
				W.exec(sql);
				success = true;
			}//try
			catch (const std::exception& e) {
				cout << "custom query ERROR:" << query << endl;
				cerr << e.what() << std::endl;
			}//catch

			if (success)
			{
				try
				{
					W.commit();
				}
				catch (const std::exception& e) {
					cout << "commit ERROR:" << endl;
					cerr << e.what() << std::endl;
					W.abort();
					success = false;
				}
			}
			else
			{
				cout << "exec ERROR:" << endl;
				W.abort();
			}//else

		}//try
		catch (const std::exception& e) {
			cout << "Work Create ERROR:" << endl;
			cerr << e.what() << std::endl;
			//W.abort();
			//exit(0);
		}//catch

			//hand back pgsql_connection
		hand_back_connection(conn);

	}//if conn
	else {
		cout << "Can't open database" << endl;
	}

	return success;

}//custom_query

postgresql_result pgsql_connection_pool::select(string& query)
{
	const char* sql;
	sql = query.c_str();

	auto conn = pgsql_connection_pickup();

	postgresql_result rows;

	try {
		if (conn)
		{
			/* Create a non-transactional object. */
			work N(reinterpret_cast<pqxx::lazyconnection&>(*conn.get()));

			/* Execute SQL query */
			result R(N.exec(sql));

			for (result::const_iterator c = R.begin(); c != R.end(); ++c)
			{
				potgresql_row* r1 = new potgresql_row();

				for (int i = 0; i < c.size(); i++)
				{
					r1->column[i] = c[i].as<string>();
					//cout << "Column[" << i << "]=" << c[i] << endl;
				}
				rows.push_back(r1);

				/*cout << "ID = " << c[0].as<int>() << endl;
				cout << "Name = " << c[1].as<string>() << endl;
				cout << "Age = " << c[2].as<int>() << endl;
				cout << "Address = " << c[3].as<string>() << endl;
				cout << "Salary = " << c[4].as<float>() << endl;*/
			}
		}//if conn
		else {
			cout << "Can't open database" << endl;
		}//if conn->Open
	}
	catch (const std::exception& e) {
		cout << "select ERROR:" << endl;
		cerr << e.what() << std::endl;
	}

	if (conn)
	{
		//hand back pgsql_connection
		hand_back_connection(conn);
	}//if 2147483647

	return rows;
}//select