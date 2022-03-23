template<typename T>
std::ostream& operator<<(std::ostream &out, const std::vector<T> &vec) {
	for (auto i : vec) {
		out << i << ' ';
	}
	return out;
}


namespace TES {

struct wstring
{
	uint16_t prefix;
	char *data = NULL;
};

struct RefID//needs improvement
{
	uint8_t byte0, byte1, byte2;
};

}


typedef float float32;
typedef uint64_t FILETIME;
typedef uint32_t formID;



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
};


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
};

std::ostream& operator<<(std::ostream &out, const struct FileLocationTable &_file_table) {
	out << "formIDArrayCountOffset: " 	<< _file_table.formIDArrayCountOffset 	<< std::endl \
	    << "unknownTable3Offset: " 		<< _file_table.unknownTable3Offset 		<< std::endl \
	    << "globalDataTable1Offset: " 	<< _file_table.globalDataTable1Offset 	<< std::endl \
	    << "globalDataTable2Offset: " 	<< _file_table.globalDataTable2Offset 	<< std::endl \
	    << "globalDataTable3Offset: " 	<< _file_table.globalDataTable3Offset 	<< std::endl \
	    << "changeFormsOffset: " 		<< _file_table.changeFormsOffset 		<< std::endl \
	    << "globalDataTable1Count: " 	<< _file_table.globalDataTable1Count 	<< std::endl \
	    << "globalDataTable2Count: " 	<< _file_table.globalDataTable2Count 	<< std::endl \
	    << "globalDataTable3Count: " 	<< _file_table.globalDataTable3Count 	<< std::endl \
	    << "changeFormCount: " 			<< _file_table.changeFormCount 			<< std::endl \
	    << "unused: " 					<< _file_table.unused;

	return out;
}


std::ostream& operator<<(std::ostream &out, struct Header & _header) {
	out << _header.playerName.data << ", " << _header.playerRaceEditorId.data << " lvl " << _header.playerLevel << " at " << _header.playerLocation.data;
	return out;
}

struct MiscStat {
	TES::wstring name;
	uint8_t category;
	int32_t value;
};


struct MiscStats {
	uint32_t count;
	MiscStat *stats = NULL;
};


struct GlobalData {
	uint32_t type;
	uint32_t length;
	uint8_t *data = NULL;
};

struct ChangeForm {
	TES::RefID formID;
	uint32_t changeFlags;
	uint8_t type;
	uint8_t version;
	uint32_t length1;//variable size (lengh of data)
	uint32_t length2;//variable size (uncompressed length,  zero - if non-compressed)
	uint8_t *data = NULL;
};


struct Unknown3Table {
	uint32_t count;
	TES::wstring *unknown = NULL;
};

template<typename T, typename U, typename V>
int universalRead(T &stream, U &dst, V amount) {

	stream.read(reinterpret_cast<char*>(&dst), amount);

	return 0;
}

template<typename T, typename U, typename V>
int universalBulkRead(T &stream, U * &dst, V &prefix, int key = 0) { //optional parameter is currently only determines whether data will be null-terminated
	if ((key & 2) >> 1 == 0) {
		universalRead(stream, prefix, sizeof(V));
	}

	dst = new U[static_cast<int>(prefix) + 1];//allocating memory
	universalRead(stream, *dst, prefix);


	if (key == 0) {
		dst[static_cast<int>(prefix)] = '\0';
	}

	return 0;

}
