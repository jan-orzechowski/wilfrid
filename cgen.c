#include "lexing.h"
#include "parsing.h"
#include "utils.h"

int gen_indent;

char* gen_buf = NULL;

#define gen_printf(...) buf_printf(gen_buf, __VA_ARGS__)

void gen_printf_newline(const char* fmt, ...)
{
    printf("\n%.*s", 2 * gen_indent, "                                                                               ");
    
    va_list args;
    va_start(fmt, args);
    vprintf(fmt, args);
    va_end(args);
}

const char* parenthesize(const char* str, bool parenthesize)
{
    const char* result = parenthesize ? xprintf("(%s)", str) : str;
    return result;
}

void gen_stmt(stmt* s);
void gen_expr(expr* e);
void gen_func_decl(decl* d);

const char* gen_expr_str(expr* e)
{
    // tymczasowa podmiana, żeby wydrukował do nowego gen_buf
    char* temp = gen_buf;
    gen_buf = NULL;

    gen_expr(e);

    const char* result = gen_buf;
    gen_buf = temp;

    return result;
}

const char* cdecl_name(type* t)
{
    switch (t->kind)
    {
        case TYPE_VOID:
        return "void";
        case TYPE_CHAR:
        return "char";
        case TYPE_INT:
        return "int";
        case TYPE_FLOAT:
        return "float";
        case TYPE_STRUCT:
        case TYPE_UNION:
        return t->symbol->name;
        default:
        assert(0);
        return NULL;
    }
}

char* type_to_cdecl(type* type, const char* name)
{
    switch (type->kind)
    {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_STRUCT:
        case TYPE_UNION:
        {
            return xprintf("%s%s%s", cdecl_name(type), *name ? " " : "", name);
        }
        case TYPE_POINTER:
        {
            return type_to_cdecl(type->pointer.base_type, parenthesize(xprintf("*%s", name), *name));
        }
        case TYPE_ARRAY:
        {
            return type_to_cdecl(type->array.base_type, parenthesize(xprintf("%s[%llu]", name, type->array.size), *name));
        }
        case TYPE_FUNCTION:
        {
            char* result = NULL;
            buf_printf(result, "%s(", parenthesize(xprintf("*%s", name), *name));
            if (type->function.param_count == 0)
            {
                buf_printf(result, "void");
            }
            else
            {
                for (size_t i = 0; i < type->function.param_count; i++)
                {
                    char* declstr = type_to_cdecl(type->function.param_types[i], "");
                    buf_printf(result, "%s%s", i == 0 ? "" : ", ", declstr);
                }
            }
            buf_printf(result, ")");
            return type_to_cdecl(type->function.ret_type, result);
        }
        default:
        assert(0);
        return NULL;
    }
}

char* typespec_to_cdecl(typespec* t, const char* name)
{
    char* result = 0;
    switch (t->kind)
    {
        case TYPESPEC_NAME:
        {
            // środkowe s to spacja, jeśli zostało podane name
            result = xprintf("%s%s%s", t->name, *name ? " " : "", name);
        }
        break;
        case TYPESPEC_POINTER:
        {
            const char* wrapped_name = parenthesize(xprintf("*%s", name), (bool)* name);
            result = typespec_to_cdecl(t->pointer.base_type, wrapped_name);
        }
        break;
        case TYPESPEC_ARRAY:
        {
            const char* wrapped_name = parenthesize(xprintf("%s[%s]", name, gen_expr_str(t->array.size_expr)), *name);
            result = typespec_to_cdecl(t->array.base_type, wrapped_name);
        }
        break;
        case TYPESPEC_FUNCTION:
        {
            buf_printf(result, "%s(", parenthesize(xprintf("*%s", name), *name));
            if (t->function.param_count == 0)
            {
                buf_printf(result, "void");
            }
            else
            {
                for (size_t i = 0; i < t->function.param_count; i++)
                {
                    buf_printf(result, "%s%s", i == 0 ? "" : ", ", typespec_to_cdecl(t->function.param_types[i], ""));
                }
            }
            buf_printf(result, ")");
            result = typespec_to_cdecl(t->function.ret_type, result);
        }
        break;
        default:
        {
            fatal("typespec not implemented");            
        }
    }
    return result;
}

