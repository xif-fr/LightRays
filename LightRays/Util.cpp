#include "Util.h"
#include <cmath>
#include <stdexcept>
#include <cassert>

float vec_t::operator! () const {
	return hypotf(x, y);
}

vec_t vec_t::rotate (float theta) const {
	float c = cosf(theta), s = sinf(theta);
	return { .x = c * x - s * y,
	         .y = s * x + c * y };
}

point_t milieu_2points (point_t a, point_t b) {
	return { .x = (a.x + b.x)/2, .y = (a.y + b.y)/2 };
}

std::vector<point_t> translate_points (vec_t by, std::vector<point_t> pts) {
	for (point_t& p : pts)
		p = p + by;
	return pts;
}

std::vector<point_t> rotate_points (point_t around, float angle, std::vector<point_t> pts) {
	for (point_t& p : pts)
		p = around + (p - around).rotate(angle);
	return pts;
}

std::vector<point_t> homothetie_points (point_t around, float coeff, std::vector<point_t> pts) {
	for (point_t& p : pts)
		p = around + coeff * (p - around);
	return pts;
}

// ⎡a  b⎤ ⎡x⎤   ⎡e⎤
// ⎣c  d⎦ ⎣y⎦ = ⎣f⎦
//
void mat22_sol (float a, float b, float c, float d, float e, float f, float& x, float& y) {
	float det = a * d - b * c;
	x = (e * d - b * f) / det;
	y = (a * f - e * c) / det;
}

const angle_interv_t angle_interv_t::cercle_entier (2*M_PI);

angle_interv_t::angle_interv_t (float theta_beg, float b) : a(theta_beg), l(0) {
	if (fabsf(b-a) > 2*M_PI+1e-6)
		throw std::domain_error("angle interval > 2π");
	if (b < 0 and a >= 0)
		b += 2*M_PI;
	l = b - a;
	a = angle_mod2pi_02(a);
//	assert(0 <= a and a <= 2*M_PI and 0 <= l and l <= 2*M_PI);
}

angle_interv_t::angle_interv_t (float lenght) : a(0), l(lenght) {
	if (l < 0 or l > 2*M_PI+1e-6)
		throw std::domain_error("invalid angle interval length");
}

angle_interv_t angle_interv_t::operator+ (float delta_theta) const {
	angle_interv_t o = *this;
	o.a = angle_mod2pi_02(a + delta_theta);
	return o;
}

float angle_mod2pi_11 (float theta) {
	while (theta > M_PI)
		theta -= 2*M_PI;
	while (theta < -M_PI)
		theta += 2*M_PI;
	return theta;
}

float angle_mod2pi_02 (float theta) {
	return theta - 2*M_PI* floorf( theta/(2*M_PI) );
}

bool angle_interv_t::inclus (float theta) const {
	theta = angle_mod2pi_02(theta);
	if (a <= theta and theta <= a+l)
		return true;
	theta += 2*M_PI;
	return (a <= theta and theta <= a+l);
}

std::pair<vec_t,vec_t> angle_interv_t::vec_a_b () const {
	return { vec_t{ .x = cosf(a),   .y = sinf(a) },
	         vec_t{ .x = cosf(a+l), .y = sinf(a+l) } };
}

float rand01 () {
	return rand()/(float)RAND_MAX;
}
