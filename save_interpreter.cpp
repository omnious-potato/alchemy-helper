#include <iostream>
#include <string>
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

template<typename T, typename U>
int oread(T &path, U amount) {
	file.read(reinterpret_cast<char*>(&path), amount);

	return 0;
}

template<typename T, typename U>
int oread_alloc(T &path, U amount) {
	//reading a char data (presumably text string) of length derived prom prefix
	//TESV::wstring type isn't zero-terminated, so we do it manually, thus size of allocated pointer is prefix + 1

	path = new char[static_cast<int>(amount) + 1];

	file.read(reinterpret_cast<char*>(path), amount * sizeof(char));

	path[amount * sizeof(char)] = '\0';

	return 0;
}

// template<typename V, typename T, typename U>
// int qqread(V &data_path_to_read, T &path_to_read, U amount_to_read) {
// 	oread(amount_to_read, sizeof(uint16_t));//WTF THIS WORKS? I AM CONFUSED (apparently template magic made a temp variable to replace constant sizeof())
//												ZERO MAGIC here sizeof(path_to_read) == amount_to_read;
// 	oread_alloc(data_path_to_read, amount_to_read);
// 	return 0;
// }


template<typename V, typename U>
int qqread(V &data, U &prefix) {
	oread(prefix, sizeof(prefix));
	oread_alloc(data, prefix);
	return 0;
}



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
	oread(headerSize, sizeof(uint32_t));
	oread(Header.version, sizeof(uint32_t));
	oread(Header.saveNumber, sizeof(uint32_t));

	qqread(Header.playerName.data, Header.playerName.prefix);

	oread(Header.playerLevel, sizeof(uint32_t));

	qqread(Header.playerLocation.data, Header.playerLocation.prefix);

	qqread(Header.gameDate.data, Header.gameDate.prefix);

	qqread(Header.playerRaceEditorId.data, Header.playerRaceEditorId.prefix);

	oread(Header.playerSex, sizeof(uint16_t));

	oread(Header.playerCurExp, sizeof(float32));
	oread(Header.playerLvlUpExp, sizeof(float32));

	oread(Header.filetime, sizeof(FILETIME));

	oread(Header.shotWidth, sizeof(uint32_t));
	oread(Header.shotHeight, sizeof(uint32_t));

	oread(Header.compressionType, sizeof(uint16_t));

	cout << Header << endl;

	//Screenshot Data - skipped as for current needs is useless
	//assuming we use Skyrim SE image data is uint8[4 * width * height]
	int bytes_per_pixel = (Header.version >= 12) ? (4) : (3);
	int bytes_to_skip = bytes_per_pixel * Header.shotHeight * Header.shotWidth * sizeof(uint8_t);
	file.ignore(bytes_to_skip);
	cout << "Skipped screenshot data of " << bytes_to_skip / 1000.0 << "kB" << endl;


	//Compression lengths - any data past that point is compressed unless there is no compression
	uint32_t uncompressedLen, compressedLen;
	oread(uncompressedLen, sizeof(uint32_t));
	oread(compressedLen, sizeof(uint32_t));


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



	//Here we decompress LZ4 compressed data
	if (int(Header.compressionType) != 2) {
		cout << "Only LZ4 compression is supported!" << endl;
		return -1;
	}



	char * compressedInput = new char[compressedLen];
	char * decompressedOutput = new char[uncompressedLen];

	file.read(compressedInput, compressedLen * sizeof(char));
	LZ4_decompress_safe(compressedInput, decompressedOutput, compressedLen, uncompressedLen);


	ofstream unpackedData("./tmp/")

	delete[] compressedInput;






	delete[] decompressedOutput;



	//char * decompressedOutput = new char[3 * compressedLen + 8];



	//delete[] compressedInput;
	//delete[] decompressedOutput;



	// uint8_t formVersion;
	// oread(formVersion, sizeof(uint8_t));

	// cout << "form version " << int(formVersion) << endl;


	// //Plugin data over here (unused)
	// uint32_t plugInfoSize;
	// oread(plugInfoSize, sizeof(uint32_t));

	// cout << " === Plugin info size: " << dec << plugInfoSize << endl;
	// cout << "=== Plugins info ===" << endl;

	// uint32_t factualPluginInfoSize = 0;

	// oread(PluginInfo.pluginCount, sizeof(uint8_t));
	// factualPluginInfoSize += sizeof(uint8_t);

	// PluginInfo.plugins = new TES::wstring[PluginInfo.pluginCount];
	// cout << (unsigned int)(PluginInfo.pluginCount) << endl;
	// cout << bitset<8>(PluginInfo.pluginCount) << endl;

	// for (int i = 0; i < PluginInfo.pluginCount; i++) {
	// 	qqread(PluginInfo.plugins[i].data, PluginInfo.plugins[i].prefix);
	// 	factualPluginInfoSize += sizeof(uint16_t) + PluginInfo.plugins[i].prefix * sizeof(char);
	// }



	// //file.ignore(plugInfoSize);

	// //Light plugin data
	// cout << "=== Light plugins info ===" << endl;

	// oread(LightPluginInfo.pluginCount, sizeof(uint16_t));
	// factualPluginInfoSize += sizeof(uint16_t);
	// LightPluginInfo.plugins =  new TES::wstring[LightPluginInfo.pluginCount];
	// cout << dec << LightPluginInfo.pluginCount << endl;
	// cout << bitset<16>(LightPluginInfo.pluginCount) << endl;

	// for (auto i = 0; i < LightPluginInfo.pluginCount; i++) {
	// 	qqread(LightPluginInfo.plugins[i].data, LightPluginInfo.plugins[i].prefix);
	// 	factualPluginInfoSize += sizeof(uint16_t) + LightPluginInfo.plugins[i].prefix * sizeof(char);
	// }

	// cout << factualPluginInfoSize << endl;

	// //Presumably we just arrived at File Location table (hopefully)
	// cout << "=== File Location Table ===" << endl;

	// oread(FileLocationTable.formIDArrayCountOffset, sizeof(uint32_t));
	// oread(FileLocationTable.unknownTable3Offset, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable1Offset, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable2Offset, sizeof(uint32_t));
	// oread(FileLocationTable.changeFormsOffset, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable3Offset, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable1Count, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable2Count, sizeof(uint32_t));
	// oread(FileLocationTable.globalDataTable3Count, sizeof(uint32_t));
	// oread(FileLocationTable.changeFormCount, sizeof(uint32_t));

	// oread(FileLocationTable.unused, 15 * sizeof(uint32_t));



	// //Global data table
	// file.close();
	// file.open(path_to_file, ios::in | ios::binary);

	// cout << dec << FileLocationTable.globalDataTable1Offset << endl;


	// //file.ignore()



	// //TODO: just skip plugins and not write that to heap


	// // oread(PluginInfo.pluginCount, sizeof(uint8_t));
	// // cout << "Plugin count:" << PluginInfo.pluginCount - '0' << endl;

	// // PluginInfo.plugins = new TES::wstring[PluginInfo.pluginCount];

	// // for (auto i = 0; i < PluginInfo.pluginCount - '0'; i++) {
	// // 	qqread(PluginInfo.plugins[i].data, PluginInfo.plugins[i].prefix, sizeof(uint16_t));
	// // }

	// // //Light plugin data over there (also unused)
	// // cout << "=== Light plugins info === " << endl;

	// // oread(LightPluginInfo.pluginCount, sizeof(uint16_t));
	// // cout << "Plugin count:" << LightPluginInfo.pluginCount - '0' << endl;

	// // LightPluginInfo.plugins = new TES::wstring[LightPluginInfo.pluginCount];

	// // for (auto i = 0; i < LightPluginInfo.pluginCount - '0'; i++) {
	// // 	qqread(LightPluginInfo.plugins[i].data, LightPluginInfo.plugins[i].prefix, sizeof(uint16_t));
	// // }

	// //freeing memory here, probably should just change pointers to smart pointers
	delete[] Header.playerName.data;
	delete[] Header.playerLocation.data;
	delete[] Header.gameDate.data;
	delete[] Header.playerRaceEditorId.data;

	// for (auto i = 0; i < PluginInfo.pluginCount; i++)
	// 	delete[] PluginInfo.plugins[i].data;
	// delete[] PluginInfo.plugins;

	file.close();

	return 0;
}