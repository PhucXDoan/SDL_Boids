// @TODO@ Implement the math functions.
internal inline vf2 polar(f32 rad)
{
	return { cosf(rad), sinf(rad) };
}

internal inline vf2 complex_mult(vf2 a, vf2 b)
{
	return { a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x };
}

internal inline f32 norm(vf2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

internal inline constexpr i32 pxd_sign(f32 f)
{
	return f < 0.0f ? -1 : f > 0.0f ? 1 : 0;
}

// @TODO@ Replace this with something better?
internal inline f32 signed_unit_curve(f32 a, f32 b, f32 x)
{
	if (x < 0.0f)
	{
		return -signed_unit_curve(a, b, -x);
	}
	else
	{
		f32 y = CLAMP(x, 0.0f, 1.0f);
		f32 z = ((a + b - 2.0f) * y * y + (-2.0f * a - b + 3.0f) * y + a) * y;
		return CLAMP(z, 0.0f, 1.0f);
	}
}

internal inline f32 dot(vf2 u, vf2 v)
{
	return u.x * v.x + u.y * v.y;
}

// @TODO@ Make this robust.
internal inline u64 rand_u64(u64* seed)
{
	*seed += 36133;
	return *seed * *seed * 20661081381 + *seed * 53660987 - 5534096 / *seed;
}

internal inline f32 rand_range(u64* seed, f32 min, f32 max)
{
	return (rand_u64(seed) % 0xFFFFFFFF / static_cast<f32>(0xFFFFFFFF)) * (max - min);
}

internal inline f32 inch_towards(f32 start, f32 end, f32 step)
{
	f32 delta = end - start;
	if (-step <= delta && delta <= step)
	{
		return end;
	}
	else
	{
		return start + step * pxd_sign(delta);
	}
}

internal inline vf2 inch_towards(vf2 start, vf2 end, f32 step)
{
	return { inch_towards(start.x, end.x, step), inch_towards(start.y, end.y, step) };
}

internal void render_line(SDL_Renderer* renderer, vf2 start, vf2 end)
{
	SDL_RenderDrawLine(renderer, static_cast<i32>(start.x), static_cast<i32>(WINDOW_HEIGHT - 1 - start.y), static_cast<i32>(end.x), static_cast<i32>(WINDOW_HEIGHT - 1 - end.y));
}

internal void render_lines(SDL_Renderer* renderer, vf2* points, i32 points_capacity)
{
	FOR_ELEMS(point, points, points_capacity - 1)
	{
		render_line(renderer, *point, points[point_index + 1]);
	}
}

internal void render_fill_rect(SDL_Renderer* renderer, vf2 bottom_left, vf2 dimensions)
{
	SDL_Rect rect = { static_cast<i32>(bottom_left.x), static_cast<i32>(WINDOW_HEIGHT - 1 - bottom_left.y - dimensions.y), static_cast<i32>(dimensions.x), static_cast<i32>(dimensions.y) };
	SDL_RenderFillRect(renderer, &rect);
}
