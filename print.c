#include "lexing.h"
#include "parsing.h"

int indent;

void print_newline(void)
{
    printf("\n%.*s", 2 * indent, "                                                                               ");
}

void print_declaration(decl* declaration);
void print_statement_block(stmt_block block);

void print_expression(expr* e)
{
    if (e == NULL)
    {
        return;
    }

    switch (e->kind)
    {
        case EXPR_INT:
        {
            printf("%d", e->number_value);
        }
        break;
        case EXPR_NAME:
        {
            printf("%s", e->identifier);
        }
        break;
        case EXPR_SIZEOF:
        {
            printf("(sizeof %s)", e->identifier);
        }
        break;
        case EXPR_UNARY:
        {
            printf("(");
            if (e->unary_expr_value.operator == TOKEN_MUL)
            {
                printf("pointer-dereference");
            }
            else
            {
                print_token_kind(e->unary_expr_value.operator);
            }
            printf(" ");
            print_expression(e->unary_expr_value.operand);
            printf(")");
        }
        break;
        case EXPR_BINARY:
        {
            printf("(");
            print_token_kind(e->binary_expr_value.operator);
            printf(" ");
            print_expression(e->binary_expr_value.left_operand);
            printf(" ");
            print_expression(e->binary_expr_value.right_operand);
            printf(")");
        }
        break;
        case EXPR_INDEX:
        {
            printf("(access-index ");
            print_expression(e->index_expr_value.index_expr);
            printf(" ");
            print_expression(e->index_expr_value.array_expr);
            printf(")");
        }
        break;
        case EXPR_CALL:
        {
            printf("(");
            print_expression(e->call_expr_value.function_expr);
            printf(" ");
            for (size_t i = 0; i < e->call_expr_value.args_num; i++)
            {
                print_expression(e->call_expr_value.args[i]);
                if (i != e->call_expr_value.args_num - 1)
                {
                    printf(" ");
                }
            }
            printf(")");
        }
        break;
        case EXPR_FIELD:
        {
            printf("(access-field ");
            print_expression(e->field_expr_value.expr);
            printf(" ");
            printf("%s", e->field_expr_value.field_name);
            printf(")");
        }
        break;
        case EXPR_TERNARY:
        {
            printf("(? ");
            print_expression(e->ternary_expr_value.condition);
            
            indent++;
            print_newline();
            print_expression(e->ternary_expr_value.if_true);
            print_newline();
            print_expression(e->ternary_expr_value.if_false);
            
            indent--;
            print_newline();
            printf(")");
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            printf("(compound ");
            for (size_t i = 0; i < e->compound_literal_expr_value.fields_count; i++)
            {
                compound_literal_field* f = e->compound_literal_expr_value.fields[i];
                if (f->field_name)
                {
                    printf("(%s ", f->field_name);
                    print_expression(f->expr);
                    printf(")");
                }
                else
                {
                    print_expression(f->expr);
                }           

                if (i != e->compound_literal_expr_value.fields_count - 1)
                {
                    printf(" ");
                }
            }
            printf(")");
        }
        break;
        default:
        {
            fatal("expression kind not handled: %d", e->kind);
        }
        break;
    }
}

void print_statement(stmt* s)
{
    if (s == NULL)
    {
        return;
    }

    switch (s->kind)
    {
        case STMT_RETURN:
        {
            printf("(return ");
            print_expression(s->return_statement.expression);
            printf(")");
        }
        break;
        case STMT_BREAK:
        {
            printf("(break)");
        }
        break;
        case STMT_CONTINUE:
        {
            printf("(continue)");
        }
        break;
        case STMT_LIST:
        {

        }
        break;
        case STMT_IF_ELSE:
        {
            printf("(if ");
            print_expression(s->if_else_statement.cond_expr);
            
            print_statement_block(s->if_else_statement.then_block);
           
            if (s->if_else_statement.else_stmt)
            {
                indent++;
                print_newline();
                printf("else");

                if (s->if_else_statement.else_stmt->kind == STMT_BLOCK)
                {
                    print_statement(s->if_else_statement.else_stmt);
                }
                else
                {
                    indent++;
                    print_newline();
                    print_statement(s->if_else_statement.else_stmt);
                    indent--;
                }                
              
                indent--;
            }

            print_newline();
            printf(")");
        }
        break;
        case STMT_WHILE:
        {
            printf("(while ");
            print_expression(s->while_statement.cond_expr);

            print_statement_block(s->while_statement.statements);

            print_newline();
            printf(")");
        }
        break;
        case STMT_DO_WHILE:
        {
            printf("(do-while ");
            print_expression(s->do_while_statement.cond_expr);;

            print_statement_block(s->do_while_statement.statements);

            print_newline();
            printf(")");
        }
        break;
        case STMT_BLOCK:
        {
            print_statement_block(s->statements_block);
        }
        break;
        case STMT_ASSIGN:
        {
            printf("(");
            print_token_kind(s->assign_statement.operation);
            printf(" ");
            print_expression(s->assign_statement.assigned_var_expr);
            if (s->assign_statement.value_expr)
            {
                printf(" ");
                print_expression(s->assign_statement.value_expr);
            }
            printf(")");
        }
        break;
        case STMT_FOR:
        {
            printf("(for ");
            print_declaration(s->for_statement.init_decl);
            printf(" ");
            print_expression(s->for_statement.cond_expr);
            printf(" ");
            print_statement(s->for_statement.incr_stmt);
           
            print_statement_block(s->for_statement.statements);
            
            print_newline();
            printf(")");
        }
        break;
        case STMT_SWITCH:
        {
            printf("(switch ");
            print_expression(s->switch_statement.var_expr);

            indent++;

            for (size_t i = 0; i < s->switch_statement.cases_num; i++)
            {
                print_newline();
                printf("(case ");
                switch_case* c = s->switch_statement.cases[i];
                for (size_t j = 0; j < c->cond_exprs_num; j++)
                {
                    expr* e = c->cond_exprs[j];
                    print_expression(e);
                    if (j != c->cond_exprs_num - 1)
                    {
                        printf(" ");
                    }
                }
                if (c->is_default)
                {
                    printf(" default");
                }
                if (c->fallthrough)
                {
                    printf(" fallthrough");
                }

                
                print_statement_block(c->statements);
                
                print_newline();
                printf(")");
            }

            indent--;
            print_newline();
            printf(")");
        }
        break;
        case STMT_DECL:
        {
            print_declaration(s->decl_statement.decl);
        }
        break;
        case STMT_EXPR:
        {
            print_expression(s->return_statement.expression);
        }
        break;
    }
}

