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



typedef union {
    int i;       
    double d;    
    char* s;   
} ResultUnion;

//
// retrieve_value
//
// Helper function to retrieve the integer value from an expression 
// Used to decrypt both the lhs and rhs expressions - deals with two cases:
// int_literal and identifier (for which interaction with the RAM module is necessary)
// Called in execute_binary_expression to compute lhs and rhs of binary expression 
// Returns false if semantic error (identifier not found in RAM), else true
//
bool retrive_value(struct UNARY_EXPR* expr, ResultUnion* result, int* type, struct RAM* memory, int line) {
  char* string_value = expr->element->element_value; 
  int expr_type = expr->element->element_type; 
  if (expr_type==ELEMENT_INT_LITERAL) {
    int num = atoi(string_value); 
    result->i=num; 
    *type=RAM_TYPE_INT; 
  } else if (expr_type==ELEMENT_REAL_LITERAL) {
    double num = atof(string_value); 
    result->d=num;  
    *type=RAM_TYPE_REAL; 
  } else if (expr_type==ELEMENT_STR_LITERAL) {
    result->s=string_value; 
    *type=RAM_TYPE_STR; 
  } else if (expr_type==ELEMENT_IDENTIFIER) {
    struct RAM_VALUE* cell_ram_value = ram_read_cell_by_name(memory, string_value); 
    if (cell_ram_value==NULL) {
      printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_value, line);
      return false; 
    }
    int ram_type = cell_ram_value->value_type; 
    if (ram_type==RAM_TYPE_INT) {
      result->i=cell_ram_value->types.i;
      *type=RAM_TYPE_INT; 
    } else if (ram_type==RAM_TYPE_REAL) {
      result->d=cell_ram_value->types.d; 
      *type=RAM_TYPE_REAL; 
    } else if (ram_type==RAM_TYPE_STR) {
      result->s = cell_ram_value->types.s;
      *type = RAM_TYPE_STR;
    }
  }
  return true; 
}

