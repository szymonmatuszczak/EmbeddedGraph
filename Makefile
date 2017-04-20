FBTouch.so: FBTouch.cpp
	g++ -shared -fPIC -o FBTouch.so FBTouch.cpp -std=c++14 -O3

FBGraphics.so: FBTouch.cpp
	g++ -shared -fPIC -o FBGraphics.so FBGraphics.cpp -std=c++14 -O3
