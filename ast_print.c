
#include "parsing.h"

char *ast_buf;
int ast_indent;

#define ast_printf(...) buf_printf(ast_buf, __VA_ARGS__)

void ast_print_newline(void)
{
    buf_printf(ast_buf, 
        "\n%.*s", 
        2 * ast_indent, 
        "                                                                               ");
}

void ast_print_decl(decl *d);
void ast_print_stmt_block(stmt_block block);
void ast_print_typespec(typespec *t);
char *get_stub_expr_name(stub_expr_kind kind);

void ast_print_token_kind(token_kind kind)
{
    const char *name = get_token_kind_name(kind);
    if (name)
    {
        ast_printf("%s", name);
    }
    else
    {
        ast_printf("TOKEN UNKNOWN: %s", kind);
    }
}

void ast_print_expr(expr *e)
{
    if (e == null)
    {
        return;
    }

    switch (e->kind)
    {
        case EXPR_INT:
        {
            ast_printf("%d", e->integer_value);
        }
        break;
        case EXPR_FLOAT:
        {
            ast_printf("%ff", e->float_value);
        }
        break;
        case EXPR_NAME:
        {
            ast_printf("%s", e->name);
        }
        break;
        case EXPR_STRING:
        {
            ast_printf("\"%s\"", e->string_value);
        }
        break;
        case EXPR_CHAR:
        {
            ast_printf("\'%s\'", e->string_value);
        }
        break;
        case EXPR_SIZE_OF:
        {
            ast_printf("(size-of ");
            ast_print_expr(e->size_of.expr);
            ast_printf(")");
        }
        break;
        case EXPR_SIZE_OF_TYPE:
        {
            ast_printf("(size-of-type ");
            ast_print_typespec(e->size_of_type.type);
            ast_printf(")");
        }
        break;
        case EXPR_UNARY:
        {
            ast_printf("(");

            if (e->unary.operator == TOKEN_MUL)
            {
                ast_printf("pointer-dereference");
            }
            else
            {
                ast_print_token_kind(e->unary.operator);
            }

            ast_printf(" ");
            ast_print_expr(e->unary.operand);
            ast_printf(")");
        }
        break;
        case EXPR_BINARY:
        {
            ast_printf("(");
            ast_print_token_kind(e->binary.operator);
            ast_printf(" ");
            ast_print_expr(e->binary.left);
            ast_printf(" ");
            ast_print_expr(e->binary.right);
            ast_printf(")");
        }
        break;
        case EXPR_INDEX:
        {
            ast_printf("(access-index ");
            ast_print_expr(e->index.index_expr);
            ast_printf(" ");
            ast_print_expr(e->index.array_expr);
            ast_printf(")");
        }
        break;
        case EXPR_CALL:
        {
            ast_printf("(");
            ast_print_expr(e->call.function_expr);

            if (e->call.method_receiver)
            {
                ast_printf(" (receiver ");
                ast_print_expr(e->call.method_receiver);
                ast_printf(")");
            }

            ast_printf(" ");

            for (size_t i = 0; i < e->call.args_num; i++)
            {
                ast_print_expr(e->call.args[i]);
                if (i != e->call.args_num - 1)
                {
                    ast_printf(" ");
                }
            }

            ast_printf(")");
        }
        break;
        case EXPR_FIELD:
        {
            ast_printf("(access-field ");
            ast_printf("%s", e->field.field_name);
            ast_printf(" ");
            ast_print_expr(e->field.expr);
            ast_printf(")");
        }
        break;
        case EXPR_TERNARY:
        {
            ast_printf("(? ");
            ast_print_expr(e->ternary.condition);
            
            ast_indent++;
            ast_print_newline();
            ast_print_expr(e->ternary.if_true);
            ast_print_newline();
            ast_print_expr(e->ternary.if_false);
            
            ast_indent--;
            ast_print_newline();
            ast_printf(")");
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            ast_printf("(compound ");
            if (e->compound.type)
            {
                ast_print_typespec(e->compound.type);
                ast_printf(" ");
            }

            for (size_t i = 0; i < e->compound.fields_count; i++)
            {
                compound_literal_field *f = e->compound.fields[i];
                if (f->field_index > 0)
                {
                    ast_printf("(index %d ", f->field_index);
                    ast_print_expr(f->expr);
                    ast_printf(")");
                }
                else if (f->field_name)
                {
                    ast_printf("(%s ", f->field_name);
                    ast_print_expr(f->expr);
                    ast_printf(")");
                }
                else
                {
                    ast_print_expr(f->expr);
                }           

                if (i != e->compound.fields_count - 1)
                {
                    ast_printf(" ");
                }
            }
            ast_printf(")");
        }
        break;
        case EXPR_CAST:
        {
            ast_printf("(cast ");
            ast_print_typespec(e->cast.type);
            ast_printf(" ");
            ast_print_expr(e->cast.expr);
            ast_printf(")");
        }
        break;
        case EXPR_NEW:
        {
            ast_printf("(new ");
            ast_print_typespec(e->new.type);
            ast_printf(")");
        }
        break;
        case EXPR_AUTO:
        {
            ast_printf("(auto ");
            ast_print_typespec(e->auto_new.type);
            ast_printf(")");
        }
        break;
        case EXPR_BOOL:
        {
            if (e->bool_value)
            {
                ast_printf("true");
            }
            else
            {
                ast_printf("false");
            }
        }
        break;
        case EXPR_NULL:
        {
            ast_printf("null");
        }
        break;
        case EXPR_STUB:
        {
            if (e->stub.original_expr)
            {
                ast_printf("(%s (", get_stub_expr_name(e->stub.kind));
                ast_print_expr(e->stub.original_expr);
                ast_printf(")");
            }
            else
            {
                ast_printf("(expr-stub %d)", e->stub.kind);
            }
        }
        break;
        default:
        {
            fatal("Expr kind not handled in ast_print_expr: %d", e->kind);
        }
        break;
    }
}

