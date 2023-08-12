/*execute.c*/

//
// Project: Execution of queries for SimpleSQL
//
// Jeremy Chung
// Northwestern University
// CS 211, Winter 2023
//

#include <assert.h> //assert
#include <ctype.h>
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcpy, strcat
#include <strings.h>

#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "parser.h"
#include "resultset.h"
#include "scanner.h"
#include "tokenqueue.h"
#include "util.h"

// removes the quotes from the databuffer
void removeQuotes(char *input) {
  int len = strlen(input);
  for (int i = 0; i < len - 2; i++) {
    input[i] = input[i + 1]; // shifts all the characters in the string over by
                             // 1
  }
  input[len - 2] =
      '\0'; // adds a null terminator at the end of the shifted string
}

// determines if the part of the databuffer is a string, int, or double
int getType(char *input) {
  char start = input[0];
  if (start == '\"' || start == '\'') { // if the start of the of the string has
                                        // quotes, it must be a string
    return 1;
  } else { // if the string doesn't have quotes, it must be an int or double
    for (int i = 0; i < strlen(input);
         i++) { // loops through the input, if it has a decimal it must be a
                // double
      start = input[i];
      if (start == '.') {
        return 2;
      }
    }
    return 3; // if the loop makes it through the input without finding a
              // string, it must be an int
  }
}

// checks if the value at the specified row and column index for columns of type
// integer fulfill the requirements of the query
bool found_row_int(int rowIndex, int colIndex, struct ResultSet *rs,
                   struct EXPR *expr) {
  int op = expr->operator; // gets the query operator

  int rsInput = resultset_getInt(
      rs, rowIndex, colIndex);      // resultset value and specified indices
  int astInput = atoi(expr->value); // query value

  // returns true if the query expression is true
  if (op == 0) {
    if (rsInput < astInput)
      return true;
  }
  if (op == 1) {
    if (rsInput <= astInput)
      return true;
  }
  if (op == 2) {
    if (rsInput > astInput)
      return true;
  }
  if (op == 3) {
    if (rsInput >= astInput)
      return true;
  }
  if (op == 4) {
    if (rsInput == astInput)
      return true;
  }
  if (op == 5) {
    if (rsInput != astInput)
      return true;
  }
  return false;
}

// checks if the value at the specified row and column index for columns of type
// double fulfill the requirements of the query
bool found_row_double(int rowIndex, int colIndex, struct ResultSet *rs,
                      struct EXPR *expr) {
  int op = expr->operator; // gets the query operator

  double rsInput = resultset_getReal(
      rs, rowIndex, colIndex);         // resultset value and specified indices
  double astInput = atof(expr->value); // query value

  // returns true if the query expression is true
  if (op == 0) {
    if (rsInput < astInput)
      return true;
  }
  if (op == 1) {
    if (rsInput <= astInput)
      return true;
  }
  if (op == 2) {
    if (rsInput > astInput)
      return true;
  }
  if (op == 3) {
    if (rsInput >= astInput)
      return true;
  }
  if (op == 4) {
    if ((rsInput - astInput) < 0.00001)
      return true;
  }
  if (op == 5) {
    if (rsInput != astInput)
      return true;
  }
  return false;
}

// checks if the value at the specified row and column index for columns of type
// string fulfill the requirements of the query
bool found_row_string(int rowIndex, int colIndex, struct ResultSet *rs,
                      struct EXPR *expr) {
  int op = expr->operator; // gets the query operator

  char *rsInput = resultset_getString(
      rs, rowIndex, colIndex);  // resultset value and specified indices
  char *astInput = expr->value; // query value
  int comp =
      strcasecmp(rsInput, astInput); // compares the strings and creates a
                                     // numeric value based off the comparison
  free(rsInput); // frees the allocated copy of the resultset data

  // returns true if the query expression is true
  if (op == 0) {
    if (comp < 0)
      return true;
  }
  if (op == 1) {
    if (comp < 0 || comp == 0)
      return true;
  }
  if (op == 2) {
    if (comp > 0)
      return true;
  }
  if (op == 3) {
    if (comp > 0 || comp == 0)
      return true;
  }
  if (op == 4) {
    if (comp == 0)
      return true;
  }
  if (op == 5) {
    if (comp != 0)
      return true;
  }
  return false;
}

