#include "ObjetsOptiques.h"
#include "sfml_c01.hpp"
#include <cmath>

Objet_MatriceTrsfUnidir::Objet_MatriceTrsfUnidir (point_t centre, float diam, float angv, mat_trsf_t m) : ObjetLigne({0,0}, {0,0}), mat_trsf(m) {
	vec_t v = diam/2 * vec_t{ -sinf(angv), cosf(angv) };
	a = centre +  v;
	b = centre + -v;
}

// Dessin d'un Objet_MatriceTrsfUnidir : on rajoute une ligne qui marque le côté sortant
//
void Objet_MatriceTrsfUnidir::dessiner (sf::RenderWindow& window, bool emphasize) const {
	ObjetLigne::dessiner(window, emphasize);
	point_t o = milieu_2points(a,b);
	window.draw(sf::c01::buildLine(o, o + 0.05 * vecteur_u_perp(), sf::Color(100,100,100)));
}

// Objet_MatriceTrsfUnidir : Application de la matrice ABCD sur le rayon intercepté, dans le sens direct uniquement
//
std::vector<Rayon> Objet_MatriceTrsfUnidir::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_ligne_t& intercept = *(intercept_ligne_t*)interception.get();
	std::vector<Rayon> rays;
	if (intercept.sens_reg) {
		float diam = !(a-b);
		float y = (1 - 2 * intercept.s_incid) * diam/2;	// élévation incidente
		float yp = tanf(intercept.ang_incid);			// pente incidente
		float y2 =  mat_trsf.A * y + mat_trsf.B * yp;	// élévation en sortie
		float y2p = mat_trsf.C * y + mat_trsf.D * yp;	// pente en sortie
		Rayon raytrsf = ray;
		raytrsf.orig = milieu_2points(a,b) + y2 * (b-a)/diam;
		raytrsf.dir_angle = atanf(y2p);
		rays.push_back(std::move(raytrsf));
	}
	return rays;
}

// Objet_Filtre : filtrage du rayon incident : simple multiplication composante par composante du spectre par le spectre de transmission
//
std::vector<Rayon> Objet_Filtre::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	Rayon ray_filtre = ray;
	ray_filtre.orig = intercept.p_incid;
	Specte::for_each_manual([&] (size_t i, float lambda, pola_t pol) -> void {
		ray_filtre.spectre.comps[i] *= transm.comps[i];
	});
	return { std::move(ray_filtre) };
}

// Miroir : simple réflexion par rapport à la normale au point incident
//
std::vector<Rayon> ObjetCourbe_Miroir::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	std::vector<Rayon> ray_emis;
	Rayon ray_refl;
	ray_refl.orig = intercept.p_incid;
	ray_refl.dir_angle = intercept.ang_normale - intercept.ang_incid; // i_refl = i par rapport à la normale
	ray_refl.spectre = ray.spectre;
	ray_emis.push_back(std::move(ray_refl));
	return ray_emis;
}

// Bilan d'énergie : accumulation des flux entrants et sortants
//
std::vector<Rayon> Objet_BilanEnergie::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	Rayon rayon = ray;
	rayon.orig = intercept.p_incid;
	if (intercept.sens_reg) {
		n_ray_in += 1;
		flux_in += rayon.spectre.intensite_tot();
	} else {
		n_ray_out += 1;
		flux_out += rayon.spectre.intensite_tot();
	}
	return { std::move(rayon) };
}