void ast_print_statement(stmt *s)
{
    if (s == null)
    {
        return;
    }

    switch (s->kind)
    {
        case STMT_RETURN:
        {
            ast_printf("(return ");
            ast_print_expr(s->return_stmt.ret_expr);
            ast_printf(")");
        }
        break;
        case STMT_BREAK:
        {
            ast_printf("(break)");
        }
        break;
        case STMT_CONTINUE:
        {
            ast_printf("(continue)");
        }
        break;
        case STMT_IF_ELSE:
        {
            ast_printf("(if ");
            ast_print_expr(s->if_else.cond_expr);
            
            ast_print_stmt_block(s->if_else.then_block);
           
            if (s->if_else.else_stmt)
            {
                ast_indent++;
                ast_print_newline();
                ast_printf("else");

                if (s->if_else.else_stmt->kind == STMT_BLOCK)
                {
                    ast_print_statement(s->if_else.else_stmt);
                }
                else
                {
                    ast_indent++;
                    ast_print_newline();
                    ast_print_statement(s->if_else.else_stmt);
                    ast_indent--;
                }                
              
                ast_indent--;
            }

            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_WHILE:
        {
            ast_printf("(while ");
            ast_print_expr(s->while_stmt.cond_expr);

            ast_print_stmt_block(s->while_stmt.stmts);

            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_DO_WHILE:
        {
            ast_printf("(do-while ");
            ast_print_expr(s->do_while_stmt.cond_expr);;

            ast_print_stmt_block(s->do_while_stmt.stmts);

            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_BLOCK:
        {
            ast_printf("(");
            ast_print_stmt_block(s->block);
            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_ASSIGN:
        {
            ast_printf("(");
            ast_print_token_kind(s->assign.operation);
            ast_printf(" ");
            ast_print_expr(s->assign.assigned_var_expr);
            if (s->assign.value_expr)
            {
                ast_printf(" ");
                ast_print_expr(s->assign.value_expr);
            }
            ast_printf(")");
        }
        break;
        case STMT_FOR:
        {
            ast_printf("(for ");
            ast_print_statement(s->for_stmt.init_stmt);
            ast_printf(" ");
            ast_print_expr(s->for_stmt.cond_expr);
            ast_printf(" ");
            ast_print_statement(s->for_stmt.next_stmt);
           
            ast_print_stmt_block(s->for_stmt.stmts);
            
            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_SWITCH:
        {
            ast_printf("(switch ");
            ast_print_expr(s->switch_stmt.var_expr);

            ast_indent++;

            for (size_t i = 0; i < s->switch_stmt.cases_num; i++)
            {
                ast_print_newline();
                ast_printf("(case ");
                switch_case *c = s->switch_stmt.cases[i];
                for (size_t j = 0; j < c->cond_exprs_num; j++)
                {
                    expr *e = c->cond_exprs[j];
                    ast_print_expr(e);
                    if (j != c->cond_exprs_num - 1)
                    {
                        ast_printf(" ");
                    }
                }
                if (c->is_default)
                {
                    ast_printf(" default");
                }
                if (c->fallthrough)
                {
                    ast_printf(" fallthrough");
                }

                
                ast_print_stmt_block(c->stmts);
                
                ast_print_newline();
                ast_printf(")");
            }

            ast_indent--;
            ast_print_newline();
            ast_printf(")");
        }
        break;
        case STMT_DECL:
        {
            ast_print_decl(s->decl_stmt.decl);
        }
        break;
        case STMT_EXPR:
        {
            ast_print_expr(s->return_stmt.ret_expr);
        }
        break;
        case STMT_DELETE:
        {
            ast_printf("(delete ");
            ast_print_expr(s->delete.expr);
            ast_printf(")");
        }
        break;
        invalid_default_case;
    }
}

void ast_print_stmt_block(stmt_block block)
{
    ast_indent++;
    for (int index = 0; index < block.stmts_count; index++)
    {
        ast_print_newline();
        ast_print_statement(block.stmts[index]);        
    }
    ast_indent--;
}

void ast_print_typespec(typespec *t)
{
    if (t != 0)
    {
        switch (t->kind)
        {
            case TYPESPEC_NAME:
            {
                ast_printf("(type %s)", t->name);
            };
            break;
            case TYPESPEC_ARRAY:
            {
                ast_printf("(array ");
                ast_print_typespec(t->array.base_type);
                ast_printf(" ");
                ast_print_expr(t->array.size_expr);
                ast_printf(")");
            };
            break;
            case TYPESPEC_LIST:
            {
                ast_printf("(list ");
                ast_print_typespec(t->list.base_type);
                ast_printf(")");
            };
            break;
            case TYPESPEC_POINTER:
            {
                ast_printf("(pointer ");
                ast_print_typespec(t->pointer.base_type);
                ast_printf(")");
            };
            break;
            case TYPESPEC_FUNCTION:
            {
                ast_printf("(function (");                
                for (size_t i = 0; i < t->function.param_count; i++)
                {
                    typespec *p = t->function.param_types[i];
                    ast_print_typespec(p);
                    if (i != t->function.param_count - 1)
                    {
                        ast_printf(" ");
                    }
                }
                ast_printf(")");
                if (t->function.ret_type)
                {
                    ast_printf(" ");
                    ast_print_typespec(t->function.ret_type);
                }
                ast_printf(")");
            };
            break;
            invalid_default_case;
        }
    }
}

void ast_print_decl(decl *d)
{
    if (d == null)
    {
        return;
    }

    switch (d->kind)
    {
        case DECL_FUNCTION:
        {
            ast_printf("(fn-decl ");

            if (d->function.is_extern)
            {
                ast_printf("extern ");
            }

            if (d->name)
            {
                ast_printf("%s", d->name);
            }

            if (d->function.method_receiver)
            {
                ast_printf(" (receiver (%s ", d->function.method_receiver->name);
                ast_print_typespec(d->function.method_receiver->type);
                ast_printf("))");
            }

            if (d->function.params.param_count > 0)
            {
                ast_printf(" ");
                for (int index = 0; index < d->function.params.param_count; index++)
                {
                    function_param *p = &d->function.params.params[index];
                    ast_printf("(%s ", p->name);
                    ast_print_typespec(p->type);
                    ast_printf(")");

                    if (index < d->function.params.param_count - 1)
                    {
                        ast_printf(" ");
                    }
                }
            }

            if (d->function.return_type)
            {
                ast_printf(" ");
                ast_print_typespec(d->function.return_type);
            }
            
            if (false == d->function.is_extern)
            {
                ast_print_stmt_block(d->function.stmts);
                ast_print_newline();
            }

            ast_printf(")");
        }
        break;
        case DECL_VARIABLE:
        {
            ast_printf("(var-decl ");

            if (d->name)
            {
                ast_printf("%s", d->name);
            }

            if (d->variable.type)
            {
                ast_printf(" ");
                ast_print_typespec(d->variable.type);
            }

            if (d->variable.expr)
            {
                ast_printf(" ");
                ast_print_expr(d->variable.expr);
            }

            ast_printf(")");
        }
        break;
        case DECL_CONST:
        {
            ast_printf("(const-decl ");
            if (d->name)
            {
                ast_printf("%s", d->name);
            }          
            if (d->const_decl.expr)
            {
                ast_printf(" ");
                ast_print_expr(d->const_decl.expr);
            }
            ast_printf(")");
        }
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            if (d->kind == DECL_UNION)
            {
                ast_printf("(union-decl");
            }
            else
            {
                ast_printf("(struct-decl");
            }

            if (d->name)
            {
                ast_printf(" %s", d->name);
            }

            ast_indent++;

            for (size_t index = 0;
                index < d->aggregate.fields_count;
                index++)
            {
                ast_print_newline();
                ast_printf("(%s", d->aggregate.fields[index].name);
                ast_printf(" ");
                ast_print_typespec(d->aggregate.fields[index].type);
                ast_printf(")");
            }

            ast_indent--;
            ast_print_newline();
            ast_printf(")");
        }
        break;
        case DECL_ENUM:
        {
            ast_printf("(enum-decl");

            ast_indent++;

            for (size_t index = 0;
                index < d->enum_decl.values_count;
                index++)
            {
                ast_print_newline();
                enum_value *value = &d->enum_decl.values[index];
                ast_printf("(%s", value->name);
                if (value->value_set)
                {
                    ast_printf(" %lld", value->value);
                }
                ast_printf(")");
            }

            ast_indent--;
            ast_print_newline();
            ast_printf(")");
        }
        break;
        invalid_default_case;
    }
}

char *get_decl_ast(decl *d)
{
    buf_free(ast_buf);
    ast_print_decl(d);
    return ast_buf;
}

char *get_stub_expr_name(stub_expr_kind kind)
{
    switch (kind)
    {
        case STUB_EXPR_NONE: return "stub-expr-none";
        case STUB_EXPR_LIST_FREE: return "stub-expr-list-free";
        case STUB_EXPR_LIST_REMOVE_AT: return "stub-expr-list-remove-at";
        case STUB_EXPR_LIST_LENGTH: return "stub-expr-list-length";
        case STUB_EXPR_LIST_CAPACITY: return "stub-expr-list-capacity";
        case STUB_EXPR_LIST_ADD: return "stub-expr-list-add";
        case STUB_EXPR_LIST_NEW: return "stub-expr-list-new";
        case STUB_EXPR_LIST_AUTO: return "stub-expr-list-auto";
        case STUB_EXPR_LIST_INDEX: return "stub-expr-list-index";
        case STUB_EXPR_CONSTRUCTOR: return "stub-expr-constructor";
        default: return xprintf("stub-expr unknown:%d", kind);
    }
}