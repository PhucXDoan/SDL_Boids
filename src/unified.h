#pragma once
#include <stdint.h>

#if DEBUG
#define ASSERT(EXPRESSION) do { if (!(EXPRESSION)) { *static_cast<int*>(0) = 0; } } while (false)
#else
#define ASSERT(EXPRESSION)
#endif

#define internal static
#define persist  static
#define global   static

#define EXPAND_(X) X
#define OVERLOADED_MACRO_2_(_0, _1, MACRO, ...)     MACRO
#define OVERLOADED_MACRO_3_(_0, _1, _2, MACRO, ...) MACRO
#define CAPACITY_OF_ARRAY_(XS)                      (sizeof(XS) / sizeof((XS)[0]))
#define CAPACITY_OF_MEMBER_(TYPE, MEMBER)           (CAPACITY_OF_ARRAY_(((TYPE*) 0)->MEMBER))
#define PUSH_TYPE_(ARENA, TYPE)                     (reinterpret_cast<TYPE*>(push_size((ARENA), sizeof(TYPE)          )))
#define PUSH_ARRAY_(ARENA, TYPE, COUNT)             (reinterpret_cast<TYPE*>(push_size((ARENA), sizeof(TYPE) * (COUNT))))
#define FOR_BUFFER_(NAME, XS, COUNT)                for (i32 (NAME##_index) = 0; (NAME##_index) < (COUNT); ++(NAME##_index)) if (const auto (NAME) = (XS)[(NAME##_index)]; false); else
#define FOR_ARRAY_(NAME, XS)                        FOR_BUFFER_(NAME, XS, ARRAY_CAPACITY(XS))
#define FOR_BUFFER_PTR_(NAME, XS, COUNT)            for (i32 (NAME##_index) = 0; (NAME##_index) < (COUNT); ++(NAME##_index)) if (const auto (NAME) = &(XS)[(NAME##_index)]; false); else
#define FOR_ARRAY_PTR_(NAME, XS)                    FOR_BUFFER_PTR_(NAME, XS, ARRAY_CAPACITY(XS))

#define ARRAY_CAPACITY(...)         (EXPAND_(OVERLOADED_MACRO_2_(__VA_ARGS__, CAPACITY_OF_MEMBER_, CAPACITY_OF_ARRAY_)(__VA_ARGS__)))
#define PUSH(...)                   (EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, PUSH_ARRAY_, PUSH_TYPE_)(__VA_ARGS__)))
#define FOR_RANGE(NAME, MINI, MAXI) for (i32 (NAME) = (MINI); (NAME) < (MAXI); ++(NAME))
#define FOR_ELEMS(...)              EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_BUFFER_, FOR_ARRAY_)(__VA_ARGS__))
#define FOR_ELEMS_PTR(...)          EXPAND_(OVERLOADED_MACRO_3_(__VA_ARGS__, FOR_BUFFER_PTR_, FOR_ARRAY_PTR_)(__VA_ARGS__))
#define IN_RANGE(X, MINI, MAXI)     ((MINI) <= (X) && (X) < (MAXI))
#define MINIMUM(X, Y)               ((X) <= (Y) ? (X) : (Y))
#define MAXIMUM(X, Y)               ((X) >= (Y) ? (X) : (Y))
#define CLAMP(X, MINI, MAXI)        ((X) < (MINI) ? (MINI) : (X) > (MAXI) ? (MAXI) : (X))
#define SWAP(X, Y)                  do { auto TEMP_SWAP_MACRO_##__LINE__ = X; X = Y; Y = TEMP_SWAP_MACRO_##__LINE__; } while (false)
#define flag_struct(NAME, TYPE)\
enum struct NAME : TYPE;\
inline TYPE operator+  (NAME  a        ) { return     static_cast<TYPE>(a);            }\
inline NAME operator&  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) & (+b)); }\
inline NAME operator|  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) | (+b)); }\
inline NAME operator^  (NAME  a, NAME b) { return     static_cast<NAME>( (+a) ^ (+b)); }\
inline NAME operator<< (NAME  a, i32  n) { return     static_cast<NAME>( (+a) << n  ); }\
inline NAME operator>> (NAME  a, i32  n) { return     static_cast<NAME>( (+a) >> n  ); }\
inline NAME operator~  (NAME  a        ) { return     static_cast<NAME>( ~+a        ); }\
inline NAME operator&= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) & (+b)); }\
inline NAME operator|= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) | (+b)); }\
inline NAME operator^= (NAME& a, NAME b) { return a = static_cast<NAME>( (+a) ^ (+b)); }\
inline NAME operator<<=(NAME& a, i32  n) { return a = static_cast<NAME>( (+a) << n  ); }\
inline NAME operator>>=(NAME& a, i32  n) { return a = static_cast<NAME>( (+a) >> n  ); }\
enum struct NAME : TYPE

