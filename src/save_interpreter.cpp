#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <bitset>
#include <variant>
#include <cassert>
#include <cmath>
#include <memory>
#include <thread>
#include <chrono>

#include "lz4.h"
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif


#include "definitions.cpp"

using namespace std;

//Aliased main file and uncompressed chunk reading functions
fstream file;

template<typename T>
int fileRead(T & dst) {
	return universalRead(file, dst, sizeof(T));
}

template<typename U, typename V>
int fileBulkRead(U & dst, V prefix) {
	return universalBulkRead(file, dst, prefix);
}


istringstream udata;

template<typename T>
int unpackedRead(T & dst) {
	return universalRead(udata, dst, sizeof(T));
}

template<typename U, typename V>
int unpackedBulkRead(U & dst, V & prefix) {
	return universalBulkRead(udata, dst, prefix);
}


int main(int argc, char const *argv[])
{
// 	This section purely extracts savefile path and
//	checks tha save has proper magic numebr dedicated for .ess files

	if (argc < 2) {
		cout << "Please provide path for savefile location" << endl;
		return 0;

	}

	string path_to_file = argv[1];

	file.open(path_to_file, ios::in | ios::binary);

	char * magic = new char[14];
	magic[13] = '\0';
	file.read(magic, 13);


	if (strncmp(magic, "TESV_SAVEGAME", 13) != 0) {
		//if(magic != "TESV_SAVEGAME"){
		//cout<< magic<<endl;
		cout << "Please provide correct Skyrim savefile!" << endl;
		delete[] magic;
		return -1;
	} else {
		delete[] magic;
	}


//	Header processing below, includes basic info like name, race, level, location of a player in that save,
//	also compression type for part of the data and screenshot dimensions.
	struct Header Header;

	uint32_t headerSize;

	fileRead(headerSize);

	fileRead(Header.version);
	fileRead(Header.saveNumber);

	fileBulkRead(Header.playerName.data, Header.playerName.prefix);

	fileRead(Header.playerLevel);

	fileBulkRead(Header.playerLocation.data, Header.playerLocation.prefix);
	fileBulkRead(Header.gameDate.data, Header.gameDate.prefix);
	fileBulkRead(Header.playerRaceEditorId.data, Header.playerRaceEditorId.prefix);

	fileRead(Header.playerSex);
	fileRead(Header.playerCurExp);
	fileRead(Header.playerLvlUpExp);
	fileRead(Header.filetime);
	fileRead(Header.shotWidth);
	fileRead(Header.shotHeight);
	fileRead(Header.compressionType);

	cout << "Parsing save \"" << Header.playerName.data << "\"" << endl;
	int end_header = file.tellg();
	cout << "Header parsed,\tC: " << end_header << endl;

//	cout << Header << endl;
//	End of Header Section


//	Screenshot Data - skipped as for current needs is useless
//	For SE - 4 bytes per pixel (RGBA),
//		LE - 3 bytes pre pixel (RGB)
	int bytes_per_pixel = (Header.version >= 12) ? (4) : (3);
	int bytes_to_skip = bytes_per_pixel * Header.shotHeight * Header.shotWidth * sizeof(uint8_t);
	file.ignore(bytes_to_skip);

//	End of screenshot section

// 	Compression/decompression part
//
//  Further implementation assumes usage of LZ4 compression, decompression is done via corresponding C library with header file "lz4.h".
//  After that section bytes are read from string stream (using 'unpackedRead' and 'unpackedBulkRead' functions opposing previously used 'fileRead', 'fileBulkRead()')
//	made from decompressed char* data and not from source savefile.

	uint32_t uncompressedLen, compressedLen;
	fileRead(uncompressedLen);
	fileRead(compressedLen);


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

	LZ4_decompress_safe(move( compressedInput), decompressedOutput, compressedLen, uncompressedLen);
	delete[] compressedInput;

	file.close();

	udata.str(string(decompressedOutput, uncompressedLen));
	delete[] decompressedOutput;

	ofstream out("./debug/uncompressed");
	out << udata.rdbuf();
	out.close();
	udata.seekg(0);
//	End of decompression section


//	Plugin section
//	Contains plugin info divided in two sections - plain PluginInfo and LightPluginInfo (mostly .esl or .esl'ified files)
	uint8_t formVersion;

	ofstream plugins("./debug/plugins");

	unpackedRead(formVersion);
	cout << "Form version: " << dec << formVersion - '\0' << endl;

	uint32_t pluginInfoSize;
	unpackedRead(pluginInfoSize);
	cout << "PluginInfoSize: " << pluginInfoSize << endl;

	int plugin_size_check = pluginInfoSize;

	unpackedRead(PluginInfo.pluginCount);
	plugin_size_check -= sizeof(PluginInfo.pluginCount);


	cout << "Plugins: " << dec << PluginInfo.pluginCount - '\0' << endl;

	PluginInfo.plugins = new TES::wstring[PluginInfo.pluginCount];


	plugins << "====ESP====" << endl;
	for (uint32_t i = 0; i < PluginInfo.pluginCount; i++) {
		unpackedBulkRead(PluginInfo.plugins[i].data, PluginInfo.plugins[i].prefix);
		plugin_size_check -= PluginInfo.plugins[i].prefix + sizeof(PluginInfo.plugins[i].prefix);

		plugins << PluginInfo.plugins[i].data << endl;
	}

	//same with light plugins
	if (Header.version >= 12 && plugin_size_check > 0) {
		unpackedRead(LightPluginInfo.pluginCount);

		cout << "Light plugins: " << dec << LightPluginInfo.pluginCount - '\0' << endl;

		LightPluginInfo.plugins = new TES::wstring[LightPluginInfo.pluginCount];


		plugins << "====ESL====" << endl;
		for (uint32_t i = 0; i < LightPluginInfo.pluginCount; i++)	{
			unpackedBulkRead(LightPluginInfo.plugins[i].data, LightPluginInfo.plugins[i].prefix);
			plugins << LightPluginInfo.plugins[i].data << endl;
		}
	}

	plugins.close();
//	End of Plugins and Light Plugins section


//	File Location Table
// 	Contains various tables offsets and counts

	struct FileLocationTable FileLocationTable;

	unpackedRead(FileLocationTable.formIDArrayCountOffset);
	unpackedRead(FileLocationTable.unknownTable3Offset);
	unpackedRead(FileLocationTable.globalDataTable1Offset);
	unpackedRead(FileLocationTable.globalDataTable2Offset);
	unpackedRead(FileLocationTable.changeFormsOffset);
	unpackedRead(FileLocationTable.globalDataTable3Offset);
	unpackedRead(FileLocationTable.globalDataTable1Count);
	unpackedRead(FileLocationTable.globalDataTable2Count);
	unpackedRead(FileLocationTable.globalDataTable3Count);
	unpackedRead(FileLocationTable.changeFormCount);


	for (int i = 0; i < 15; ++i)
		unpackedRead(FileLocationTable.unused[i]);
	cout << FileLocationTable << endl;
//	End of File Location Table

// Global Data Table 1, 2 and 3
// Too bored to parse data content fully over there
	vector<struct GlobalData> globalDataTable1(FileLocationTable.globalDataTable1Count);
	vector<struct GlobalData> globalDataTable2(FileLocationTable.globalDataTable2Count);
	vector<struct GlobalData> globalDataTable3(FileLocationTable.globalDataTable3Count);


	//First and Second table are read from udata sequentially and third is being seeked by and offset given in FileLocationTable

	cout << "GlobalData tables: ";

	int delta = FileLocationTable.globalDataTable1Offset - udata.tellg();
	int reset_marker = udata.tellg();

	for (uint32_t i = 0; i < FileLocationTable.globalDataTable1Count; i++)	{
		unpackedRead(globalDataTable1[i].type);
		cout << globalDataTable1[i].type << ' ';
		unpackedBulkRead(globalDataTable1[i].data, globalDataTable1[i].length);
	}

	for (uint32_t i = 0; i < FileLocationTable.globalDataTable2Count; i++)	{
		unpackedRead(globalDataTable2[i].type);
		cout << globalDataTable2[i].type << ' ';
		unpackedBulkRead(globalDataTable2[i].data, globalDataTable2[i].length);
	}

	udata.seekg(FileLocationTable.globalDataTable3Offset - delta);
	for (uint32_t i = 0; i < FileLocationTable.globalDataTable3Count; i++) {
		unpackedRead(globalDataTable3[i].type);
		cout << globalDataTable3[i].type << ' ';
		unpackedBulkRead(globalDataTable3[i].data, globalDataTable3[i].length);
	}
	cout << endl;

	//Parsing Global Data tables for some info
	struct MiscStats MiscStats;

	istringstream globalData(string(reinterpret_cast<char*>(globalDataTable1[0].data), globalDataTable1[0].length));

	universalRead(globalData, MiscStats.count, sizeof(uint32_t));
	MiscStats.stats = new MiscStat[MiscStats.count];

	for (uint32_t i = 0; i < MiscStats.count; i++)
	{
		universalBulkRead(globalData, MiscStats.stats[i].name.data, MiscStats.stats[i].name.prefix);
		universalRead(globalData, MiscStats.stats[i].category, sizeof(uint8_t));
		universalRead(globalData, MiscStats.stats[i].value, sizeof(int32_t));
	}
	//By here globalData string stream should be empty
	if (!globalData.rdbuf()->in_avail() == 0)
		cerr << "String stream was not emptied!" << endl;

//Change Forms

	vector<struct ChangeForm> changeForms(FileLocationTable.changeFormCount);

	//forms are read by a given offset in FileLocationTable
	udata.seekg(FileLocationTable.changeFormsOffset - delta);

	// ofstream e("./debug/dasuka");

	for (uint32_t i = 0; i < FileLocationTable.changeFormCount; i++)
	{
		unpackedRead(changeForms[i].formID.byte0);
		unpackedRead(changeForms[i].formID.byte1);
		unpackedRead(changeForms[i].formID.byte2);

		unpackedRead(changeForms[i].changeFlags);
		unpackedRead(changeForms[i].type);
		unpackedRead(changeForms[i].version);

		unsigned int length_size = pow(2, (changeForms[i].type & 0xC0) >> 6);

		universalRead(udata, changeForms[i].length1, length_size);
		universalRead(udata, changeForms[i].length2, length_size);

		if (changeForms[i].length2 == 0) {
			changeForms[i].data = new uint8_t[changeForms[i].length1];

			udata.read(reinterpret_cast<char *>(changeForms[i].data), changeForms[i].length1);
		} else {
			//not implemented yet (haven't even seen those)
			char * compressed_data = new char[changeForms[i].length1];

			changeForms[i].data = new uint8_t[changeForms[i].length2];

			udata.read(reinterpret_cast<char *>(compressed_data), changeForms[i].length1);

			delete[] compressed_data;
		}
		// e << changeForms[i].length1<<'|'<<changeForms[i].length2 <<endl;

	}
//End of change forms section


//FormID array and worldSpace array

	udata.seekg(FileLocationTable.formIDArrayCountOffset - delta);

	uint32_t formIDArrayCount;
	unpackedRead(formIDArrayCount);

	vector<formID> formIDArray(formIDArrayCount);

	for (uint32_t i = 0; i < formIDArrayCount; ++i) {
		unpackedRead(formIDArray[i]);
	}

	uint32_t visitedWorldspaceArrayCount;
	unpackedRead(visitedWorldspaceArrayCount);

	vector<formID> visitedWorldspaceArray(visitedWorldspaceArrayCount);

	for (uint32_t i = 0; i < visitedWorldspaceArrayCount; ++i)
	{
		unpackedRead(visitedWorldspaceArray[i]);
	}
//End of FormID array


//UnknownTable3
	Unknown3Table unknown3Table;

	unpackedRead(unknown3Table.count);
	unknown3Table.unknown = new TES::wstring[unknown3Table.count];

//memory leak, not interested enough to fix this as contains no useful data rn
	// for (uint32_t i = 0; i < unknown3Table.count; ++i)
	// {		
	// 	unpackedRead(unknown3Table.unknown[i].prefix);
	// 	unknown3Table.unknown[i].data = new char[unknown3Table.unknown[i].prefix + 1];
	// 	unknown3Table.unknown[i].data[unknown3Table.unknown[i].prefix] = '\0';
	// 	udata.read(unknown3Table.unknown[i].data, unknown3Table.unknown[i].prefix);
	// }

//End of unknown table


	udata.clear();



//MANUAL MEMORY FREEING

	for (int i = 0; i < unknown3Table.count; ++i)
	{
		delete[] unknown3Table.unknown[i].data;
	}
	delete[] unknown3Table.unknown;


	for (int i = 0; i < FileLocationTable.changeFormCount; i++) {
		delete[] changeForms[i].data;
	}


	for (int i = 0; i < MiscStats.count; ++i) {
		delete[] MiscStats.stats[i].name.data;
	}
	delete[] MiscStats.stats;


	for (int i = 0; i < FileLocationTable.globalDataTable1Count; ++i)
		delete[] globalDataTable1[i].data;

	for (int i = 0; i < FileLocationTable.globalDataTable2Count; ++i)
		delete[] globalDataTable2[i].data;

	for (int i = 0; i < FileLocationTable.globalDataTable3Count; ++i)
		delete[] globalDataTable3[i].data;


	for (uint16_t i = 0; i < LightPluginInfo.pluginCount; ++i)
		delete[] LightPluginInfo.plugins[i].data;
	delete[] LightPluginInfo.plugins;

	for (uint8_t i = 0; i < PluginInfo.pluginCount; ++i)
		delete[] PluginInfo.plugins[i].data;
	delete[] PluginInfo.plugins;

	delete[] Header.playerName.data;
	delete[] Header.playerLocation.data;
	delete[] Header.gameDate.data;
	delete[] Header.playerRaceEditorId.data;

	return 0;
}
