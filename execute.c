/*execute.c*/

//
// Executes NuPython code in C - First part: executes only straight-line code - that is function calls, assignments, and pass statements
// Traverses the program graph, interprets the statements, and executes them by interacting with a RAM memory module for variable storage. 
// Second part: The executor now supports type int, real, string, and boolean. The executor now also supports a wider range of operand-operator combos and relationak 
// operators are also now supported. Function calls now extend from just print to including input, int, and float. Lastly, execution of while loops
// are supported. 
//
//LOG FOR MYSELF: Sunday - Friday afternoon: 26.5 hours 
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


// **IMPORTANT
// Union that allows functions to handle multiple types of return values: 
// int, double, and char*. By using this union, functions can return results of varying types through pass-by-reference. 
// Particularly useful for functions like execute_binary_expression, which need to process expressions 
// involving different types and return the result in a flexible manner.
typedef union {
    int i;       
    double d;    
    char* s;   
} ResultUnion;

//
// retrieve_value
//
// Helper function to retrieve the value from an expression 
// Used to decrypt both the lhs and rhs expressions - deals with all types: 
// int, real, str, identifier 
// Called in execute_binary_expression to compute lhs and rhs of binary expression 
// Returns false if semantic error (identifier not found in RAM), else true
//
bool retrieve_value(struct UNARY_EXPR* expr, ResultUnion* result, int* type, struct RAM* memory, int line) {
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
    struct RAM_VALUE* cell_ram_value; 
    if (expr->expr_type==UNARY_PTR_DEREF) { //handle ptr deref case, first get address that identifier is binded to, then use address to get actual value
      struct RAM_VALUE* address_val = ram_read_cell_by_name(memory, string_value); 
      if (address_val==NULL) {
        printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_value, line);
        return false;   
      }
      if (address_val->value_type!=RAM_TYPE_PTR) {
        printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
        return false; 
      }
      int address = address_val->types.i; 
      struct RAM_VALUE* pointer_deref_ram_value = ram_read_cell_by_addr(memory, address); 
      if (pointer_deref_ram_value==NULL) {
        printf("**SEMANTIC ERROR: '%s' contains invalid address (line %d)\n", string_value, line); 
        return false; 
      }
      cell_ram_value = pointer_deref_ram_value; 
    }
    if (expr->expr_type!=UNARY_PTR_DEREF) {
      cell_ram_value = ram_read_cell_by_name(memory, string_value); 
      if (cell_ram_value==NULL) {
        printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_value, line);
        return false; 
    }
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
    } else if (ram_type==RAM_TYPE_PTR) {
      result->i=cell_ram_value->types.i; 
      *type=RAM_TYPE_PTR; 
    }
  } 
  return true; 
}

//
// operator_int_evaluate
//
// Helper function that handles integer operations (lhs and rhs are integers)
// It takes care of common operations like addition, subtraction, multiplication, 
// division, modulus, and also comparison operators like equality or greater than.
// The function figures out what type the result is (int or boolean) and passes the result 
// and result type back to the caller (pass by reference)
// Throws error and returns false if there's a problem (div by zero or invalid operators)
//
bool operator_int_evaluate(struct EXPR* expr, int result_lhs, int result_rhs, int* result_int_operation, int* type, int line, bool p_arithmetic) {
  int res; 
  int operator = expr->operator; 
  if (operator==OPERATOR_PLUS) {
    res = result_lhs + result_rhs; 
    if (p_arithmetic) { // pointer arithmetic case: evaluate int res as normal but type should be returned to caller as RAM_TYPE_PTR
      *type=RAM_TYPE_PTR; 
    } else {
      *type=RAM_TYPE_INT; 
    }
  } else if (operator==OPERATOR_MINUS) {
    res = result_lhs - result_rhs; 
    if (p_arithmetic) {
      *type=RAM_TYPE_PTR; 
    } else {
      *type=RAM_TYPE_INT; 
    }
  } else if (operator==OPERATOR_ASTERISK) {
    res = result_lhs * result_rhs; 
    *type=RAM_TYPE_INT;
  } else if (operator==OPERATOR_DIV) {
    if (result_rhs==0) {
      printf("**SEMANTIC ERROR: ZeroDivisionError: division by zero (line %d)\n", line);
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
  } else {
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false; 
  }
  *result_int_operation = res; 
  return true; 
}

//
// operator_real_evaluate
//
// Helper function that handles real operations (lhs and rhs are real)
// It takes care of common operations like addition, subtraction, multiplication, 
// division, modulus, and also comparison operators like equality or greater than.
// The function figures out what type the result is (real or boolean) and passes the result 
// and result type back to the caller (pass by reference)
// Throws error and returns false if there's a problem (div by zero or invalid operators)
//
bool operator_real_evaluate(struct EXPR* expr, double result_lhs, double result_rhs, double* result_real_operation, int* type, int line) {
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
      printf("**SEMANTIC ERROR: ZeroDivisionError: division by zero (line %d)\n", line);
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
  } else { 
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false; 
  }
  *result_real_operation = res; 
  return true; 
}

