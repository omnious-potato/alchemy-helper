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

std::ostream& operator<<(std::ostream &out, const struct FileLocationTable &_file_table) {
	out << "formID offset: " << _file_table.formIDArrayCountOffset << endl \
	    << "globalDataTable1Offset: " << _file_table.globalDataTable1Offset << endl \
	    << "globalDataTable2Offset: " << _file_table.globalDataTable2Offset << endl \
	    << "globalDataTable3Offset: " << _file_table.globalDataTable3Offset;
	return out;
}


struct GlobalData {
	uint32_t type;
	uint32_t length;
	uint8_t *data = NULL;
};

template<typename T, typename U, typename V>
int basic_plain_read(T &stream, U &dst, V amount) {

	stream.read(reinterpret_cast<char*>(&dst), amount);

	return 0;
}

template<typename T, typename U, typename V>
int basic_prefixed_read(T &stream, U * &dst, V prefix, int optional = 0) { //optional parameter is currently only determines whether char* string will be null-terminated

	basic_plain_read(stream, prefix, sizeof(V));
	dst = new U[static_cast<int>(prefix) + 1];//allocating memory
	basic_plain_read(stream, *dst, prefix);

	if (optional == 0)
		//dst[static_cast<int>(prefix)] = '\0';
		dst[static_cast<int>(prefix)] = '\0';


	return 0;
}


fstream file;

template<typename T>
int fread(T &dst) {
	return basic_plain_read(file, dst, sizeof(T));
}

template<typename U, typename V>
int ffread(U &dst, V prefix) {
	return basic_prefixed_read(file, dst, prefix);
}


istringstream udata;

template<typename T>
int uread(T &dst) {
	return basic_plain_read(udata, dst, sizeof(T));
}

template<typename U, typename V>
int uuread(U &dst, V prefix) {
	return basic_prefixed_read(udata, dst, prefix);
}