#define vf2(X, Y      ) (vf2 { static_cast<f32>(X), static_cast<f32>(Y)                                           })
#define vf3(X, Y, Z   ) (vf2 { static_cast<f32>(X), static_cast<f32>(Y), static_cast<f32>(Z)                      })
#define vf4(X, Y, Z, W) (vf2 { static_cast<f32>(X), static_cast<f32>(Y), static_cast<f32>(Z), static_cast<f32>(W) })

#if DEBUG
#include <windows.h>
#include <stdio.h>
#define DEBUG_printf(FSTR, ...) do { char TEMP_DEBUG_PRINTF_##__LINE__[512]; sprintf_s(TEMP_DEBUG_PRINTF_##__LINE__, sizeof(TEMP_DEBUG_PRINTF_##__LINE__), (FSTR), __VA_ARGS__); OutputDebugStringA(TEMP_DEBUG_PRINTF_##__LINE__); } while (false)
#define DEBUG_NOP               do {} while (false)
#endif

typedef uint8_t     byte;
typedef uint64_t    memsize;
typedef const char* strlit;
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef uint8_t     bool8;
typedef uint16_t    bool16;
typedef uint32_t    bool32;
typedef uint64_t    bool64;
typedef float       f32;
typedef double      f64;

struct vf2
{
	union
	{
		struct { f32 x; f32 y; };
		struct { f32 i; f32 j; };
		f32 coordinates[2];
	};
};

struct vf3
{
	union
	{
		struct { f32 x; f32 y; f32 z; };
		struct { f32 i; f32 j; f32 k; };
		struct { f32 r; f32 g; f32 b; };
		f32 coordinates[3];
		vf2 xy;
		vf2 ij;
		vf2 rb;
	};
};

struct vf4
{
	union
	{
		struct { f32 x; f32 y; f32 z; f32 w; };
		struct { f32 i; f32 j; f32 k; f32 l; };
		struct { f32 r; f32 g; f32 b; f32 a; };
		f32 coordinates[4];
		vf2 xy;
		vf2 ij;
		vf2 rb;
		vf3 xyz;
		vf3 ijk;
		vf3 rbg;
	};
};

struct memarena
{
	memsize size;
	byte*   base;
	memsize used;
};

internal inline byte* push_size(memarena* arena, memsize size)
{
	ASSERT(arena->used + size <= arena->size);
	byte* allocation = arena->base + arena->used;
	arena->used += size;
	return allocation;
}

