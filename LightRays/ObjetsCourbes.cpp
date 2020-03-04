#include "ObjetsCourbes.h"
#include <cmath>
#include <stdexcept>

///------------------------ ObjetCourbe ------------------------///

#define INTERCEPTION_DIST_MINIMALE 0.0001

// Relai de ObjetCourbe::essai_intercept_courbe
//
Objet::intercept_t ObjetCourbe::essai_intercept (const Rayon& ray) const {
	auto intercept = this->essai_intercept_courbe(ray);
	if (intercept.has_value()) {
		float dist2 = (ray.orig - (*intercept)->p_incid).norm2();
		// on introduit une distance minimale qu'un rayon peut parcourit avant d'être intercepté
		// par une courbe pour éviter qu'un objet ré-intercepte immédiatement le rayon qu'il vient
		// d'émettre, ce qui arriverait souvent en simple précision
		if (dist2 < INTERCEPTION_DIST_MINIMALE*INTERCEPTION_DIST_MINIMALE)
			return { .dist2 = Inf, .intercept_struct = nullptr };
		else
			return { .dist2 = dist2,
					 .intercept_struct = std::static_pointer_cast<void>(*intercept) };
	} else
		return { .dist2 = Inf, .intercept_struct = nullptr };
}

std::optional<point_t> ObjetCourbe::point_interception (std::shared_ptr<void> intercept_struct) const {
	if (intercept_struct) {
		return ((intercept_courbe_t*)intercept_struct.get())->p_incid;
	} else
		return std::nullopt;
}

///------------------------ ObjetLigne ------------------------///

ObjetLigne::ObjetLigne (point_t a, float l, float ang) :
	a( a ),
	b( a + l * vec_t{ cosf(ang), sinf(ang) } ) {}

vec_t ObjetLigne::vecteur_u_perp () const {
	vec_t v_perp = (b - a).rotate_p90();
	return v_perp / !v_perp;
}

// Routine d'intersection segment avec demi-droite
//
std::optional<ObjetLigne::intersection_segdd_t> ObjetLigne::intersection_segment_demidroite (point_t a, point_t b, point_t dd_orig, float angle) {
	// cas particulier d'alignement non pris en compte
	vec_t u_dd = { .x = cosf(angle),
	               .y = sinf(angle) };
	vec_t v_seg = a - b;
	float s, t;
	mat22_sol(v_seg.x, -u_dd.x,
	          v_seg.y, -u_dd.y,
	          dd_orig.x - b.x, dd_orig.y - b.y,
	          s, t);
	if ((0 <= s and s <= 1) and t >= 0) {
		return intersection_segdd_t{ .v_seg = v_seg, .u_dd = u_dd, .s_seg = s, .t_dd = t };
	} else
		return std::nullopt;
}

// Test d'interception du rayon sur la ligne.
//
std::optional<std::shared_ptr<ObjetCourbe::intercept_courbe_t>> ObjetLigne::essai_intercept_courbe (const Rayon& ray) const {
	// `intersection_segment_demidroite` n'est pas directement intégrée ici car elle sert ailleurs
	auto isect = ObjetLigne::intersection_segment_demidroite(a, b, ray.orig, ray.dir_angle);
	if (not isect.has_value())
		return std::nullopt;
	std::shared_ptr<intercept_ligne_t> intercept = std::make_shared<intercept_ligne_t>();
	// point d'incidence
	intercept->p_incid = ray.orig + isect->t_dd * isect->u_dd;
	intercept->s_incid = isect->s_seg;
	// angle d'incidence
	float seg_angle = atan2f(isect->v_seg.y, isect->v_seg.x);  // pourrait être calculé une bonne fois pour toutes
	float alpha = angle_mod2pi_11(ray.dir_angle);
	float i = alpha - (seg_angle - M_PI/2);
	if (fabsf(angle_mod2pi_11(i)) < M_PI/2) {
		intercept->ang_normale = seg_angle + M_PI/2;
		intercept->ang_incid = i;
		intercept->sens_reg = true;
	} else {
		intercept->ang_incid = i - M_PI;
		if (intercept->ang_incid < -M_PI/2) intercept->ang_incid += 2*M_PI;
		intercept->ang_normale = seg_angle - M_PI/2;
		intercept->sens_reg = false;
	}
	return std::shared_ptr<intercept_courbe_t>(intercept);
}

///------------------------ ObjetArc ------------------------///

