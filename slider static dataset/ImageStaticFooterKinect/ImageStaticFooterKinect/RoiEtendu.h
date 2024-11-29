#pragma once
#include <opencv2/core/types.hpp>
#define WIDTH 1280 //80 * 8 = 640
#define HEIGHT 720 // 60 * 8 = 480

class RoiEtendu
{
public:
		RoiEtendu() = default;
		RoiEtendu( int x, int y, int largeur, int hauteur )
		{
		d_x = x;
		d_y = y;
		d_direction = 0;
		d_roi = cv::Rect( x, y, largeur, hauteur );
		}
		int x() const { return d_x; }
		int y() const { return d_y; }
		int direction() const { return d_direction; }
		cv::Rect rectangle() const { return d_roi; }
		/*
		void deplacerRoi(int new_x, int new_y)
		{
			if( direction != 0 ) {
				d_x = new_x;
				d_y = new_y;

				d_roi = cv::Rect( d_x, d_y, WIDTH / 6, HEIGHT / 5 );
			}
		}
		*/
		void deplacerRoi( int new_x, int new_y ) {
			if( d_direction == 0 ) {
				// Si la direction est 0, les images cessent de se déplacer
				return;
			}

			// Calcul de la nouvelle position en tenant compte de la direction
			int deplacement = (d_direction == -1) ? 5 : -5;
			d_x = (new_x + WIDTH + deplacement) % WIDTH;

			// Mise à jour de la position de la zone_roi
			d_roi = cv::Rect( d_x, d_y, WIDTH / 6, HEIGHT / 5 );
		}


		void deplacer()
		{
			if( d_direction == -1 )
			{
				deplacerVersGauche();
			}
			else if( d_direction == 1 )
			{
				deplacerVersDroite();
			}
			else {
				std::cout << "Pas de mouvement" << std::endl;
			}
		}
		void deplacerVersGauche()
		{
			d_x = (d_x + 5 ) % WIDTH;
			deplacerRoi( d_x, d_y );
			std::cout << "Vers la droite" << std::endl;

		}
		void deplacerVersDroite()
		{
			d_x = (d_x - 5) % WIDTH;
			deplacerRoi( d_x , d_y );
			std::cout << "Vers la gauche" << std::endl;

		}

		void changerDirection( int new_direction )
		{
			d_direction = new_direction;
		}

private:
		int d_x;
		int d_y;
		int d_direction;
		cv::Rect d_roi;
};

