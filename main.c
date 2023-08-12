/*main.c*/

//
// Jeremy Chung
// CS 211
// Winter 2023
// Northwestern University
//

// other header files
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// local header files
#include "analyzer.h"
#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "database.h" 
#include "util.h"

// for ast enums
char *functions[5] = {"MIN", "MAX", "SUM", "AVG", "COUNT"}; 
char *expressions[7] = {"<", "<=", ">", ">=", "=", "<>", "like"};

// takes in a query structure pointer and database structure pointer and returns nothing.
// prints the first 5 lines of data from a table
void execute_query(struct QUERY *qu, struct Database *od){
  struct TableMeta* dbtables = od->tables; 
  struct TableMeta table; 
  
  char* tableName = qu->q.select->table; 
  for(int i = 0; i < od->numTables; i++){ //loops through all the tables in the database
    if(icmpStrings(tableName, dbtables[i].name) == 0){ // checks if the user table name is the same as the database tablename
      table = od->tables[i]; 
      tableName = dbtables[i].name; 
      break;
    }
  }

  // combines all the elements of the filename into one string 
  char filename[DATABASE_MAX_ID_LENGTH + 1] = "";
  strcat(filename, od->name); 
  strcat(filename, "/");
  strcat(filename, tableName);
  strcat(filename, ".data");

  // opens the file and errors if the file cannot be found
  FILE *file = fopen(filename, "r"); 
  if(file == NULL) { 
    printf("ERROR: File not found.\n");
    exit(-1);
  }

  // dynamically allocates enough memory
  int bufferSize = (table.recordSize + 3); 
  char *buffer = (char*) malloc(sizeof(char) * bufferSize);
  if(buffer == NULL){
    panic("out of memory"); 
  }

  // loops 5 times to print the 5 lines
  for(int i = 0; i < 5; i++) { 
    if(fgets(buffer, bufferSize, file) == NULL){ // checks if null
      break;
    }
    printf("%s", buffer);
  }
  
}

// takes in a query structure pointer and returns nothing
// prints a representation of the abstract syntax tree.
void print_ast(struct QUERY* qu){
  printf("**QUERY AST**\n");
  
  struct SELECT* select = qu->q.select; 
  printf("Table: %s\n", select->table);

  // COLUMNS
  struct COLUMN* columns = select->columns; 
  while(true){  // traverses the linked list through all the columns in the query
      if (columns == NULL){
         break; 
      }
      if(columns->function != -1){ // checks in the column contains a function and prints with the appropriate function
        printf("Select column: %s(%s.%s)\n", functions[columns->function], select->table, columns->name);
      }
      else{ // otherwise print normally 
        printf("Select column: %s.%s\n", columns->table, columns->name);
      }
      columns = columns->next; 
   }
  
  //JOIN
  struct JOIN* join = select->join; 
  if (join == NULL){
    printf("Join (NULL)\n");
  }
  else{
    struct COLUMN* cl = join->left; 
    struct COLUMN* cr = join->right; 
    printf("Join %s On %s.%s = %s.%s\n", join->table, cl->table, cl->name, cr->table, cr->name); // print the join part of the query
  }

  struct WHERE* where = select->where; 
  if (where == NULL){
    printf("Where (NULL)\n");
  }
  else{
    struct EXPR* expr = where->expr; 
    struct COLUMN* exprCol = expr->column; 
    if (expr->litType == 2){ // checks if the expr structure is a string
      if (strchr(expr->value, '\'')){ // checks if the string contains single quotes
        printf("Where %s.%s %s \"%s\"\n", exprCol->table, exprCol->name, expressions[expr->operator], expr->value); // if the expr structure has single quotes, print with double quotes
      }
      else {
        printf("Where %s.%s %s \'%s\'\n", exprCol->table, exprCol->name, expressions[expr->operator], expr->value); // else print with single quotes
      }
    }
    else {
      printf("Where %s.%s %s %s\n", exprCol->table, exprCol->name, expressions[expr->operator], expr->value); // normal printing of the where part of the query 
    }
  }

  
  struct ORDERBY* orderby = select->orderby; 
  if (orderby == NULL){
    printf("Order By (NULL)\n");
  }
  else{
    if(orderby->column->function != -1){ // checks if there's a function in the column
       printf("Order By %s(%s.%s) ", functions[orderby->column->function], orderby->column->table, orderby->column->name); 
    }
    else{
      printf("Order By %s.%s ", orderby->column->table, orderby->column->name);
    }

    if(orderby->ascending){ // checks if ascending or descending 
      printf("ASC\n"); 
    }
    else{
      printf("DESC\n");
    }
  }
  
  
  // prints the limit part of the query if any exists
  struct LIMIT* limit = select->limit; 
  if (limit == NULL){
    printf("Limit (NULL)\n");
  }
  else{
    printf("Limit %d\n", limit->N);
  }

  // prints the into part of the query if any exists
  struct INTO* into = select->into; 
  if (into == NULL){
    printf("Into (NULL)\n");
  }
  else{
    printf("Into %s\n", into->table);
  }

  printf("**END OF QUERY AST**\n");
}


// takes in a database structure pointer and returns nothing
// prints out a representation of the database schema
void print_schema(struct Database *od){
  printf("**DATABASE SCHEMA**\n");
  printf("Database: %s\n", od->name);
  
  struct TableMeta* table = od->tables; 
  for(int i = 0; i < od->numTables; i++){ // loops through all the tables in the database
    printf("Table: %s\n", table[i].name); 
    printf("  Record Size: %d\n", table[i].recordSize);
    
    struct ColumnMeta* columns = table[i].columns; 
    for(int j = 0; j < table[i].numColumns; j++){ // loops through all the columns in each table
      char colType[7];
      char indexType[15];

      // checks the datatype in the column
      if (columns[j].colType == 1){
        strcpy(colType, "int");
      }
      else if (columns[j].colType == 2){
        strcpy(colType, "real");
      }
      else if (columns[j].colType == 3){
        strcpy(colType, "string");
      }

      // checks the index type of the column 
      if (columns[j].indexType == 0){
        strcpy(indexType, "non-indexed");
      }
      else if (columns[j].indexType == 1){
        strcpy(indexType, "indexed");
      }
      else if (columns[j].indexType == 2){
        strcpy(indexType, "unique indexed");
      }
      
      printf("  Column: %s, %s, %s\n", columns[j].name, colType, indexType);
    }
  }
  
  printf("**END OF DATABASE SCHEMA**\n");
}

int main(){
  char database[DATABASE_MAX_ID_LENGTH+1];
  printf("database? ");
  scanf("%s", database); // takes in the user input of the name of the database

  struct Database *open_database = database_open(database); // open the database
  
  if(open_database == NULL){
    printf("**Error: unable to open database ‘%s’\n", database);
    exit(-1);
  }

  print_schema(open_database); // prints out a representation of the dataabse

  parser_init(); // initializes the pareser

  // loops until errors or the user exits 
  while(true){
    printf("query? ");
    struct TokenQueue* tq = parser_parse(stdin); // parses through the inputs and gets a token queue

    if(tq == NULL){
      if(parser_eof()){
        break; 
      }
    }
    else{
      struct QUERY* query = analyzer_build(open_database, tq); // analyzes the token queue to create a query
      if (query != NULL){
        print_ast(query); // prints the represetnation of the abstract syntax tree of the query
        
        execute_query(query, open_database); // prints the first 5 lines of the data from the table
      }
    }
  }
  
  database_close(open_database);
  
  return 0;
}
