#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


//include what you want to serialize(string.vector etc.see more in include flode)
#include <boost/serialization/string.hpp>


#include <bits/stdc++.h>
using namespace std;

class classA{
private:
	string s;
public:
	classA(string x):s(x){}
	void print(){
		cout<<s<<endl;
	}

	//overload serialize
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version) {
		ar & s;
	}
};

void serializeToFile(){
	string s;
	fstream fout,fin;

	cin>>s;
	classA* tmpa=new classA(s);
	
	tmpa->print();

	//serializa
	fout.open("serialization file",ios::out|ios::binary);
	boost::archive::text_oarchive out(fout);
	out<<*tmpa;
	fout.close();

	//deserialize
	fin.open("serialization file",ios::in|ios::binary);
	boost::archive::text_iarchive in(fin);
	classA *tmpb=new classA("init");
	tmpb->print();
	in>>*tmpb;
	tmpb->print();

}

void serializeToString(){
	stringstream sin,sout;
	string s,ts;
	cin>>s;
	classA* tmpa=new classA(s);

	tmpa->print();

	//serialize
	boost::archive::text_oarchive out(sout);
	out<<*tmpa;
	out<<*tmpa;
	//sout.str("");
	//out<<*tmpa;
	ts=sout.str();
	cout<<ts<<endl<<"----\n";
	
	//deserialize
	sin.str(ts);
	boost::archive::text_iarchive in(sin);
	classA* tmpb=new classA("1232");
	in>>*tmpb;
	tmpb->print();

}

int main(){
	serializeToString();
	return 0;

}
