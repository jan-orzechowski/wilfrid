#include "lexing.h"
#include "parsing.h"
#include "utils.h"

void generate_stmt(stmt* s);

void generate_c_decl(type* t)
{
    switch (t->kind)
    {
        case TYPE_VOID:
        {
            printf("void");
        }
        break;    
        case TYPE_INT:
        {
            printf("int");
        }
        break;
        case TYPE_CHAR:
        {
            printf("char");
        }
        break;
        case TYPE_FLOAT:
        {
            printf("float");
        }
        break;
        case TYPE_STRUCT:
        {
            printf("%s", t->name);
        }
        break;
        case TYPE_UNION:
        {

        }
        break;
        case TYPE_NAME:
        {

        }
        break;
        case TYPE_ARRAY:
        {
            printf("(");
            generate_c_decl(t->array.base_type);
            printf(")[%lld]", t->array.size);
        }
        break;
        case TYPE_POINTER:
        {
            printf("*(");
            generate_c_decl(t->pointer.base_type);
            printf(")");
        }
        break;
        case TYPE_FUNCTION:
        {

        }
        break;
    }
}

void generate_statement()
{

}

void generate_const_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_CONST);

}

void generate_enum_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_ENUM_CONST);

}

void generate_expr(expr* e)
{
    switch (e->kind)
    {
        case EXPR_INT:
        {

        }
        break;
        case EXPR_FLOAT:
        {

        }
        break;
        case EXPR_CHAR:
        {

        }
        break;
        case EXPR_NAME:
        {

        }
        break;
        case EXPR_UNARY:
        {

        }
        break;
        case EXPR_BINARY:
        {

        }
        break;
        case EXPR_TERNARY:
        {

        }
        break;
        case EXPR_CALL:
        {

        }
        break;
        case EXPR_FIELD:
        {

        }
        break;
        case EXPR_INDEX:
        {

        }
        break;
        case EXPR_SIZEOF:
        {

        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {

        }
        break;
        case EXPR_NONE:
        default:
        {
            fatal("unimplemented expr kind");
        }
    }
}

void generate_type_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_TYPE);
    assert(sym->decl->kind == DECL_STRUCT);    
    printf("typedef struct %s {", sym->type->name);
    for (size_t i = 0; i < sym->type->aggregate.fields_count; i++)
    {
        type_aggregate_field* f = sym->type->aggregate.fields[i];
        generate_c_decl(f->type);
        printf("%s;", f->name);        
    }
    printf("} %s;", sym->decl->name);
    print_newline();
}

void generate_stmt_block(stmt_block block)
{
    printf("{");
    for (size_t i = 0; i < block.stmts_count; i++)
    {
        stmt* s = block.stmts[i];
        generate_stmt(s);
    }
    printf("}");
}

void generate_stmt(stmt* s)
{
    switch (s->kind)
    {
        case STMT_RETURN:
        {
            printf("return ");
            generate_expr(s->return_stmt.ret_expr);
        }
        break;
        case STMT_BREAK:
        {
            printf("break;");
        }
        break;
        case STMT_CONTINUE:
        {
            printf("continue;");
        }
        break;
        case STMT_DECL:
        {
            // skąd wziąć tutaj type?
            generate_c_decl(0);
        }
        break;
        case STMT_IF_ELSE:
        {
            printf("if (");
            generate_expr(s->if_else.cond_expr);
            printf(") {\n");
            generate_stmt_block(s->if_else.then_block);
            printf("}");
            if (s->if_else.else_stmt)
            {
                generate_stmt(s->if_else.else_stmt);
            }
        }
        break;
        case STMT_WHILE:
        {
            printf("while (");
            generate_expr(s->while_stmt.cond_expr);
            printf(")\n");
            generate_stmt_block(s->while_stmt.stmts);
        }
        break;
        case STMT_DO_WHILE:
        {
            printf("do");
            generate_stmt_block(s->do_while_stmt.stmts);
            printf("while (");
            generate_expr(s->do_while_stmt.cond_expr);
            printf(")");
        }
        break;
        case STMT_FOR:
        {

        }
        break;
        case STMT_ASSIGN:
        {

        }
        break;
        case STMT_SWITCH:
        {

        }
        break;
        case STMT_EXPR:
        {

        }
        break;
        case STMT_BLOCK:
        {

        }
        break;
        case STMT_NONE:
        {

        }
        break;
    }
    print_newline();
}

void generate_function_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_FUNCTION);

    generate_c_decl(sym->type->function.ret_type);
    printf(" %s(", sym->name);    
    if (sym->type->function.param_count == 0)
    {
        printf("void");
    }
    else
    {
        assert(sym->type->function.param_count == sym->decl->function.params.param_count);
        for (size_t i = 0; i < sym->type->function.param_count; i++)
        {
            type* param_type = sym->type->function.param_types[i];
            const char* param_name = sym->decl->function.params.params[i].name;
            generate_c_decl(param_type);
            printf(" %s", param_name);
            if (i != sym->type->function.param_count - 1)
            {
                printf(", ");
            }
        }
    }
    printf(")\n{");
    print_newline();
    
    generate_stmt_block(sym->decl->function.stmts);

    print_newline();
    printf("}");
    print_newline();
}

void generate_var_decl(symbol* sym)
{
    assert(sym->kind == SYMBOL_VARIABLE);
 
}

void generate(symbol* sym)
{
    assert(sym);

    switch (sym->kind)
    {
        case SYMBOL_NONE: 
        {
            fatal("unresolved symbol");
        } 
        break;
        case SYMBOL_VARIABLE:
        {
            generate_var_decl(sym);
        }
        break;
        case SYMBOL_CONST:
        {
            generate_const_decl(sym);
        }
        break;
        case SYMBOL_FUNCTION:
        {
            generate_function_decl(sym);
        }
        break;
        case SYMBOL_TYPE:
        {
            generate_type_decl(sym);
        }
        break;
        case SYMBOL_ENUM_CONST:
        {
            generate_enum_decl(sym);
        }
        break;
        default:
        {
            fatal("generation for symbol kind not implemented");
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
        generate(resolved[i]);
    }

    debug_breakpoint;
}