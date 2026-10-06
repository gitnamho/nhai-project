#include "../src/core.hpp"
#include <iostream>
int main(){int n=0;auto check=[&](bool x){catalog::require(x,"test failed");++n;};try{
 auto d=catalog::Database::parse("분류,조회키,표시명,상세정보\n인터페이스,A,원본,설명\n서버,host,서버,상세");auto original=d.serialize();
 check(d.importSheet("IP,호스트\n192.0.2.10,host-01","서버 추가")=="서버 추가");check(d.get(2,"조회키")=="192.0.2.10");check(d.get(2,"표시명")=="host-01");check(d.get(2,"상세정보").empty());check(d.get(0,"표시명")=="원본");
 check(d.importSheet("anything,title,description\nID_A,이름,여러 정보","임의 제목")=="임의 제목");check(d.get(3,"상세정보")=="여러 정보");
 d.importSheet("아무제목,임의제목\nFIRST,첫 항목\nSECOND,둘째 항목,선택 설명","첫 행 제외");check(d.filtered("첫 행 제외","").size()==2);check(d.lookup("FIRST").size()==1);check(d.get(4,"상세정보").empty());
 auto unchanged=[&](auto f){auto before=d.serialize();bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected&&d.serialize()==before);};
 unchanged([&]{d.importSheet("키,명\nX,x","서버");});unchanged([&]{d.importSheet("키,명\nX,x","");});unchanged([&]{d.importSheet("키,명\nX,x\nX,y","중복키");});unchanged([&]{d.importSheet("a,b,c,d\nA,B,C,D","네열");});unchanged([&]{d.importSheet("a,b\n","빈파일");});unchanged([&]{d.importSheet("a\nx","한열");});
 d.renameCategory("서버 추가","이름 변경");check(d.filtered("서버 추가","").empty());check(d.filtered("이름 변경","").size()==1);check(d.get(2,"조회키")=="192.0.2.10");unchanged([&]{d.renameCategory("이름 변경","서버");});unchanged([&]{d.renameCategory("이름 변경","");});unchanged([&]{d.deleteCategory("");});
 d.deleteCategory("첫 행 제외");check(d.lookup("FIRST").empty());check(d.rows.size()==4);check(catalog::Database::parse(d.serialize()).rows==d.rows);check(catalog::Database::parse(original).rows.size()==2);
 auto reordered=catalog::Database::parse("IP,표시명,분류,상세정보,조회키\n192.0.2.1,이름,새 분류,상세,A");check(reordered.rows[0]==catalog::Row({"새 분류","A","이름","상세"}));
 std::cout<<"PASS: "<<n<<" positional import/category assertions\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
