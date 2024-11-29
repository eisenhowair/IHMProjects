#pragma once
class Cercle
{
public:
	
			Cercle( int x, int y, int ray )
			{
				d_x = x;
				d_y = y;
				d_rayon = ray;
			}

			int x() const { return d_x; }
			int y() const { return d_y; }
			int rayon() const { return d_rayon; }
private:
			int d_x;
			int d_y;
			int d_rayon;
		
	
};

