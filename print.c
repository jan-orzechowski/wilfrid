#include "lexing.h"
#include "parsing.h"

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
            printf(" %d", e->number_value);
        }
        break;
        case EXPR_NAME:
        {
            printf(" %s", e->identifier);
        }
        break;
        case EXPR_SIZEOF:
        {
            printf("( sizeof %s)", e->identifier);
        }
        break;
        case EXPR_UNARY:
        {
            printf("(");
            if (e->binary_expr_value.operator == TOKEN_MUL)
            {
                printf("pointer dereference");
            }
            else
            {
                print_token_kind(e->binary_expr_value.operator);
            }
            print_expression(e->unary_expr_value.operand);
            printf(")");
        }
        break;
        case EXPR_BINARY:
        {
            printf("(");
            print_token_kind(e->binary_expr_value.operator);
            print_expression(e->binary_expr_value.left_operand);
            print_expression(e->binary_expr_value.right_operand);
            printf(")");
        }
        break;
        case EXPR_INDEX:
        {
            printf("(access index");
            print_expression(e->index_expr_value.index_expr);
            printf(" in");
            print_expression(e->index_expr_value.array_expr);
            printf(")");
        }
        break;
        case EXPR_CALL:
        {
            printf("(call");
            print_expression(e->call_expr_value.function_expr);
            printf(" with args (");
            for (size_t i = 0; i < e->call_expr_value.args_num; i++)
            {
                print_expression(e->call_expr_value.args[i]);
                if (i != e->call_expr_value.args_num - 1)
                {
                    printf(", ");
                }
            }
            printf("))");
        }
        break;
        case EXPR_FIELD:
        {
            printf("(access field ");
            printf("%s", e->field_expr_value.field_name);
            printf(" in");
            print_expression(e->field_expr_value.expr);
            printf(")");
        }
        break;
        case EXPR_TERNARY:
        {
            printf("(ternary condition:(");
            print_expression(e->ternary_expr_value.condition);
            printf(") if true:(");
            print_expression(e->ternary_expr_value.if_true);
            printf(") if false: (");
            print_expression(e->ternary_expr_value.if_false);
            printf("))");
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            printf("(literal: ");
            for (size_t i = 0; i < e->compound_literal_expr_value.fields_count; i++)
            {
                printf("(");
                compound_literal_field* f = e->compound_literal_expr_value.fields[i];
                if (f->field_name)
                {
                    printf("%s:", f->field_name);
                    print_expression(f->expr);
                }
                else
                {
                    print_expression(f->expr);
                }
                printf(")");
                if (i != e->compound_literal_expr_value.fields_count - 1)
                {
                    printf(", ");
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
            printf("return: (");
            print_expression(s->return_statement.expression);
            printf(")");
        };
        break;
        case STMT_BREAK:
        {
            printf("break ");
        };
        break;
        case STMT_CONTINUE:
        {
            printf("continue ");
        };
        break;
        case STMT_LIST:
        {

        };
        break;
        case STMT_IF_ELSE:
        {
            printf("if cond:(");
            print_expression(s->if_else_statement.cond_expr);
            printf(") then:");
            printf("{");
            print_statement_block(s->if_else_statement.then_block);
            printf("}");
            printf(")");
            if (s->if_else_statement.else_stmt)
            {
                printf("else: {");
                print_statement(s->if_else_statement.else_stmt);
                printf("}");
            }
            printf(")");
        };
        break;
        case STMT_WHILE:
        {
            printf("while cond:(");
            print_expression(s->while_statement.cond_expr);
            printf("{");
            print_statement_block(s->while_statement.statements);
            printf("}");
        };
        break;
        case STMT_DO_WHILE:
        {
            printf("do while cond:(");
            print_expression(s->do_while_statement.cond_expr);
            printf("{");
            print_statement_block(s->do_while_statement.statements);
            printf("}");
        };
        break;
        case STMT_BLOCK:
        {
            printf("{");
            print_statement_block(s->statements_block);
            printf("}");
        };
        break;
        case STMT_ASSIGN:
        {
            print_token_kind(s->assign_statement.operation);
            printf("(");
            print_expression(s->assign_statement.assigned_var_expr);
            print_expression(s->assign_statement.value_expr);
            printf(")");
        };
        break;
        case STMT_FOR:
        {
            printf("(for: (");
            print_declaration(s->for_statement.init_decl);
            print_expression(s->for_statement.cond_expr);
            print_statement(s->for_statement.incr_stmt);
            printf(")");
            printf("{");
            print_statement_block(s->for_statement.statements);
            printf("}");
            printf(")");
        };
        break;
        case STMT_SWITCH:
        {
            printf("(switch: (");
            print_expression(s->switch_statement.var_expr);
            printf(")");
            for (size_t i = 0; i < s->switch_statement.cases_num; i++)
            {
                printf("(case: (conditions:");
                switch_case* c = s->switch_statement.cases[i];
                for (size_t j = 0; j < c->cond_exprs_num; j++)
                {
                    expr* e = c->cond_exprs[j];
                    print_expression(e);
                    if (j != c->cond_exprs_num - 1)
                    {
                        printf(", ");
                    }
                }
                if (c->is_default)
                {
                    printf(", default");
                }
                if (c->fallthrough)
                {
                    printf(", fallthrough");
                }
                printf(")");

                printf("{");
                print_statement_block(c->statements);
                printf("}");
            }
            printf(")");
        };
        break;
        case STMT_DECL:
        {
            print_declaration(s->decl_statement.decl);
        }
        break;
        case STMT_EXPR:
        {
            print_expression(s->return_statement.expression);
        };
        break;
    }
}

