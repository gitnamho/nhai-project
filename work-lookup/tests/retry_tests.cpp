#include "../src/retry.hpp"
#include <iostream>
#include <stdexcept>
int main(){using clipboard_retry::Result;int checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("retry test failed");++checks;};
 try{int calls=0,sleeps=0;unsigned elapsed=0;
 auto r=clipboard_retry::acquire([&]{return ++calls==4;},[]{return true;},[&](unsigned ms){++sleeps;elapsed+=ms;},41,30);check(r==Result::Acquired);check(calls==4&&sleeps==3&&elapsed==90);
 calls=0;sleeps=0;r=clipboard_retry::acquire([&]{++calls;return true;},[]{return true;},[&](unsigned){++sleeps;},41,30);check(r==Result::Acquired&&calls==1&&sleeps==0);
 calls=0;sleeps=0;elapsed=0;r=clipboard_retry::acquire([&]{++calls;return false;},[]{return true;},[&](unsigned ms){++sleeps;elapsed+=ms;},41,30);check(r==Result::Busy);check(calls==41&&sleeps==40&&elapsed==1200);
 calls=0;sleeps=0;r=clipboard_retry::acquire([&]{++calls;return false;},[]{return false;},[&](unsigned){++sleeps;},41,30);check(r==Result::Canceled&&calls==0&&sleeps==0);
 calls=0;sleeps=0;bool targetSame=true;r=clipboard_retry::acquire([&]{++calls;return false;},[&]{return targetSame;},[&](unsigned){++sleeps;targetSame=false;},41,30);check(r==Result::Canceled&&calls==1&&sleeps==1);
 calls=0;elapsed=0;sleeps=0;r=clipboard_retry::acquire([&]{++calls;return false;},[]{return true;},[&](unsigned ms){elapsed+=ms;++sleeps;},101,30);check(r==Result::Busy&&calls==101&&sleeps==100&&elapsed==3000);
 calls=0;bool sequenceSame=true;r=clipboard_retry::acquire([&]{++calls;return false;},[&]{return sequenceSame;},[&](unsigned){sequenceSame=false;},101,30);check(r==Result::Canceled&&calls==1);
 calls=0;bool ownerSame=true;r=clipboard_retry::acquire([&]{++calls;return false;},[&]{return ownerSame;},[&](unsigned){ownerSame=false;},101,30);check(r==Result::Canceled&&calls==1);
 std::cout<<"PASS: "<<checks<<" clipboard acquisition/retry assertions (injected functions, not Windows API)\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
