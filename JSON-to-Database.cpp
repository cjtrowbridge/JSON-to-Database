#include <iostream>
#include <fstream>
#include <vector>
#include <sqlite3.h>
#include <json/json.h>

// SQLite callback function to print error messages
int printError(void* data, int argc, char** argv, char** azColName)
{
    std::cerr << "SQLite error: " << argv[0] << std::endl;
    return 0;
}

// Function to recursively create tables for nested arrays
void createNestedTables(const Json::Value& value, const std::string& prefix, sqlite3* database)
{
    for (Json::Value::const_iterator it = value.begin(); it != value.end(); ++it)
    {
        const std::string& key = it.key().asString();
        const Json::Value& nestedValue = *it;
        
        // If the value is another array, create a new table
        if (nestedValue.isArray())
        {
            std::string tableName = prefix + "_" + key;
            
            // Create the table with an ID column
            std::string createTableQuery = "CREATE TABLE IF NOT EXISTS " + tableName + "(id INTEGER PRIMARY KEY AUTOINCREMENT)";
            sqlite3_exec(database, createTableQuery.c_str(), NULL, 0, NULL);
            
            // Get the column names and types from the first element of the nested array
            const Json::Value& firstElement = nestedValue[0];
            for (Json::Value::const_iterator nestedIt = firstElement.begin(); nestedIt != firstElement.end(); ++nestedIt)
            {
                const std::string& nestedKey = nestedIt.key().asString();
                const Json::Value& nestedValue = *nestedIt;
                
                // Determine the data type of the nested value
                std::string dataType = nestedValue.isNumeric() ? "INTEGER" : "TEXT";
                
                // Add the column to the table
                std::string addColumnQuery = "ALTER TABLE " + tableName + " ADD COLUMN " + nestedKey + " " + dataType;
                sqlite3_exec(database, addColumnQuery.c_str(), NULL, 0, NULL);
            }
            
            // Recursively create tables for nested arrays
            createNestedTables(nestedValue, tableName, database);
        }
    }
}

int main()
{
    // Specify the path to the JSON file
    std::string jsonFilePath = "path/to/your/json/file.json";
    
    // Read the JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open JSON file: " << jsonFilePath << std::endl;
        return 1;
    }
    
    std::string jsonData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Parse the JSON data
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::istringstream jsonStream(jsonData);
    std::string parseError;
    if (!Json::parseFromStream(builder, jsonStream, &root, &parseError))
    {
        std::cerr << "Failed to parse JSON: " << parseError << std::endl;
        return 1;
    }
    
    // Open a connection to the SQLite database
    sqlite3* database;
    if (sqlite3_open("database.db", &database) != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(database) << std::endl;
        sqlite3_close(database);
        return 1;
    }
    
    // Create a table for the root array
    std::string createRootTableQuery = "CREATE TABLE IF NOT EXISTS root (id INTEGER PRIMARY KEY AUTOINCREMENT)";
    sqlite3_exec(database, createRootTableQuery.c_str(), NULL, 0, NULL);
    
    // Get the column names and types from the first element of the root array
    const Json::Value& firstElement = root[0];
    for (Json::Value::const_iterator it = firstElement.begin(); it != firstElement.end(); ++it)
    {
        const std::string& key = it.key().asString();
        const Json::Value& value = *it;
        
        // Determine the data type of the value
        std::string dataType = value.isNumeric() ? "INTEGER" : "TEXT";
        
        // Add the column to the root table
        std::string addColumnQuery = "ALTER TABLE root ADD COLUMN " + key + " " + dataType;
        sqlite3_exec(database, addColumnQuery.c_str(), NULL, 0, NULL);
    }
    
    // Recursively create tables for nested arrays
    createNestedTables(root, "root", database);
    
    // Populate the tables with data
    std::string rootInsertQuery = "INSERT INTO root DEFAULT VALUES";
    sqlite3_exec(database, rootInsertQuery.c_str(), NULL, 0, NULL);
    
    // Function to recursively insert data into nested tables
    std::function<void(const Json::Value&, const std::string&, int)> insertNestedData = [&](const Json::Value& value, const std::string& prefix, int parentId)
    {
        for (Json::Value::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            const std::string& key = it.key().asString();
            const Json::Value& nestedValue = *it;
            
            // If the value is another array, insert data into the corresponding table
            if (nestedValue.isArray())
            {
                std::string tableName = prefix + "_" + key;
                
                // Insert a row into the table and get the generated ID
                std::string insertQuery = "INSERT INTO " + tableName + " DEFAULT VALUES";
                sqlite3_exec(database, insertQuery.c_str(), NULL, 0, NULL);
                int childId = sqlite3_last_insert_rowid(database);
                
                // Update the parent table with the foreign key relationship
                std::string updateQuery = "UPDATE " + prefix + " SET " + key + " = " + std::to_string(childId) + " WHERE id = " + std::to_string(parentId);
                sqlite3_exec(database, updateQuery.c_str(), NULL, 0, NULL);
                
                // Recursively insert data into nested tables
                insertNestedData(nestedValue, tableName, childId);
            }
        }
    };
    
    // Insert data into nested tables
    insertNestedData(root, "root", 1);
    
    // Close the database connection
    sqlite3_close(database);
    
    std::cout << "Database created successfully!" << std::endl;
    
    return 0;
}
