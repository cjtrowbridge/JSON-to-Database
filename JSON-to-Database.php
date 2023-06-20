<?php

// Specify the path to the JSON file
$jsonFilePath = 'path/to/your/json/file.json';

// Read the JSON file
$jsonData = file_get_contents($jsonFilePath);

// Decode the JSON data
$data = json_decode($jsonData, true);

// Create a new SQLite database
$database = new SQLite3('database.db');

// Function to create tables for nested arrays
function createNestedTables($array, $prefix, $database)
{
    foreach ($array as $key => $value) {
        // If the value is another array, create a new table
        if (is_array($value)) {
            // Generate a table name using the prefix and current key
            $tableName = $prefix . '_' . $key;
            
            // Create the table with an ID column
            $createTableQuery = "CREATE TABLE IF NOT EXISTS $tableName (id INTEGER PRIMARY KEY AUTOINCREMENT)";
            $database->exec($createTableQuery);
            
            // Get the column names and types from the first element of the nested array
            $nestedArray = reset($value);
            foreach ($nestedArray as $nestedKey => $nestedValue) {
                // Determine the data type of the nested value
                $dataType = is_numeric($nestedValue) ? 'INTEGER' : 'TEXT';
                
                // Add the column to the table
                $addColumnQuery = "ALTER TABLE $tableName ADD COLUMN $nestedKey $dataType";
                $database->exec($addColumnQuery);
            }
            
            // Recursively create tables for nested arrays
            createNestedTables($value, $tableName, $database);
        }
    }
}

// Create a table for the root array
$createRootTableQuery = 'CREATE TABLE IF NOT EXISTS root (id INTEGER PRIMARY KEY AUTOINCREMENT)';
$database->exec($createRootTableQuery);

// Get the column names and types from the first element of the root array
$rootArray = reset($data);
foreach ($rootArray as $key => $value) {
    // Determine the data type of the value
    $dataType = is_numeric($value) ? 'INTEGER' : 'TEXT';
    
    // Add the column to the root table
    $addColumnQuery = "ALTER TABLE root ADD COLUMN $key $dataType";
    $database->exec($addColumnQuery);
}

// Recursively create tables for nested arrays
createNestedTables($data, 'root', $database);

// Populate the tables with data
$rootInsertQuery = 'INSERT INTO root DEFAULT VALUES';
$database->exec($rootInsertQuery);

// Function to recursively insert data into nested tables
function insertNestedData($array, $prefix, $parentId, $database)
{
    foreach ($array as $key => $value) {
        // If the value is another array, insert data into the corresponding table
        if (is_array($value)) {
            // Generate a table name using the prefix and current key
            $tableName = $prefix . '_' . $key;
            
            // Insert a row into the table and get the generated ID
            $insertQuery = "INSERT INTO $tableName DEFAULT VALUES";
            $database->exec($insertQuery);
            $childId = $database->lastInsertRowID();
            
            // Update the parent table with the foreign key relationship
            $updateQuery = "UPDATE $prefix SET $key = $childId WHERE id = $parentId";
            $database->exec($updateQuery);
            
            // Recursively insert data into nested tables
            insertNestedData($value, $tableName, $childId, $database);
        }
    }
}

// Insert data into nested tables
insertNestedData($data, 'root', 1, $database);

// Close the database connection
$database->close();

echo 'Database created successfully!';
