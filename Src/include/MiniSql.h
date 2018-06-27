#ifndef __MiniSql_H__
#define __MiniSql_H__

#include "../INTERPRETER/interpreter.h"
#include "../GLOBAL/global.h"
#include "../BPLUSTREE/bptree.h"
#include "../BUFFER/Buffer.h"
#include "../RECORD/Record.h"

void CreateTable(TB_Create_Info tb_create_info, std::string path = std::string("./"));
void InsertRecord(TB_Insert_Info tb_insert_info, std::string path = std::string("./"));

// 打印整张表数据
std::vector<RecordHead> ShowTable(std::string table_name, std::string path = std::string("./"));

// 取出指定地址的数据
RecordHead GetDbfRecord(std::string table_name, FileAddr fd, std::string path = std::string("./"));
#endif //__MiniSql_H__
