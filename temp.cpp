#include <iostream>
using namespace std;
class CData
{
public:
	CData(){};
	CData(const char* ch) : data(ch)
	{
		std::cout << "CData(const char* ch)" << std::endl;
	}
	CData(const CData& str) : data(str.data) 
	{
		std::cout << "CData(const std::string& str)" << std::endl;
	}
	CData(CData&& str) : data(str.data)
	{
		std::cout << "CData(std::string&& str)" << std::endl;
	}
	~CData()
	{
		std::cout << "~CData()" << std::endl;
	}
public:
	std::string data;
};

CData* Creator(const CData &t,bool test)
{
	cout<<"Enter"<<endl;
	CData a(t);
	if(test){
		return &a;
	}else{
		return nullptr;
	}
}

int main(){
    CData* t1=Creator(CData("123"),true);
	CData* t2=Creator(CData("123"),false);
    return 0;
}