// Construction de l'arc de cercle à partir de deux points et du rayon
//
ObjetArc::ObjetArc (point_t a, point_t b, float radius, bool inv_interieur) : c({0,0}), R(radius), ang(0,0), inv_int(inv_interieur) {
	// translation
	vec_t ab = b - a;
	// vérification
	float ab2 = ab.norm2();
	if (ab2 > 4*R*R)
		throw std::domain_error("ObjetArc(A,B,R) : points A et B trop éloignés");
	// rotation
	float theta0 = atan2f(ab.y, ab.x) - M_PI/2;
	// calcul dans le repère 'prime'
	vec_t c_ = { .x = -sqrtf( R*R - ab2/4 ),
	             .y = sqrtf(ab2)/2           };
	float theta_ = atanf( c_.y / c_.x );
	angle_interv_t ang_ ( theta_, -theta_ );
	// dé-rotation
	ang = ang_ + theta0;
	c_ = c_.rotate(theta0);
	// dé-translation
	c = a + c_;
}

std::pair<point_t,point_t> ObjetArc::objet_extremit () const {
	vec_t u_a, u_b;
	std::tie(u_a,u_b) = this->ang.vec_a_b();
	return { c + R * u_a,
	         c + R * u_b };
}

// Routine d'interception du rayon sur l'arc de cercle.
//
std::optional<std::shared_ptr<ObjetArc::intercept_courbe_t>> ObjetArc::essai_intercept_courbe (const Rayon& ray) const {
	/// intersection arc de cercle / demi-droite
	// (notations de `lois.pdf`)
	vec_t oc = c - ray.orig;
	float b = !oc / R;  // d/R
	float base_ang = atan2f(oc.y, oc.x);
	// angles relatifs à l'axe OC
	float alpha = angle_mod2pi_11( ray.dir_angle - base_ang );
	angle_interv_t thetaAB = ang + -base_ang;
	// intersection avec le cecle si en dessous de l'angle critique
	float y = b * sinf(alpha);
	if ((b > 1.00001 and fabsf(alpha) >= M_PI/2) or fabsf(y) > 1)
		return std::nullopt;
	// angles repérant les points d'intersection avec le cercle
	float arcsiny = asinf(y);
	float theta1 = alpha - arcsiny + M_PI,
	      theta2 = alpha + arcsiny;
	/// si on est bien sur notre arc de cercle
	std::shared_ptr<intercept_arc_t> intercept = std::make_shared<intercept_arc_t>();
	// θ1 n'est accessible que si la rayon vient de l'extérieur
	if ( b > 1.00001 and thetaAB.inclus(theta1) ) {
		intercept->sens_reg = !inv_int; // ext vers int du cerlce
		intercept->theta_incid = intercept->ang_normale = theta1 + base_ang;
		intercept->ang_incid = M_PI - theta1 + alpha; // c'est un angle relatif, pas besoin de base_ang
	}
	// test de θ1 pour b<1 ou si θ1 a échoué pour b>1
	else if ( thetaAB.inclus(theta2) ) {
		intercept->sens_reg = inv_int; // int vers ext du cercle
		intercept->theta_incid = theta2 + base_ang;
		intercept->ang_normale = intercept->theta_incid + M_PI; // normale vers l'intérieur du cercle
		intercept->ang_incid = alpha - theta2;
	}
	else
		return std::nullopt;
	// calcul du point d'incidence
	intercept->p_incid = c + R * vec_t{ .x = cosf(intercept->theta_incid),
	                                    .y = sinf(intercept->theta_incid) };
	return std::shared_ptr<intercept_courbe_t>(intercept);
}

///------------------------ ObjetComposite ------------------------///

// Calcul de l'extension approximative d'un objet composite :
//  • position = barycentre
//  • extension = distance (position objet - barycentre) maximale,
//                + extension (objet) maximale
//  (non-optimal, mais se comporte bien dans la plupart des cas)
//
Objet::extension_t ObjetComposite::objet_extension () const {
	std::vector<vec_t> pos (comp.size());
	vec_t bary = {0,0};
	float rayon_max = 0;
	for (const auto& obj : comp) {
		extension_t ext = obj->objet_extension();
		pos.push_back( (vec_t)ext.pos );
		bary += (vec_t)ext.pos;
		rayon_max = std::max(rayon_max, ext.rayon);
	}
	bary /= comp.size();
	float pos_max = 0;
	for (vec_t p : pos)
		pos_max = std::max(pos_max, !(p - bary));
	return { .pos = {bary.x,bary.y}, .rayon = rayon_max + pos_max };
}

