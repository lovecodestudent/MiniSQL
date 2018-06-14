#include "bptree.h"

BTree::BTree(char *_idx_name)
	:idx_name(_idx_name)
{
	auto &buffer = GetGlobalFileBuffer();
	auto pMemFile = buffer[_idx_name];

	// 如果索引文件不存在则创建
	if (!pMemFile)
	{
		// 创建索引文件
		buffer.CreateFile(_idx_name);
		pMemFile = buffer[_idx_name];

		// 创建一个结点
		BTNode leaf_node;
		leaf_node.node_type = NodeType::LEAF;
		leaf_node.count_valid_key = 0;
		FileAddr leaf_node_fd= buffer[idx_name]->AddRecord(&leaf_node, sizeof(leaf_node));

		// 将结点的地址写入文件头的预留空间区
		memcpy(buffer[idx_name]->GetFileFirstPage()->GetFileCond()->reserve, &leaf_node_fd, sizeof(leaf_node_fd));
	}
	file_id = pMemFile->fileId;
}

void BTree::InsertNotFull(FileAddr x, KeyAttr k, FileAddr k_fd)
{
	auto px = FileAddrToMemPtr(x);
	int i = px->count_valid_key-1;
	if (px->node_type == NodeType::LEAF)
	{
		while (i >= 0 && k < px->key[i])
		{
			px->key[i + 1] = px->key[i];
			px->children[i + 1] = px->children[i];
			i--;
		}
		px->key[i + 1] = k;
		px->children[i + 1] = k_fd;
		px->count_valid_key += 1;
	}
	else
	{
		while (i >= 0 && k < px->key[i])
			i -= 1;
		// 如果插入的值比内节点的值还小
		if (i < 0)
		{
			i = 0;
			px->key[i] = k;
		}
		assert(i >= 0);
		FileAddr ci = px->children[i];
		auto pci = FileAddrToMemPtr(ci);
		if (pci->count_valid_key == MaxKeyCount)
		{
			SplitChild(x, i, ci);
			if (k > px->key[i + 1])
				i += 1;
		}
		InsertNotFull(px->children[i], k, k_fd);
	}
}

void BTree::SplitChild(FileAddr x, int i, FileAddr y)
{
	auto pMemPageX = GetGlobalClock()->GetMemAddr(file_id, x.filePageID);
	auto pMemPageY = GetGlobalClock()->GetMemAddr(file_id, y.filePageID);
	pMemPageX->isModified = true;
	pMemPageY->isModified = true;

	BTNode*px = FileAddrToMemPtr(x);
	BTNode*py = FileAddrToMemPtr(y);
	BTNode z;
	FileAddr z_fd;
	
	z.node_type = py->node_type;
	z.count_valid_key = MaxKeyCount / 2;
	for (int i = MaxKeyCount / 2; i < MaxKeyCount; i++)
	{
		z.key[i - MaxKeyCount / 2] = py->key[i];
		z.children[i - MaxKeyCount / 2] = py->children[i];
	}
	py->count_valid_key = MaxKeyCount / 2;

	int j;
	for ( j= px->count_valid_key-1; j> i; j--)
	{
		px->key[j+1] = px->key[j];
		px->children[j+1] = px->children[j];
	}
	
	j++;// j should be i+1;
	px->key[j] = z.key[0];
	if (py->node_type == NodeType::LEAF)
	{
		z.next = py->next;
		z_fd = GetGlobalFileBuffer()[idx_name]->AddRecord(&z, sizeof(z));
		py->next = z_fd;
	}
	else
		z_fd = GetGlobalFileBuffer()[idx_name]->AddRecord(&z, sizeof(z));
	
	px->children[j] = z_fd;
	px->count_valid_key++;
	//FileAddr fd_z = GetGlobalFileBuffer()[idx_name]->AddRecord()
}

FileAddr BTree::Search(KeyAttr search_key)
{
	auto pMemPage = GetGlobalClock()->GetMemAddr(file_id, 0);
	auto pfilefd = (FileAddr*)pMemPage->GetFileCond()->reserve;  // 找到根结点的地址
	return Search(search_key, *pfilefd);
}

FileAddr BTree::Search(KeyAttr search_key, FileAddr node_fd)
{
	BTNode* pNode = FileAddrToMemPtr(node_fd);

	if (pNode->node_type == NodeType::LEAF)
	{
		return SearchLeafNode(search_key, node_fd);
	}
	else
	{
		return SearchInnerNode(search_key, node_fd);
	}
}