void gen_aggregate(decl* decl)
{
    assert(decl->kind == DECL_STRUCT || decl->kind == DECL_UNION);
    gen_printf_newline("%s %s {", decl->kind == DECL_STRUCT ? "struct" : "union", decl->name);
    gen_indent++;
    for (size_t i = 0; i < decl->aggregate.fields_count; i++)
    {
        aggregate_field item = decl->aggregate.fields[i];
        gen_printf_newline("%s;", typespec_to_cdecl(item.type, item.name));        
    }
    gen_indent--;
    gen_printf_newline("};");
}

void gen_stmt_block(stmt_block block)
{
    gen_printf("{");
    gen_indent++;
    for (size_t i = 0; i < block.stmts_count; i++)
    {
        gen_stmt(block.stmts[i]);
    }
    gen_indent--;
    gen_printf_newline("}");
}

void gen_simple_stmt(stmt* stmt)
{
    switch (stmt->kind)
    {
        case STMT_EXPR:
        gen_expr(stmt->expr);
        break;
        case STMT_DECL:
        //gen_printf("%s = ", type_to_cdecl(stmt->init.expr->type, stmt->init.name));
        //gen_expr(stmt->decl.decl.);
        break;
        case STMT_ASSIGN:
        {
            gen_expr(stmt->assign.assigned_var_expr);
            if (stmt->assign.value_expr)
            {
                gen_printf(" %s ", get_token_kind_name(stmt->assign.operation));
                gen_expr(stmt->assign.value_expr);
            }
            else
            {
                gen_printf("%s", get_token_kind_name(stmt->assign.operation));
            }
        }      
        break;
        default:
        assert(0);
    }
}

void gen_stmt(stmt* stmt)
{
    switch (stmt->kind)
    {
        case STMT_RETURN:
        gen_printf_newline("return");
        if (stmt->expr)
        {
            gen_printf(" ");
            gen_expr(stmt->expr);
        }
        gen_printf(";");
        break;
        case STMT_BREAK:
        gen_printf_newline("break;");
        break;
        case STMT_CONTINUE:
        gen_printf_newline("continue;");
        break;
        case STMT_BLOCK:
        gen_printf_newline("");
        gen_stmt_block(stmt->block);
        break;
        case STMT_IF_ELSE:
        {
            gen_printf_newline("if (");
            gen_expr(stmt->if_else.cond_expr);
            gen_printf(") ");
            gen_stmt_block(stmt->if_else.then_block);

            gen_stmt(stmt->if_else.else_stmt);
        }        
        break;
        case STMT_WHILE:
        {
            gen_printf_newline("while (");
            gen_expr(stmt->while_stmt.cond_expr);
            gen_printf(") ");
            gen_stmt_block(stmt->while_stmt.stmts);
        }
        break;
        case STMT_DO_WHILE:
        {
            gen_printf_newline("do ");
            gen_stmt_block(stmt->while_stmt.stmts);
            gen_printf(" while (");
            gen_expr(stmt->while_stmt.cond_expr);
            gen_printf(");");
        }
        break;
        case STMT_FOR:
        {
            gen_printf_newline("for (");
            if (stmt->for_stmt.init_decl)
            {
             /*   type_to_cdecl(stmt->for_stmt.init_decl->variable.type,
                    stmt->for_stmt.init_decl->variable.expr);
                gen_simple_stmt(stmt->for_stmt.init_decl);*/
            }
            gen_printf(";");
            if (stmt->for_stmt.cond_expr)
            {
                gen_printf(" ");
                gen_expr(stmt->for_stmt.cond_expr);
            }
            gen_printf(";");
            if (stmt->for_stmt.next_stmt)
            {
                gen_printf(" ");
                gen_simple_stmt(stmt->for_stmt.next_stmt);
            }
            gen_printf(") ");
            gen_stmt_block(stmt->for_stmt.stmts);
        }
        break;
        case STMT_SWITCH:
        {
            gen_printf_newline("switch (");
            gen_expr(stmt->switch_stmt.var_expr);
            gen_printf(") {");
            for (size_t i = 0; i < stmt->switch_stmt.cases_num; i++)
            {
                switch_case* switch_case = stmt->switch_stmt.cases[i];
                for (size_t j = 0; j < switch_case->cond_exprs_num; j++)
                {
                    gen_printf_newline("case ");
                    gen_expr(switch_case->cond_exprs[j]);
                    gen_printf(":");

                }
                if (switch_case->is_default)
                {
                    gen_printf_newline("default:");
                }
                gen_printf(" ");

                gen_stmt_block(switch_case->stmts);

                if (false == switch_case->fallthrough)
                {
                    gen_printf_newline("break;");
                }
            }
            gen_printf_newline("}");
        }
        break;
        default:
        {
            gen_printf_newline("");
            gen_simple_stmt(stmt);
            gen_printf(";");
        }      
        break;
    }
}

