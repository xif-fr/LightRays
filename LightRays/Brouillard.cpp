#include "Brouillard.h"
#include "ObjetsCourbes.h"
#include <cmath>

Objet::extension_t Objet_Brouillard::objet_extension () const {
	vec_t v = { lx*reso_x/2, ly*reso_y/2 };
	return {
		.pos = o + v,
		.rayon = std::max(v.x, v.y)
	};
}

// Test d'interception avec le brouillard : parcours du rayon à travers les cellules
//  et décision si oui ou non on diffuserait le rayon
//
Objet::intercept_t Objet_Brouillard::essai_intercept (const Rayon& ray) const {
	
	if (ray.spectre.intensite_tot() < intens_cutoff)
		return { .dist2 = Inf, .intercept_struct = nullptr };
	
	// on test si le rayon passe sur le rectange bornant le brouillard
	std::vector<ObjetLigne::intersection_segdd_t> isects;
	auto test_isect = [&] (point_t a, point_t b) {
		auto isect = ObjetLigne::intersection_segment_demidroite(a, b, ray.orig, ray.dir_angle);
		if (isect.has_value())
			isects.push_back(isect.value());
	};
	point_t o2 = o + vec_t{lx*reso_x,0},
	        o3 = o + vec_t{lx*reso_x,ly*reso_y},
	        o4 = o + vec_t{0,ly*reso_y};
	test_isect(o , o2);
	test_isect(o2, o3);
	test_isect(o3, o4);
	test_isect(o4, o );
//	assert(isects.size() <= 2); // le rayon ne peut avoir que 1 (si source interne) ou 2 intersections (ou zéro) avec les bords
	
	if (isects.size() != 0) {
		
		float s;
		float s_fin = isects[0].t_dd;
		if (isects.size() == 1) {  // rayon émis à l'intérieur du brouillard
			s = 0;
		} else {  // rayon provenant de l'expérieur du brouillard
			s = isects[1].t_dd;
			if (s > s_fin)
				std::swap(s, s_fin);
		}
		float ds = std::min(reso_x,reso_y) / 2;
		vec_t u_ray = isects[0].u_dd;
		// le rayon parcourt le brouillard jusqu'à s_fin par incréments de ds
		// avec une probabilité donnée par `densit_brouillard` d'être diffusé à chaque pas, pour la méthode 1
		// et avec une probabilité de `diffus_partielle_syst_proba`
		intercept_brouillard_t p;
		float proba_acc = 0;
		
		while (s < s_fin) {
			p.p_diff = ray.orig + s * u_ray;
			vec_t v = p.p_diff - this->o;
			p.x_diff = floorf( v.x / reso_x );
			p.y_diff = floorf( v.y / reso_y );
			if (0 <= p.x_diff and p.x_diff < (int)this->lx and 0 <= p.y_diff and p.y_diff < (int)this->ly) {
				
				float proba = ds / diffus_partielle_syst_libreparcours;
				if (proba > 1e-5) {
					// méthode par diffusion systématique partielle
					proba_acc += proba;
					p.fraction_transmis = std::max<float>(0, 1 - ds/proba * this->densit_brouillard(p.x_diff, p.y_diff));
				} else {
					// méthode par diffusion probabiliste complète
					proba = ds * this->densit_brouillard(p.x_diff, p.y_diff);
					proba_acc += proba;
					p.fraction_transmis = 0;
				}
				bool diffuse = parcours_deterministe ? (proba_acc >= 1) : (rand01() < proba);
				if (diffuse) {
					// rayon diffusé
					s += /*rand01() */ ds;
					p.p_diff = ray.orig + s * u_ray;
					return { .dist2 = s*s,
					         .intercept_struct = std::make_shared<intercept_brouillard_t>(std::move(p)) };
				}
				
			}
			s += ds;
		}
		
	}
	return { .dist2 = Inf, .intercept_struct = nullptr };
};

// Ré-émission du rayon intercepté par le brouillard
//
std::vector<Rayon> Objet_Brouillard::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	const intercept_brouillard_t& intercept = *(intercept_brouillard_t*)interception.get();
	std::vector<Rayon> rayons;
	
	// Nombre de rayons secondaires
	size_t n_re_emit = std::max<size_t>(1, lroundf(
		n_re_emit_par_intens * ray.spectre.intensite_tot() * (1-intercept.fraction_transmis))
	);
	float fact_I_re_emit = 1./n_re_emit;
	
	// On laisse vivre le rayon incident seulement si il a une intensité > 0.01
	if (intercept.fraction_transmis > 0.01) {
		Rayon ray_trsm = ray;
		ray_trsm.spectre.for_each([&] (float lambda, pola_t, float& I) {
			I *= intercept.fraction_transmis;
		});
		ray_trsm.orig = intercept.p_diff;
		rayons.push_back(std::move(ray_trsm));
		
		fact_I_re_emit *= 1 - intercept.fraction_transmis;
	}
	
	// Émission des rayons secondaires
	for (size_t k = 0; k < n_re_emit; k++) {
		
		float ang_diff = 2*M_PI * rand01();
		Rayon ray_diff;
		ray_diff.orig = intercept.p_diff;
		ray_diff.dir_angle = ray.dir_angle + ang_diff;
		
		ray_diff.spectre = ray.spectre;
		ray_diff.spectre.for_each([&] (float lambda, pola_t, float& I) {
			I *= this->directivite_diffus(ang_diff, lambda) * fact_I_re_emit;
		});
		
		rayons.push_back(std::move(ray_diff));
	}
	return rayons;
}

#include "sfml_c01.hpp"

void Objet_Brouillard::dessiner (sf::RenderWindow& window, bool emphasize) const {
	for (size_t x = 0; x < this->lx; x++) {
		for (size_t y = 0; y < this->ly; y++) {
			sf::RectangleShape dens;
			sf::c01::setRectShape(dens, o + vec_t{reso_x*x, reso_y*y}, {reso_x, reso_y});
			dens.setFillColor(sf::Color(255,255,255, std::min(255.f, this->densit_brouillard(x,y)) ));
			window.draw(dens);
		}
	}
}

std::optional<point_t> Objet_Brouillard::point_interception (std::shared_ptr<void> interception) const {
	if (interception) {
		return ((intercept_brouillard_t*)interception.get())->p_diff;
	} else
		return std::nullopt;
}
