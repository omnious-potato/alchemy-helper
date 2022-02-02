#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>

using namespace std;

//simple .ESS file format savefile interpreter for SKyrim SE 
//as defined in corresponding uesp.net webpage


int main(int argc, char const *argv[])
{
	if(argc < 2){
		cout<<"Please provide path for savefile location"<<endl;
		return 0;
	}

	string path_to_file = argv[1];
	cout<<path_to_file<<endl;

	fstream file(path_to_file, ios::in | ios::binary);

	char magic[13] = {0};

	file.read(magic, 13);

	if(strcmp(magic, "TESV_SAVEGAME") != 0){
		cout<<"Please provide correct Skyrim savefile!"<<endl;
		return -1;
	}

	cout<<magic<<endl;	


	uint32_t headerSize;
	file.read(headerSize, 4);
	cout<<headerSize<<dec<<endl;	




	return 0;
}