#include <stdio.h>
#include <cstring>
#include <iostream>
#include <string>
#include "postgresql_connection_pool.hpp"

using namespace std;

int main()
{
	//create object
	pgsql_connection_pool DB;

	//set config
	DB.db_name = "odinpool";
	DB.db_host = "127.0.0.1";
	DB.db_port = "5432";
	DB.db_user = "root";
	DB.db_password = "password123";
	DB.max_connections = 20;  //optional, default 20

	if (!DB.connect()) //connects to DATABASE
	{
		cout << "Cannot Connect To Database...Exiting";
		exit(0);
	}

	string username = "mehdi_sadighian";
	string password = "mypassword";

	//insert example:
	string query = "insert into users (username,password) VALUES ";
	query += "(";
	query += "\'"; query += username; query += "\',";
	query += "\'"; query += password; query += "\'";
	query += ");";

	if (!DB.custom_query(query))
	{
		cout<< "Cannot insert into Database" << endl;
	}

	//update example:
	query = "update users set password = '123' where username = 'mehdi_sadighian';";

	if (!DB.custom_query(query))
	{
		cout << "Cannot insert into Database" << endl;
	}

	//select example:
	//maximum number of 10 rows  can be selected, if you want to select more that 10 rows you need to change some
	//varibles inside class :string column[10];  and max_selected_columns
	query = "select * from users;";

	postgresql_result result = DB.select(query);

	for (int i = 0; i < result.size(); i++)
	{
		int j = 0;
		while ((!result[i]->column[j].empty()) && j < DB.max_selected_columns)
		{
			cout << "row[" << i << "]column[" << j << "]=" << result[i]->column[j] << endl;
			j++;
		}
	}


	//create dtabase example:
	//creating a database named test_db
	bool db_created = DB.create_database("test_db");


}//main
