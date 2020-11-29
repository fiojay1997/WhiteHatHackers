#include <iostream>
#include <stdlib.h>
#include <mysql_connection.h>
#include <driver.h>
#include <exception.h>
#include <resultset.h>
#include <statement.h>
#include <string>

using namespace sql;

const char * host = "white-hat-hackers.cbkoexjpndtw.us-east-1.rds.amazonaws.com";
const char * username = "jayjay1997";
const char * password = "12345678";

int get_data()
{
    try
    {
        sql::Driver * driver;
        sql::Connection * con;
        sql::Statement * statement;
        sql::ResultSet * res;

        driver = get_driver_instance();
        con = driver->connect(host, username, password);
        if (con != NULL)
            std::cout << "success" << std::endl;
   
        con->setSchema("api_data");

        statement = con->createStatement(); 
        res = statement->executeQuery("SELECT * FROM weather");
        while (res->next())
        {
            std::cout << "Mysql replies: ";
	    std::cout << res->getString("date") << std::endl;
        }

        delete res;
        delete statement;
        delete con;
    
        return 0;
    }
    catch (sql::SQLException &e)
    {
        std::cout << "connection failed" << std::endl;
	return -1;
    }
}

int main()
{
    int error_code = get_data();
    if (error_code != 0)
    {
        std::cout << "Query failed" << std::endl;
	return -1;
    }
    return 0;
}