void print_statement_block(stmt_block block)
{
    indent++;
    for (int index = 0; index < block.statements_count; index++)
    {
        print_newline();
        print_statement(block.statements[index]);        
    }
    indent--;
}

void print_type(type* t)
{
    if (t != 0)
    {
        switch (t->kind)
        {
            case TYPE_NAME:
            {
                printf("(type %s)", t->name);
            };
            break;
            case TYPE_ARRAY:
            {
                printf("(array ");
                print_type(t->array.base_type);
                printf(" ");
                print_expression(t->array.size_expr);
                printf(")");
            };
            break;
            case TYPE_POINTER:
            {
                printf("(pointer ");
                print_type(t->pointer.base_type);
                printf(")");
            };
            break;
            case TYPE_FUNCTION:
            {
                printf("(function (");                
                for (size_t i = 0; i < t->function.parameter_count; i++)
                {
                    type* p = t->function.parameter_types[i];
                    print_type(p);
                    if (i != t->function.parameter_count - 1)
                    {
                        printf(" ");
                    }
                }
                printf(")");
                if (t->function.returned_type)
                {
                    printf(" ");
                    print_type(t->function.returned_type);
                }
                printf(")");
            };
            break;
        }
    }
}

void print_declaration(decl* declaration)
{
    if (declaration == NULL)
    {
        return;
    }

    switch (declaration->kind)
    {
        case DECL_FUNCTION:
        {
            printf("(fn-decl ");
            if (declaration->identifier)
            {
                printf("%s", declaration->identifier);
            }

            if (declaration->function_declaration.parameters.param_count > 0)
            {
                printf(" ");
                for (int index = 0; index < declaration->function_declaration.parameters.param_count; index++)
                {
                    function_param* p = &declaration->function_declaration.parameters.params[index];
                    printf("(%s ", p->identifier);
                    print_type(p->type);
                    printf(")");

                    if (index < declaration->function_declaration.parameters.param_count - 1)
                    {
                        printf(" ");
                    }
                }
            }

            if (declaration->function_declaration.return_type)
            {
                printf(" ");
                print_type(declaration->function_declaration.return_type);
            }
            
            print_statement_block(declaration->function_declaration.statements);

            print_newline();
            printf(")");
        }
        break;
        case DECL_VARIABLE:
        {
            printf("(var-decl ");

            if (declaration->identifier)
            {
                printf("%s", declaration->identifier);
            }

            if (declaration->variable_declaration.type)
            {
                printf(" ");
                print_type(declaration->variable_declaration.type);
            }

            if (declaration->variable_declaration.expression)
            {
                printf(" ");
                print_expression(declaration->variable_declaration.expression);
            }

            printf(")");
        }
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            if (declaration->kind == DECL_UNION)
            {
                printf("(union-decl");
            }
            else
            {
                printf("(struct-decl");
            }

            if (declaration->identifier)
            {
                printf(" %s", declaration->identifier);
            }

            indent++;

            for (size_t index = 0;
                index < declaration->aggregate_declaration.fields_count;
                index++)
            {
                print_newline();
                printf("(%s", declaration->aggregate_declaration.fields[index].identifier);
                printf(" ");
                print_type(declaration->aggregate_declaration.fields[index].type);
                printf(")");
            }

            indent--;
            print_newline();
            printf(")");
        }
        break;
        case DECL_ENUM:
        {
            printf("(enum-decl");

            indent++;

            for (size_t index = 0;
                index < declaration->enum_declaration.values_count;
                index++)
            {
                print_newline();
                enum_value* value = &declaration->enum_declaration.values[index];
                printf("(%s", value->identifier);
                if (value->value_set)
                {
                    printf(" %d", value->value);
                }
                printf(")");
            }

            indent--;
            print_newline();
            printf(")");
        }
        break;
        case DECL_TYPEDEF:
        {
            printf("(typedef-decl %s", declaration->typedef_declaration.name);
            printf(" ");
            print_type(declaration->typedef_declaration.type);
            printf(")");
        }
        break;
        default:
        {
            assert("invalid default case");
        }
        break;
    }
}
