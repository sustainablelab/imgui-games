#pragma once

#include <cstdlib>
#include <cstring>
#include <cmath>


inline int imin(const int lhs, const int rhs)
{
    return ((lhs <= rhs) * lhs) + ((rhs < lhs) * rhs);
}

inline float clampf(const float v, const float vmin, const float vmax)
{
    return std::fmax(vmin, std::fmin(v, vmax));
}


struct Vec2
{
    float x;
    float y;
};


struct Line
{
    Vec2 tail;
    Vec2 head;
};


inline Vec2 vec2_sub(const Vec2* const lhs, const Vec2* const rhs)
{
    return Vec2{lhs->x - rhs->x, lhs->y - rhs->y};
}

inline void vec2_scale_compound_add(Vec2* const lhs, const Vec2* const rhs, const float scale)
{
    lhs->x += rhs->x * scale;
    lhs->y += rhs->y * scale;
}

inline void vec2_compound_add(Vec2* const lhs, const Vec2* const rhs)
{
    lhs->x += rhs->x;
    lhs->y += rhs->y;
}

inline void vec2_negate(Vec2* const dst, const Vec2* const src)
{
    dst->x = -src->x;
    dst->y = -src->y;
}

inline void vec2_set_zero(Vec2* const dst)
{
    std::memset(dst, 0, sizeof(Vec2));
}

inline void vec2_set_zero_n(Vec2* const dst, const int n)
{
    std::memset(dst, 0, sizeof(Vec2) * n);
}

inline void vec2_set(Vec2* const dst, const Vec2* const set_value)
{
    std::memcpy(dst, set_value, sizeof(Vec2));
}

inline void vec2_set_n(Vec2* const dst, const Vec2* const set_value, const int n)
{
    for (int i = 0; i < n; ++i)
    {
        vec2_set(dst + i, set_value);
    }
}

inline void vec2_copy_n(Vec2* const dst, const Vec2* const src, const int n)
{
    std::memcpy(dst, src, sizeof(Vec2) * n);
}

inline void vec2_set_random_uniform_unit(Vec2* const dst)
{
    #define MAX_RAND_I 10000
    #define MAX_RAND_F 10000.f
    #define N_RAND 2.f * ((std::rand() % MAX_RAND_I) / MAX_RAND_F) - 1.f

    dst->x = N_RAND;
    dst->y = N_RAND;

    #undef MAX_RAND_I
    #undef MAX_RAND_F
    #undef N_RAND
}

inline void vec2_set_random_uniform_scaled(Vec2* const dst, const float scale)
{
    vec2_set_random_uniform_unit(dst);
    dst->x *= scale;
    dst->y *= scale;
}

inline float vec2_dot(const Vec2* const lhs, const Vec2* const rhs)
{
    return lhs->x * rhs->x + lhs->y * rhs->y;
}

inline Vec2 vec2_lerp(const Vec2* const lhs, const Vec2* const rhs, const float alpha)
{
    const float beta = 1.f - alpha;
    return Vec2{lhs->x * alpha + rhs->x * beta, lhs->y * alpha + rhs->y * beta};
}

inline float vec2_length_squared(Vec2* const src)
{
    return vec2_dot(src, src);
}

inline float vec2_length_manhattan(Vec2* const src)
{
    return std::abs(src->x) + std::abs(src->y);
}

inline float vec2_dist_squared(Vec2* const lhs, Vec2* const rhs)
{
    const float dx = lhs->x - rhs->x;
    const float dy = lhs->y - rhs->y;
    return dx * dx + dy * dy;
}

inline float vec2_near(Vec2* const lhs, Vec2* const rhs, const float tol)
{
    const float dx = lhs->x - rhs->x;
    const float dy = lhs->y - rhs->y;
    return std::abs(dx) < tol and std::abs(dy) < tol;
}

inline void vec2_clamp(Vec2* const dst, const float vmin, const float vmax)
{
    dst->x = clampf(dst->x, vmin, vmax);
    dst->y = clampf(dst->y, vmin, vmax);
}

inline void vec2_scale(Vec2* const src, const float scale)
{
    src->x *= scale;
    src->y *= scale;
}

inline void vec2_reflect(Vec2* const r, const Vec2* const d, const Vec2* const n)
{
    // r = d - 2 * dot(d, n) * n
    const float dot_dn = vec2_dot(d, n);

    r->x = d->x - 2.f * dot_dn * n->x;
    r->y = d->y - 2.f * dot_dn * n->y;
}

inline void vec2_project(Vec2* const r, const Vec2* const d, const Vec2* const n)
{
    // r = dot(d, n) * n
    const float dot_dn = vec2_dot(d, n);
    r->x = dot_dn * n->x;
    r->y = dot_dn * n->y;
}

inline float vec2_cross_product(const Vec2* const lhs, const Vec2* const rhs)
{
    return (lhs->x * rhs->y) - (lhs->y * rhs->x);
}

inline bool vec2_segment_segment_intercept(
    Vec2* intercept,
    const Vec2* const p,
    const Vec2* const p_head,
    const Vec2* const q,
    const Vec2* const q_head)
{
    const Vec2 r{p_head->x - p->x, p_head->y - p->y};
    const Vec2 s{q_head->x - q->x, q_head->y - q->y};
    const float r_cross_s = vec2_cross_product(&r, &s);

    // Parallel case
    if (std::abs(r_cross_s) > 0.f)
    {
      const Vec2 q_m_p{q->x - p->x, q->y - p->y};
      const float q_m_p_cross_r = vec2_cross_product(&q_m_p, &r);
      const float u = q_m_p_cross_r / r_cross_s;
      const float t = vec2_cross_product(&q_m_p, &s) / r_cross_s;

      // Intersection on segment
      if (0 <= u && u <= 1 && 0 <= t && t <= 1)
      {
          intercept->x = p->x + t * r.x;
          intercept->y = p->y + t * r.y;
          return true;
      }
    }

    // Intersection off segment
    return false;
}