void gen_expr(expr* e)
{
    switch (e->kind)
    {
        case EXPR_INT:
        {
            gen_printf("%lld", e->number_value);
        }
        break;
        case EXPR_FLOAT:
        {
            gen_printf("%f", e->number_value);
        }
        break;
        //case EXPR_CHAR:
        //{
        //    // TODO: proper quoted string escaping
        //    gen_printf("\"%s\"", e->str_val);
        //}
        //break;
        case EXPR_NAME:
        {
            gen_printf("%s", e->name);
        }
        break;
        case EXPR_UNARY:
        {
            gen_printf("%s(", get_token_kind_name(e->unary.operator));
            gen_expr(e->unary.operand);
            gen_printf(")");
        }
        break;
        case EXPR_BINARY:
        {
            gen_printf("(");
            gen_expr(e->binary.left);
            gen_printf(") %s (", get_token_kind_name(e->binary.operator));
            gen_expr(e->binary.right);
            gen_printf(")");
        }
        break;
        case EXPR_TERNARY:
        {
            gen_printf("(");
            gen_expr(e->ternary.condition);
            gen_printf(" ? ");
            gen_expr(e->ternary.if_true);
            gen_printf(" : ");
            gen_expr(e->ternary.if_false);
            gen_printf(")");
        }
        break;
        case EXPR_CALL:
        {
            gen_expr(e->call.function_expr);
            gen_printf("(");
            for (size_t i = 0; i < e->call.args_num; i++)
            {
                if (i != 0)
                {
                    gen_printf(", ");
                }
                gen_expr(e->call.args[i]);
            }
            gen_printf(")");
        }
        break;
        case EXPR_FIELD:
        {
            gen_expr(e->field.expr);
            gen_printf(".%s", e->field.field_name);
        }
        break;
        case EXPR_INDEX:
        {
            gen_expr(e->index.array_expr);
            gen_printf("[");
            gen_expr(e->index.index_expr);
            gen_printf("]");
        }
        break;
        case EXPR_SIZEOF:
        {
            gen_printf("sizeof(");
            gen_expr(e->size_of.expr);
            gen_printf(")");
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            if (e->compound.type)
            {
                gen_printf("(%s){", typespec_to_cdecl(e->compound.type, ""));
            }
            else
            {
                // todo
                //gen_printf("(%s){", type_to_cdecl(e->type, ""));
            }
            for (size_t i = 0; i < e->compound.fields_count; i++)
            {
                if (i != 0)
                {
                    gen_printf(", ");
                }
                compound_literal_field* field = e->compound.fields[i];
                gen_printf(".%s = ", field->field_name);                
                gen_expr(field->expr);
            }
            gen_printf("}");
        }
        break;
        case EXPR_NONE:
        default:
        {
            fatal("unimplemented expr kind");
        }
    }
}

void gen_aggregate_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_TYPE);
    assert(sym->decl->kind == DECL_STRUCT);
    printf("typedef struct %s {", sym->type->name);
    for (size_t i = 0; i < sym->type->aggregate.fields_count; i++)
    {
        type_aggregate_field* f = sym->type->aggregate.fields[i];
        type_to_cdecl(f->type, f->name);
    }
    printf("} %s;", sym->decl->name);
    print_newline();
}