int main(int argc, char const *argv[])
{
	if (argc < 2) {
		cout << "Please provide path for savefile location" << endl;
		return 0;
	}

	string path_to_file = argv[1];

	file.open(path_to_file, ios::in | ios::binary);

	char magic[13] = {0};

	file.read(magic, 13);


	if (strcmp(magic, "TESV_SAVEGAME") != 0) {
		cout << magic << endl;
		cout << "Please provide correct Skyrim savefile!" << endl;
		return -1;
	}



//	Header processing below, includes basic info like name, race, level, location of a player in that save,
//	also compression type for part of the data and screenshot dimensions.

	uint32_t headerSize;

	fread(headerSize);
	fread(Header.version);
	fread(Header.saveNumber);

	ffread(Header.playerName.data, Header.playerName.prefix);

	fread(Header.playerLevel);

	ffread(Header.playerLocation.data, Header.playerLocation.prefix);
	ffread(Header.gameDate.data, Header.gameDate.prefix);
	ffread(Header.playerRaceEditorId.data, Header.playerRaceEditorId.prefix);

	fread(Header.playerSex);
	fread(Header.playerCurExp);
	fread(Header.playerLvlUpExp);
	fread(Header.filetime);
	fread(Header.shotWidth);
	fread(Header.shotHeight);
	fread(Header.compressionType);

	cout << Header << endl;

//	End of Header Section



//	Screenshot Data - skipped as for current needs is useless
//	For SE - 4 bytes per pixel (RGBA),
//		LE - 3 bytes pre pixel (RGB)
	int bytes_per_pixel = (Header.version >= 12) ? (4) : (3);
	int bytes_to_skip = bytes_per_pixel * Header.shotHeight * Header.shotWidth * sizeof(uint8_t);
	file.ignore(bytes_to_skip);
	cout << "Skipped screenshot data of " << bytes_to_skip / 1000.0 << "kB" << endl;
//	End of screenshot section



// 	Compression/decompression part
//
//  Further implementation assumes usage of LZ4 compression, decompression is done via corresponding C library with header file "lz4.h".
//  After that section bytes are read from string stream (using 'uread' and 'uuread' functions opposing previously used 'fread', 'ffread()')
//	made from decompressed char* data and not from source savefile.

	uint32_t uncompressedLen, compressedLen;
	fread(uncompressedLen);
	fread(compressedLen);


	cout << "Compression type: ";
	switch (int(Header.compressionType)) {
	case 0:
		cout << "NO COMPRESSION" << endl;
		break;
	case 1:
		cout << "zLib (please note and report)" << endl;
		break;
	case 2:
		cout << "LZ4 Block Format" << endl;
		break;
	}

	cout << "Uncompressed size: " << uncompressedLen / 1048576.0 << " MiB" << endl;
	cout << "Compressed size: " << compressedLen / 1048576.0 << " MiB" << endl;

	if (int(Header.compressionType) != 2) {
		cout << "Only LZ4 compression is supported!" << endl;
		return -1;
	}

	char * compressedInput = new char[compressedLen];
	char * decompressedOutput = new char[uncompressedLen];

	//Here we decompress LZ4 compressed data
	file.read(compressedInput, compressedLen);
	LZ4_decompress_safe(compressedInput, decompressedOutput, compressedLen, uncompressedLen);
	delete[] compressedInput;


	ofstream out("./REST");//should be empty, because there no footer after compressed chunk of savefile
	out << file.rdbuf();
	file.close();

	udata.str(string(decompressedOutput, uncompressedLen));
	delete[] decompressedOutput;
//	End of decompression section



//	Plugin section
//	Contains plugin info divided in two sections - plain PluginInfo and LightPluginInfo (mostly .esl or .esl'ified files)
	uint8_t formVersion;
	//udata.read(reinterpret_cast<char*>(&formVersion), sizeof(uint8_t));
	uread(formVersion);
	cout << "Form version: " << dec << formVersion - '\0' << endl;

	uint32_t pluginInfoSize;

	uread(pluginInfoSize);

	uread(PluginInfo.pluginCount);
	cout << "Plugins: " << dec << PluginInfo.pluginCount - '\0' << endl;

	PluginInfo.plugins = new TES::wstring[PluginInfo.pluginCount];


	for (uint i = 0; i < PluginInfo.pluginCount; i++) {
		uuread(PluginInfo.plugins[i].data, PluginInfo.plugins[i].prefix);
		//cout<<PluginInfo.plugins[i].data<<endl;
	}

	//same with light plugins
	uread(LightPluginInfo.pluginCount);
	cout << "Light plugins: " << dec << LightPluginInfo.pluginCount - '\0' << endl;

	LightPluginInfo.plugins = new TES::wstring[LightPluginInfo.pluginCount];

	for (uint i = 0; i < LightPluginInfo.pluginCount; i++) {
		uuread(LightPluginInfo.plugins[i].data, LightPluginInfo.plugins[i].prefix);
		//cout<<LightPluginInfo.plugins[i].data<<endl;
	}

//	End of Plugins and Light Plugins section



//	File Location Table
// 	Contains various tables offsets
//	(pretty much useless to us because by that far we already at globalDataTable1 and this offsets probably measured for uncompressed data)

	uread(FileLocationTable.formIDArrayCountOffset);
	uread(FileLocationTable.formIDArrayCountOffset);
	uread(FileLocationTable.unknownTable3Offset);
	uread(FileLocationTable.globalDataTable1Offset);
	uread(FileLocationTable.globalDataTable2Offset);
	uread(FileLocationTable.changeFormsOffset);
	uread(FileLocationTable.globalDataTable3Offset);
	uread(FileLocationTable.globalDataTable1Count);
	uread(FileLocationTable.globalDataTable2Count);
	uread(FileLocationTable.globalDataTable3Count);
	uread(FileLocationTable.changeFormCount);

	for (int i = 0; i < 15; ++i)
		uread(FileLocationTable.unused[i]);

	cout << FileLocationTable << endl;

//	End of File Location Table


// Global Data Table 1(first)
// Character stats should be there

	ofstream dbg("./DEBUG");
	out<<udata.rdbuf();
	return 0;

	// vector<struct GlobalData> globalDataTable1(FileLocationTable.globalDataTable1Count);

	// for (uint32_t i = 0; i < FileLocationTable.globalDataTable1Count; ++i)
	// {
	// 	uread(globalDataTable1[i].type);
	// 	uread(globalDataTable1[i].length);
	// 	cout<<globalDataTable1[i].type<<"|"<<globalDataTable1[i].length<<endl;


	// 	globalDataTable1[i].data = new uint8_t[globalDataTable1[i].length];
		
	// 	for (uint32_t j = 0; j < globalDataTable1[i].length; ++j)
	// 	{
	// 		uread(globalDataTable1[i].data[j]);

	// 	}


	// }



//	freeing memory here, probably should just change pointers to smart pointers

	for (uint32_t i = 0; i < FileLocationTable.globalDataTable1Count; ++i)
		delete[] globalDataTable1[i].data;


	for (uint16_t i = 0; i < LightPluginInfo.pluginCount; ++i)
		delete[] LightPluginInfo.plugins[i].data;
	delete LightPluginInfo.plugins;

	for (uint8_t i = 0; i < PluginInfo.pluginCount; ++i)
		delete[] PluginInfo.plugins[i].data;
	delete PluginInfo.plugins;

	delete[] Header.playerName.data;
	delete[] Header.playerLocation.data;
	delete[] Header.gameDate.data;
	delete[] Header.playerRaceEditorId.data;

	file.close();

	return 0;
}