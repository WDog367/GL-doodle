#include "file_utils.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

void loadObj(vector<float> &vertices, vector<unsigned int> &elements, char *fileName) {
	ifstream file;
	//istringstream sline;
	string line;
	string token;

	int in_int;
	float in_float;

	file.open(fileName);
	if (!fileName) {
		cerr << "Error loading .obj: " << fileName << endl;
		return;
	}
	//.obj format:
	//v lines have vertices
	//f lines have face (indices)
	while (getline(file, line)) {

		if (line.substr(0, 2) == "v ") {
			//Loading a vertex
			istringstream sline(line.substr(2));
			//getting the three vertex coordinates on this line
			for (int i = 0; i < 3; i++) {//there might be a fourth component, but it's not necessarily useful
				sline >> in_float;
				vertices.push_back(in_float);
			}
			vertices.push_back(1.0f);
			
		}
		else if (line.substr(0, 2) == "f ") {
			//Loading a face
			istringstream sline(line.substr(2));;

			for (int i = 0; i < 3; i++) {
				string temp;

				sline >> token;
				if (token.find("/") != -1) {
					in_int = atoi(token.substr(0, token.find("/")).c_str());
				}
				else {
					in_int = atoi(token.c_str());
				}
				elements.push_back(in_int);

			}
		}
	}


}
