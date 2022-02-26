#define StringBuffer_Stack(NAME, CAPACITY) (NAME); char (STRING_STACK_BUFFER_##NAME##__LINE__)[(CAPACITY)]; (NAME) = { 0, ARRAY_CAPACITY(STRING_STACK_BUFFER_##NAME##__LINE__), (STRING_STACK_BUFFER_##NAME##__LINE__) }

#if DEBUG
#define DEBUG_print_StringBuffer(STRING_BUFFER)\
do\
{\
	DEBUG_printf("\"");\
	FOR_ELEMS(c, (STRING_BUFFER)->data, (STRING_BUFFER)->count)\
	{\
		DEBUG_printf("%c", *c);\
	}\
	DEBUG_printf("\"");\
}\
while (false)
#endif

struct StringBuffer
{
	i32   count;
	i32   capacity;
	char* data;
};

internal inline void push_char(StringBuffer* string_buffer, char c)
{
	ASSERT(string_buffer->count < string_buffer->capacity);
	string_buffer->data[string_buffer->count] = c;
	++string_buffer->count;
}

internal inline bool32 string_buffer_equal(StringBuffer* string_buffer, strlit cstr)
{
	for (i32 i = 0;; ++i)
	{
		if (i == string_buffer->count)
		{
			return cstr[i] == '\0';
		}
		else if (cstr[i] == '\0' || cstr[i] != string_buffer->data[i])
		{
			return false;
		}
	}
}

internal inline bool32 string_buffer_equal(strlit cstr, StringBuffer* string_buffer)
{
	return string_buffer_equal(string_buffer, cstr);
}

/* @DOCUMENTATION@
	Parsing of the string buffer is done according to these rules:
		> Legal characters are digits (0-9) and '-'.
		> Must have least one digit.
		> There can be at most one '-'.
		> If '-' exists, it must be the first character.
		> The integer must be representable.
	Note that:
		> No other representation is allowed (e.g. hexadecimal).
		> The value of `result` is undefined if a rule is violated.
*/
// @TODO@ Make this robust.
bool32 string_buffer_parse_i32(StringBuffer* string_buffer, i32* result)
{
	if (string_buffer->count > 0)
	{
		bool32 is_negative = string_buffer->data[0] == '-';
		*result = 0;

		FOR_ELEMS(c, string_buffer->data + (is_negative ? 1 : 0), string_buffer->count - (is_negative ? 1 : 0))
		{
			if (IN_RANGE(*c, '0', '9' + 1))
			{
				*result *= 10; // @TODO@ Overflow!
				*result += *c - '0';
			}
			else
			{
				return false;
			}
		}

		if (is_negative)
		{
			*result *= -1;
		}

		return true;
	}
	else
	{
		return false;
	}
}

/* @DOCUMENTATION@
	Parsing of the string buffer is done according to these rules:
		> Legal characters are digits (0-9), '.', and '-'.
		> Must have least one digit.
		> There can be at most one '.'.
		> There can be at most one '-'.
		> If '-' exists, it must be the first character.
	Note that:
		> No other representation is allowed (e.g. `INFINITY`).
		> The precision of the result compared to the string is not guaranteed equal.
		> The value of `result` is undefined if a rule is violated.
*/
// @TODO@ Make this robust.
bool32 string_buffer_parse_f32(StringBuffer* string_buffer, f32* result)
{
	if (string_buffer->count > 0)
	{
		bool32 is_negative   = string_buffer->data[0] == '-';
		bool32 has_digit     = false;
		i32    decimal_index = -1;
		f32    whole         = 0.0f;
		f32    fractional    = 0.0f;

		FOR_ELEMS(c, string_buffer->data + (is_negative ? 1 : 0), string_buffer->count - (is_negative ? 1 : 0))
		{
			if (IN_RANGE(*c, '0', '9' + 1))
			{
				whole     *= 10.0f; // @TODO@ Overflow!
				whole     += *c - '0';
				has_digit  = true;
			}
			else if (*c == '.')
			{
				decimal_index = c_index + is_negative;
				break;
			}
			else
			{
				return false;
			}
		}

		if (decimal_index != -1)
		{
			for (i32 c_index = string_buffer->count - 1; c_index > decimal_index; --c_index)
			{
				if (IN_RANGE(string_buffer->data[c_index], '0', '9' + 1))
				{
					fractional += string_buffer->data[c_index] - '0';
					fractional /= 10.0f;
					has_digit   = true;
				}
				else
				{
					return false;
				}
			}
		}

		if (has_digit)
		{
			*result = (is_negative ? -1.0f : 1.0f) * (whole + fractional);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
