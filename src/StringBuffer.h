#pragma once

#define STRING_BUFFER_STACK_SINGLE_(NAME, CAPACITY)\
(NAME);\
char STRING_BUFFER_STACK_SINGLE_##__LINE__##_##NAME##[(CAPACITY)];\
(NAME) = { 0, ARRAY_CAPACITY(STRING_BUFFER_STACK_SINGLE_##__LINE__##_##NAME), STRING_BUFFER_STACK_SINGLE_##__LINE__##_##NAME };

#define STRING_BUFFER_STACK_ARRAY_(NAME, CAPACITY, AMOUNT)\
(NAME)[(AMOUNT)];\
char STRING_BUFFER_STACK_ARRAY_##__LINE__##_##NAME##[(AMOUNT)][(CAPACITY)];\
do\
{\
	for (memsize STRING_BUFFER_STACK_ARRAY_##__LINE__##_I = 0; STRING_BUFFER_STACK_ARRAY_##__LINE__##_I < (AMOUNT); ++STRING_BUFFER_STACK_ARRAY_##__LINE__##_I)\
	{\
		(NAME)[STRING_BUFFER_STACK_ARRAY_##__LINE__##_I] = { 0, ARRAY_CAPACITY(STRING_BUFFER_STACK_ARRAY_##__LINE__##_##NAME##[(STRING_BUFFER_STACK_ARRAY_##__LINE__##_I)]), STRING_BUFFER_STACK_ARRAY_##__LINE__##_##NAME##[(STRING_BUFFER_STACK_ARRAY_##__LINE__##_I)] };\
	}\
}\
while (false)

#define STRING_BUFFER_STACK(...) EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, STRING_BUFFER_STACK_ARRAY_, STRING_BUFFER_STACK_SINGLE_)(__VA_ARGS__))
#define FOR_STRING_BUFFER(NAME, STRING_BUFFER) for (memsize NAME##_index = 0; NAME##_index < (STRING_BUFFER)->count; ++NAME##_index) if (const char NAME = (STRING_BUFFER)->buffer[NAME##_index]; false); else

#if DEBUG
#define DEBUG_print_StringBuffer(STRING_BUFFER)\
DEBUG_printf("\"");\
for (memsize DEBUG_PRINT_STRING_BUFFER_##__LINE__##_I = 0; DEBUG_PRINT_STRING_BUFFER_##__LINE__##_I < (STRING_BUFFER)->count; ++DEBUG_PRINT_STRING_BUFFER_##__LINE__##_I)\
{\
	if ((STRING_BUFFER)->buffer[DEBUG_PRINT_STRING_BUFFER_##__LINE__##_I] == '"')\
	{\
		DEBUG_printf("\\\"");\
	}\
	else\
	{\
		DEBUG_printf("%c", (STRING_BUFFER)->buffer[DEBUG_PRINT_STRING_BUFFER_##__LINE__##_I]);\
	}\
}\
DEBUG_printf("\"");
#endif

struct StringBuffer
{
	memsize count;
	memsize capacity;
	char*   buffer;
};

void string_buffer_eat(StringBuffer* string_buffer, memsize amount)
{
	ASSERT(amount <= string_buffer->count);

	string_buffer->count -= amount;

	FOR_RANGE(i, 0, string_buffer->count)
	{
		string_buffer->buffer[i] = string_buffer->buffer[amount + i];
	}
}

bool32 string_buffer_equal(StringBuffer* string_buffer, strlit cstr)
{
	for (memsize i = 0; i < string_buffer->count; ++i)
	{
		if (cstr[i] == '\0' || string_buffer->buffer[i] != cstr[i])
		{
			return false;
		}
	}

	return cstr[string_buffer->count] == '\0';
}

void string_buffer_add(StringBuffer* string_buffer, char c)
{
	ASSERT(string_buffer->count < string_buffer->capacity);
	string_buffer->buffer[string_buffer->count] = c;
	++string_buffer->count;
}

void string_buffer_add(StringBuffer* string_buffer, strlit c)
{
	for (memsize i = 0; c[i] != '\0'; ++i)
	{
		string_buffer_add(string_buffer, c[i]);
	}
}

// @TODO@ Improve! Right now this is a hack.
void string_buffer_add(StringBuffer* string_buffer, f32 f)
{
	char buffer[32];
	snprintf(buffer, ARRAY_CAPACITY(buffer), "%f", f);

	FOR_ELEMS(it, buffer)
	{
		if (*it == '\0')
		{
			break;
		}
		else
		{
			string_buffer_add(string_buffer, *it);
		}
	}
}

// @TODO@ Improve! Right now this is a hack.
void string_buffer_add(StringBuffer* string_buffer, i32 d)
{
	char buffer[32];
	snprintf(buffer, ARRAY_CAPACITY(buffer), "%d", d);

	FOR_ELEMS(it, buffer)
	{
		if (*it == '\0')
		{
			break;
		}
		else
		{
			string_buffer_add(string_buffer, *it);
		}
	}
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
		bool32 is_negative = string_buffer->buffer[0] == '-';
		*result = 0;

		FOR_ELEMS(c, string_buffer->buffer + (is_negative ? 1 : 0), string_buffer->count - (is_negative ? 1 : 0))
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
		bool32 is_negative   = string_buffer->buffer[0] == '-';
		bool32 has_digit     = false;
		i32    decimal_index = -1;
		f32    whole         = 0.0f;
		f32    fractional    = 0.0f;

		FOR_ELEMS(c, string_buffer->buffer + (is_negative ? 1 : 0), string_buffer->count - (is_negative ? 1 : 0))
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
			for (memsize c_index = string_buffer->count; --c_index > decimal_index;)
			{
				if (IN_RANGE(string_buffer->buffer[c_index], '0', '9' + 1))
				{
					fractional += string_buffer->buffer[c_index] - '0';
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
