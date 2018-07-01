/****************************************************************************************************************************************

** 文件名: global.h

** Copyright (c)

** 创建人: ReFantasy/TDL

** 日  期: 2018-06-23

** 描  述: 定义全局常量和数据结构

** 版  本: 1.00

****************************************************************************************************************************************/
#ifndef __GLOBAL_H__
#define __GLOBAL_H__
 
#include <vector>
#include <string>
#include <direct.h>
#include <algorithm>

//#define NDEBUG

#include "../Src/ERROR/error.h"
extern "C"
{
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
}

/***********************************************************************************
*
*    定义记录各个字段的类型
*    字段类型 I---int  C---字符串  D---Doouble
*
***********************************************************************************/

enum class Column_Type { I, C, D };

/********************************************************  Buffer Module  ***************************************************************/

constexpr int FILE_PAGESIZE = 8192;	// 内存页(==文件页)大小
constexpr int MEM_PAGEAMOUNT = 4096;	// 内存页数量
constexpr int MAX_FILENAME_LEN = 256;	// 文件名（包含路径）最大长度

/****************************************************************************************************************************************/





/********************************************************  B+tree Module  ***************************************************************/

constexpr int RecordColumnCount = 12 * 4;  // 记录字段数量限制,假设所有字段都是字符数组，一个字符数组字段需要4个字符->CXXX
constexpr int ColumnNameLength = 16;     // 单个字段名称长度限制
constexpr int bptree_t = 3;                         // B+tree's degree, bptree_t >= 2
constexpr int MaxKeyCount = 2 * bptree_t;            // the max number of keys in a b+tree node
constexpr int MaxChildCount = 2 * bptree_t;          // the max number of child in a b+tree node

/***********************************************************************************
*
*    定义索引文件关键字属性
*
***********************************************************************************/
class KeyAttr
{
public:
	using Key_Value = union {
		char                StrValue[ColumnNameLength];	     //字符串指针 
		int   		        IntValue;		 //整形值
		double 		        DoubleValue;     //浮点型值	
	};
	Column_Type type;
	Key_Value value;

	bool operator<(const KeyAttr &rhs)const;
	bool operator>(const KeyAttr &rhs)const;
	bool operator==(const KeyAttr &rhs)const;
	bool operator<=(const KeyAttr &rhs)const;
	bool operator>=(const KeyAttr &rhs)const;
	bool operator!=(const KeyAttr &rhs)const;

};
std::ostream& operator<<(std::ostream &os, const KeyAttr &key);

// 数据类型的字符串形式转换为枚举类型
Column_Type StrConvertToEnumType(std::string str_type);
enum class CmdType
{
	TABLE_CREATE, TABLE_DROP, TABLE_SHOW, TABLE_SELECT, TABLE_INSERT, TABLE_UPDATE, TABLE_DELETE,
	DB_CREATE, DB_DROP, DB_SHOW, DB_USE,
	QUIT

};

/*********************************************************
*             文件地址,定位文件中的位置
**********************************************************/
class FileAddr
{
	friend class FILECOND;
public:
	void SetFileAddr(const unsigned long _filePageID, const unsigned int  _offSet);
	void ShiftOffset(const int OFFSET);

	unsigned long filePageID;     // 文件页编号
	unsigned int  offSet;         // 页内偏移量

	bool operator==(const FileAddr &rhs) const
	{
		return (this->filePageID == rhs.filePageID && this->offSet == rhs.offSet);
	}
	bool operator!=(const FileAddr &rhs) const
	{
		return !(this->filePageID == rhs.filePageID && this->offSet == rhs.offSet);
	}
	bool operator<(const FileAddr &rhs)const
	{
		return (this->filePageID < rhs.filePageID) || ((this->filePageID == rhs.filePageID) && (this->offSet < rhs.offSet));
	}
};





/****************************************************************************************************************************************/


/************************************************************************
*    类名：有意字串类
*    功能：将命令字符串解析为有意字串
*	 有意字串定义：有意字串即指一个以回车或空格或标志符来分割的有独立含义的字符
*                 串,标识符主要有逗号、括号、比较运算符、分号等。标识符也算作
*                 有意字串
************************************************************************/
class SensefulStr
{
public:
	SensefulStr(std::string srcstr = "");
	void SetSrcStr(std::string senstr);
	std::vector<std::string> GetSensefulStr()const;
private:
	void Parse();
	std::string src_str;  // 原始命令字符串
	std::vector<std::string> sen_str; // 解析后的又一字串
};


/************************************************************************
*    表创建信息
************************************************************************/
struct TB_Create_Info
{
	using ColumnInfo = struct ColumnInfo
	{
		std::string name;
		Column_Type type;
		bool isPrimary;       // 是否主键
		int length;           // 数据大小，只用于字符串字段
	};

	std::string table_name;
	std::vector<ColumnInfo> columns_info;  //字段信息向量

};

/************************************************************************
*    表插入信息
************************************************************************/
struct TB_Insert_Info
{
	using InsertInfo = struct {
		std::string column_name;
		std::string column_value;
	};

	std::string table_name;
	std::vector<InsertInfo> insert_info;
};

class CompareCell;
struct TB_Select_Info
{
	std::string table_name;                        // 选择的表名
	std::vector<std::string> name_selected_column; // 选择的字段名字
	std::vector<CompareCell> vec_cmp_cell;         // 选择条件
};

// 目录定位和切换 用于数据库和表的使用
class CatalogPosition
{
	friend bool UseDatabase(std::string db_name, CatalogPosition &cp);
public:
	CatalogPosition();
	bool ResetRootCatalog(std::string root_new);  // 重置根目录

	void SwitchToDatabase();// 转到数据库列表目录下


	bool SwitchToDatabase(std::string db_name);// 转到具体的数据库下

	std::string GetCurrentPath()const;
	std::string GetRootPath()const;
	std::string SetCurrentPath(std::string cur);
	bool GetIsInSpeDb() { return isInSpeDb; }
private:
	static bool isInSpeDb;          //是否在某个具体的数据库目录下
	std::string root; // 根目录，数据库文件的保存位置
	std::string current_catalog;
};
CatalogPosition& GetCp();


// file name convert .idx to .dbf 
std::string IdxToDbf(std::string idx_name);
// file name convert .dbf to .idx 
std::string DbfToIdx(std::string idx_name);

// str to int
int StrToInt(std::string str);

std::string StrToLower(std::string str);

#endif