// Relai du test d'interception sur chaque sous-objet, et interception
//  par l'objet le plus proche sur le chemin du rayon.
//
Objet::intercept_t ObjetComposite::essai_intercept (const Rayon& ray) const {
	intercept_composite_t interception { .courbe_intercept = comp.end() };
	float dist2_min = Inf;
	// Test d'interception du rayon contre toutes les courbes composantes;
	//  la première interception en terme de distance entre l'origine du rayon
	//  et le point d'incidence est choisie (recherche de minimum)
	for (auto it = comp.begin(); it != comp.end(); it++) {
		std::optional<std::shared_ptr<ObjetCourbe::intercept_courbe_t>> intercept
		= (*it)->essai_intercept_courbe(ray);
		if (intercept.has_value()) {
			float dist2 = (ray.orig - (*intercept)->p_incid).norm2();
			if (dist2 < INTERCEPTION_DIST_MINIMALE*INTERCEPTION_DIST_MINIMALE)
				continue;
			if (dist2 < dist2_min) {
				interception.courbe_intercept = it;
				interception.intercept_struct = *intercept;
				dist2_min = dist2;
			}
		}
	}
	return { .dist2 = dist2_min,
	         .intercept_struct = (interception.courbe_intercept == comp.end()) ?
	                             nullptr :
	                             std::make_shared<intercept_composite_t>(interception)
	};
}

std::optional<point_t> ObjetComposite::point_interception (std::shared_ptr<void> interception) const {
	if (interception) {
		const intercept_composite_t& intercept = *(intercept_composite_t*)interception.get();
		if (intercept.courbe_intercept != comp.end())
			return intercept.intercept_struct->p_incid;
	}
	return std::nullopt;
}

// Simple ré-émission par la courbe qui a intercepté le rayon
//
std::vector<Rayon> ObjetComposite::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	const intercept_composite_t& intercept = *(intercept_composite_t*)interception.get();
	return (*intercept.courbe_intercept)->re_emit(ray, intercept.intercept_struct);
}

// Test de fermeture
//
void ObjetComposite::test_fermeture () const {
	if (comp.size() == 0)
		throw std::logic_error("ObjetComposite : vide");
	auto verif_egaux = [] (point_t p, point_t p_bis) {
		if ((p - p_bis).norm2() > 1e-10)
			throw std::logic_error("ObjetComposite : courbes non bout-à-bout");
	};
	point_t beg, cur;
	std::tie(beg, cur) = this->comp[0]->objet_extremit();
	for (size_t i = 1; i < this->comp.size(); i++) {
		point_t cur_bis, next;
		std::tie(cur_bis, next) = this->comp[i]->objet_extremit();
		verif_egaux(cur, cur_bis);
		cur = next;
	}
	verif_egaux(beg, cur);
}

///------------------------ Affichages ------------------------///

#include "sfml_c01.hpp"

void ObjetLigne::dessiner (sf::RenderWindow& window, bool emphasize) const {
	auto line = sf::c01::buildLine(this->a, this->b, emphasize ? sf::Color::White : sf::Color(200,200,200));
	window.draw(line);
}

void ObjetCourbe::dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> interception) const {
	const intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	// dessin du rayon source -> objet
	auto c = ray.spectre.rgb256_noir_intensite(false, std::nullopt);
	auto line = sf::c01::buildLine(ray.orig,
	                               intercept.p_incid,
	                               sf::Color(std::get<0>(c), std::get<1>(c), std::get<2>(c), 255));
	window.draw(line);
	// dessin de la normale au point d'interception
	vec_t v = 0.03 * vec_t{ .x = cosf(intercept.ang_normale), .y = sinf(intercept.ang_normale) };
	line = sf::c01::buildLine(intercept.p_incid+(-0.2)*v,
	                          intercept.p_incid+(+0.8)*v,
	                          intercept.sens_reg ? sf::Color(255,200,200) : sf::Color(200,200,255));
	window.draw(line);
}

void ObjetArc::dessiner (sf::RenderWindow& window, bool emphasize) const {
	size_t N = R * 100 * ang.longueur();
	if (N < 3) N = 3;
	std::vector<point_t> pts (N+1);
	for (size_t k = 0; k <= N; k++) {
		float theta = ang.beg() + ang.longueur() * (float) k / N;
		pts[k] = c + R * vec_t{ .x = cosf(theta),
		                        .y = sinf(theta) };
	}
	auto arc = sf::c01::buildLineStrip(pts, sf::Color::White);
	window.draw(arc);
}

void ObjetComposite::dessiner (sf::RenderWindow& window, bool emphasize) const {
	for (const auto& p : this->comp)
		p->dessiner(window, emphasize);
}

void ObjetComposite::dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> interception) const {
	const intercept_composite_t& intercept = *(intercept_composite_t*)interception.get();
	(*intercept.courbe_intercept)->dessiner_interception(window, ray, intercept.intercept_struct);
}
