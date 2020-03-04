/*******************************************************************************
 * Écrans : objets absorbant les rayons et accumulant l'intensité pour
 *  l'afficher ou l'enregistrer ("CCD").
 *******************************************************************************/

#ifndef _LIGHTRAYS_ECRAN_H_
#define _LIGHTRAYS_ECRAN_H_

#include "ObjetsCourbes.h"
#include <vector>
#include "Rayon.h"

//------------------------------------------------------------------------------
// Classe de base virtuelle des écrans. Ne fait rien.

class Ecran_Base : virtual public Objet {
protected:
	size_t n_acc; // nombre de frames accumulées
public:
	float luminosite; // coefficient de conversion intensité réelle -> intensité affichée
	
	Ecran_Base (float lumino = 1.) : n_acc(0), luminosite(lumino) {}
	virtual ~Ecran_Base () {}
	
	// appellé à la fin de chaque frame
	virtual void commit () { n_acc++; }
	// réinitialisation de l'écran
	virtual void reset () = 0;
};

//------------------------------------------------------------------------------
// Écran en forme de segment. Matrice (1D) de pixels, affichable ou enregistrable.

class EcranLigne_Multi : virtual public Ecran_Base, virtual public ObjetLigne {
protected:
	std::vector<Specte> bins_intensit; // matrice de pixels; un spectre par pixel
public:
	std::optional<float> epaisseur_affich = std::nullopt; // épaisseur de l'écran affiché
	
	// Construction à partir des points A et B et du nombre de pixels par unité de longueur
	EcranLigne_Multi (point_t pos_a, point_t pos_b, float pixel_density, float lumino = 1.);
	// Construction à partir des points A et B et du nombre de pixels
	EcranLigne_Multi (point_t pos_a, point_t pos_b, uint16_t pixels_n, float lumino = 1.);
	
	virtual ~EcranLigne_Multi () {}
	
	// Absorption et accumulation des rayons; pas de ré-émission
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override;
	// Réinitialisation de l'écran
	virtual void reset () override;
	
	// Récupération de la matrice de pixels RGB ou spectres :
	struct pixel_t {
		float s1, s_mid, s2; // abscice du début, du milieu et de la fin du pixel
		uint8_t r, g, b; // valeurs RGB (les mêmes qu'affichées)
		bool sat; // saturation du pixel
		Specte spectre; // spectre accumulé sur le pixel
	};
	std::vector<pixel_t> matrice_pixels () const;
	// Dessin de cette matrice de pixels
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
};

//------------------------------------------------------------------------------
// Écran en forme de segment avec un unique pixel

class EcranLigne_Mono : virtual public Ecran_Base, virtual public ObjetLigne {
protected:
	Specte intensit; // spectre accumulé
public:
	
	EcranLigne_Mono (point_t pos_a, point_t pos_b, float lumino = 1.);
	virtual ~EcranLigne_Mono () {}
	
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override;
	virtual void reset () override;
	
	struct pixel_t { uint8_t r, g, b; bool sat; };
	pixel_t pixel () const;
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
};

#endif
