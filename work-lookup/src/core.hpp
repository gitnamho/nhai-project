#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <cctype>
namespace catalog {
using Row=std::vector<std::string>;
inline void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
inline std::string trim(std::string s){auto space=[](unsigned char c){return c==' '||c=='\r'||c=='\n'||c=='\t';};while(!s.empty()&&space(s.back()))s.pop_back();size_t i=0;while(i<s.size()&&space(s[i]))++i;return s.substr(i);}
inline bool controls(const std::string& s){for(unsigned char c:s)if(c<32||c==127)return true;return false;}
inline std::vector<Row> parseRows(std::string text){
 require(text.size()<=20*1024*1024,"CSV는 최대 20MB입니다.");
 if(text.compare(0,3,"\xef\xbb\xbf")==0)text.erase(0,3);
 std::vector<Row> rows;Row row;std::string field;bool quoted=false,after=false;
 auto finishField=[&](){row.push_back(field);field.clear();after=false;require(row.size()<=32,"CSV는 최대 32열입니다.");};
 auto finishRow=[&](){finishField();bool empty=true;for(auto& v:row)if(!v.empty())empty=false;if(!empty)rows.push_back(row);row.clear();require(rows.size()<=50001,"CSV는 최대 50,000개 항목입니다.");};
 for(size_t i=0;i<text.size();++i){char c=text[i];require(c!=0,"CSV에 NUL 문자가 있습니다.");
  if(quoted){if(c=='"'){if(i+1<text.size()&&text[i+1]=='"'){field+='"';++i;}else{quoted=false;after=true;}}else field+=c;}
  else if(c==',')finishField();else if(c=='\r'||c=='\n'){finishRow();if(c=='\r'&&i+1<text.size()&&text[i+1]=='\n')++i;}
  else if(c=='"'){require(field.empty()&&!after,"CSV 따옴표 형식이 잘못되었습니다.");quoted=true;}
  else{require(!after,"닫는 따옴표 뒤에 잘못된 문자가 있습니다.");field+=c;}
  require(field.size()<=32000,"CSV 필드가 너무 깁니다.");
 }
 require(!quoted,"CSV 따옴표가 닫히지 않았습니다.");if(!row.empty()||!field.empty()||after)finishRow();return rows;
}
inline std::string encodeRows(const std::vector<Row>& rows,bool bom=true){std::string out=bom?"\xef\xbb\xbf":"";for(auto& row:rows){bool first=true;for(auto& field:row){if(!first)out+=',';first=false;bool quote=field.find_first_of(",\"\r\n")!=std::string::npos||trim(field)!=field;if(quote)out+='"';for(char c:field){out+=c;if(quote&&c=='"')out+='"';}if(quote)out+='"';}out+="\r\n";}return out;}
struct Database{
 Row columns={"분류","조회키","표시명","상세정보"};std::vector<Row> rows;
 size_t column(const std::string& name)const{auto i=std::find(columns.begin(),columns.end(),name);require(i!=columns.end(),"필수 열이 없습니다.");return i-columns.begin();}
 std::string get(size_t r,const std::string& name)const{return rows.at(r).at(column(name));}
 void validate()const{
  require(columns==Row({"분류","조회키","표시명","상세정보"}),"칼럼은 분류, 조회키, 표시명, 상세정보 네 개로 고정됩니다.");require(rows.size()<=50000,"항목은 최대 50,000개입니다.");std::set<std::string> headers,unique;
  for(auto& c:columns)require(!c.empty()&&c==trim(c)&&c.size()<=180&&!controls(c)&&headers.insert(c).second,"열 이름이 비어 있거나 중복/형식 오류가 있습니다.");
  for(auto name:{"분류","조회키","표시명","상세정보"})column(name);
  size_t cat=column("분류"),key=column("조회키"),name=column("표시명");
  for(auto& row:rows){require(row.size()==columns.size(),"CSV의 행과 열 개수가 다릅니다.");require(!row[cat].empty()&&row[cat]==trim(row[cat])&&row[cat].size()<=240&&!controls(row[cat]),"분류는 앞뒤 공백 없는 텍스트여야 합니다.");require(!row[key].empty()&&row[key]==trim(row[key])&&row[key].size()<=600&&!controls(row[key]),"조회키는 앞뒤 공백 없는 텍스트여야 합니다.");require(!trim(row[name]).empty()&&row[name].size()<=3000,"표시명을 입력하세요.");require(unique.insert(row[cat]+std::string(1,'\0')+row[key]).second,"같은 분류 안의 조회키가 중복됩니다.");for(auto& v:row)require(v.size()<=24000&&v.find('\0')==std::string::npos,"필드가 너무 길거나 잘못된 문자가 있습니다.");}
 }
 static Database parse(const std::string& text){auto all=parseRows(text);require(!all.empty(),"CSV 헤더가 없습니다.");Row headers=all.front();std::set<std::string> unique;for(auto& c:headers){c=trim(c);require(!c.empty()&&unique.insert(c).second,"CSV 열 이름이 비어 있거나 중복됩니다.");}Database d;std::vector<size_t> selected;for(auto& name:d.columns){auto at=std::find(headers.begin(),headers.end(),name);require(at!=headers.end(),"필수 열이 없습니다.");selected.push_back(at-headers.begin());}for(size_t i=1;i<all.size();++i){require(all[i].size()==headers.size(),"CSV의 행과 열 개수가 다릅니다.");Row row;for(auto c:selected)row.push_back(all[i][c]);row[0]=trim(row[0]);row[1]=trim(row[1]);d.rows.push_back(std::move(row));}d.validate();return d;}
 std::string serialize()const{validate();std::vector<Row> all={columns};all.insert(all.end(),rows.begin(),rows.end());auto text=encodeRows(all);require(text.size()<=20*1024*1024,"CSV는 최대 20MB입니다.");return text;}
 std::vector<size_t> lookup(const std::string& input,bool interfaces=false)const{
  require(input.size()<=40000,"조회 텍스트가 너무 깁니다.");std::set<std::string> candidates;candidates.insert(trim(input));std::string token;
  for(unsigned char c:input){if(c<=32||std::string(",;\"'<>()[ ]{}=/\\!?|+").find(c)!=std::string::npos){if(!token.empty())candidates.insert(token);token.clear();}else token+=char(c);}if(!token.empty())candidates.insert(token);
  std::vector<size_t> found;size_t cat=column("분류"),key=column("조회키");for(size_t i=0;i<rows.size();++i)if((!interfaces||rows[i][cat]=="인터페이스")&&candidates.count(rows[i][key]))found.push_back(i);return found;
 }
 void merge(const Database& incoming){incoming.validate();Database next=*this;for(auto& col:incoming.columns)if(std::find(next.columns.begin(),next.columns.end(),col)==next.columns.end()){next.columns.push_back(col);for(auto& r:next.rows)r.push_back("");}
  std::map<std::pair<std::string,std::string>,size_t> index;for(size_t i=0;i<next.rows.size();++i)index[{next.get(i,"분류"),next.get(i,"조회키")}]=i;
  for(size_t i=0;i<incoming.rows.size();++i){auto k=std::make_pair(incoming.get(i,"분류"),incoming.get(i,"조회키"));size_t target;auto at=index.find(k);if(at==index.end()){target=next.rows.size();index[k]=target;next.rows.emplace_back(next.columns.size());}else target=at->second;for(size_t c=0;c<incoming.columns.size();++c)next.rows[target][next.column(incoming.columns[c])]=incoming.rows[i][c];}next.validate();*this=next;
 }
 std::vector<std::string> categories()const{std::vector<std::string> result;std::set<std::string> seen;size_t c=column("분류");for(auto& row:rows)if(seen.insert(row[c]).second)result.push_back(row[c]);return result;}
 std::vector<size_t> filtered(const std::string& category,const std::string& needle)const{std::vector<size_t> result;size_t c=column("분류");for(size_t i=0;i<rows.size();++i){auto& row=rows[i];if(!category.empty()&&row[c]!=category)continue;bool found=needle.empty();for(auto& value:row)if(value.find(needle)!=std::string::npos)found=true;if(found)result.push_back(i);}return result;}
 void renameCategory(const std::string& oldName,std::string name){name=trim(name);auto cats=categories();require(!oldName.empty()&&std::find(cats.begin(),cats.end(),oldName)!=cats.end(),"분류 탭을 선택하세요.");require(!name.empty()&&name.size()<=240&&!controls(name),"분류명을 입력하세요. 최대 80자입니다.");require(oldName==name||std::find(cats.begin(),cats.end(),name)==cats.end(),"같은 분류명이 이미 있습니다.");Database next=*this;for(auto& row:next.rows)if(row[0]==oldName)row[0]=name;next.validate();*this=std::move(next);}
 void deleteCategory(const std::string& name){auto cats=categories();require(!name.empty()&&std::find(cats.begin(),cats.end(),name)!=cats.end(),"삭제할 분류 탭을 선택하세요.");rows.erase(std::remove_if(rows.begin(),rows.end(),[&](const Row& row){return row[0]==name;}),rows.end());}
 std::string importSheet(const std::string& text,std::string name){
  name=trim(name);require(!name.empty()&&name.size()<=240&&!controls(name),"분류명을 입력하세요. 최대 80자입니다.");auto cats=categories();require(std::find(cats.begin(),cats.end(),name)==cats.end(),"같은 분류명이 이미 있습니다. 다른 이름을 입력하세요.");
  auto all=parseRows(text);size_t first=1;require(all.size()>first,"CSV에 가져올 항목이 없습니다.");
  Database incoming;for(size_t i=0;i<all.size();++i){auto& r=all[i];require(r.size()==2||r.size()==3,"CSV는 조회키, 표시명, 선택 상세정보 순서의 2~3칼럼이어야 합니다. 분류 칼럼은 넣지 마세요.");if(i<first)continue;incoming.rows.push_back({name,trim(r[0]),r[1],r.size()==3?r[2]:""});}
  incoming.validate();Database next=*this;next.merge(incoming);next.serialize();*this=std::move(next);return name;
 }

};
struct Shortcut{unsigned modifiers=0,key=0x77;bool interfaces=false;
 void validate()const{require((modifiers&~7u)==0,"보조 키가 잘못되었습니다.");bool function=key>=0x70&&key<=0x7a;require(function||(key>='A'&&key<='Z')||(key>='0'&&key<='9'),"F1~F11, 영문자 또는 숫자를 선택하세요.");require(function||modifiers!=0,"문자/숫자는 보조 키와 조합하세요.");}
 std::string label()const{std::string s;if(modifiers&2)s+="Ctrl+";if(modifiers&1)s+="Alt+";if(modifiers&4)s+="Shift+";if(key>=0x70&&key<=0x7a)s+='F'+std::to_string(key-0x6f);else s+=char(key);return s;}
};
}
