#ifndef _LIGHTRAYS_UTIL_H_
#define _LIGHTRAYS_UTIL_H_

#include <utility>
#include <vector>

#ifdef NOSTDOPTIONAL
	#include <boost/optional.hpp>
	namespace std {
		using boost::optional;
		const boost::none_t nullopt = boost::none;
	}
#else
	#include <optional>
#endif

#define Inf std::numeric_limits<float>::infinity()
#define NaN std::numeric_limits<float>::signaling_NaN()

/// Vecteur 2D
struct vec_t {
	float x, y;
	void  operator/= (float k)       { x /= k; y /= k; }
	void  operator*= (float k)       { x *= k; y *= k; }
	void  operator+= (vec_t o)       { x += o.x; y += o.y; }
	void  operator-= (vec_t o)       { x -= o.x; y -= o.y; }
	vec_t operator*  (float k) const { return vec_t{ k*x, k*y }; }
	vec_t operator/  (float k) const { return vec_t{ x/k, y/k }; }
	vec_t operator+  (vec_t o) const { return vec_t{ x+o.x, y+o.y }; }
	vec_t operator-  ()        const { return vec_t{ -x, -y }; }
	vec_t operator-  (vec_t o) const { return vec_t{ x-o.x, y-o.y }; }
	float operator|  (vec_t o) const { return x*o.x + y*o.y ; }	// produit scalaire
	float norm2      ()        const { return x*x + y*y; }		// norme au carré
	float operator!  ()        const;							// norme
	vec_t rotate     (float theta) const;						// rotation d'un angle `theta`
	vec_t rotate_p90 ()        const { return vec_t { -y, +x }; }
	vec_t rotate_m90 ()        const { return vec_t { +y, -x }; }
};
inline vec_t operator* (double k, const vec_t& v) { return v*k; }

/// Point 2D
struct point_t {
	float x, y;
	vec_t operator- (point_t o) const { return vec_t{ x-o.x, y-o.y }; }		// vecteur entre deux points
	explicit operator vec_t () const { return vec_t{ x, y }; }
	point_t operator+ (vec_t v) const { return point_t{ x+v.x, y+v.y }; }	// translation du point par un vecteur
};

// Milieu entre deux points
point_t milieu_2points (point_t a, point_t b);

// Transation d'un ensemble de points
std::vector<point_t> translate_points (vec_t by, std::vector<point_t>);
// Rotation d'un ensemble de points autour d'un centre `around` d'un angle `angle`
std::vector<point_t> rotate_points (point_t around, float angle, std::vector<point_t>);
// Homothétie d'un ensemble de points autout d'un centre `around` par un facter `coeff`
std::vector<point_t> homothetie_points (point_t around, float coeff, std::vector<point_t>);


/// Intervalle d'angle dans ℝ/2πℝ
struct angle_interv_t {
private:
	float a, l; // angle de début (0 ≤ a ≤ 2π) et longueur de l'intervalle (0 ≤ l ≤ 2π)
public:
	angle_interv_t (const angle_interv_t&) = default;
	static const angle_interv_t cercle_entier;
	
	// construction à partir de l'angle de début et de fin
	// si (theta_beg > 0 > theta_end) numériquement, interprète l'intervalle comme [theta_beg,theta_end+2π]
	// si |theta_end-theta_beg| > 2π, lances une exception (n'est pas censé arriver dans ce programme)
	angle_interv_t (float theta_beg, float theta_end);
	// construction d'un intervalle de longeur donnée, partant de θ=0
	angle_interv_t (float lenght);
	
	angle_interv_t operator+ (float delta_theta) const;	// rotation de l'intervalle
	float longueur () const { return l; }				// longueur de l'intervalle, entre 0 et 2π
	bool inclus (float theta) const;					// test si un angle (dans ℝ/2πℝ) est inclus dans l'intervalle
	float beg () const { return a; }					// angle de début, entre 0 et 2π
	float end () const { return a+l; }					// angle de fin, entre 0 et 4π
	std::pair<vec_t,vec_t> vec_a_b () const;			// vecteurs unité définissant les angles début et fin
};

// réduit un angle à [-π,+π]
float angle_mod2pi_11 (float);
// réduit un angle à [0,2π]
float angle_mod2pi_02 (float);

/// Divers

// Réslution du système linéaire
// ⎡a  b⎤ ⎡x⎤   ⎡e⎤
// ⎣c  d⎦ ⎣y⎦ = ⎣f⎦
void mat22_sol (float a, float b, float c, float d, float e, float f, float& x, float& y);

// Nombre au hasard entre 0 et 1
float rand01 ();

#endif
