/*******************************************************************************
 * Objets optiques définissant différents milieux : dioptres entre deux milieux
 * d'indices différents : loi de Snell-Descartes et coefficients de Fresnel
 *******************************************************************************/

// Notes : l'incide de réfraction `n` est réel, donc pas d'absorption, pas de comportement métalique.
// De plus, les objets sont supposés être des dioptres entre du vide (n=1) et un milieu (n≠1).
// Pour faire des interfaces entre milieux, ça nécessiterait pas mal de travail.
#warning To do

#ifndef _LIGHTRAYS_OBJET_MILIEU_H_
#define _LIGHTRAYS_OBJET_MILIEU_H_

#include "ObjetsCourbes.h"

//------------------------------------------------------------------------------
// Dioptres de type courbe (1D) entre le vide et un milieu d'indice n, fixe
// (`n_fixe`) ou dépendnant de λ (`n_lambda`). Implémentation des lois de
// Snell-Descartes et des coefficients de Fresnel en intensité.

class ObjetCourbe_Milieux : virtual public ObjetCourbe {
public:
	
	std::function<float(float)> n_lambda; // n(λ)    /!\ coûteux -> N_COULEURS rayons réfractés à lancer
	float n_fixe; // n indépendant de λ, considéré si `indice_refr_lambda` est nul
	
	ObjetCourbe_Milieux (float incide_refr_fixe) : ObjetCourbe(), n_lambda(nullptr), n_fixe(incide_refr_fixe) {}
	ObjetCourbe_Milieux (std::function<float(float)> indice_refr_lambda) : ObjetCourbe(), n_lambda(indice_refr_lambda) {}
	
	ObjetCourbe_Milieux& operator= (const ObjetCourbe_Milieux&) = default;
	ObjetCourbe_Milieux (const ObjetCourbe_Milieux&) = default;
	virtual ~ObjetCourbe_Milieux () {}
	
	// Ré-émission du rayons intercepté en un rayon réfléchi et un rayon réfracté
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override final;
};

//------------------------------------------------------------------------------
// Dioptre linéaire : ObjetLigne + ObjetCourbe_Milieux

class ObjetLigne_Milieux : virtual public ObjetCourbe_Milieux, virtual public ObjetLigne {
public:
	// relai des arguments aux contructeurs de ObjetLigne et ObjetCourbe_Milieux
	// (toutes les combinaisons sont possibles)
	template<typename incide_refr_t, typename... Args> ObjetLigne_Milieux (incide_refr_t incide_refr, Args&&... x) :
		ObjetCourbe_Milieux(incide_refr), ObjetLigne(x...) {}
	
	virtual ~ObjetLigne_Milieux () {}
	
	// Dessin (intérieur coloré)
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
};

//------------------------------------------------------------------------------
// Dioptre en forme d'arc de cercle : ObjetArc + ObjetCourbe_Milieux

class ObjetArc_Milieux : virtual public ObjetCourbe_Milieux, virtual public ObjetArc {
public:
	// relai des arguments aux contructeurs de ObjetArc et ObjetCourbe_Milieux
	template<typename incide_refr_t, typename... Args> ObjetArc_Milieux (incide_refr_t incide_refr, Args&&... x) :
		ObjetCourbe_Milieux(incide_refr), ObjetArc(x...) {}
	
	virtual ~ObjetArc_Milieux () {}
};

//------------------------------------------------------------------------------
// Utilitaire : Objet composite fermé, délimité par des lignes
//  `ObjetLigne_Milieux`, à partir d'une liste de points `points`,
//  et d'indice de réfraction intérieur `incide_refr`.

class ObjetComposite_LignesMilieu : virtual public ObjetComposite {
private:
	template<typename incide_refr_t>
	static std::vector< std::unique_ptr<ObjetCourbe> > construct_liste (std::vector<point_t> points, incide_refr_t n) {
		std::vector< std::unique_ptr<ObjetCourbe> > objs;
		size_t N = points.size();
		for (size_t i = 0; i < N; i++)
			objs.emplace_back(new ObjetLigne_Milieux(n, points[ i ], points[ (i+1)%N ]));
		return objs;
	}
public:
	// construction à partir de la liste de points
	template<typename incide_refr_t> ObjetComposite_LignesMilieu (std::vector<point_t> points, incide_refr_t incide_refr) :
		ObjetComposite(construct_liste(points,incide_refr)) {}
	
	virtual ~ObjetComposite_LignesMilieu () {}
	
	// simple translation : positionnement du premier point de la chaine en `o`
	void re_positionne (point_t o);
};

#endif
