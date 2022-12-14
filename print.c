#include "lexing.h"
#include "parsing.h"

int indent;

void print_newline(void)
{
    printf("\n%.*s", 2 * indent, "                                                                               ");
}

void print_decl(decl* d);
void print_stmt_block(stmt_block block);
void print_typespec(typespec* t);

void print_expr(expr* e)
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
            printf("%s", e->name);
        }
        break;
        case EXPR_SIZEOF:
        {
            printf("(sizeof ");
            print_expr(e->size_of.expr);
            printf(")");
        }
        break;
        case EXPR_UNARY:
        {
            printf("(");
            if (e->unary.operator == TOKEN_MUL)
            {
                printf("pointer-dereference");
            }
            else
            {
                print_token_kind(e->unary.operator);
            }
            printf(" ");
            print_expr(e->unary.operand);
            printf(")");
        }
        break;
        case EXPR_BINARY:
        {
            printf("(");
            print_token_kind(e->binary.operator);
            printf(" ");
            print_expr(e->binary.left);
            printf(" ");
            print_expr(e->binary.right);
            printf(")");
        }
        break;
        case EXPR_INDEX:
        {
            printf("(access-index ");
            print_expr(e->index.index_expr);
            printf(" ");
            print_expr(e->index.array_expr);
            printf(")");
        }
        break;
        case EXPR_CALL:
        {
            printf("(");
            print_expr(e->call.function_expr);
            printf(" ");
            for (size_t i = 0; i < e->call.args_num; i++)
            {
                print_expr(e->call.args[i]);
                if (i != e->call.args_num - 1)
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
            printf("%s", e->field.field_name);
            printf(" ");
            print_expr(e->field.expr);
            printf(")");
        }
        break;
        case EXPR_TERNARY:
        {
            printf("(? ");
            print_expr(e->ternary.condition);
            
            indent++;
            print_newline();
            print_expr(e->ternary.if_true);
            print_newline();
            print_expr(e->ternary.if_false);
            
            indent--;
            print_newline();
            printf(")");
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            printf("(compound ");
            if (e->compound.type)
            {
                print_typespec(e->compound.type);
                printf(" ");
            }

            for (size_t i = 0; i < e->compound.fields_count; i++)
            {
                compound_literal_field* f = e->compound.fields[i];
                if (f->field_name)
                {
                    printf("(%s ", f->field_name);
                    print_expr(f->expr);
                    printf(")");
                }
                else
                {
                    print_expr(f->expr);
                }           

                if (i != e->compound.fields_count - 1)
                {
                    printf(" ");
                }
            }
            printf(")");
        }
        break;
        default:
        {
            fatal("expr kind not handled: %d", e->kind);
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
            print_expr(s->return_stmt.ret_expr);
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
        case STMT_IF_ELSE:
        {
            printf("(if ");
            print_expr(s->if_else.cond_expr);
            
            print_stmt_block(s->if_else.then_block);
           
            if (s->if_else.else_stmt)
            {
                indent++;
                print_newline();
                printf("else");

                if (s->if_else.else_stmt->kind == STMT_BLOCK)
                {
                    print_statement(s->if_else.else_stmt);
                }
                else
                {
                    indent++;
                    print_newline();
                    print_statement(s->if_else.else_stmt);
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
            print_expr(s->while_stmt.cond_expr);

            print_stmt_block(s->while_stmt.stmts);

            print_newline();
            printf(")");
        }
        break;
        case STMT_DO_WHILE:
        {
            printf("(do-while ");
            print_expr(s->do_while_stmt.cond_expr);;

            print_stmt_block(s->do_while_stmt.stmts);

            print_newline();
            printf(")");
        }
        break;
        case STMT_BLOCK:
        {
            printf("(");
            print_stmt_block(s->block);
            print_newline();
            printf(")");
        }
        break;
        case STMT_ASSIGN:
        {
            printf("(");
            print_token_kind(s->assign.operation);
            printf(" ");
            print_expr(s->assign.assigned_var_expr);
            if (s->assign.value_expr)
            {
                printf(" ");
                print_expr(s->assign.value_expr);
            }
            printf(")");
        }
        break;
        case STMT_FOR:
        {
            printf("(for ");
            print_decl(s->for_stmt.init_decl);
            printf(" ");
            print_expr(s->for_stmt.cond_expr);
            printf(" ");
            print_statement(s->for_stmt.incr_stmt);
           
            print_stmt_block(s->for_stmt.stmts);
            
            print_newline();
            printf(")");
        }
        break;
        case STMT_SWITCH:
        {
            printf("(switch ");
            print_expr(s->switch_stmt.var_expr);

            indent++;

            for (size_t i = 0; i < s->switch_stmt.cases_num; i++)
            {
                print_newline();
                printf("(case ");
                switch_case* c = s->switch_stmt.cases[i];
                for (size_t j = 0; j < c->cond_exprs_num; j++)
                {
                    expr* e = c->cond_exprs[j];
                    print_expr(e);
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

                
                print_stmt_block(c->stmts);
                
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
            print_decl(s->decl.decl);
        }
        break;
        case STMT_EXPR:
        {
            print_expr(s->return_stmt.ret_expr);
        }
        break;
    }
}

void print_stmt_block(stmt_block block)
{
    indent++;
    for (int index = 0; index < block.stmts_count; index++)
    {
        print_newline();
        print_statement(block.stmts[index]);        
    }
    indent--;
}

void print_typespec(typespec* t)
{
    if (t != 0)
    {
        switch (t->kind)
        {
            case TYPESPEC_NAME:
            {
                printf("(type %s)", t->name);
            };
            break;
            case TYPESPEC_ARRAY:
            {
                printf("(array ");
                print_typespec(t->array.base_type);
                printf(" ");
                print_expr(t->array.size_expr);
                printf(")");
            };
            break;
            case TYPESPEC_POINTER:
            {
                printf("(pointer ");
                print_typespec(t->pointer.base_type);
                printf(")");
            };
            break;
            case TYPESPEC_FUNCTION:
            {
                printf("(function (");                
                for (size_t i = 0; i < t->function.param_count; i++)
                {
                    typespec* p = t->function.param_types[i];
                    print_typespec(p);
                    if (i != t->function.param_count - 1)
                    {
                        printf(" ");
                    }
                }
                printf(")");
                if (t->function.ret_type)
                {
                    printf(" ");
                    print_typespec(t->function.ret_type);
                }
                printf(")");
            };
            break;
        }
    }
}

void print_decl(decl* d)
{
    if (d == NULL)
    {
        return;
    }

    switch (d->kind)
    {
        case DECL_FUNCTION:
        {
            printf("(fn-decl ");
            if (d->name)
            {
                printf("%s", d->name);
            }

            if (d->function.params.param_count > 0)
            {
                printf(" ");
                for (int index = 0; index < d->function.params.param_count; index++)
                {
                    function_param* p = &d->function.params.params[index];
                    printf("(%s ", p->name);
                    print_typespec(p->type);
                    printf(")");

                    if (index < d->function.params.param_count - 1)
                    {
                        printf(" ");
                    }
                }
            }

            if (d->function.return_type)
            {
                printf(" ");
                print_typespec(d->function.return_type);
            }
            
            print_stmt_block(d->function.stmts);

            print_newline();
            printf(")");
        }
        break;
        case DECL_VARIABLE:
        {
            printf("(var-decl ");

            if (d->name)
            {
                printf("%s", d->name);
            }

            if (d->variable.type)
            {
                printf(" ");
                print_typespec(d->variable.type);
            }

            if (d->variable.expr)
            {
                printf(" ");
                print_expr(d->variable.expr);
            }

            printf(")");
        }
        break;
        case DECL_CONST:
        {
            printf("(const-decl ");
            if (d->name)
            {
                printf("%s", d->name);
            }          
            if (d->const_decl.expr)
            {
                printf(" ");
                print_expr(d->const_decl.expr);
            }
            printf(")");
        }
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            if (d->kind == DECL_UNION)
            {
                printf("(union-decl");
            }
            else
            {
                printf("(struct-decl");
            }

            if (d->name)
            {
                printf(" %s", d->name);
            }

            indent++;

            for (size_t index = 0;
                index < d->aggregate.fields_count;
                index++)
            {
                print_newline();
                printf("(%s", d->aggregate.fields[index].name);
                printf(" ");
                print_typespec(d->aggregate.fields[index].type);
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
                index < d->enum_decl.values_count;
                index++)
            {
                print_newline();
                enum_value* value = &d->enum_decl.values[index];
                printf("(%s", value->name);
                if (value->value_set)
                {
                    printf(" %lld", value->value);
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
            printf("(typedef-decl %s", d->typedef_decl.name);
            printf(" ");
            print_typespec(d->typedef_decl.type);
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
