#pragma once
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
using namespace std;
using namespace cv;

class Oeuvre
{
public:

	Oeuvre() = default;
	Oeuvre( string chemin, Mat artwork )
	{
		d_chemin = chemin;
		d_artwork = artwork.clone();

	}
	// Opérateur de copie
	Oeuvre( const Oeuvre& other ) {
		// Copiez les membres de 'other' dans l'objet actuel
		d_chemin = other.d_chemin;
		d_artwork = other.d_artwork.clone();  // Utilisez clone pour copier l'image
		d_desc = other.d_desc;
	}

	void setDescription( string desc )
	{
		d_desc = desc;
	}
	string getDescription() const
	{
		return d_desc;
	}
	string getChemin()
	{
		return d_chemin;
	}
	Mat getArtwork()
	{
		return d_artwork;
	}

	Mat d_artwork;



private:
	string d_desc, d_chemin;
};

