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

struct RefID//needs improvement
{
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
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
	TES::wstring *plugins = NULL;
} PluginInfo;


struct LightPluginInfo {
	uint16_t pluginCount;
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

template <unsigned int N>
ostream& operator<<(ostream &out, const uint32_t (&arr)[N]) {
	for (unsigned long i = 0; i < N; ++i)
	{
		out << arr[i] << ' ';
	}
	return out;
}

ostream& operator<<(ostream &out, const struct FileLocationTable &_file_table) {
	out << "formIDArrayCountOffset: " 	<< _file_table.formIDArrayCountOffset 	<< endl \
	    << "unknownTable3Offset: " 		<< _file_table.unknownTable3Offset 		<< endl \
	    << "globalDataTable1Offset: " 	<< _file_table.globalDataTable1Offset 	<< endl \
	    << "globalDataTable2Offset: " 	<< _file_table.globalDataTable2Offset 	<< endl \
	    << "globalDataTable3Offset: " 	<< _file_table.globalDataTable3Offset 	<< endl \
	    << "changeFormsOffset: " 		<< _file_table.changeFormsOffset 		<< endl \
	    << "globalDataTable1Count: " 	<< _file_table.globalDataTable1Count 	<< endl \
	    << "globalDataTable2Count: " 	<< _file_table.globalDataTable2Count 	<< endl \
	    << "globalDataTable3Count: " 	<< _file_table.globalDataTable3Count 	<< endl \
	    << "changeFormCount: " 			<< _file_table.changeFormCount 			<< endl \
	    << "unused: " 					<< _file_table.unused;

	return out;
}


struct MiscStat{
	TES::wstring name;
	uint8_t category;
	int32_t value;
};


struct MiscStats{
	uint32_t count;
	MiscStat *stats = NULL;
}MiscStats;


struct GlobalData {
	uint32_t type;
	uint32_t length;
	uint8_t *data = NULL;//note: originally it's uint8, but for purpose of having output stream it's changes to char
};

struct ChangeForm {
	TES::RefID formID;
	uint32_t changeFlags;
	uint8_t type;
	uint8_t version;
	uint32_t length1;//variable size
	uint32_t length2;//variable size
	uint8_t *data = NULL;
};

template<typename T, typename U, typename V>
int universalRead(T &stream, U &dst, V amount) {

	stream.read(reinterpret_cast<char*>(&dst), amount);

	return 0;
}

template<typename T, typename U, typename V>
int universalBulkRead(T &stream, U * &dst, V &prefix, int key = 0) { //optional parameter is currently only determines whether data will be null-terminated
	if((key & 2) >> 1 == 0){
		universalRead(stream, prefix, sizeof(V));	
	}
	
	dst = new U[static_cast<int>(prefix) + 1];//allocating memory
	universalRead(stream, *dst, prefix);

	if (key & 1 == 0)
		dst[static_cast<int>(prefix)] = '\0';

	return 0;
}


fstream file;

template<typename T>
int fileRead(T &dst) {
	return universalRead(file, dst, sizeof(T));
}

template<typename U, typename V>
int fileBulkRead(U &dst, V prefix) {
	return universalBulkRead(file, dst, prefix);
}


istringstream udata;

template<typename T>
int unpackedRead(T &dst) {
	return universalRead(udata, dst, sizeof(T));
}

template<typename U, typename V>
int unpackedBulkRead(U &dst, V &prefix) {
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

	char * magic = new char[13];
	file.read(magic, 13);

	if (strncmp(magic, "TESV_SAVEGAME", 13) != 0) {
		delete[] magic;
		cout << "Please provide correct Skyrim savefile!" << endl;
		return -1;
	}


//	Header processing below, includes basic info like name, race, level, location of a player in that save,
//	also compression type for part of the data and screenshot dimensions.

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
	LZ4_decompress_safe(compressedInput, decompressedOutput, compressedLen, uncompressedLen);
	delete[] compressedInput;


	ofstream out("./REST");//should be empty, because there should be no footer after compressed chunk of savefile
	out << file.rdbuf();
	out.close();
	file.close();

	udata.str(string(decompressedOutput, uncompressedLen));
	delete[] decompressedOutput;
//	End of decompression section


//	Plugin section
//	Contains plugin info divided in two sections - plain PluginInfo and LightPluginInfo (mostly .esl or .esl'ified files)
	uint8_t formVersion;
	//udata.read(reinterpret_cast<char*>(&formVersion), sizeof(uint8_t));
	unpackedRead(formVersion);
	cout << "Form version: " << dec << formVersion - '\0' << endl;

	uint32_t pluginInfoSize;
	unpackedRead(pluginInfoSize);
	cout << "PluginInfoSize: " << pluginInfoSize << endl;

	unpackedRead(PluginInfo.pluginCount);
	cout << "Plugins: " << dec << PluginInfo.pluginCount - '\0' << endl;

	PluginInfo.plugins = new TES::wstring[PluginInfo.pluginCount];


	for (uint32_t i = 0; i < PluginInfo.pluginCount; i++) {
		unpackedBulkRead(PluginInfo.plugins[i].data, PluginInfo.plugins[i].prefix);
		//cout << PluginInfo.plugins[i].data << endl;
	}

	//same with light plugins
	unpackedRead(LightPluginInfo.pluginCount);
	cout << "Light plugins: " << dec << LightPluginInfo.pluginCount - '\0' << endl;

	LightPluginInfo.plugins = new TES::wstring[LightPluginInfo.pluginCount];

	for (uint32_t i = 0; i < LightPluginInfo.pluginCount; i++)	{
		unpackedBulkRead(LightPluginInfo.plugins[i].data, LightPluginInfo.plugins[i].prefix);
		//cout << LightPluginInfo.plugins[i].data << endl;
	}
//	End of Plugins and Light Plugins section


//	File Location Table
// 	Contains various tables offsets and counts

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


// Global Data Table 1, Global Data Table 2
// Too bored rn to analyze data content over there

	vector<struct GlobalData> globalDataTable1(FileLocationTable.globalDataTable1Count);
	vector<struct GlobalData> globalDataTable2(FileLocationTable.globalDataTable2Count);

	for (uint32_t i = 0; i < FileLocationTable.globalDataTable1Count; ++i)	{
		unpackedRead(globalDataTable1[i].type);
		unpackedBulkRead(globalDataTable1[i].data, globalDataTable1[i].length);
	}

	for (uint32_t i = 0; i < FileLocationTable.globalDataTable2Count; ++i)	{
		unpackedRead(globalDataTable2[i].type);
		unpackedBulkRead(globalDataTable2[i].data, globalDataTable2[i].length);
	}

	//Here we parse one of the elements of Global Data
	//Type 0 is MiscData
	assert(globalDataTable1[0].type == 0);	
	
	istringstream globalData;
	globalData.str(string(reinterpret_cast<char*>(globalDataTable1[0].data), globalDataTable1[0].length));

	universalRead(globalData, MiscStats.count, sizeof(uint32_t));
	MiscStats.stats = new MiscStat[MiscStats.count];

	for (uint32_t i = 0; i < MiscStats.count; ++i)
	{
		universalBulkRead(globalData, MiscStats.stats[i].name.data, MiscStats.stats[i].name.prefix);
		universalRead(globalData, MiscStats.stats[i].category, sizeof(uint8_t));
		universalRead(globalData, MiscStats.stats[i].value, sizeof(int32_t));
		
		//cout<<MiscStats.stats[i].name.data<<'|';
	}
	

	//Type 1 is PlayerLocation
	assert(globalDataTable1[1].type == 1);
//End of Global Data Table data

//Change Forms section

	vector<struct ChangeForm> changeForms(FileLocationTable.changeFormCount);
	
	for(uint32_t i = 0; i < FileLocationTable.changeFormCount; i++){
		unpackedRead(changeForms[i].formID);
		unpackedRead(changeForms[i].changeFlags);
		unpackedRead(changeForms[i].type);
		unpackedRead(changeForms[i].version);

		uint8_t data_form_type = changeForms[i].type & 63; //six lower bits representung type of the form
		uint8_t data_form_length = (changeForms[i].type - data_form_type) >> 6;

		cout<<pow(2, data_form_length)<<endl;

		universalRead(udata, changeForms[i].length1, pow(2, data_form_length));
		universalRead(udata, changeForms[i].length2, pow(2, data_form_length));
		
		changeForms[i].data = new uint8_t[changeForms[i].length1];		
		universalRead(udata, changeForms[i].data, changeForms[i].length1 * sizeof(uint8_t));
	}
	cout<<"SIZE:"<<sizeof(changeForms[0].formID)<<endl;
//End of Change Forms Section

//	freeing memory here, probably should just change pointers to smart pointers

	for(uint32_t i = 0; i < FileLocationTable.changeFormCount; i++){
		delete[] changeForms[i].data;
	}


	for (uint32_t i = 0; i < FileLocationTable.globalDataTable1Count; ++i)
		delete[] globalDataTable1[i].data;

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

	delete magic;

	return 0;
}