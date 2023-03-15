#include "lexing.h"
#include "parsing.h"
#include "utils.h"

bool generate_line_hints = true;

int gen_indent;

char* gen_buf = null;
source_pos gen_pos;

#define gen_printf(...) buf_printf(gen_buf, __VA_ARGS__)
#define gen_printf_newline(...) (_gen_printf_newline(), gen_printf(__VA_ARGS__))

void _gen_printf_newline()
{
    gen_printf("\n%.*s", 2 * gen_indent, "                                                                               ");
    gen_pos.line++;
}

const char* parenthesize(const char* str, bool parenthesize)
{
    const char* result = parenthesize ? xprintf("(%s)", str) : str;
    return result;
}

void gen_line_hint(source_pos pos)
{
    if (generate_line_hints 
        && (gen_pos.line != pos.line || gen_pos.filename != pos.filename))
    {
        gen_printf_newline("#line %d ", pos.line);
        if (gen_pos.filename != pos.filename)
        {
            gen_printf("\"%s\" ", pos.filename);
        }
        gen_pos = pos;
        gen_pos.filename = pos.filename;
    }
}

void gen_stmt(stmt* s);
void gen_expr(expr* e);
void gen_func_decl(decl* d);

const char* gen_expr_str(expr* e)
{
    // tymczasowa podmiana, żeby wydrukował do nowego gen_buf
    char* temp = gen_buf;
    gen_buf = null;

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
        case TYPE_BOOL:
            return "bool";
        case TYPE_STRUCT:
        case TYPE_UNION:
            return t->symbol->name;
        invalid_default_case;
            
    }
    return null;
}

char* type_to_cdecl(type* type, const char* name)
{
    switch (type->kind)
    {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        case TYPE_STRUCT:
        case TYPE_UNION:
        {
            return xprintf("%s%s%s", cdecl_name(type), name ? " " : "", name ? name : "");
        }
        break;
        case TYPE_POINTER:
        {
            return type_to_cdecl(type->pointer.base_type, parenthesize(xprintf("*%s", name), name));
        }
        break;
        case TYPE_ARRAY:
        {
            return type_to_cdecl(type->array.base_type, parenthesize(xprintf("%s[%llu]", name, type->array.size), name));
        }
        break;
        case TYPE_LIST:
        {
            return xprintf("___list_hdr___ %s", parenthesize(xprintf("*%s", name), name));
        }
        break;
        case TYPE_FUNCTION:
        {
            char* result = null;
            buf_printf(result, "%s(", xprintf("*%s", name));
            if (type->function.param_count == 0 
                && type->function.receiver_type == null)
            {
                buf_printf(result, "void");
            }
            else
            {
                if (type->function.receiver_type)
                {
                    char* declstr = type_to_cdecl(type->function.receiver_type, "");
                    buf_printf(result, "%s%s", type->function.param_count == 0 ? "" : ", ", declstr);
                }

                for (size_t i = 0; i < type->function.param_count; i++)
                {
                    char* declstr = type_to_cdecl(type->function.param_types[i], "");
                    buf_printf(result, "%s%s", i == 0 ? "" : ", ", declstr);
                }
            }
            buf_printf(result, ")");
            return type_to_cdecl(type->function.return_type, result);
        }
        break;
        invalid_default_case;       
    }
    return null;
}

