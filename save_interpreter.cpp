#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <bitset>

#include "lz4.h"


template<typename T>
std::ostream& operator<<(std::ostream &out, const std::vector<T> &vec) {
	for (auto i : vec) {
		out << i << ' ';
	}
	return out;
}

using namespace std;

//plain .ESS file format savefile interpreter for TESV:Skyrim
//as defined in corresponding uesp.net webpage


namespace TES {

struct wstring
{
	uint16_t prefix;
	char *data = NULL;
};

}


typedef float float32;
typedef uint64_t FILETIME;


struct Header {
	uint32_t version;
	uint32_t saveNumber;
	TES::wstring playerName;
	uint32_t playerLevel;
	TES::wstring playerLocation;
	TES::wstring gameDate;
	TES::wstring playerRaceEditorId;
	uint16_t playerSex;
	float32 playerCurExp;
	float32 playerLvlUpExp;
	FILETIME filetime;
	uint32_t shotWidth;
	uint32_t shotHeight;
	uint16_t compressionType;
} Header;

std::ostream& operator<<(std::ostream &out, const struct Header &_header) {
	out << _header.playerName.data << ", " << _header.playerRaceEditorId.data << " lvl " << _header.playerLevel << " at " << _header.playerLocation.data;
	return out;
}


struct PluginInfo {
	uint8_t pluginCount;
	//vector<TES::wstring> plugins;
	TES::wstring *plugins = NULL;
} PluginInfo;


struct LightPluginInfo {
	uint16_t pluginCount;
	//vector<TES::wstring> plugins;
	TES::wstring *plugins = NULL;
} LightPluginInfo;


struct FileLocationTable {
	uint32_t formIDArrayCountOffset;
	uint32_t unknownTable3Offset;
	uint32_t globalDataTable1Offset;
	uint32_t globalDataTable2Offset;
	uint32_t changeFormsOffset;
	uint32_t globalDataTable3Offset;
	uint32_t globalDataTable1Count;
	uint32_t globalDataTable2Count;
	uint32_t globalDataTable3Count;
	uint32_t changeFormCount;
	uint32_t unused[15];
} FileLocationTable;

fstream file;
istringstream udata;


// template<typename T, typename U, typename V>
// int basic_read(T &stream, U &destination, V amount){
// 	stream.read(reinterpret_cast<char*>(&destination), amount);

// 	return 0;
// }

// template<typename U, typename V>
// int fread(U &dest, V amount){
// 	return basic_read(file, dest, amount);
// }

template<typename T, typename U>
int fread(T &path, U amount) {
	file.read(reinterpret_cast<char*>(&path), amount);

	return 0;
}

template<typename T, typename U>
int fread_alloc(T &path, U amount) {
	//reading a char data (presumably text string) of length derived prom prefix
	//TESV::wstring type isn't zero-terminated, so we do it manually, thus size of allocated pointer is prefix + 1

	path = new char[static_cast<int>(amount) + 1];

	file.read(reinterpret_cast<char*>(path), amount * sizeof(char));

	path[amount * sizeof(char)] = '\0';

	return 0;
}

template<typename V, typename U>
int ffread(V &data, U &prefix) {
	fread(prefix, sizeof(prefix));
	fread_alloc(data, prefix);
	return 0;
}


// template<typename T, typename U>
// int uread(T &path, U amount){
// 	udata.read(reinterpret_cast<char*>(&path), amount);

// 	return 0;
// }




int main(int argc, char const *argv[])
{
	if (argc < 2) {
		cout << "Please provide path for savefile location" << endl;
		return 0;
	}

	string path_to_file = argv[1];
	// cout << path_to_file << endl;

	file.open(path_to_file, ios::in | ios::binary);

	char magic[13] = {0};

	file.read(magic, 13);


	if (strcmp(magic, "TESV_SAVEGAME") != 0) {
		cout << magic << endl;
		cout << "Please provide correct Skyrim savefile!" << endl;
		return -1;
	}

	//cout << "=== Reading header data ===" << endl;
	//header bullshit over here

	uint32_t headerSize;
	fread(headerSize, sizeof(uint32_t));
	fread(Header.version, sizeof(uint32_t));
	fread(Header.saveNumber, sizeof(uint32_t));

	ffread(Header.playerName.data, Header.playerName.prefix);

	fread(Header.playerLevel, sizeof(uint32_t));

	ffread(Header.playerLocation.data, Header.playerLocation.prefix);

	ffread(Header.gameDate.data, Header.gameDate.prefix);

	ffread(Header.playerRaceEditorId.data, Header.playerRaceEditorId.prefix);

	fread(Header.playerSex, sizeof(uint16_t));

	fread(Header.playerCurExp, sizeof(float32));
	fread(Header.playerLvlUpExp, sizeof(float32));

	fread(Header.filetime, sizeof(FILETIME));

	fread(Header.shotWidth, sizeof(uint32_t));
	fread(Header.shotHeight, sizeof(uint32_t));

	fread(Header.compressionType, sizeof(uint16_t));

	cout << Header << endl;

	//Screenshot Data - skipped as for current needs is useless
	//assuming we use Skyrim SE image data is uint8[4 * width * height]
	int bytes_per_pixel = (Header.version >= 12) ? (4) : (3);
	int bytes_to_skip = bytes_per_pixel * Header.shotHeight * Header.shotWidth * sizeof(uint8_t);
	file.ignore(bytes_to_skip);
	cout << "Skipped screenshot data of " << bytes_to_skip / 1000.0 << "kB" << endl;


	//Compression lengths - any data past that point is compressed unless there is no compression
	uint32_t uncompressedLen, compressedLen;
	fread(uncompressedLen, sizeof(uint32_t));
	fread(compressedLen, sizeof(uint32_t));


	cout << "Compression type: ";
	switch (int(Header.compressionType)) {
	case 0:
		cout << "NO COMPRESSION" << endl;
		break;
	case 1:
		cout << "zLib (please investigate)" << endl;
		break;
	case 2:
		cout << "LZ4 Block Format" << endl;
		break;
	}

	cout << "Uncompressed: " << uncompressedLen / 1048576.0 << " MiB" << endl;
	cout << "Compressed: " << compressedLen / 1048576.0 << " MiB" << endl;
	
	if (int(Header.compressionType) != 2) {
		cout << "Only LZ4 compression is supported!" << endl;
		return -1;
	}

	char * compressedInput = new char[compressedLen];
	char * decompressedOutput = new char[uncompressedLen];

	//Here we decompress LZ4 compressed data
	file.read(compressedInput, compressedLen * sizeof(char));
	LZ4_decompress_safe(compressedInput, decompressedOutput, compressedLen, uncompressedLen);
	delete[] compressedInput;

	fstream temp("./TEMP.dat", ios::trunc | ios::binary);
	temp.write(decompressedOutput, uncompressedLen);
	temp.close();

	
	//string str(decompressedOutput, uncompressedLen);
	udata.str(string(decompressedOutput, uncompressedLen));
	delete[] decompressedOutput;
	
	

	uint8_t formVersion;
	udata.read(reinterpret_cast<char*>(&formVersion), sizeof(uint8_t));
	cout<<dec<<formVersion - '\0';

	//freeing memory here, probably should just change pointers to smart pointers
	
	delete[] Header.playerName.data;
	delete[] Header.playerLocation.data;
	delete[] Header.gameDate.data;
	delete[] Header.playerRaceEditorId.data;

	file.close();

	return 0;
}