void gen_func(decl* decl)
{
    assert(decl->kind == DECL_FUNCTION);
    gen_func_decl(decl);
    gen_printf(" ");
    gen_stmt_block(decl->function.stmts);
}
//
//void gen_function_decl(symbol* sym)
//{
//    assert(sym->kind == SYMBOL_FUNCTION);
//
//    gen_c_decl(sym->type->function.ret_type);
//    printf(" %s(", sym->name);    
//    if (sym->type->function.param_count == 0)
//    {
//        printf("void");
//    }
//    else
//    {
//        assert(sym->type->function.param_count == sym->decl->function.params.param_count);
//        for (size_t i = 0; i < sym->type->function.param_count; i++)
//        {
//            type* param_type = sym->type->function.param_types[i];
//            const char* param_name = sym->decl->function.params.params[i].name;
//            gen_c_decl(param_type);
//            printf(" %s", param_name);
//            if (i != sym->type->function.param_count - 1)
//            {
//                printf(", ");
//            }
//        }
//    }
//    printf(")\n{");
//    print_newline();
//    
//    gen_stmt_block(sym->decl->function.stmts);
//
//    print_newline();
//    printf("}");
//    print_newline();
//}

void gen_var_decl(decl* decl, symbol* sym)
{
    assert(decl->kind == DECL_VARIABLE);
 
    if (decl->variable.type)
    {
        gen_printf_newline("%s", typespec_to_cdecl(decl->variable.type, sym->name));
    }
    else
    {
        gen_printf_newline("%s", type_to_cdecl(sym->type, sym->name));
    }
    if (decl->variable.expr)
    {
        gen_printf(" = ");
        gen_expr(decl->variable.expr);
    }
    gen_printf(";");
}

void gen_func_decl(decl* d)
{
    assert(d->kind == DECL_FUNCTION);
    if (d->function.return_type)
    {
        gen_printf_newline("%s(", typespec_to_cdecl(d->function.return_type, d->name));
    }
    else
    {
        gen_printf_newline("void %s(", d->name);
    }
    if (d->function.params.param_count == 0)
    {
        gen_printf("void");
    }
    else
    {
        for (size_t i = 0; i < d->function.params.param_count; i++)
        {
            function_param param = d->function.params.params[i];
            if (i != 0)
            {
                gen_printf(", ");
            }
            gen_printf("%s", typespec_to_cdecl(param.type, param.name));
        }
    }
    gen_printf(")");
}

void gen_forward_decls(symbol** resolved)
{
    for (size_t i = 0; i < buf_len(resolved); i++)
    {        
        symbol* sym = resolved[i];
        decl* d = sym->decl;
        if (d)
        {
            switch (d->kind)
            {
                case DECL_STRUCT:
                {
                    gen_printf_newline("typedef struct %s %s;", sym->name, sym->name);
                }
                break;
                case DECL_UNION:
                {
                    gen_printf_newline("typedef union %s %s;", sym->name, sym->name);
                }
                break;
                case DECL_FUNCTION:
                {
                    gen_func_decl(sym->decl);
                }
                gen_printf(";");
                break;
                default:
                // Do nothing.
                break;
            }
        }
        else
        {
            debug_breakpoint;
        }
    }
}

void gen_symbol(symbol* sym)
{
    assert(sym);

    decl* decl = sym->decl;
    if (!decl)
    {
        return;
    }

    switch (decl->kind)
    {
        case DECL_VARIABLE:
        {
            gen_var_decl(decl, sym);
        }
        break;
        case DECL_CONST:
        {
            gen_printf_newline("enum { %s = ", sym->name);
            gen_expr(decl->const_decl.expr);
            gen_printf(" };");
        }
        break;
        case DECL_FUNCTION:
        {
            gen_func_decl(decl);
        }
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            gen_aggregate_decl(sym);
        }
        break;
        case DECL_ENUM:
        {
            gen_printf_newline("enum { %s = ", sym->name);
            gen_expr(decl->const_decl.expr);
            gen_printf(" };");
            break;
        }
        case DECL_TYPEDEF:
        {
            gen_printf_newline("typedef %s;", type_to_cdecl(sym->type, sym->name));
        }
        break;
        default:
        {
            fatal("generation for decl kind not implemented");
        }
        break;
    }
}

void cgen_test(void)
{
    char* test_strs[] = {
        "const x = 10",
        "fn power_2(n:int):int{return n*n}",        
        "fn main(){\
            let i : int = 5\
            let j = power_2(i)\
        }",
    };
    
    symbol** resolved = resolve(test_strs, sizeof(test_strs) / sizeof(test_strs[0]), false);

    for (size_t i = 0; i < buf_len(resolved); i++)
    {
        gen_symbol(resolved[i]);
    }

    debug_breakpoint;
}