#include "ObjetMilieux.h"
#include <cmath>

void ObjetComposite_LignesMilieu::re_positionne (point_t o) {
	std::optional<vec_t> trsl = std::nullopt;
	for (auto& obj : this->comp) {
		ObjetLigne_Milieux& ligne = *dynamic_cast<ObjetLigne_Milieux*>(&(*obj));
		if (not trsl.has_value())
			trsl = o - ligne.a;
		ligne.a = ligne.a + *trsl;
		ligne.b = ligne.b + *trsl;
	}
}

// Réflexion et réfraction sur un dioptre : lois de Snell-Descartes
//  et coefficients de Fresnel. Voir lois.pdf pour plus de détails.
// Deux cas : indice de réfraction indep. de λ (peu cher) et dépendant
//  de λ (cher car séparation en N_COULEURS différentes)
//
std::vector<Rayon> ObjetCourbe_Milieux::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	std::vector<Rayon> rays_emis;
	
	Rayon ray_refl;
	ray_refl.orig = intercept.p_incid;
	ray_refl.dir_angle = intercept.ang_normale - intercept.ang_incid; // i_refl = i par rapport à la normale
	ray_refl.spectre = ray.spectre;
	
	auto refracte = [&intercept,&rays_emis] (Rayon& ray_refl, float n_out, float n_in) {
		
		float gamma = intercept.sens_reg ? n_out/n_in : n_in/n_out; // n1/n2
		
		float s = gamma * sinf(intercept.ang_incid);
		
		if (fabsf(s) <= 1) { // on a un rayon transmis (i <= i_critique)
			
			Rayon ray_trsm;
			ray_trsm.orig = intercept.p_incid;
			ray_trsm.dir_angle = (intercept.ang_normale + M_PI) + asinf(s);
			
			float b = sqrtf( 1 - s*s );
			float cosi = cosf(intercept.ang_incid);
			float a_TE = gamma * cosi, a_TM = cosi / gamma;
			float r_coeff[2];
			r_coeff [PolTE] = (a_TE - b) / (a_TE + b);
			r_coeff [PolTM] = (a_TM - b) / (a_TM + b);
			
			Specte::for_each_manual([&] (uint8_t i, float lambda, pola_t pol) {
				float R = r_coeff[pol] * r_coeff[pol];
				ray_trsm.spectre.comps[i] = (1-R) * ray_refl.spectre.comps[i];
				ray_refl.spectre.comps[i] =   R   * ray_refl.spectre.comps[i];
			});
			
			rays_emis.push_back(ray_trsm);
		}
		// sinon, pas de rayon transmis (i > i_critique), et ray_refl est déjà prêt à être envoyé
		
		rays_emis.push_back(ray_refl);
	};
	
	// indice de réfraction fixe
	if (not (bool)n_lambda) {
		refracte (ray_refl, /*n_out*/1., /*n_in*/this->n_fixe);
	}
	// indice de réfraction dépendant de la longueur d'onde
	//  -> séparation de toutes les composantes en rayons monochromatiques, car les directions sont différentes
	else {
		for (color_id_t i = 0; i < N_COULEURS; i++) {
			Rayon ray_refl_mono = ray_refl;
			ray_refl_mono.spectre.for_each_cid([&] (color_id_t cid, pola_t pol, float& I) {
				if (cid != i)
					I = 0;
			});
			float n_in = n_lambda(lambda_color[i]); // variable
			refracte (ray_refl_mono, /*n_out*/1., n_in);
		}
	}
	
	return rays_emis;
}

#include "sfml_c01.hpp"

void ObjetLigne_Milieux::dessiner (sf::RenderWindow& window, bool emphasize) const {
	ObjetLigne::dessiner(window, emphasize);
	sf::ConvexShape rect_milieu(4);
	vec_t v_perp = vecteur_u_perp() * 0.05;
	rect_milieu.setPoint(0, sf::c01::toWin(a));
	rect_milieu.setPoint(1, sf::c01::toWin(b));
	rect_milieu.setPoint(2, sf::c01::toWin(b+v_perp));
	rect_milieu.setPoint(3, sf::c01::toWin(a+v_perp));
	rect_milieu.setFillColor(sf::Color(255,255,255,20));
	window.draw(rect_milieu);
}