void print_statement_block(stmt_block block)
{
    for (int index = 0; index < block.statements_count; index++)
    {
        print_statement(block.statements[index]);
        if (index != block.statements_count - 1)
        {
            printf(", ");
        }
    }
}

void print_type(type* t)
{
    if (t != 0)
    {
        switch (t->kind)
        {
            case TYPE_NAME:
            {
                printf("type: %s", t->name);
            };
            break;
            case TYPE_ARRAY:
            {
                printf(" array of length: ");
                print_expression(t->array.size_expr);
                printf(" of:");
                print_type(t->array.base_type);
            };
            break;
            case TYPE_POINTER:
            {
                printf(" pointer to ");
                print_type(t->pointer.base_type);
            };
            break;
            case TYPE_FUNCTION:
            {
                printf(" function accepting (");
                for (size_t i = 0; i < t->function.parameter_count; i++)
                {
                    type* p = t->function.parameter_types[i];
                    print_type(p);
                    if (i != t->function.parameter_count - 1)
                    {
                        printf(", ");
                    }
                }
                printf(")");
                if (t->function.returned_type)
                {
                    printf("returning (");
                    print_type(t->function.returned_type);
                    printf(")");
                }
                printf(")");
            };
            break;
        }
    }
}

void print_function_param(function_param param)
{
    printf("(param name: %s", param.identifier);
    print_type(param.type);
    printf(")");
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
            printf("(fn decl");
            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }

            if (declaration->function_declaration.parameters.param_count > 0)
            {
                printf(" params: ");
                for (int index = 0; index < declaration->function_declaration.parameters.param_count; index++)
                {
                    print_function_param(declaration->function_declaration.parameters.params[index]);
                }
            }

            if (declaration->function_declaration.return_type)
            {
                printf(" return type: ");
                print_type(declaration->function_declaration.return_type);
            }
            printf(" body: {");
            print_statement_block(declaration->function_declaration.statements);
            printf("})");
        };
        break;
        case DECL_VARIABLE:
        {
            printf("(var decl");

            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }

            if (declaration->variable_declaration.type)
            {
                printf(" type: ");
                print_type(declaration->variable_declaration.type);
            }

            if (declaration->variable_declaration.expression)
            {
                printf(" exp: ");
                print_expression(declaration->variable_declaration.expression);
            }

            printf(")");
        };
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            if (declaration->kind == DECL_UNION)
            {
                printf("(union decl");
            }
            else
            {
                printf("(struct decl");
            }

            if (declaration->identifier)
            {
                printf(" name: %s", declaration->identifier);
            }

            for (size_t index = 0;
                index < declaration->aggregate_declaration.fields_count;
                index++)
            {
                printf("(name: %s", declaration->aggregate_declaration.fields[index].identifier);
                printf(" type: ");
                print_type(declaration->aggregate_declaration.fields[index].type);
                printf(")");
            }

            printf(")");
        };
        break;
        case DECL_ENUM:
        {
            printf("(enum decl");

            for (size_t index = 0;
                index < declaration->enum_declaration.values_count;
                index++)
            {
                enum_value* value = &declaration->enum_declaration.values[index];
                printf("(flag: %s", value->identifier);
                if (value->value_set)
                {
                    printf(" value: %d", value->value);
                }
                printf(")");
            }

            printf(")");
        };
        break;
        case DECL_TYPEDEF:
        {
            printf("(typedef name: ");
            printf(declaration->typedef_declaration.name);
            printf(" type:");
            print_type(declaration->typedef_declaration.type);
            printf(")");
        };
        break;
        default:
        {
            assert("invalid default case");
        };
        break;
    }
}