//
// operator_str_concat_evaluate
//
// Helper function that handles string operations (lhs and rhs are strings)
// It takes care of operations like string concat and comparison operators like equality or greater than.
// The function figures out what type the result is (string or boolean) and passes the result 
// and result type back to the caller (pass by reference)
// Throws error and returns false if there's a problem (invalid operators)
//
bool operator_str_concat_evaluate(struct EXPR* expr, char* result_lhs, char* result_rhs, ResultUnion* result_string_operation, int* type, int line) {
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
  } else {
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false; 
  }
  return true; 
}

//
// execute_binary_expression 
//
// Executes binary expression by combining lhs and rhs values with appropriate operator 
// Makes use of helper functions retrieve_value, operator_int_evaluate, operator_real_evaluate, and operator_str_concat_evaluate
// Places answer in pass by reference variable result, and the type in result_type, returns false if there was a semantic error, else true
//
bool execute_binary_expression(struct EXPR* expr, ResultUnion* result, int* result_type, struct RAM* memory, int line) {
  struct UNARY_EXPR* lhs=expr->lhs; 
  struct UNARY_EXPR* rhs=expr->rhs; 

  if (lhs==NULL || rhs==NULL) {
    return false; 
  }

  ResultUnion result_lhs, result_rhs; 
  int type_lhs= -1; 
  int type_rhs = -1; 

  bool lhs_success = retrieve_value(lhs, &result_lhs, &type_lhs, memory, line); 
  bool rhs_success = retrieve_value(rhs, &result_rhs, &type_rhs, memory, line);

  if (!lhs_success || !rhs_success) {
    return false; 
  }

  if (type_lhs==-1 || type_rhs==-1) {
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false;
  }
  bool p_arithmetic = false; 

  if (type_lhs==RAM_TYPE_INT && type_rhs == RAM_TYPE_INT) {
    int result_int_operation; 
    int type; 
    bool success = operator_int_evaluate(expr, result_lhs.i, result_rhs.i, &result_int_operation, &type, line, p_arithmetic); 
    if (!success) {
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
    bool success = operator_real_evaluate(expr, result_lhs.d, result_rhs.d, &result_real_operation, &type, line); 
    if (!success) {
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
    bool success = operator_real_evaluate(expr, lhs_real, rhs_real, &result_real_operation, &type, line); 
    if (!success) {
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
    bool success = operator_str_concat_evaluate(expr, lhs_str, rhs_str, &result_string_operation, &type, line); 
    if (!success) {
      return false; 
    }
    if (type==RAM_TYPE_STR) {
      result->s = result_string_operation.s; 
      *result_type=RAM_TYPE_STR; 
    } else if (type==RAM_TYPE_BOOLEAN) {
      result->i=result_string_operation.i; 
      *result_type=RAM_TYPE_BOOLEAN; 
    }
  } else if (type_lhs==RAM_TYPE_PTR && type_rhs==RAM_TYPE_INT) { // pointer arithmetic case 
    bool p_arithmetic = true; 
    int result_int_operation; 
    int type; 
    bool success = operator_int_evaluate(expr, result_lhs.i, result_rhs.i, &result_int_operation, &type, line, p_arithmetic); 
    if (!success) {
      return false; 
    }
    result->i=result_int_operation; 
    *result_type=RAM_TYPE_PTR; 
  } else {  
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
    return false; 
  }
  return true; 
}

//
// execute_unary_expression 
//
// Executes unary expression - figures out type of expresion and appropriately assigns resulting value to the result union. 
// Handles int, str, real, boolean, and identifier literals
//
bool execute_unary_expression(struct EXPR* expr, struct RAM* memory, char* string_rhs, ResultUnion* result, int* result_type, int line, bool is_address, bool is_pointer_deref) {
  int assignment_type = expr->lhs->element->element_type; 
  if (assignment_type==ELEMENT_INT_LITERAL) {
    int num = atoi(string_rhs); 
    result->i=num; 
    *result_type=RAM_TYPE_INT; 
  } else if (assignment_type==ELEMENT_STR_LITERAL) {
    result->s=string_rhs; 
    *result_type=RAM_TYPE_STR; 
  } else if (assignment_type==ELEMENT_REAL_LITERAL) {
    double num = atof(string_rhs); 
    result->d=num; 
    *result_type=RAM_TYPE_REAL; 
  } else if (assignment_type==ELEMENT_TRUE) {
    result->i=1; 
    *result_type=RAM_TYPE_BOOLEAN; 
  } else if (assignment_type==ELEMENT_FALSE) {
    result->i=0; 
    *result_type=RAM_TYPE_BOOLEAN; 
  } else if (assignment_type==ELEMENT_IDENTIFIER) {
    struct RAM_VALUE* val; 
    if (is_address) {
      int address = ram_get_addr(memory, string_rhs); 
      if (address==-1) {
        printf("**SEMANTIC ERROR: name '%s' is not defined (line '%d')\n", string_rhs, line); 
        return false; 
      }
      // in the case of address, assignment binds the int address location of variable (using ram_get_addr)
      result->i=address; 
      *result_type=RAM_TYPE_PTR; 
      return true; 
    }
    if (is_pointer_deref) { //handle ptr deref case, first get address that identifier is binded to, then use address to get actual value
      struct RAM_VALUE* address_val = ram_read_cell_by_name(memory, string_rhs); 
      if (address_val==NULL) {
        printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_rhs, line);
        return false;   
      }
      if (address_val->value_type!=RAM_TYPE_PTR) {
        printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", line); 
        return false; 
      }
      int address = address_val->types.i; 
      struct RAM_VALUE* pointer_deref_ram_value = ram_read_cell_by_addr(memory, address); 
      if (pointer_deref_ram_value==NULL) {
        printf("**SEMANTIC ERROR: '%s' contains invalid address (line %d)\n", string_rhs, line); 
        return false; 
      }
      val=pointer_deref_ram_value; 
    }
    if (!is_pointer_deref) {
      val = ram_read_cell_by_name(memory, string_rhs); 
      if (val==NULL) {
        printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", string_rhs, line);
        return false; 
      }
    }
    int ram_type=val->value_type; 
    if (ram_type==RAM_TYPE_INT) {
      result->i=val->types.i; 
      *result_type=RAM_TYPE_INT; 
    } else if (ram_type==RAM_TYPE_REAL) {
      result->d=val->types.d; 
      *result_type=RAM_TYPE_REAL; 
    } else if (ram_type==RAM_TYPE_STR) {
      result->s=val->types.s; 
      *result_type=RAM_TYPE_STR;
    } else if( ram_type==RAM_TYPE_BOOLEAN) {
      result->i=val->types.i; 
      *result_type=RAM_TYPE_BOOLEAN;
    }
  }
  return true; 
}

//
// execute_expression
//
// Executes ANY expression, conditionally determines whether to execute_binary_expression or execute_unary_expression 
// based on expression type, returns result and result_type back to caller (execute_assignment)
//
bool execute_expression(struct EXPR* expr, struct RAM* memory, ResultUnion* result_main, int* result_type_main, int line) {
  ResultUnion result; 
  int result_type; 
  if (expr->isBinaryExpr) {
    bool success = execute_binary_expression(expr, &result, &result_type, memory, line); 
    if (!success) {
      return false; 
    }
  } else {
    bool is_address = false; 
    bool is_pointer_deref = false; 
    struct UNARY_EXPR* unary_expr = expr->lhs; 
    if (unary_expr->expr_type==UNARY_ADDRESS_OF) { // address case, i.e.: '&x'
      is_address=true; 
    }
    if (unary_expr->expr_type==UNARY_PTR_DEREF) {
      is_pointer_deref=true; 
    }
    char* string_rhs = expr->lhs->element->element_value;
    bool success = execute_unary_expression(expr, memory, string_rhs, &result, &result_type, line, is_address, is_pointer_deref); 
    if (!success) {
      return false; 
    }
  }
  *result_type_main=result_type; 
  if (result_type==RAM_TYPE_INT) {
    result_main->i=result.i; 
  } else if (result_type==RAM_TYPE_STR) {
    result_main->s=result.s; 
  } else if (result_type==RAM_TYPE_REAL) {
    result_main->d=result.d; 
  } else if (result_type==RAM_TYPE_BOOLEAN) {
    result_main->i=result.i; 
  } else if (result_type==RAM_TYPE_PTR) {
    result_main->i=result.i; 
  }
  return true; 
}


//
// execute_input
//
// Handles the input function, taking the user input, allocating space for it, and then writing this string value 
// to memory via a RAM_VALUE 
//
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

//
// execute_int
//
// Handles the int function, converting the value marked by the identifier to an int before storing it
// in memory, prints a semantic error and returns false if the int conversion fails
//
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


//
// execute_real
//
// Handles the real function, converting the value marked by the identifier to a real before storing it
// in memory, prints a semantic error and returns false if the real conversion fails
//
bool execute_real(struct VALUE* rhs, struct RAM* memory, char* var_name, int line) {
  char* identifier = rhs->types.function_call->parameter->element_value;
  struct RAM_VALUE* ram_return_value = ram_read_cell_by_name(memory, identifier); 
  struct RAM_VALUE i; 
  char* string_val = ram_return_value->types.s; 
  double string_to_real = atof(string_val); 
  if (string_to_real==0.0 && !(strspn(string_val, "0.")==strlen(string_val))) {
    printf("**SEMANTIC ERROR: invalid string for float() (line %d)\n", line); 
    return false; 
  }
  i.types.d=string_to_real; 
  i.value_type=RAM_TYPE_REAL; 
  ram_write_cell_by_name(memory, i, var_name); 
  return true; 
}

//
// execute_function
//
// Executes one of the three functions^ (input, real, float) with conditional branching based on the function name
// Propogates a false-return back to caller, otherwise returns true which signifies function successful function completion
//
bool execute_function(struct RAM* memory, char* var_name, struct FUNCTION_CALL* func_call, char* func_name, struct VALUE* rhs, int line) {
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
    return true; 
}

//
// create_ram_value
//
// Helper function to create a ram value given a result (of type ResultUnion) and the result's type
// Returns the ram value 
//
struct RAM_VALUE create_ram_value (ResultUnion result, int result_type) {
  struct RAM_VALUE i; 
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
  } else if (result_type==RAM_TYPE_PTR) {
    i.types.i=result.i; 
    i.value_type=RAM_TYPE_PTR; 
  }
  return i; 
}

//
// execute_assignment
//
// Executes an assignment statement, handles: 
// 1. Unary expressions
// 2. Binary expressions 
// 3. Assigning to functions input, float, and int
// Writes result of assignment RHS to memory under the var_name of the assignment 
// Returns false if any error propogates up to this point and stops execution, returns true otherwise 
//

bool execute_assignment(struct STMT* stmt, struct RAM* memory) {
  char* var_name = stmt->types.assignment->var_name; 
  int line = stmt->line;
  struct VALUE* rhs = stmt->types.assignment->rhs; 

  if (rhs->value_type==VALUE_EXPR) {
    struct EXPR* expr = rhs->types.expr; 
    ResultUnion result_main; 
    int result_type_main; 
    bool success = execute_expression(expr, memory, &result_main, &result_type_main, line); 
    if (!success) {
      return false; 
    }
    struct RAM_VALUE i = create_ram_value(result_main, result_type_main); 
    ram_write_cell_by_name(memory, i, var_name); 

  } else if (rhs->value_type==VALUE_FUNCTION_CALL) {
    struct FUNCTION_CALL* func_call=rhs->types.function_call; 
    char* func_name = func_call->function_name; 
    bool success = execute_function(memory, var_name, func_call, func_name, rhs, line); 
    if (!success) {
      return false; 
    }
  }
  return true; 
}



//
// execute_function_call
//
// Executes the print() function call which handles all types: int, real, str, boolean, identifier 
// Returns false if there was a semantic error (identifier name not previously written to memory), returns true otherwise
// 
bool execute_function_call(struct STMT* stmt, struct RAM* memory) {
  struct ELEMENT* element = stmt->types.function_call->parameter; 
  if (element==NULL) {
    printf("\n"); 
    return true; 
  }
  int line = stmt->line; 
  int elem_type = element->element_type; 

    if (elem_type==ELEMENT_INT_LITERAL) {
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
    } else if (ram_type==RAM_TYPE_PTR) {
      printf("%d\n", cell_ram_value->types.i); 
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
    } else if (stmt->stmt_type==STMT_WHILE_LOOP) {
      // **WHILE LOOP HANDLING, uses SAME function execute_expression as assignment execution, result and result type
      // written to result_while_loop and result_type. The expression is true if the result is NOT the integer 0, otherwise false. 
      // If the expression is true, stmt continues INSIDE the loop body, otherwise it skips the loop body and goes to the next statement. 
      struct STMT_WHILE_LOOP* while_loop = stmt->types.while_loop; 
      struct EXPR* while_loop_condition = stmt->types.while_loop->condition; 
      ResultUnion result_while_loop;  
      int result_type; 
      bool success = execute_expression(while_loop_condition, memory, &result_while_loop, &result_type, stmt->line); 
      if (!success) {
        return; 
      }
      bool condition = ((result_type==RAM_TYPE_BOOLEAN || result_type==RAM_TYPE_INT) && result_while_loop.i!=0); 
      if (condition) {
        stmt=while_loop->loop_body; 
      } else {
        stmt=while_loop->next_stmt; 
      }
    } else if (stmt->stmt_type==STMT_PASS) {
      stmt=stmt->types.pass->next_stmt; //done for pass, just move onto next statement
    } 
  }
}