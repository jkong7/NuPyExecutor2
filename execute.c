/*execute.c*/

//
// Executes NuPython code in C - for project 06, it executes only straight-line code - that is function calls, assignments, and pass statements
// Traverses the program graph, interprets the statements, and executes them by interacting with a RAM memory module for variable storage.
//
// Jonathan Kong 
// Norhtwestern University 
// CS 211
// Fall 2024 
// 
// Starter code: Prof. Joe Hummel
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // true, false
#include <string.h>
#include <assert.h>
#include <math.h> 

#include "programgraph.h"
#include "ram.h"
#include "execute.h"


//
// retrieve_value
//
// Helper function to retrieve the integer value from an expression 
// Used to decrypt both the lhs and rhs expressions - deals with two cases:
// int_literal and identifier (for which interaction with the RAM module is necessary)
// Called in execute_binary_expression to compute lhs and rhs of binary expression 
// Returns false if semantic error (identifier not found in RAM), else true
//
bool retrive_value(struct UNARY_EXPR* expr, int* result, struct RAM* memory, int line) {
  char* string_value = expr->element->element_value; 
  if (expr->element->element_type==ELEMENT_INT_LITERAL) {
    int num = atoi(string_value); 
    *result=num; 
  } else if (expr->element->element_type==ELEMENT_IDENTIFIER) {
    struct RAM_VALUE* cell_ram_value = ram_read_cell_by_name(memory, string_value); 
    if (cell_ram_value==NULL) {
      printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_value, line);
      return false; 
    }
    int return_value_from_memory=cell_ram_value->types.i; 
    *result=return_value_from_memory; 
  }
  return true; 
}

//
// execute_binary_expression 
//
// Executes binary expression by combining lhs and rhs values with appropriate operator 
// Places answer in pass by reference variable result, and returns false if semantic error, else true
//
bool execute_binary_expression(struct EXPR* expr, int* result, struct RAM* memory, int line) {
  struct UNARY_EXPR* lhs=expr->lhs; 
  struct UNARY_EXPR* rhs=expr->rhs; 
  int result_lhs; 
  int result_rhs; 
  bool lhs_success = retrive_value(lhs, &result_lhs, memory, line); 
  bool rhs_success = retrive_value(rhs, &result_rhs, memory, line); 
  if (!lhs_success || !rhs_success) {
    return false; 
  }
  int res; 
  if (expr->operator==OPERATOR_PLUS) {
    res = result_lhs + result_rhs; 
  } else if (expr->operator==OPERATOR_MINUS) {
    res = result_lhs - result_rhs; 
  } else if (expr->operator==OPERATOR_ASTERISK) {
    res = result_lhs * result_rhs; 
  } else if (expr->operator==OPERATOR_DIV) {
    res = result_lhs / result_rhs; 
  } else if (expr->operator==OPERATOR_MOD) {
    res = result_lhs % result_rhs; 
  } else if (expr->operator==OPERATOR_POWER) {
    res = pow(result_lhs, result_rhs); 
  }
  *result=res; 
  return true; 
}

//
// execute_assignment
//
// Executes an assignment statement, handles both binary expressions and unary expressions, supporting 
// int literals and identifiers. Writes to RAM memory. Returns false if semantic error and stops execution, 
// returns true otherwise
//

bool execute_assignment(struct STMT* stmt, struct RAM* memory) {
  char* var_name = stmt->types.assignment->var_name; 
  char* string_rhs = NULL; 
  int line = stmt->line;

  struct VALUE* rhs = stmt->types.assignment->rhs; 
  struct RAM_VALUE i;

  if (rhs->value_type==VALUE_EXPR) {
    struct EXPR* expr = rhs->types.expr; 

    if (expr->isBinaryExpr) {
      int result; 
      bool success = execute_binary_expression(expr, &result, memory, line); 
      if (!success) {
        return false; 
      }
      i.types.i=result; 
      i.value_type=RAM_TYPE_INT; 
      ram_write_cell_by_name(memory, i, var_name); 
    } else {
    string_rhs = expr->lhs->element->element_value;
    if (expr->lhs->expr_type==UNARY_ELEMENT) {
      if (expr->lhs->element->element_type==ELEMENT_INT_LITERAL) {
          int num = atoi(string_rhs); 
          i.types.i=num; 
          i.value_type=RAM_TYPE_INT; 
          ram_write_cell_by_name(memory, i, var_name);
      } else if (expr->lhs->element->element_type==ELEMENT_IDENTIFIER) {
        struct RAM_VALUE* val = ram_read_cell_by_name(memory, string_rhs); 
        if (val==NULL) {
          printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_rhs, line);
          return false; 
        }
        i.types.i=val->types.i;
        i.value_type=RAM_TYPE_INT; 
        ram_write_cell_by_name(memory, i, var_name); 
      } 
    }
    }
  }
  return true; 
} 

//
// execute_function_call
//
// Executes a function call statement, handles print() with int literals, string literals,
// and identifiers (prints value assigned to identifier variable). Returns false if semantic error and stops execution, 
// returns true otherwise
// 
bool execute_function_call(struct STMT* stmt, struct RAM* memory) {
  struct ELEMENT* element = stmt->types.function_call->parameter; 
  int line = stmt->line; 


  if (element==NULL) {
    printf("\n"); 
  } else if (element->element_type==ELEMENT_INT_LITERAL) {
    char* str_literal = element->element_value; 
    int num_literal = atoi(str_literal); 
    printf("%d\n", num_literal); 
  } else if (element->element_type==ELEMENT_IDENTIFIER) {
    char* identifier = element->element_value;  
    struct RAM_VALUE* cell_ram_value = ram_read_cell_by_name(memory, identifier); 
    if (cell_ram_value==NULL) {
      printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", identifier, line); 
      return false; 
    }

    int return_value_from_memory = cell_ram_value->types.i; 
        printf("%d\n", return_value_from_memory);
  } else if (element->element_type==ELEMENT_STR_LITERAL) { 
    char* str_literal = element->element_value; 
    printf("%s\n", str_literal); 
  }
  return true; 
}

//
// execute
//
// Given a nuPython program graph and a memory, 
// executes the statements in the program graph.
// If a semantic error occurs (e.g. type error),
// and error message is output, execution stops,
// and the function returns.
//

void execute(struct STMT* program, struct RAM* memory)
{
  struct STMT* stmt = program; 
  while (stmt!=NULL) {
    if (stmt->stmt_type==STMT_ASSIGNMENT) {
      bool success = execute_assignment(stmt, memory);
      if (!success) {
        return; 
      }
      stmt=stmt->types.assignment->next_stmt; 
    } else if (stmt->stmt_type==STMT_FUNCTION_CALL) {
      bool success = execute_function_call(stmt, memory); 
      if (!success) {
        return; 
      }
      stmt=stmt->types.function_call->next_stmt; 
    } else if (stmt->stmt_type==STMT_PASS) {
      stmt=stmt->types.pass->next_stmt; //done for pass, just move onto next statement
    } 
  }
}
