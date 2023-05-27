bool illegal_op_flag;

int64_t eval_long_unary_op(token_kind op, int64_t val)
{
    switch (op)
    {
        case TOKEN_ADD:         return +val;
        case TOKEN_SUB:         return -val;
        case TOKEN_NOT:         return !val;
        case TOKEN_BITWISE_NOT: return ~val;
        case TOKEN_INC:         return ++val;
        case TOKEN_DEC:         return --val;
        default: fatal("operation not implemented"); return 0;
    }
}

int64_t eval_long_binary_op(token_kind op, int64_t left, int64_t right)
{
    switch (op)
    {
        case TOKEN_ADD:         return left + right;
        case TOKEN_SUB:         return left - right;
        case TOKEN_MUL:         return left * right;
        case TOKEN_DIV:         return (right != 0) ? left / right : 0;
        case TOKEN_MOD:         return (right != 0) ? left % right : 0;
        case TOKEN_BITWISE_AND: return left & right;
        case TOKEN_BITWISE_OR:  return left | right;
        case TOKEN_LEFT_SHIFT:  return left << right;
        case TOKEN_RIGHT_SHIFT: return left >> right;
        case TOKEN_XOR:         return left ^ right;
        case TOKEN_EQ:          return left == right;
        case TOKEN_NEQ:         return left != right;
        case TOKEN_LT:          return left < right;
        case TOKEN_LEQ:         return left <= right;
        case TOKEN_GT:          return left > right;
        case TOKEN_GEQ:         return left >= right;
        case TOKEN_AND:         return left && right;
        case TOKEN_OR:          return left || right;
        default: fatal("operation not implemented"); return 0;
    }
}

uint64_t eval_ulong_unary_op(token_kind op, uint64_t val)
{
    switch (op)
    {
        case TOKEN_ADD:         return +val;
        case TOKEN_SUB:         return val; // tutaj jest różnica z signed types
        case TOKEN_NOT:         return !val;
        case TOKEN_BITWISE_NOT: return ~val;
        case TOKEN_INC:         return ++val;
        case TOKEN_DEC:         return --val;
        default: fatal("operation not implemented"); return 0;
    }
}

uint64_t eval_ulong_binary_op(token_kind op, uint64_t left, uint64_t right)
{
    switch (op)
    {
        case TOKEN_ADD:         return left + right;
        case TOKEN_SUB:         return left - right;
        case TOKEN_MUL:         return left * right;
        case TOKEN_DIV:         return (right != 0) ? left / right : 0;
        case TOKEN_MOD:         return (right != 0) ? left % right : 0;
        case TOKEN_BITWISE_AND: return left & right;
        case TOKEN_BITWISE_OR:  return left | right;
        case TOKEN_LEFT_SHIFT:  return left << right;
        case TOKEN_RIGHT_SHIFT: return left >> right;
        case TOKEN_XOR:         return left ^ right;
        case TOKEN_EQ:          return left == right;
        case TOKEN_NEQ:         return left != right;
        case TOKEN_LT:          return left < right;
        case TOKEN_LEQ:         return left <= right;
        case TOKEN_GT:          return left > right;
        case TOKEN_GEQ:         return left >= right;
        case TOKEN_AND:         return left && right;
        case TOKEN_OR:          return left || right;
        default: fatal("operation not implemented"); return 0;
    }
}

float eval_float_unary_op(token_kind op, float val)
{
    switch (op)
    {
        case TOKEN_ADD:         return +val;
        case TOKEN_SUB:         return -val;
        case TOKEN_NOT:         return !val;
        case TOKEN_INC:         return ++val;
        case TOKEN_DEC:         return --val;
        case TOKEN_BITWISE_NOT: illegal_op_flag = true; return 0;
        default: fatal("operation not implemented"); return 0;
    }
}

float eval_float_binary_op(token_kind op, float left, float right)
{
    switch (op)
    {
        case TOKEN_ADD:         return left + right;
        case TOKEN_SUB:         return left - right;
        case TOKEN_MUL:         return left * right;
        case TOKEN_DIV:         return (right != 0.0f) ? left / right : 0.0f;
        case TOKEN_MOD:         illegal_op_flag = true; return 0.0f;
        case TOKEN_BITWISE_AND: illegal_op_flag = true; return 0.0f;
        case TOKEN_BITWISE_OR:  illegal_op_flag = true; return 0.0f;
        case TOKEN_LEFT_SHIFT:  illegal_op_flag = true; return 0.0f;
        case TOKEN_RIGHT_SHIFT: illegal_op_flag = true; return 0.0f;
        case TOKEN_XOR:         illegal_op_flag = true; return 0.0f;
        case TOKEN_EQ:          return (left == right) ? 1.0f : 0.0f;
        case TOKEN_NEQ:         return (left != right) ? 1.0f : 0.0f;
        case TOKEN_LT:          return (left < right)  ? 1.0f : 0.0f;
        case TOKEN_LEQ:         return (left <= right) ? 1.0f : 0.0f;
        case TOKEN_GT:          return (left > right)  ? 1.0f : 0.0f;
        case TOKEN_GEQ:         return (left >= right) ? 1.0f : 0.0f;
        case TOKEN_AND:         return (left && right) ? 1.0f : 0.0f;
        case TOKEN_OR:          return (left || right) ? 1.0f : 0.0f;
        default: fatal("operation not implemented"); return 0.0f;
    }
}