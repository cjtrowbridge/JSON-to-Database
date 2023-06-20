import json
import sqlite3

# Function to create tables for nested arrays
def create_nested_tables(value, prefix, database):
    for key, nested_value in value.items():
        # If the value is another dictionary, create a new table
        if isinstance(nested_value, dict):
            table_name = f"{prefix}_{key}"
            
            # Create the table with an ID column
            create_table_query = f"CREATE TABLE IF NOT EXISTS {table_name} (id INTEGER PRIMARY KEY AUTOINCREMENT)"
            database.execute(create_table_query)
            
            # Get the column names and types from the nested dictionary
            nested_keys = nested_value.keys()
            for nested_key in nested_keys:
                nested_value_type = "INTEGER" if isinstance(nested_value[nested_key], int) else "TEXT"
                add_column_query = f"ALTER TABLE {table_name} ADD COLUMN {nested_key} {nested_value_type}"
                database.execute(add_column_query)
            
            # Recursively create tables for nested dictionaries
            create_nested_tables(nested_value, table_name, database)

# Load JSON data from file
json_file_path = 'path/to/your/json/file.json'
with open(json_file_path) as file:
    json_data = json.load(file)

# Create SQLite database and cursor
database = sqlite3.connect('database.db')
cursor = database.cursor()

# Create a table for the root dictionary
cursor.execute("CREATE TABLE IF NOT EXISTS root (id INTEGER PRIMARY KEY AUTOINCREMENT)")

# Get the keys and values from the root dictionary
root_keys = json_data.keys()
for key in root_keys:
    value_type = "INTEGER" if isinstance(json_data[key], int) else "TEXT"
    cursor.execute(f"ALTER TABLE root ADD COLUMN {key} {value_type}")

# Recursively create tables for nested dictionaries
create_nested_tables(json_data, "root", cursor)

# Function to insert data into nested tables
def insert_nested_data(value, prefix, parent_id):
    for key, nested_value in value.items():
        # If the value is another dictionary, insert data into the corresponding table
        if isinstance(nested_value, dict):
            table_name = f"{prefix}_{key}"
            
            # Insert a row into the table and get the generated ID
            cursor.execute(f"INSERT INTO {table_name} DEFAULT VALUES")
            child_id = cursor.lastrowid
            
            # Update the parent table with the foreign key relationship
            cursor.execute(f"UPDATE {prefix} SET {key} = ? WHERE id = ?", (child_id, parent_id))
            
            # Recursively insert data into nested tables
            insert_nested_data(nested_value, table_name, child_id)

# Insert data into nested tables
insert_nested_data(json_data, "root", 1)

# Commit the changes and close the database connection
database.commit()
database.close()

print("Database created successfully!")