inline bool vec2_segment_segment_intercept_check(
    const Vec2* const p,
    const Vec2* const p_head,
    const Vec2* const q,
    const Vec2* const q_head)
{
    const Vec2 r{p_head->x - p->x, p_head->y - p->y};
    const Vec2 s{q_head->x - q->x, q_head->y - q->y};
    const float r_cross_s = vec2_cross_product(&r, &s);

    // Parallel case
    if (std::abs(r_cross_s) > 0.f)
    {
      const Vec2 q_m_p{q->x - p->x, q->y - p->y};
      const float q_m_p_cross_r = vec2_cross_product(&q_m_p, &r);
      const float u = q_m_p_cross_r / r_cross_s;
      const float t = vec2_cross_product(&q_m_p, &s) / r_cross_s;
      return 0 <= u && u <= 1 && 0 <= t && t <= 1;
    }

    // Intersection off segment
    return false;
}

inline bool vec2_within_aabb(const Vec2* const top, const Vec2* const bot, const Vec2* const point, const float tolerance)
{
    const float min_x = std::fmin(top->x, bot->x);
    const float min_y = std::fmin(top->y, bot->y);
    const float max_x = std::fmax(top->x, bot->x);
    const float max_y = std::fmax(top->y, bot->y);
    return ((min_x - tolerance) < point->x && point->x < (max_x + tolerance)) &&
           ((min_y - tolerance) < point->y && point->y < (max_y + tolerance));
}

inline bool vec2_near_line(const Line* const line, const Vec2* const point, const float tolerance)
{
    // Rearrangement of:
    //
    // d_perp = | (head.x - tail.x) * (tail.y - point.y) - (tail.x - point.x) * (head.y - tail.y) |  < tolerance
    //          -----------------------------------------------------------------------------------
    //                          sqrt((head.x - tail.x) ^ 2 + (head.y - tail.y) ^ 2)
    //
    const float dx_ht = line->head.x - line->tail.x;
    const float dy_ht = line->head.y - line->tail.y;
    const float dx_tp = line->tail.x - point->x;
    const float dy_tp = line->tail.y - point->y;
    const float num = dx_ht * dy_tp - dx_tp * dy_ht;
    const float den_sq = dx_ht * dx_ht + dy_ht * dy_ht;
    return (num * num) < (tolerance * tolerance) * den_sq;
}

inline bool vec2_near_segment(const Line* const line, const Vec2* const point, const float tolerance)
{
    return vec2_near_line(line, point, tolerance) &&
           ((point->x > line->tail.x && point->x < line->head.x) ||
            (point->x > line->head.x && point->x < line->tail.x));
}

// Same as vec2_near_line, but uses pre-computed normal vector associated with line
inline bool vec2_near_line_with_normal(const Line* const line, const Vec2* const normal, const Vec2* const point, const float tolerance)
{
    const Vec2 r{line->tail.x - point->x, line->tail.y - point->y};
    return std::abs(vec2_dot(normal, &r)) < tolerance;
}

// Same as vec2_near_segment, but uses pre-computed normal vector associated with line
inline bool vec2_near_segment_with_normal(const Line* const line, const Vec2* const normal, const Vec2* const point, const float tolerance)
{
    return vec2_near_line_with_normal(line, normal, point, tolerance) &&
           ((point->x > line->tail.x && point->x < line->head.x) ||
            (point->x > line->head.x && point->x < line->tail.x));
}

inline bool vec2_above_line_with_normal(const Line* const line, const Vec2* const normal, const Vec2* const point)
{
    Vec2 r{vec2_sub(point, &line->tail)};
    return vec2_dot(normal, &r) > 0.f;
}

inline Vec2 line_to_normal(const Line* const line)
{
    Vec2 normal;
    normal.x = -(line->head.y - line->tail.y);
    normal.y = line->head.x - line->tail.x;
    const float d = std::sqrt(vec2_length_squared(&normal));
    normal.x /= d;
    normal.y /= d;
    return normal;
}

struct AABB
{
    Vec2 min_corner;
    Vec2 max_corner;
};

inline void aabb_initialize(AABB* const aabb, const Vec2* const point0, const Vec2* const point1)
{
    aabb->max_corner.x = std::fmax(point0->x, point1->x);
    aabb->max_corner.y = std::fmax(point0->y, point1->y);
    aabb->min_corner.x = std::fmin(point0->x, point1->x);
    aabb->min_corner.y = std::fmin(point0->y, point1->y);
}

inline AABB aabb_create(const Vec2 point0, const Vec2 point1)
{
    AABB aabb;
    aabb_initialize(&aabb, &point0, &point1);
    return aabb;
}

inline bool aabb_within(const AABB* const aabb, const Vec2* const point)
{
    return (point->x > aabb->min_corner.x) &&
           (point->y > aabb->min_corner.y) &&
           (point->x < aabb->max_corner.x) &&
           (point->y < aabb->max_corner.y);
}