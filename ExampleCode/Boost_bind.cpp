#include <bits/stdc++.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

class classA{
public:
	void memberFun(std::string msg){
		std::cout<<msg<<std::endl;
	}
	virtual void virtualFun(std::string msg)=0;
};

class classB: public classA{
	void virtualFun(std::string msg){
		std::cout<<msg<<std::endl;
	}
};

void normalFun(std::string str){
	std::cout<<str<<std::endl;
}


int main(){

	//Polymorphism test
	classA *p1=new classB();
	boost::bind(&classA::virtualFun,p1,"Polymorphism test")();

	//Member function test
	classB p2;
	classB* p3=&p2;
	classB& p4=p2;
	boost::bind(&classB::memberFun,p2,"Member function test using instance")();
	boost::bind(&classB::memberFun,p3,"Member function test using pointer")();
	boost::bind(&classB::memberFun,p4,"Member function test using reference")();
	boost::bind(&classB::memberFun,_1,_2)(p4,"Member function test using parameter");

	//normal function test
	boost::bind(normalFun,"normal function test")();

	/* function API
	 * ambiguous with functional in c++/<version>/function.hpp:23
	 * so do not use namspace std
	*/
	boost::function<void(std::string)>fun=boost::bind(normalFun,_1);
	fun("normal function test with function API");
}