bool operator_int_evaluate(struct EXPR* expr, int result_lhs, int result_rhs, int* result_int_operation, int* type) {
  int res; 
  int operator = expr->operator; 
  if (operator==OPERATOR_PLUS) {
    res = result_lhs + result_rhs; 
    *type=RAM_TYPE_INT; 
  } else if (operator==OPERATOR_MINUS) {
    res = result_lhs - result_rhs; 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_ASTERISK) {
    res = result_lhs * result_rhs; 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_DIV) {
    if (result_rhs==0) {
      return false; 
    }
    res = result_lhs / result_rhs; 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_MOD) {
    res = result_lhs % result_rhs; 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_POWER) {
    res = (int)pow(result_lhs, result_rhs); 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_EQUAL) {
    res = (double)(result_lhs==result_rhs); 
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_NOT_EQUAL) {
    res = (double)(result_lhs!=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_LT) {
    res = (double)(result_lhs<result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_LTE) {
    res = (double)(result_lhs<=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_GT) {
    res = (double)(result_lhs>result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_GTE) {
    res = (double)(result_lhs>=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  }
  *result_int_operation = res; 
  return true; 
}

bool operator_real_evaluate(struct EXPR* expr, double result_lhs, double result_rhs, double* result_real_operation, int* type) {
  double res; 
  int operator = expr->operator; 
  if (operator==OPERATOR_PLUS) {
    res = result_lhs + result_rhs; 
    *type=RAM_TYPE_REAL; 
  } else if (operator==OPERATOR_MINUS) {
    res = result_lhs - result_rhs; 
    *type=RAM_TYPE_REAL;
  } else if (operator==OPERATOR_ASTERISK) {
    res = result_lhs * result_rhs; 
    *type=RAM_TYPE_REAL;
  } else if (operator==OPERATOR_DIV) {
    if (result_rhs==0.0) {
      return false; 
    }
    res = result_lhs / result_rhs; 
    *type=RAM_TYPE_REAL;
  } else if (operator==OPERATOR_MOD) {
    res = fmod(result_lhs, result_rhs); 
    *type=RAM_TYPE_REAL;
  } else if (operator==OPERATOR_POWER) {
    res = pow(result_lhs, result_rhs); 
    *type=RAM_TYPE_REAL;
  } else if (operator==OPERATOR_EQUAL) {
    res = (result_lhs==result_rhs); 
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_NOT_EQUAL) {
    res = (result_lhs!=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_LT) {
    res = (result_lhs<result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_LTE) {
    res = (result_lhs<=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_GT) {
    res = (result_lhs>result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  } else if (operator==OPERATOR_GTE) {
    res = (result_lhs>=result_rhs); 
    *type=RAM_TYPE_BOOLEAN;
  }
  *result_real_operation = res; 
  return true; 
}

//expr, lhs_str, rhs_str, &result_string_operation, &type
bool operator_str_concat_evaluate(struct EXPR* expr, char* result_lhs, char* result_rhs, ResultUnion* result_string_operation, int* type) {
  int operator = expr->operator; 
  int str_comp = strcmp(result_lhs, result_rhs); 
  if (operator==OPERATOR_PLUS) {
    size_t length_lhs = strlen(result_lhs); 
    size_t length_rhs = strlen(result_rhs);
    int concat_length = length_lhs + length_rhs + 1; 
    char* concat = (char*)malloc(concat_length*sizeof(char)); 
    strcpy(concat, result_lhs); 
    strcat(concat, result_rhs); 
    result_string_operation->s=concat; 
    *type=RAM_TYPE_STR; 
  } else if (operator==OPERATOR_EQUAL) {
    result_string_operation->i = (str_comp==0) ? 1 : 0; 
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_NOT_EQUAL) {
    result_string_operation->i = (str_comp!=0) ? 1 : 0;
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_LT) {
    result_string_operation->i = (str_comp<0) ? 1 : 0;
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_LTE) {
    result_string_operation->i = (str_comp<=0) ? 1 : 0; 
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_GT) {
    result_string_operation->i = (str_comp>0) ? 1 : 0; 
    *type=RAM_TYPE_BOOLEAN; 
  } else if (operator==OPERATOR_GTE) {
    result_string_operation->i = (str_comp>=0) ? 1 : 0; 
    *type=RAM_TYPE_BOOLEAN; 
  }
  return true; 
}

//
// execute_binary_expression 
//
// Executes binary expression by combining lhs and rhs values with appropriate operator 
// Places answer in pass by reference variable result, and returns false if semantic error, else true
//
bool execute_binary_expression(struct EXPR* expr, ResultUnion* result, int* result_type, struct RAM* memory, int line) {
  struct UNARY_EXPR* lhs=expr->lhs; 
  struct UNARY_EXPR* rhs=expr->rhs; 

  ResultUnion result_lhs, result_rhs; 
  int type_lhs, type_rhs; 

  bool lhs_success = retrive_value(lhs, &result_lhs, &type_lhs, memory, line); 
  bool rhs_success = retrive_value(rhs, &result_rhs, &type_rhs, memory, line);

  if (!lhs_success || !rhs_success) {
    return false; 
  }


  if (type_lhs==RAM_TYPE_INT && type_rhs == RAM_TYPE_INT) {
    int result_int_operation; 
    int type; 
    bool success = operator_int_evaluate(expr, result_lhs.i, result_rhs.i, &result_int_operation, &type); 
    if (!success) {
      printf("**SEMANTIC ERROR: ZeroDivisionError: division by zero (line %d)\n", line);
      return false; 
    }
    result->i=result_int_operation;
    if (type==RAM_TYPE_INT) {
      *result_type=RAM_TYPE_INT; 
    }  else if (type==RAM_TYPE_BOOLEAN) {
      *result_type=RAM_TYPE_BOOLEAN; 
    }
  } else if (type_lhs==RAM_TYPE_REAL && type_rhs==RAM_TYPE_REAL) {
    double result_real_operation; 
    int type; 
    bool success = operator_real_evaluate(expr, result_lhs.d, result_rhs.d, &result_real_operation, &type); 
    if (!success) {
      printf("**SEMANTIC ERROR: ZeroDivisionError: division by zero (line %d)\n", line);
      return false; 
    }
    if (type==RAM_TYPE_REAL) {
      result->d=result_real_operation; 
      *result_type=RAM_TYPE_REAL; 
    } else if (type==RAM_TYPE_BOOLEAN) {
      result->i=(int)result_real_operation; 
      *result_type=RAM_TYPE_BOOLEAN; 
    }
  } else if ((type_lhs == RAM_TYPE_INT && type_rhs == RAM_TYPE_REAL) || (type_lhs == RAM_TYPE_REAL && type_rhs == RAM_TYPE_INT)) {
    double lhs_real =(type_lhs == RAM_TYPE_INT) ? (double)result_lhs.i : result_lhs.d;
    double rhs_real = (type_rhs == RAM_TYPE_INT) ? (double)result_rhs.i : result_rhs.d;
    double result_real_operation; 
    int type; 
    bool success = operator_real_evaluate(expr, lhs_real, rhs_real, &result_real_operation, &type); 
    if (!success) {
      printf("**SEMANTIC ERROR: ZeroDivisionError: division by zero (line %d)\n", line);
      return false; 
    }
    if (type==RAM_TYPE_REAL) {
      result->d=result_real_operation; 
      *result_type=RAM_TYPE_REAL; 
    } else if (type==RAM_TYPE_BOOLEAN) {
      result->i=(int)result_real_operation; 
      *result_type=RAM_TYPE_BOOLEAN; 
    }
  } else if (type_lhs==RAM_TYPE_STR && type_rhs==RAM_TYPE_STR) {
    char* lhs_str = result_lhs.s;
    char* rhs_str = result_rhs.s;
    ResultUnion result_string_operation; 
    int type; 
    bool success = operator_str_concat_evaluate(expr, lhs_str, rhs_str, &result_string_operation, &type); 
    if (!success) {
      //Print semantic error here 
      return false; 
    }
    if (type==RAM_TYPE_STR) {
      result->s = result_string_operation.s; 
      *result_type=RAM_TYPE_STR; 
    } else if (type==RAM_TYPE_BOOLEAN) {
      result->i=result_string_operation.i; 
      *result_type=RAM_TYPE_BOOLEAN; 
    }
  } else {
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false; 
  }
  return true; 
}



void execute_input(struct VALUE* rhs, struct RAM* memory, char* var_name) {
    struct FUNCTION_CALL* func = rhs->types.function_call; 
    char* func_name = func->function_name; 
    printf("%s", func->parameter->element_value);

    char line[256]; 
    fgets(line, sizeof(line), stdin); 
    line[strcspn(line, "\r\n")] = '\0';

    char* input_string = (char*)malloc(sizeof(line)+1); 
    strcpy(input_string, line); 
    struct RAM_VALUE i; 
    i.types.s=input_string; 
    i.value_type=RAM_TYPE_STR; 
    ram_write_cell_by_name(memory, i, var_name);
}

bool execute_int(struct VALUE* rhs, struct RAM* memory, char* var_name, int line) {
  char* identifier = rhs->types.function_call->parameter->element_value;
  struct RAM_VALUE* ram_return_value = ram_read_cell_by_name(memory, identifier); 
  struct RAM_VALUE i; 
  char* string_val = ram_return_value->types.s; 
  int string_to_num = atoi(string_val); 
  if (string_to_num==0 && !(strspn(string_val, "0")==strlen(string_val))) {
    printf("**SEMANTIC ERROR: invalid string for int() (line %d)\n", line); 
    return false; 
  }
  i.types.i=string_to_num; 
  i.value_type=RAM_TYPE_INT; 
  ram_write_cell_by_name(memory, i, var_name); 
  return true; 
}

bool execute_real(struct VALUE* rhs, struct RAM* memory, char* var_name, int line) {
  char* identifier = rhs->types.function_call->parameter->element_value;
  struct RAM_VALUE* ram_return_value = ram_read_cell_by_name(memory, identifier); 
  struct RAM_VALUE i; 
  char* string_val = ram_return_value->types.s; 
  double string_to_real = atof(string_val); 
  if (string_to_real==0 && !(strspn(string_val, "0")==strlen(string_val))) {
    printf("**SEMANTIC ERROR: invalid string for float() (line %d)\n", line); 
    return false; 
  }
  i.types.d=string_to_real; 
  i.value_type=RAM_TYPE_REAL; 
  ram_write_cell_by_name(memory, i, var_name); 
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
      ResultUnion result; 
      int result_type; 
      bool success = execute_binary_expression(expr, &result, &result_type, memory, line); 
      if (!success) {
        return false; 
      }
      if (result_type==RAM_TYPE_INT) {
        i.types.i=result.i; 
        i.value_type=RAM_TYPE_INT; 
      } else if (result_type==RAM_TYPE_REAL) {
        i.types.d=result.d; 
        i.value_type=RAM_TYPE_REAL; 
      } else if (result_type==RAM_TYPE_STR) {
        i.types.s=result.s; 
        i.value_type=RAM_TYPE_STR; 
      } else if (result_type==RAM_TYPE_BOOLEAN) {
        i.types.i=result.i; 
        i.value_type=RAM_TYPE_BOOLEAN; 
      }
      ram_write_cell_by_name(memory, i, var_name); 
    } else {

    string_rhs = expr->lhs->element->element_value;

    if (expr->lhs->expr_type==UNARY_ELEMENT) {

      int assignment_type = expr->lhs->element->element_type; 
      if (assignment_type==ELEMENT_INT_LITERAL) {
          int num = atoi(string_rhs); 
          i.types.i=num; 
          i.value_type=RAM_TYPE_INT; 
          ram_write_cell_by_name(memory, i, var_name);
      } else if (assignment_type==ELEMENT_STR_LITERAL) {
        i.types.s=string_rhs; 
        i.value_type=RAM_TYPE_STR; 
        ram_write_cell_by_name(memory, i, var_name); 
      } else if (assignment_type==ELEMENT_REAL_LITERAL) {
        double num = atof(string_rhs); 
        i.types.d=num; 
        i.value_type=RAM_TYPE_REAL; 
        ram_write_cell_by_name(memory, i, var_name); 
      } else if (assignment_type==ELEMENT_TRUE) {
        i.types.i=1; 
        i.value_type=RAM_TYPE_BOOLEAN; 
        ram_write_cell_by_name(memory, i, var_name); 
      } else if (assignment_type==ELEMENT_FALSE) {
        i.types.i=0; 
        i.value_type=RAM_TYPE_BOOLEAN; 
        ram_write_cell_by_name(memory, i, var_name); 
      } else if (assignment_type==ELEMENT_IDENTIFIER) {
        struct RAM_VALUE* val = ram_read_cell_by_name(memory, string_rhs); 
        if (val==NULL) {
          printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_rhs, line);
          return false; 
        }
        int ram_type=val->value_type; 
        if (ram_type==RAM_TYPE_INT) {
          i.types.i=val->types.i; 
          i.value_type=RAM_TYPE_INT; 
        } else if (ram_type==RAM_TYPE_REAL) {
          i.types.d=val->types.d; 
          i.value_type=RAM_TYPE_REAL;
        } else if (ram_type==RAM_TYPE_STR) {
          i.types.s=val->types.s; 
          i.value_type=RAM_TYPE_STR;
        } else if (ram_type==RAM_TYPE_BOOLEAN) {
          i.types.i=val->types.i; 
          i.value_type=RAM_TYPE_BOOLEAN;
        }
        ram_write_cell_by_name(memory, i, var_name); 
      } 
    }
    }
  } else if (rhs->value_type==VALUE_FUNCTION_CALL) {
    struct FUNCTION_CALL* func_call=rhs->types.function_call; 
    char* func_name = func_call->function_name; 
    if (strcmp(func_name, "input")==0) {
      execute_input(rhs, memory, var_name); 
    } else if (strcmp(func_name, "int")==0) {
      bool success = execute_int(rhs, memory, var_name, line); 
      if (!success) {
        return false; 
      }
    } else if (strcmp(func_name, "float")==0) {
      bool success = execute_real(rhs, memory, var_name, line); 
      if (!success) {
        return false; 
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
  int elem_type = element->element_type; 

  if (element==NULL) {
    printf("\n"); 
  } else if (elem_type==ELEMENT_INT_LITERAL) {
    char* str_literal = element->element_value;
    int num = atoi(str_literal); 
    printf("%d\n", num);
  } else if (elem_type == ELEMENT_REAL_LITERAL) {
    char* str_literal = element->element_value;
    double num = atof(str_literal); 
    printf("%f\n", num); 
  } else if (elem_type==ELEMENT_STR_LITERAL) { 
    char* str_literal = element->element_value; 
    printf("%s\n", str_literal); 
  } else if (elem_type==ELEMENT_TRUE) {
    printf("True\n");
  } else if (elem_type==ELEMENT_FALSE) {
    printf("False\n");
  } else if (elem_type==ELEMENT_IDENTIFIER) {
    char* identifier = element->element_value;  
    struct RAM_VALUE* cell_ram_value = ram_read_cell_by_name(memory, identifier); 
    if (cell_ram_value==NULL) {
      printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", identifier, line); 
      return false; 
    }
    int ram_type = cell_ram_value->value_type; 
    if (ram_type==RAM_TYPE_REAL) {
      printf("%f\n", cell_ram_value->types.d);
    } else if (ram_type==RAM_TYPE_INT) {
        printf("%d\n", cell_ram_value->types.i);
    } else if (ram_type==RAM_TYPE_STR) {
      printf("%s\n", cell_ram_value->types.s);
    } else if (ram_type==RAM_TYPE_BOOLEAN) {
      if (cell_ram_value->types.i==1) {
        printf("True\n");
      } else {
        printf("False\n"); 
      }
    }
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