//
// execute_query
//
// execute a select query, which for now means open the underlying
// .data file and output the first 5 lines.
//
void execute_query(struct Database *db, struct QUERY *query) {
  if (db == NULL)
    panic("db is NULL (execute)");
  if (query == NULL)
    panic("query is NULL (execute)");

  if (query->queryType != SELECT_QUERY) {
    printf("**INTERNAL ERROR: execute() only supports SELECT queries.\n");
    return;
  }

  struct SELECT *select = query->q.select; // alias for less typing:

  //
  // the query has been analyzed and so we know it's correct: the
  // database exists, the table(s) exist, the column(s) exist, etc.
  //

  //
  // (1) we need a pointer to the table meta data, so find it:
  //
  struct TableMeta *tablemeta = NULL;
  struct ResultSet *rs = resultset_create();
  for (int t = 0; t < db->numTables; t++) {
    if (icmpStrings(db->tables[t].name, select->table) == 0) // found it:
    {
      tablemeta = &db->tables[t];
      for (int i = 1; i < tablemeta->numColumns + 1;
           i++) { // loops through each tablemeta and create a resultset column
                  // with matching information
        resultset_insertColumn(rs, i, tablemeta->name,
                               tablemeta->columns[i - 1].name, NO_FUNCTION,
                               tablemeta->columns[i - 1].colType);
      }
      break;
    }
  }

  assert(tablemeta != NULL);

  //
  // (2) open the table's data file
  //
  // the table exists within a sub-directory under the executable
  // where the directory has the same name as the database, and with
  // a "TABLE-NAME.data" filename within that sub-directory:
  //
  char path[(2 * DATABASE_MAX_ID_LENGTH) + 10];

  strcpy(path, db->name); // name/name.data
  strcat(path, "/");
  strcat(path, tablemeta->name);
  strcat(path, ".data");

  FILE *datafile = fopen(path, "r");
  if (datafile == NULL) // unable to open:
  {
    printf("**INTERNAL ERROR: table's data file '%s' not found.\n", path);
    panic("execution halted");
    exit(-1);
  }

  //
  // (3) allocate a buffer for input, and start reading the data:
  //
  int dataBufferSize =
      tablemeta->recordSize + 3; // ends with $\n + null terminator
  char *dataBuffer = (char *)malloc(sizeof(char) * dataBufferSize);
  if (dataBuffer == NULL)
    panic("out of memory");

  int rowCount = 1;
  while (true) {
    fgets(dataBuffer, dataBufferSize, datafile);

    if (feof(datafile)) // end of the data file, we're done
      break;

    // breaks up the databuffer by adding null terminators in the blank spaces
    int length = strlen(dataBuffer);
    char *cp = dataBuffer; // create a pointer to the databuffer to work with
    for (int i = 0; i < length; i++) {
      if (*cp == '\"') { // if the databuffer has a quote, loop through and
                         // advance the pointer until another quote is found
        cp++;
        i++;
        while (*cp != '\"') {
          cp++;
          i++;
        }
      } else if (*cp == '\'') { // if the databuffer has a single quote, loop
                                // through and advance the pointer until another
                                // single quote is found
        cp++;
        i++;
        while (*cp != '\'') {
          cp++;
          i++;
        }
      }
      if (*cp ==
          ' ') { // if a blank space is found, add a null string terminator
        *cp = '\0';
      }
      cp++;
    }

    // adds the parts of the databuffer to their appropriate columns
    rowCount = resultset_addRow(rs);
    char *temp = dataBuffer; // create a pointer to the databuffer to work with
    for (int i = 0; i < tablemeta->numColumns; i++) {
      int type = getType(temp); // determines wether the part of the databuffer
                                // is a string, int, or double
      bool removedQuotes = false; // boolean to keep track of whether the part
                                  // of the databuffer has quotes (is a string)

      if (type == 1) {      // string with quotes
        removeQuotes(temp); // removes the quotes
        removedQuotes = true;
        resultset_putString(rs, rowCount, i + 1, temp);
      }
      if (type == 2) { // double
        resultset_putReal(rs, rowCount, i + 1, atof(temp));
      }
      if (type == 3) { // int
        resultset_putInt(rs, rowCount, i + 1, atoi(temp));
      }

      char *index = temp; // creates a pointer of temp to work with
      while (
          *index !=
          '\0') { // advance the pointer until the next null terminator is found
        index++;
      }
      if (removedQuotes) { // if quotes were removed, advanced the pointer 2
                           // more spaces to account for the removed quotes
        index += 2;
      }
      index++;
      temp = index; // set the temp back
    }
  }
  free(dataBuffer);

  // evaluates the where clause of a query
  if (select->where != NULL) {
    struct EXPR *expr = select->where->expr;
    struct COLUMN *exprCol = expr->column;

    struct RSColumn *currcol = rs->columns;
    int colNum = resultset_findColumn(
        rs, 1, exprCol->table,
        exprCol->name); // finds theindex of the column that matches the where
                        // part of the query
    for (int i = 1; i < colNum; i++) { // loops to the correct index
      currcol = currcol->next;
    }

    for (int i = rs->numRows; i > 0;
         i--) { // loops through every row of the column and evaluates the value
                // agains the query, deleting the row if it doesn't
      if (currcol->coltype == COL_TYPE_INT) {
        if (!(found_row_int(i, colNum, rs, expr))) { // compares the ints
          resultset_deleteRow(rs, i);
        }
      }
      if (currcol->coltype == COL_TYPE_REAL) {
        if (!(found_row_double(i, colNum, rs, expr))) { // compares the doubles
          resultset_deleteRow(rs, i);
        }
      }
      if (currcol->coltype == COL_TYPE_STRING) {
        if (!(found_row_string(i, colNum, rs, expr))) { // compares the strings
          resultset_deleteRow(rs, i);
        }
      }
    }
  }

  // deletes columns not specified in the query
  bool found = false;
  struct RSColumn *RSCol = rs->columns;
  for (int i = 0; i < rs->numCols;
       i++) {      // loop through for each column in the result set
    found = false; // tracks if the column was found
    char *RSColName = RSCol->colName;
    struct COLUMN *temp = select->columns;
    while (temp != NULL) { // loops through all query parts
      if (strcasecmp(RSColName, temp->name) ==
          0) { // checksif the column in the resultset matches one found in a
               // query
        found = true;
      }
      temp = temp->next;
    }
    if (!found) { // if the column in the resultset was not found in the query,
                  // delete the column
      RSCol = RSCol->next;
      resultset_deleteColumn(rs, i + 1);
      i--;
    } else {
      RSCol = RSCol->next;
    }
  }

  // reorders the resultset columns to match the query
  struct COLUMN *temp = select->columns;
  int colIndex = 1;
  while (temp != NULL) { // loop through ast to reorder resultset columns
    int initialIndex = 1;
    struct RSColumn *RSCol = rs->columns;
    for (int i = 1; i < rs->numCols + 1;
         i++) { // loops through the result set and finds the index it should be
                // moved to
      if (strcasecmp(RSCol->colName, temp->name) == 0) {
        initialIndex = i;
      }
      RSCol = RSCol->next;
    }
    resultset_moveColumn(rs, initialIndex, colIndex); // moves the column
    temp = temp->next;
    colIndex++;
  }

  // applies a function to the resultset columns if the ast columns has a
  // function
  struct COLUMN *temp2 = select->columns;
  colIndex = 1;
  while (temp2 != NULL) {        // loops through all the columns in the query
    if (temp2->function != -1) { // checks if the query has a function and if so
                                 // it applies the function to the resultset
      resultset_applyFunction(rs, temp2->function, colIndex);
    }
    temp2 = temp2->next;
    colIndex++;
  }

  // applies limit to the resultset by deleting any rows past the limit
  struct LIMIT *limit = select->limit;
  if (limit != NULL) {
    while (limit->N !=
           rs->numRows) { // deletes the rows starting from the last row until
                          // the number of rows matches the limit in the query
      resultset_deleteRow(rs, rs->numRows);
    }
  }

  resultset_print(rs);
  resultset_destroy(rs);
  analyzer_destroy(query);
  //
  // done!
  //
}