void BTree::Insert(KeyAttr k, FileAddr k_fd)
{
	// 如果该关键字已经存在则插入失败
	//try
	//{
	//	auto key_fd = Search(k);
	//	if (key_fd != FileAddr{ 0,0 })
	//		throw ERROR::KEY_INSERT_FAILED;
	//}
	//catch (const ERROR error)
	//{
	//	DispatchError(error);
	//	std::cout << std::endl;
	//	return;
	//}

	// 得到根结点的fd
	FileAddr root_fd = *(FileAddr*)GetGlobalFileBuffer()[idx_name]->GetFileFirstPage()->GetFileCond()->reserve;
	auto proot = FileAddrToMemPtr(root_fd);
	if (proot->count_valid_key == MaxKeyCount)
	{
		BTNode s;
		s.node_type = NodeType::ROOT;
		s.count_valid_key = 1;
		s.key[0] = proot->key[0];
		s.children[0] = root_fd;
		FileAddr s_fd = GetGlobalFileBuffer()[idx_name]->AddRecord(&s, sizeof(BTNode));
		*(FileAddr*)GetGlobalFileBuffer()[idx_name]->GetFileFirstPage()->GetFileCond()->reserve = s_fd;
		GetGlobalFileBuffer()[idx_name]->GetFileFirstPage()->isModified = true;
		SplitChild(s_fd, 0, s.children[0]);
		InsertNotFull(s_fd, k, k_fd);
	}
	else
	{
		InsertNotFull(root_fd, k, k_fd);
	}
}

void BTree::PrintBTree()
{
	std::queue<FileAddr> fds;
	// 得到根结点的fd
	FileAddr root_fd = *(FileAddr*)GetGlobalFileBuffer()[idx_name]->GetFileFirstPage()->GetFileCond()->reserve;
	fds.push(root_fd);
	while (!fds.empty())
	{
		// 打印该结点的所有的关键字
		FileAddr tmp = fds.front();
		fds.pop();
		auto pNode = FileAddrToMemPtr(tmp);
		for (int i = 0; i < pNode->count_valid_key; i++)
		{
			std::cout << pNode->key[i] << std::endl;
			if(pNode->node_type!=NodeType::LEAF)
				fds.push(pNode->children[i]);
		}
	}
}

FileAddr BTree::SearchInnerNode(KeyAttr search_key, FileAddr node_fd)
{
	FileAddr fd_res;
	BTNode* pNode = FileAddrToMemPtr(node_fd);
	for (int i = MaxKeyCount - 1; i >= 0; i--)
	{
		if (pNode->key[i] <= search_key)
		{
			fd_res = pNode->children[i];
			break;
		}
	}
	if (fd_res == FileAddr{ 0,0 })
	{
		return fd_res;
	}	
	else
	{
		BTNode* pNextNode = FileAddrToMemPtr(fd_res);
		if (pNextNode->node_type == NodeType::LEAF)
			return SearchLeafNode(search_key, fd_res);
		else
			SearchInnerNode(search_key, fd_res);
	}
}

FileAddr BTree::SearchLeafNode(KeyAttr search_key, FileAddr node_fd)
{

	BTNode* pNode = FileAddrToMemPtr(node_fd);
	for (int i = 0; i <pNode->count_valid_key; i++)
	{
		if (pNode->key[i] == search_key)
		{
			return pNode->children[i];
		}
	}
	return FileAddr{ 0,0 };
}

BTNode * BTree::FileAddrToMemPtr(FileAddr node_fd)
{
	auto pMemPage = GetGlobalClock()->GetMemAddr(file_id, node_fd.filePageID);
	pMemPage->isModified = true;
	return (BTNode*)((char*)pMemPage->Ptr2PageBegin + node_fd.offSet+sizeof(FileAddr));
}

std::ostream& operator<<(std::ostream &os, const KeyAttr &key)
{

	os << key.x << " " << key.s;
	return os;

}


void BTreeTest()
{
	using std::string;
	using std::vector;
	auto &buffer = GetGlobalFileBuffer();
	string idx_name = "test.idx";
	string dbf_name = "test.dbf";

	// 删除已有的文件
	remove(idx_name.c_str());
	remove(dbf_name.c_str());

	// 创建文件
	//buffer.CreateFile(idx_name.c_str());  //索引文件需要用索引树创建才能初始化b+树的根结点
	buffer.CreateFile(dbf_name.c_str());

	// 创建索引树
	BTree tree(const_cast<char*>(idx_name.c_str()));

	// 生成随机关键字
	srand(time(0));
	const int key_count = 8000;
	vector<KeyAttr> keys;
	vector<FileAddr> rec_fds;
	for (int i = 0; i < key_count; i++)
	{
		if (i == 14)
			int a = 1;
		KeyAttr key;
		key.x = rand();
		key.s[0] = rand()%32+32;
		key.s[1] = '\0';
		keys.push_back(key);
		char kk[60];
		FileAddr fd = buffer[dbf_name.c_str()]->AddRecord(kk, sizeof(kk));
		rec_fds.push_back(fd);

		tree.Insert(key, fd);
	}

	tree.PrintBTree();
}