internal inline constexpr bool32 operator+ (vf2  v       ) { return v.x || v.y;               }
internal inline constexpr bool32 operator+ (vf3  v       ) { return v.x || v.y || v.z;        }
internal inline constexpr bool32 operator+ (vf4  v       ) { return v.x || v.y || v.z || v.w; }
internal inline constexpr vf2    operator- (vf2  v       ) { return { -v.x, -v.y             }; }
internal inline constexpr vf3    operator- (vf3  v       ) { return { -v.x, -v.y, -v.z       }; }
internal inline constexpr vf4    operator- (vf4  v       ) { return { -v.x, -v.y, -v.z, -v.w }; }
internal inline constexpr bool32 operator==(vf2  u, vf2 v) { return u.x == v.x && u.y == v.y;                             }
internal inline constexpr bool32 operator==(vf3  u, vf3 v) { return u.x == v.x && u.y == v.y && u.z == v.z;               }
internal inline constexpr bool32 operator==(vf4  u, vf4 v) { return u.x == v.x && u.y == v.y && u.z == v.z && u.w == v.w; }
internal inline constexpr bool32 operator!=(vf2  u, vf2 v) { return !(u == v); }
internal inline constexpr bool32 operator!=(vf3  u, vf3 v) { return !(u == v); }
internal inline constexpr bool32 operator!=(vf4  u, vf4 v) { return !(u == v); }
internal inline constexpr vf2    operator+ (vf2  u, vf2 v) { return { u.x + v.x, u.y + v.y                       }; }
internal inline constexpr vf3    operator+ (vf3  u, vf3 v) { return { u.x + v.x, u.y + v.y, u.z + v.z            }; }
internal inline constexpr vf4    operator+ (vf4  u, vf4 v) { return { u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w }; }
internal inline constexpr vf2    operator- (vf2  u, vf2 v) { return { u.x - v.x, u.y - v.y                       }; }
internal inline constexpr vf3    operator- (vf3  u, vf3 v) { return { u.x - v.x, u.y - v.y, u.z - v.z            }; }
internal inline constexpr vf4    operator- (vf4  u, vf4 v) { return { u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w }; }
internal inline constexpr vf2    operator/ (vf2  v, f32 k) { return { v.x / k, v.y / k                   }; }
internal inline constexpr vf3    operator/ (vf3  v, f32 k) { return { v.x / k, v.y / k, v.z / k          }; }
internal inline constexpr vf4    operator/ (vf4  v, f32 k) { return { v.x / k, v.y / k, v.z / k, v.w / k }; }
internal inline constexpr vf2    operator* (vf2  v, f32 k) { return { v.x * k, v.y * k                   }; }
internal inline constexpr vf3    operator* (vf3  v, f32 k) { return { v.x * k, v.y * k, v.z * k          }; }
internal inline constexpr vf4    operator* (vf4  v, f32 k) { return { v.x * k, v.y * k, v.z * k, v.w * k }; }
internal inline constexpr vf2    operator* (f32  k, vf2 v) { return v * k; }
internal inline constexpr vf3    operator* (f32  k, vf3 v) { return v * k; }
internal inline constexpr vf4    operator* (f32  k, vf4 v) { return v * k; }
internal inline constexpr vf2&   operator+=(vf2& u, vf2 v) { return u = u + v; }
internal inline constexpr vf3&   operator+=(vf3& u, vf3 v) { return u = u + v; }
internal inline constexpr vf4&   operator+=(vf4& u, vf4 v) { return u = u + v; }
internal inline constexpr vf2&   operator-=(vf2& u, vf2 v) { return u = u - v; }
internal inline constexpr vf3&   operator-=(vf3& u, vf3 v) { return u = u - v; }
internal inline constexpr vf4&   operator-=(vf4& u, vf4 v) { return u = u - v; }
internal inline constexpr vf2&   operator*=(vf2& v, f32 k) { return v = v * k; }
internal inline constexpr vf3&   operator*=(vf3& v, f32 k) { return v = v * k; }
internal inline constexpr vf4&   operator*=(vf4& v, f32 k) { return v = v * k; }
internal inline constexpr vf2&   operator/=(vf2& v, f32 k) { return v = v / k; }
internal inline constexpr vf3&   operator/=(vf3& v, f32 k) { return v = v / k; }
internal inline constexpr vf4&   operator/=(vf4& v, f32 k) { return v = v / k; }

constexpr f32 TAU = 6.28318530717958647692f;