char* typespec_to_cdecl(typespec* t, const char* name)
{
    char* result = 0;
    switch (t->kind)
    {
        case TYPESPEC_NAME:
        {
            // środkowe s to spacja, jeśli zostało podane name
            result = xprintf("%s%s%s", t->name, name ? " " : "", name ? name : "");
            debug_breakpoint;
        }
        break;
        case TYPESPEC_POINTER:
        {
            // false podane zamiast name tymczasowo
            const char* wrapped_name = parenthesize(xprintf("*%s", name), false); 
            result = typespec_to_cdecl(t->pointer.base_type, wrapped_name);
        }
        break;
        case TYPESPEC_ARRAY:
        {
            const char* wrapped_name = parenthesize(xprintf("%s[%s]", name, gen_expr_str(t->array.size_expr)), *name);
            result = typespec_to_cdecl(t->array.base_type, wrapped_name);
        }
        break;
        case TYPESPEC_LIST:
        {
            result = xprintf("___list_hdr___ %s", parenthesize(xprintf("*%s", name), name));
        }
        break;
        case TYPESPEC_FUNCTION:
        {
            buf_printf(result, "%s(", parenthesize(xprintf("*%s", name), name));
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
        invalid_default_case;
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
        gen_line_hint(item.pos);
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
        {
            gen_expr(stmt->expr);
        }
        break;
        case STMT_DECL:
        {
            switch (stmt->decl_stmt.decl->kind)
            {
                case DECL_VARIABLE:
                {
                    if (stmt->decl_stmt.decl->variable.type == 0)
                    {
                        if (stmt->decl_stmt.decl->variable.expr->resolved_type)
                        {
                            char* decl_str = type_to_cdecl(
                                stmt->decl_stmt.decl->variable.expr->resolved_type,
                                stmt->decl_stmt.decl->name);
                            gen_printf(decl_str);
                            debug_breakpoint;                        
                        }
                        else
                        {
                            fatal("no type in variable declaration");
                        }
                    }
                    else
                    {
                        char* decl_str = typespec_to_cdecl(
                            stmt->decl_stmt.decl->variable.type,
                            stmt->decl_stmt.decl->name);
                        gen_printf(decl_str);
                        debug_breakpoint;
                    }                  

                    if (stmt->decl_stmt.decl->variable.expr)
                    {                       
                        gen_printf(" = ");
                        gen_expr(stmt->decl_stmt.decl->variable.expr);                        
                    }
                    else
                    {
                        // tutaj można by dać warning
                        debug_breakpoint;
                        gen_printf(" = {0}");
                    }
                }
                break;
                case DECL_CONST:
                default:
                {
                    fatal("only variable declarations allowed in statements");
                }
                break;
            }    
        }
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
        case STMT_DELETE:
        {
            if (stmt->delete.expr->resolved_type->kind == TYPE_LIST)
            {
                gen_printf("___list_free___(");
                gen_expr(stmt->delete.expr);
                gen_printf(")");
            }
            else
            {
                gen_printf("___free___(");
                gen_expr(stmt->delete.expr);
                gen_printf(")");
            }
        }
        break;
        invalid_default_case;
    }
}

void gen_stmt(stmt* stmt)
{
    gen_line_hint(stmt->pos);
    switch (stmt->kind)
    {
        case STMT_RETURN:
        {
            gen_printf_newline("return");
            if (stmt->expr)
            {
                gen_printf(" ");
                gen_expr(stmt->expr);
            }
            gen_printf(";");
        }
        break;
        case STMT_BREAK:
        {
            gen_printf_newline("break;");
        }
        break;
        case STMT_CONTINUE:
        {
            gen_printf_newline("continue;");
        }
        break;
        case STMT_BLOCK:
        {
            gen_printf_newline("");
            gen_stmt_block(stmt->block);
        }
        break;
        case STMT_IF_ELSE:
        {
            gen_printf_newline("if (");
            gen_expr(stmt->if_else.cond_expr);
            gen_printf(") ");
            gen_stmt_block(stmt->if_else.then_block);
            if (stmt->if_else.else_stmt)
            {
                gen_printf(" else ");
                gen_stmt(stmt->if_else.else_stmt);
            }
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
            if (stmt->for_stmt.init_stmt)
            {
                gen_simple_stmt(stmt->for_stmt.init_stmt);
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

void gen_expr_stub(expr* e)
{
    assert(e->kind == EXPR_STUB);
    assert(e->stub.original_expr);

    expr* receiver = null;
    if (e->stub.original_expr->kind == EXPR_CALL)
    {
        assert(e->stub.original_expr->call.method_receiver);
        receiver = e->stub.original_expr->call.method_receiver;
        assert(receiver->resolved_type);
        assert(receiver->resolved_type->kind == TYPE_LIST);
    }

    switch (e->stub.kind)
    {
        case STUB_EXPR_LIST_CAPACITY:
        {               
            gen_printf("___get_list_capacity___("), 
            gen_expr(receiver);
            gen_printf(")");
        }
        break;
        case STUB_EXPR_LIST_LENGTH:
        {
            gen_printf("___get_list_length___(");
            gen_expr(receiver);
            gen_printf(")");
        }
        break;
        case STUB_EXPR_LIST_REMOVE_AT:
        {
            fatal("not implemented");
        }
        break;
        case STUB_EXPR_LIST_FREE:
        {
            gen_printf("___list_free___(");
            gen_expr(receiver);
            gen_printf(")");
        }
        break;
        case STUB_EXPR_LIST_ADD:
        {
            assert(e->stub.original_expr->call.args_num == 1);
            gen_printf("___list_add___(");
            gen_expr(receiver);
            gen_printf(",(");
            gen_expr(e->stub.original_expr->call.args[0]);
            gen_printf("),");
            assert(receiver->resolved_type->kind == TYPE_LIST);
            type* base_type = receiver->resolved_type->list.base_type;
            char* type_str = type_to_cdecl(base_type, null);
            gen_printf("%s)", type_str);
        }
        break;
        case STUB_EXPR_CONSTRUCTOR:
        {
            assert(e->stub.original_expr->kind == EXPR_CALL);
            assert(e->stub.original_expr->call.resolved_function);
            gen_expr(e->stub.original_expr);
        }
        break;
        invalid_default_case;
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
        case EXPR_STRING:
        {
            gen_printf("\"%s\"", e->string_value);
        }
        break;
        case EXPR_NULL:
        {
            gen_printf("0");
        }
        break;
        case EXPR_BOOL:
        {
            gen_printf("%s", e->bool_value ? "true" : "false");
        }
        break;
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
            assert(e->call.resolved_function);
            assert(e->call.resolved_function->mangled_name);

            gen_printf(e->call.resolved_function->mangled_name);
            gen_printf("(");
            
            if (e->call.method_receiver)
            {
                gen_expr(e->call.method_receiver);
                if (e->call.args_num != 0)
                {
                    gen_printf(", ");
                }
            }
                        
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
        case EXPR_CAST:
        {
            gen_printf("(%s)(", typespec_to_cdecl(e->cast.type, null));
            gen_expr(e->cast.expr);
            gen_printf(")");
        }
        break;
        case EXPR_FIELD:
        {
            gen_expr(e->field.expr);
            assert(e->field.expr->resolved_type);
            if (e->field.expr->resolved_type->kind == TYPE_POINTER)
            {
                gen_printf("->%s", e->field.field_name);
                debug_breakpoint;
            }
            else
            {
                gen_printf(".%s", e->field.field_name);
            }
        }
        break;
        case EXPR_INDEX:
        {
            gen_expr(e->index.array_expr);
            if (e->index.array_expr->resolved_type->kind == TYPE_LIST)
            {
                gen_printf("->buffer");
            }
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
                char* decl = typespec_to_cdecl(e->compound.type, null);
                gen_printf("(%s){", decl);
            }
            else
            {
                char* decl = type_to_cdecl(e->resolved_type, null);
                gen_printf("(%s){", decl);
            }

            for (size_t i = 0; i < e->compound.fields_count; i++)
            {
                if (i != 0)
                {
                    gen_printf(", ");
                }

                compound_literal_field* field = e->compound.fields[i];
                if (field->field_name)
                {
                    gen_printf(".%s = ", field->field_name);                
                }

                gen_expr(field->expr);
            }

            gen_printf("}");
        }
        break;
        case EXPR_STUB:
        {
            gen_expr_stub(e);
        }
        break;
        case EXPR_NONE:
        case EXPR_NEW: 
        {
            if (e->new.type->kind == TYPESPEC_LIST)
            {
                assert(e->resolved_type);
                gen_printf("___list_initialize___(8, sizeof(%s), 0)",
                    e->resolved_type->list.base_type->name);
            }
            else
            {
                char* c = typespec_to_cdecl(e->new.type, null);
                gen_printf("(%s*)___alloc___(sizeof(%s))", c, c);
            }
        }
        break;
        case EXPR_AUTO: 
        {
            if (e->new.type->kind == TYPESPEC_LIST)
            {
                assert(e->resolved_type);
                gen_printf("___list_initialize___(8, sizeof(%s), 1)",
                    e->resolved_type->list.base_type->name);
            }
            else
            {
                char* c = typespec_to_cdecl(e->new.type, null);
                gen_printf("(%s*)___managed_alloc___(sizeof(%s))", c, c);
            }
        } 
        break;
        invalid_default_case;
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

void gen_func_decl(decl* d, const char* mangled_name)
{
    assert(d->kind == DECL_FUNCTION);

    if (d->function.return_type)
    {
        char* decl_str = typespec_to_cdecl(d->function.return_type, mangled_name);
        gen_printf_newline("%s(", decl_str);
    }
    else
    {
        gen_printf_newline("void %s(", mangled_name);
    }

    if (d->function.params.param_count == 0
        && d->function.method_receiver == null)
    {
        gen_printf("void");
    }
    else
    {
        if (d->function.method_receiver)
        {
            char* decl_str = typespec_to_cdecl(
                d->function.method_receiver->type, 
                d->function.method_receiver->name);
            gen_printf("%s", decl_str);
            if (d->function.params.param_count > 0)
            {
                gen_printf(", ");
            }
        }

        for (size_t i = 0; i < d->function.params.param_count; i++)
        {
            function_param param = d->function.params.params[i];
            if (i != 0)
            {
                gen_printf(", ");
            }

            char* decl_str = typespec_to_cdecl(param.type, param.name);
            gen_printf("%s", decl_str);
        }
    }
    gen_printf(")"); // bez średnika, bo potem może być ciało
}

void gen_forward_decls(symbol** resolved)
{
    for (size_t i = 0; i < buf_len(resolved); i++)
    {        
        symbol* sym = resolved[i];
        if (sym->decl)
        {
            switch (sym->decl->kind)
            {
                gen_line_hint(sym->decl->pos);
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
                    if (sym->decl->function.is_extern == false)
                    {
                        gen_func_decl(sym->decl, sym->mangled_name);
                        gen_printf(";");
                    }
                }
                break;
                case DECL_CONST:
                case DECL_VARIABLE:
                case DECL_ENUM:
                default:
                {
                    // nie potrzeba forward declaration
                };
                break;
            }
        }
        else
        {
            fatal("symbol without declaration");
        }
    }
}

void gen_symbol_decl(symbol* sym)
{
    // zakładamy, że forward declarations zostały już wygenerowane

    assert(sym);

    decl* decl = sym->decl;
    if (!decl)
    {
        return;
    }

    gen_line_hint(decl->pos);
    switch (decl->kind)
    {
        case DECL_VARIABLE:
        {
            gen_var_decl(decl, sym);
        }
        break;
        case DECL_CONST:
        {
            if (sym->type == type_int)
            {
                gen_printf_newline("enum { %s = ", sym->name);
                gen_expr(decl->const_decl.expr);
                gen_printf(" };");
            }
            else
            {            
                char* decl_str = type_to_cdecl(
                    sym->type, sym->name);
                gen_printf_newline(decl_str);                
                gen_printf(" = ");
                gen_expr(decl->const_decl.expr);
                gen_printf(";");
            }
        }
        break;
        case DECL_FUNCTION:
        {
            if (decl->function.is_extern == false)
            {
                gen_func_decl(decl, sym->mangled_name);
                gen_stmt_block(decl->function.stmts);
                gen_printf(";");
            }
        }
        break;
        case DECL_STRUCT:
        case DECL_UNION:
        {
            gen_aggregate(sym->decl);
        }
        break;
        case DECL_ENUM:
        {
            gen_printf_newline("enum { %s = ", sym->name);
            gen_expr(decl->const_decl.expr);
            gen_printf(" };");
            break;
        }      
        break;
        default:
        {
            fatal("generation for decl kind not implemented");
        }
        break;
    }
}


void gen_entry_point(void)
{
    symbol* main_found = null;
    const char* main_str = str_intern("main");

    for (symbol** it = global_symbols_list;
        it != buf_end(global_symbols_list);
        it++)
    {
        symbol* sym = *it;
        if (sym->name == main_str)
        {
            if (main_found)
            {
                fatal("only one function 'main' allowed");
            }
            else
            {
                main_found = sym;
            }
        }
    }

    if (main_found == null)
    {
        fatal("entry point function 'main' not defined");
    }
    
    if (main_found->mangled_name == str_intern("___main___0l___0s___0v"))
    {
        gen_printf(
"int main(int argc, char** argv) {\
  string* buf = 0;\
  for(int i = 0; i < argc; i++) {\
    string s = get_string(argv[i]);\
    buf_push(buf, s);\
  }\
  ___main___0l___0s___0v(buf);\
  buf_free(buf);\
}");

    }
    else if (main_found->mangled_name == str_intern("___main___0v"))
    {
        gen_printf(
"int main(int argc, char** argv) {\
  ___main___0v();\
}");
    }
    else
    {
        fatal("main has incorrect signature: either main(){} or main(string[]){} allowed");
    }
}

void gen_common_includes(void)
{
    char* test_file = "common_include.c";
    string_ref file_buf = read_file(test_file);
    gen_printf(file_buf.str);

#if 0
    gen_printf(
"#include <stddef.h>\n\
#include <stdlib.h>\n\
#include <stdio.h>\n\
#include <stdarg.h>\n\
#include <string.h>\n\
#include <stdint.h>\n\
#include <stdbool.h>"
);
#endif
}

void cgen_test(void)
{
    generate_line_hints = false;

    char* test_strs[] = {
#if 0
        "const x = 10",        
        "let y := (vec3){1, 2, 3}",
        "fn power_2(n:int):int{return n*n}",        
        "fn _main(){\
            let i : int = 5\
            let j = power_2(i)\
        }",
        "struct vec3 { x: int, y: int, z: int }",
        "const y = (float)12",
        "let u := (bool)(y > 11) && (y < 30)",
        "let i := (int)12 + 1",
#endif
        "fn f(){\
            let list := new int[]\
            list[10] = 12\
            delete list\
        }",
        
    };
    
    symbol** resolved = resolve_test_decls(test_strs, sizeof(test_strs) / sizeof(test_strs[0]), false);

    gen_printf_newline("// FORWARD DECLARATIONS\n");

    gen_forward_decls(resolved);

    gen_printf_newline("\n// DECLARATIONS\n");

    for (size_t i = 0; i < buf_len(resolved); i++)
    {
        gen_symbol_decl(resolved[i]);
    }

    debug_breakpoint;

    printf("%s\n", gen_buf);

    debug_breakpoint;
}

char* get_typespec_mangled_name(typespec* typ)
{
    assert(typ);
    char* result = null;
    buf_printf(result, "___");
    switch (typ->kind)
    {
        case TYPESPEC_NAME:
        {
            if (0 == strcmp(typ->name, "int"))
            {
                buf_printf(result, "0i");
            }
            else if (0 == strcmp(typ->name, "char"))
            {
                buf_printf(result, "0c");
            }
            else if (0 == strcmp(typ->name, "string"))
            {
                buf_printf(result, "0s");
            }
            else if (0 == strcmp(typ->name, "float"))
            {
                buf_printf(result, "0f");
            }
            else if (0 == strcmp(typ->name, "bool"))
            {
                buf_printf(result, "0b");
            }
            else if (0 == strcmp(typ->name, "null"))
            {
                buf_printf(result, "0n");
            }
            else if (0 == strcmp(typ->name, "void"))
            {
                buf_printf(result, "0v");
            }
            else
            {
                buf_printf(result, typ->name);
            }
        }
        break;    
        case TYPESPEC_ARRAY:
        {
            // ___0a_16___type
            buf_printf(result, "0a");

            resolved_expr* e = resolve_expr(typ->array.size_expr);
            size_t arr_count = e->val;

            buf_printf(result, xprintf("_%lld", arr_count));
            buf_printf(result, get_typespec_mangled_name(typ->array.base_type));
        }
        break;
        case TYPESPEC_LIST:
        {
            // ___0l___type
            buf_printf(result, "0l");
            buf_printf(result, get_typespec_mangled_name(typ->list.base_type));
        }
        break;
        case TYPESPEC_POINTER:
        {
            buf_printf(result, "0p");
            buf_printf(result, get_typespec_mangled_name(typ->pointer.base_type));
        }
        break;
        case TYPESPEC_NONE:
        invalid_default_case;
    }
    return result;
}

char* get_function_mangled_name(decl* dec)
{
    // ___function_name___arg_type1___arg_type2___ret_type
    
    assert(dec);
    assert(dec->kind == DECL_FUNCTION);    

    if (dec->function.is_extern)
    {
        return dec->name;
    }

    char* mangled = null;
           
    if (dec->function.method_receiver)
    {
        typespec* t = dec->function.method_receiver->type;
        char* mangled_rec = get_typespec_mangled_name(t);
        buf_printf(mangled, mangled_rec);
    }

    buf_printf(mangled, "___");
    buf_printf(mangled, dec->name);
    for (size_t i = 0; i < dec->function.params.param_count; i++)
    {
        typespec* t = dec->function.params.params[i].type;
        char* mangled_arg = get_typespec_mangled_name(t);
        buf_printf(mangled, mangled_arg);
    }

    if (dec->function.return_type)
    {
        char* mangled_ret = get_typespec_mangled_name(dec->function.return_type);
        buf_printf(mangled, mangled_ret);
    }
    else
    {
        buf_printf(mangled, "___0v");
    }
    
    const char* result = str_intern(mangled);
    buf_free(mangled);
    return result;
}

void mangled_names_test()
{
    // uwaga: reordering podczas resolve może zepsuć test
    char* test_strs[] = {
        "struct tee { i: int }",
        "union zet { s: tee, t: zet* }",
        "fn funkcja (x: int, y: int) { return } ",     
        "fn funkcja ( t: tee, z: zet, i: int[16]*) { return }",
        "fn funkcja ( t: tee, z: zet*, i: int[16]*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[16]*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[16]*) : int { return 1 }",
        "fn funkcja ( t: zet, z: zet, i: int[17]*) { return }",
        "fn funkcja ( t: zet*, z: int[17]*, i: zet*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[]) { return }",
        "fn (i: int) funkcja (o: int) { return }",
        "fn (s: int) funkcja () : int { return s }",
        "fn (x: int) funkcja () : { }",
    };

    char* cmp_strs[] = {
        "not tested",
        "not tested",
        "___funkcja___0i___0i___0v",       
        "___funkcja___tee___zet___0p___0a_16___0i___0v",
        "___funkcja___tee___0p___zet___0p___0a_16___0i___0v",
        "___funkcja___0p___tee___zet___0p___0a_16___0i___0v",
        "___funkcja___0p___tee___zet___0p___0a_16___0i___0i",
        "___funkcja___zet___zet___0p___0a_17___0i___0v",
        "___funkcja___0p___zet___0p___0a_17___0i___0p___zet___0v",
        "___funkcja___0p___tee___zet___0l___0i___0v",
        "___0i___funkcja___0i___0v",
        "___0i___funkcja___0i",
        "___0i___funkcja___0v",
    };

    assert((sizeof(test_strs) / sizeof(test_strs[0])) == sizeof(cmp_strs) / sizeof(cmp_strs[0]))

    symbol** resolved = resolve_test_decls(test_strs, sizeof(test_strs) / sizeof(test_strs[0]), false);
    for (size_t i = 2 /* dwa pierwsze pomijamy!*/; i < buf_len(resolved); i++)
    {
        symbol* sym = resolved[i]; 
        char* mangled_name = get_function_mangled_name(sym->decl);
        if (0 != strcmp(mangled_name, cmp_strs[i]))
        {
            // błąd!
            debug_breakpoint;
        }
    }
    debug_breakpoint;
}