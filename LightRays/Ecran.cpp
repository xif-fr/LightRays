#include "Ecran.h"
#include <algorithm>
#include <cmath>
#include "sfml_c01.hpp"

///------------------------ EcranLigne_Multi ------------------------///

EcranLigne_Multi::EcranLigne_Multi (point_t a, point_t b, float bin_dens, float lumino) :
	EcranLigne_Multi (a, b, (uint16_t)(bin_dens * !(b-a)), lumino) {}

EcranLigne_Multi::EcranLigne_Multi (point_t a, point_t b, uint16_t bins_n, float lumino) :
	Ecran_Base(lumino),
	ObjetLigne(a, b),
	bins_intensit( std::max<size_t>(1, bins_n) ),
	epaisseur_affich(std::nullopt)
	{ this->reset(); }

void EcranLigne_Multi::reset () {
	n_acc = 0;
	for (Specte& intensit : bins_intensit) {
		intensit.for_each([&] (float, pola_t, float& I) -> void {
			I = 0;
		});
	}
}

// Accumulation des rayons dans les pixels
//
std::vector<Rayon> EcranLigne_Multi::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_ligne_t& intercept = *(intercept_ligne_t*)interception.get();
	size_t N = bins_intensit.size();
	ssize_t k_bin = floorf(intercept.s_incid * N);
	if (k_bin == -1) k_bin = 0;
	if (k_bin == (ssize_t)N) k_bin = N-1;
	Specte::for_each_manual([&] (size_t i, float lambda, pola_t pol) -> void {
		bins_intensit[k_bin].comps[i] += ray.spectre.comps[i];
	});
	return {};
}

// Retrourne la matrice de pixels traitée
//
std::vector<EcranLigne_Multi::pixel_t> EcranLigne_Multi::matrice_pixels () const {
	size_t N = this->bins_intensit.size();
	// dans chaque pixel, c'est une puissance qu'on accumule, proportionnelle
	// à la taille d'un pixel ( |b-a|/N ) pour une source omnidirectionnelle
	// -> il faut diviser par la taille d'un pixel pour avoir d'intensité
	// lumineuse (dont la luminosité RGB affichée est proportionnelle)
	float ItoL = this->luminosite / this->n_acc * (float)N / !(b-a);
	std::vector<pixel_t> mat (N);
	for (size_t k = 0; k < N; k++) {
		mat[k].s1 =   k   / (float)N;
		mat[k].s2 = (k+1) / (float)N;
		mat[k].s_mid = (mat[k].s1 + mat[k].s2) / 2;
		mat[k].spectre = bins_intensit[k];
		mat[k].spectre.for_each([&] (float, pola_t, float& I) -> void {
			I *= ItoL;
		});
		std::tie(mat[k].r, mat[k].g, mat[k].b, mat[k].sat) = mat[k].spectre.rgb256_noir_intensite(false);
	}
	return mat;
}

// Dessin de la matrice de pixels
//
void EcranLigne_Multi::dessiner (sf::RenderWindow& window, bool emphasize) const {
	std::vector<pixel_t> pix =
		this->matrice_pixels();
	vec_t v = a - b;
	auto p = [&] (float s) { return b + v * s; };
	if (epaisseur_affich.has_value()) {
		vec_t perp = v.rotate_p90() / (!v) * (*epaisseur_affich);
		for (size_t k = 0; k < pix.size(); k++) {
			auto color = sf::Color(pix[k].r, pix[k].g, pix[k].b);
			vec_t lng = v * (pix[k].s2 - pix[k].s1);
			sf::ConvexShape rect = sf::c01::buildParallelogram(p(pix[k].s1), lng, perp, color);
			window.draw(rect);
		}
	} else {
		for (size_t k = 0; k < pix.size(); k++) {
			if (pix[k].sat) { // saturation
				auto c = sf::c01::buildCircleShapeCR(p(pix[k].s_mid), 0.003);
				c.setFillColor(sf::Color::Yellow);
				window.draw(c);
			}
			auto color = sf::Color(pix[k].r, pix[k].g, pix[k].b);
			auto line = sf::c01::buildLine(p(pix[k].s1), p(pix[k].s2), color, color);
			window.draw(line);
		}
	}
}

///------------------------ EcranLigne_Mono ------------------------///

EcranLigne_Mono::EcranLigne_Mono (point_t a, point_t b, float lumino) :
	Ecran_Base(lumino),
	ObjetLigne(a, b),
	intensit()
	{ this->reset(); }

void EcranLigne_Mono::reset () {
	n_acc = 0;
	intensit.for_each([&] (float, pola_t, float& I) -> void {
		I = 0;
	});
}

// Accumulations des rayons sur l'écran
//
std::vector<Rayon> EcranLigne_Mono::re_emit (const Rayon& ray, std::shared_ptr<void>) {
	Specte::for_each_manual([&] (size_t i, float lambda, pola_t pol) -> void {
		intensit.comps[i] += ray.spectre.comps[i];
	});
	return {};
}

// Pixel traité
//
EcranLigne_Mono::pixel_t EcranLigne_Mono::pixel () const {
	float ItoL = this->luminosite / this->n_acc / !(b-a);
	pixel_t pix;
	Specte sp = intensit;
	sp.for_each([&] (float, pola_t, float& I) -> void {
		I *= ItoL;
	});
	std::tie(pix.r, pix.g, pix.b, pix.sat) = sp.rgb256_noir_intensite(false);
	return pix;
}

// Dessin du pixel
//
void EcranLigne_Mono::dessiner (sf::RenderWindow& window, bool emphasize) const {
	pixel_t pix = this->pixel();
	if (pix.sat) {
		auto c = sf::c01::buildCircleShapeCR(milieu_2points(a,b), 0.003);
		c.setFillColor(sf::Color::Yellow);
		window.draw(c);
	}
	auto color = sf::Color(pix.r, pix.g, pix.b);
	auto line = sf::c01::buildLine(a, b, color, color);
	window.draw(line);
}
