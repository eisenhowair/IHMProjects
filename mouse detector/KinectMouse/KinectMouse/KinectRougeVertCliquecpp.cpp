#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <windows.h>
#include <cstdlib> // pour obtenir le random
#include <ctime> // pour obtenir le random
#include <cmath> // calcul du point dans le cercle
#include "Cercle.h"

#define WIDTH 1280  //80 * 8 = 640
#define HEIGHT 720 // 60 * 8 = 480

using namespace std;
using namespace cv;

vector<Point> filtrePoints( vector<Point> ensemble_point )
{
	int tableau[8] = { 0 };
	vector<Point> ensemble_filtre;

	//On regarde dans quelle zone il y a assez de pixel
	for( const Point& p : ensemble_point )
	{
		if( p.y < HEIGHT / 2 )
		{
			if( p.x < WIDTH / 4 )
			{
				tableau[0]++;
			}
			else if( (p.x >= WIDTH / 4) && (p.x < WIDTH / 4 * 2) )
			{
				tableau[1]++;
			}
			else if( (p.x >= WIDTH / 4 * 2) && (p.x < WIDTH / 4 * 3) )
			{
				tableau[2]++;
			}
			else if( (p.x >= WIDTH / 4 * 3) && (p.x < WIDTH) )
			{
				tableau[3]++;
			}
		}
		else
		{
			if( p.x < WIDTH / 4 )
			{
				tableau[4]++;
			}
			else if( (p.x >= WIDTH / 4) && (p.x < WIDTH / 4 * 2) )
			{
				tableau[5]++;
			}
			else if( (p.x >= WIDTH / 4 * 2) && (p.x < WIDTH / 4 * 3) )
			{
				tableau[6]++;
			}
			else if( (p.x >= WIDTH / 4 * 3) && (p.x < WIDTH) )
			{
				tableau[7]++;
			}
		}
	}
	//On prend uniquement les pixels des zones non négligeables
	for( const Point& p : ensemble_point )
	{
		if( p.y < HEIGHT / 2 )
		{
			if( (p.x < WIDTH / 4) && (tableau[0] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4) && (p.x < WIDTH / 4 * 2) && (tableau[1] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4 * 2) && (p.x < WIDTH / 4 * 3) && (tableau[2] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4 * 3) && (p.x < WIDTH) && (tableau[3] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
		}
		else
		{
			if( (p.x < WIDTH / 4) && (tableau[4] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4) && (p.x < WIDTH / 4 * 2) && (tableau[5] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4 * 2) && (p.x < WIDTH / 4 * 3) && (tableau[6] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
			else if( (p.x >= WIDTH / 4 * 3) && (p.x < WIDTH) && (tableau[7] > 800) )
			{
				ensemble_filtre.push_back( p );
			}
		}
	}

	return ensemble_filtre;

}

Point trouveMilieu( vector<Point>& ensemble_red_points, int filtrage_necessaire )
{
	int sommeX = 0, sommeY = 0;
	if( filtrage_necessaire )
	{
		vector<Point> ensemble_points_filtre = filtrePoints( ensemble_red_points );
		if( !ensemble_points_filtre.empty() ) {
			for( const Point& p : ensemble_points_filtre ) {
				sommeX += p.x;
				sommeY += p.y;
			}
			return Point( sommeX / ensemble_points_filtre.size(), sommeY / ensemble_points_filtre.size() );

		}
		else {
			return Point( WIDTH / 2, HEIGHT / 2 );
		}
	}


	else {
		for( const Point& p : ensemble_red_points ) {
			sommeX += WIDTH - p.x;
			sommeY += p.y;
		}
		return Point( sommeX / ensemble_red_points.size(), sommeY / ensemble_red_points.size() );
	}


}

int genere_coordinate( int min, int max )
{
	return min + std::rand() % (max - min + 1);
}

bool point_dans_cercle( Point p, Cercle cible )
{
	return pow( p.x - cible.x(), 2 ) + pow( p.y - cible.y(), 2 ) < pow( cible.rayon(), 2 );
}

Cercle genere_cercle()
{
	int coord_radius = genere_coordinate( 30, WIDTH / 2 );

	// pour que le cercle ne dépasse pas de la frame
	int coord_x = genere_coordinate( coord_radius, WIDTH - coord_radius );
	int coord_y = genere_coordinate( coord_radius, HEIGHT - coord_radius );
	Cercle new_cercle( coord_x, coord_y, coord_radius );
	return new_cercle;
}


int main()
{
	VideoCapture cap;
	Mat frame, BW_Frame;
	Mat lower_red, upper_red, red_mask, hsv;
	Mat lower_green, upper_green, green_mask,green_mask_temp;
	Vec3b point_hsv;
	size_t width = (size_t)GetSystemMetrics( SM_CXSCREEN );
	size_t height = (size_t)GetSystemMetrics( SM_CYSCREEN );


	//Pour le cercle 
	srand( time( 0 ) );
	Cercle cible = genere_cercle();
	Scalar line_Color( 0, 0, 0 );
	int thickness = 5;
	bool compteur_initialise = false;
	int score = 0, compte_a_rebours = 0; // pour laisser un léger temps entre les cercles

	cap.open( 0 );
	cap.set( CAP_PROP_FRAME_WIDTH, WIDTH );
	cap.set( CAP_PROP_FRAME_HEIGHT, HEIGHT );


	if( !cap.isOpened() ) {
		cerr << "Erreur lors de la lecture video" << endl;
		return -1;
	}
	namedWindow( "flux_cam", WINDOW_NORMAL );
	namedWindow( "Frame Rouge", WINDOW_NORMAL );
	namedWindow( "Frame Verte", WINDOW_NORMAL );


	resizeWindow( "flux_cam", WIDTH, HEIGHT );
	resizeWindow( "Frame Rouge", 800, 600 );
	resizeWindow( "Frame Verte", 800, 600 );



	while( 1 ) {
		if( compteur_initialise == true )
		{
			compte_a_rebours--;
		}
		cap >> frame;

		if( frame.empty() ) {
			break;
		}
		// Conversion en hsv
		cvtColor( frame, hsv, cv::COLOR_BGR2HSV );


		// definition du filtre/masque pour rouge
		inRange( hsv, Scalar( 0, 140, 140 ), Scalar( 5, 255, 255 ), lower_red );
		inRange( hsv, Scalar( 160, 120, 120 ), Scalar( 180, 255, 255 ), upper_red );
		addWeighted( lower_red, 1.0, upper_red, 1.0, 0.0, red_mask );


		inRange( hsv, Scalar( 30, 80, 80 ), Scalar( 90, 255, 255 ), lower_green );
		inRange( hsv, Scalar( 150, 130, 130 ), Scalar( 170, 255, 255 ), upper_green );
		addWeighted( lower_green, 1.0, upper_green, 1.0, 0.0, green_mask_temp );

		subtract( green_mask_temp, red_mask, green_mask );



		// liste des points rouges puis verts avec coordonnées
		vector<cv::Point> red_pixel_coordinates;
		vector<cv::Point> green_pixel_coordinates;

		findNonZero( red_mask, red_pixel_coordinates );
		findNonZero( green_mask, green_pixel_coordinates );
		cout << "Nombre pixel ROUGE: " << red_pixel_coordinates.size() << " Nombre pixel VERT: " << green_pixel_coordinates.size() << endl;
		if( red_pixel_coordinates.size() > 300 )
		{

			Point centre_detection_red = trouveMilieu( red_pixel_coordinates, 1 );
			double relativeX = 1 - static_cast<double>(centre_detection_red.x) / static_cast<double>(WIDTH);
			double relativeY = static_cast<double>(centre_detection_red.y) / static_cast<double>(HEIGHT);

			int screen_width = GetSystemMetrics( SM_CXSCREEN );
			int screen_height = GetSystemMetrics( SM_CYSCREEN );

			int absoluteX = static_cast<int>(relativeX * screen_width);
			int absoluteY = static_cast<int>(relativeY * screen_height);
			SetCursorPos( absoluteX, absoluteY );
		}
		
		if( green_pixel_coordinates.size() > 300 && compte_a_rebours <= 0 )
		{
			compteur_initialise = true;
			Point centre_detection_green = trouveMilieu( green_pixel_coordinates, 1 );

			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

			if( (centre_detection_green.x < WIDTH / 4) && (centre_detection_green.y >= HEIGHT / 3) )
			{
				//clique droit
				input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				SendInput( 1, &input, sizeof( INPUT ) );

				input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				SendInput( 1, &input, sizeof( INPUT ) );

				cout << "Clique droit" << endl;
				compte_a_rebours = 8;

			}
			else if(   (centre_detection_green.x > WIDTH / 4 * 3) && (centre_detection_green.y >= HEIGHT / 3) )
			{
				cout << "Dans le if du clic gauche" << endl;
				// clique gauche
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				SendInput( 1, &input, sizeof( INPUT ) );

				input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				UINT result = SendInput( 1, &input, sizeof( INPUT ) );
				if( result == 0 ) {
					cout << "Erreur lors de l'envoi de l'événement de clic gauche. Code d'erreur : " << GetLastError() << endl;
				}
				else {
					cout << "Clic gauche envoyé avec succès." << endl;
				}
				compte_a_rebours = 8;


			}
			else if( centre_detection_green.y < HEIGHT / 3 )
			{
				HINSTANCE result = ShellExecute( NULL, L"open", L"osk.exe", NULL, NULL, SW_SHOWNORMAL );
				if( (int)result > 32 ) {
					cout << "Clavier apparait" << endl;
					compte_a_rebours = 8;
				}
				else {
					cerr << "Erreur d'ouverture du clavier" << endl;
				}


			}
		}
		else {
			//cout << "Nombre de pixel : " << green_pixel_coordinates.size() << " et compte a rebours a: " << compte_a_rebours << endl;
		}



		// on retourne l'écran (mirroir) pour utiliser plus instinctivement
		flip( frame, frame, +1 );
		flip( red_mask, red_mask, +1 );
		flip( green_mask, green_mask, +1 );

		// affichage des frames
		imshow( "flux_cam", frame );
		//imshow( "Frame Verte", green_mask );
		//imshow( "Frame Rouge", red_mask );



		if( waitKey( 30 ) >= 0 ) {

			break;
		}


	}

	return 0;

}
