#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include "core.hpp"
#include "retry.hpp"
#include <memory>
namespace platform {
inline std::wstring wide(const std::string& s,UINT cp=CP_UTF8){if(s.empty())return {};int n=MultiByteToWideChar(cp,cp==CP_UTF8?MB_ERR_INVALID_CHARS:0,s.data(),(int)s.size(),nullptr,0);if(!n)throw std::runtime_error("문자 인코딩을 읽을 수 없습니다. UTF-8 CSV인지 확인하세요.");std::wstring w(n,0);MultiByteToWideChar(cp,cp==CP_UTF8?MB_ERR_INVALID_CHARS:0,s.data(),(int)s.size(),&w[0],n);return w;}
inline std::string utf8(const std::wstring& w){if(w.empty())return {};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);if(!n)throw std::runtime_error("문자열 인코딩이 잘못되었습니다.");std::string s(n,0);WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),(int)w.size(),&s[0],n,nullptr,nullptr);return s;}
inline std::wstring text(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,0);GetWindowTextW(h,&s[0],n+1);s.resize(n);return s;}
struct Handle {HANDLE h=INVALID_HANDLE_VALUE;explicit Handle(HANDLE value):h(value){}~Handle(){if(h!=INVALID_HANDLE_VALUE&&h!=nullptr)CloseHandle(h);}Handle(const Handle&)=delete;};
inline bool exists(const std::wstring& p){DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
inline std::wstring parent(const std::wstring& p){auto n=p.find_last_of(L"\\/");return n==std::wstring::npos?L".":p.substr(0,n);}
inline std::wstring full(const std::wstring& p){DWORD n=GetFullPathNameW(p.c_str(),0,nullptr,nullptr);catalog::require(n>0&&n<32768,"파일 경로가 잘못되었습니다.");std::wstring out(n,0);GetFullPathNameW(p.c_str(),n,&out[0],nullptr);out.resize(wcslen(out.c_str()));return out;}
inline std::string read(const std::wstring& p){Handle f(CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));catalog::require(f.h!=INVALID_HANDLE_VALUE,"파일을 열 수 없습니다. 경로와 권한을 확인하세요.");LARGE_INTEGER sz{};catalog::require(GetFileSizeEx(f.h,&sz)&&sz.QuadPart<=20*1024*1024,"파일은 최대 20MB입니다.");std::string bytes((size_t)sz.QuadPart,0);DWORD got=0;if(!bytes.empty())catalog::require(ReadFile(f.h,&bytes[0],(DWORD)bytes.size(),&got,nullptr)&&got==bytes.size(),"파일 읽기에 실패했습니다. 다시 읽으세요.");return bytes;}
inline std::string asUtf8(std::string bytes,bool cp949=false){try{wide(bytes);return bytes;}catch(...){if(!cp949)throw;return utf8(wide(bytes,949));}}
inline void createDir(const std::wstring& p){catalog::require(CreateDirectoryW(p.c_str(),nullptr)||GetLastError()==ERROR_ALREADY_EXISTS,"폴더를 만들 수 없습니다.");}
inline void writeAtomic(const std::wstring& path,const std::string& bytes,const std::string* expected=nullptr,bool expectedMissing=false,bool backup=false){
 catalog::require(bytes.size()<=20*1024*1024,"파일은 최대 20MB입니다.");
 Handle lock(CreateFileW((path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr));catalog::require(lock.h!=INVALID_HANDLE_VALUE,"다른 작업이 파일을 저장 중입니다. 잠시 후 다시 시도하세요.");
 bool old=exists(path);if(expected)catalog::require(old&&read(path)==*expected,"CSV가 외부에서 변경되었습니다. 편집을 내보내서 보관한 후 다시 읽으세요.");if(expectedMissing)catalog::require(!old,"동일 경로에 파일이 생겼습니다. 다른 경로를 선택하세요.");
 std::wstring temp=path+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount())+L".tmp";
 try{ {Handle f(CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));catalog::require(f.h!=INVALID_HANDLE_VALUE,"임시 파일을 만들 수 없습니다.");DWORD wrote;catalog::require(WriteFile(f.h,bytes.data(),(DWORD)bytes.size(),&wrote,nullptr)&&wrote==bytes.size()&&FlushFileBuffers(f.h),"파일 저장에 실패했습니다.");}
  // Recheck while holding our cooperative lock. Uncooperative editors can still
  // race after this check; ReplaceFile itself remains an atomic replacement.
  if(expected)catalog::require(exists(path)&&read(path)==*expected,"CSV가 외부에서 변경되었습니다. 다시 읽으세요.");
  std::wstring bp;if(old&&backup){std::wstring folder=parent(path)+L"\\backups";createDir(folder);SYSTEMTIME t;GetLocalTime(&t);wchar_t stamp[80];swprintf(stamp,80,L"\\catalog-%04u%02u%02u-%02u%02u%02u-%03u-%lu.csv",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,GetCurrentProcessId());bp=folder+stamp;}
  if(old)catalog::require(ReplaceFileW(path.c_str(),temp.c_str(),bp.empty()?nullptr:bp.c_str(),0,nullptr,nullptr),"파일 교체에 실패했습니다. 기존 파일은 삭제하지 않습니다.");else catalog::require(MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH),"파일을 저장할 수 없습니다.");
 }catch(...){DeleteFileW(temp.c_str());throw;}
}
inline std::wstring choose(HWND owner,bool save,const std::wstring& initial=L""){wchar_t buf[32768]{};if(initial.size()<32768)wcscpy(buf,initial.c_str());OPENFILENAMEW of{};of.lStructSize=sizeof(of);of.hwndOwner=owner;of.lpstrFilter=L"CSV 파일 (*.csv)\0*.csv\0모든 파일\0*.*\0";of.lpstrFile=buf;of.nMaxFile=32768;of.lpstrDefExt=L"csv";of.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileNameW(&of):GetOpenFileNameW(&of))?full(buf):L"";}
inline bool modifiers(){return (GetAsyncKeyState(VK_CONTROL)&0x8000)||(GetAsyncKeyState(VK_MENU)&0x8000)||(GetAsyncKeyState(VK_SHIFT)&0x8000)||(GetAsyncKeyState(VK_LWIN)&0x8000)||(GetAsyncKeyState(VK_RWIN)&0x8000);}
template<class Valid>
inline clipboard_retry::Result openClipboardRetry(HWND owner,unsigned attempts,Valid valid){
 return clipboard_retry::acquire([&]{return OpenClipboard(owner)!=FALSE;},valid,[](unsigned ms){Sleep(ms);},attempts,30);
}
inline bool clipboardUnchanged(DWORD sequence,DWORD pid){DWORD actual=0;GetWindowThreadProcessId(GetClipboardOwner(),&actual);return GetClipboardSequenceNumber()==sequence&&actual==pid;}
struct ClipItem{UINT format;HANDLE value;int kind;};
struct Snapshot{
 std::vector<ClipItem> items;DWORD sequence=0;
 ~Snapshot(){for(auto& i:items)if(i.value){if(i.kind==1)DeleteObject(i.value);else if(i.kind==2)DeleteEnhMetaFile((HENHMETAFILE)i.value);else GlobalFree(i.value);}}
 void take(HWND owner,HWND target=nullptr){auto acquired=openClipboardRetry(owner,41,[&]{return !target||GetForegroundWindow()==target;});catalog::require(acquired!=clipboard_retry::Result::Canceled,"기존 클립보드 확인 중 선택한 창이 바뀌었습니다.");catalog::require(acquired==clipboard_retry::Result::Acquired,"기존 클립보드 확인: 1.2초 동안 다른 프로그램의 점유가 풀리지 않았습니다. 잠시 후 다시 시도하세요.");try{catalog::require(!target||GetForegroundWindow()==target,"선택한 창이 바뀌었습니다.");size_t total=0;UINT format=0;while((format=EnumClipboardFormats(format))!=0){catalog::require(items.size()<64,"클립보드 형식이 너무 많습니다.");wchar_t name[128]{};if(format>=0xc000){GetClipboardFormatNameW(format,name,128);if(wcscmp(name,L"DataObject")==0||wcscmp(name,L"Ole Private Data")==0)continue;}
 HANDLE src=GetClipboardData(format);catalog::require(src!=nullptr,"기존 클립보드 내용을 보존할 수 없습니다.");HANDLE copy=nullptr;int kind=0;
 if(format==CF_BITMAP){BITMAP bm{};catalog::require(GetObjectW(src,sizeof(bm),&bm)>0,"이미지를 보존할 수 없습니다.");size_t sz=(size_t)std::abs(bm.bmHeight)*(size_t)bm.bmWidthBytes;total+=sz;catalog::require(total<=16*1024*1024,"클립보드가 16MB를 넘습니다.");copy=CopyImage(src,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);kind=1;}
 else if(format==CF_ENHMETAFILE){UINT sz=GetEnhMetaFileBits((HENHMETAFILE)src,0,nullptr);total+=sz;catalog::require(total<=16*1024*1024,"클립보드가 16MB를 넘습니다.");copy=CopyEnhMetaFileW((HENHMETAFILE)src,nullptr);kind=2;}
 else{bool known=format==CF_TEXT||format==CF_OEMTEXT||format==CF_UNICODETEXT||format==CF_LOCALE||format==CF_DIB||format==CF_DIBV5||format==CF_HDROP||format>=0xc000;catalog::require(known,"보존할 수 없는 클립보드 형식입니다. 텍스트를 복사한 뒤 다시 시도하세요.");SIZE_T sz=GlobalSize(src);catalog::require(sz>0&&sz<=16*1024*1024-total,"클립보드가 크거나 지원하지 않는 형식입니다.");void* ptr=GlobalLock(src);catalog::require(ptr!=nullptr,"클립보드 내용을 읽을 수 없습니다.");copy=GlobalAlloc(GMEM_MOVEABLE,sz);if(copy){void* dst=GlobalLock(copy);if(dst){memcpy(dst,ptr,sz);GlobalUnlock(copy);}else{GlobalFree(copy);copy=nullptr;}}GlobalUnlock(src);total+=sz;}
 catalog::require(copy!=nullptr,"클립보드 복제에 실패했습니다.");try{items.push_back({format,copy,kind});}catch(...){if(kind==1)DeleteObject(copy);else if(kind==2)DeleteEnhMetaFile((HENHMETAFILE)copy);else GlobalFree(copy);throw;}}
 sequence=GetClipboardSequenceNumber();CloseClipboard();}catch(...){CloseClipboard();throw;}}
 bool restore(HWND owner,DWORD sequence,DWORD pid){auto acquired=openClipboardRetry(owner,101,[&]{return clipboardUnchanged(sequence,pid);});if(acquired==clipboard_retry::Result::Canceled)return false;catalog::require(acquired==clipboard_retry::Result::Acquired,"기존 클립보드 복원: 3초 동안 점유가 풀리지 않아 복원하지 못했습니다. 현재 클립보드에 조회한 텍스트가 남아 있을 수 있습니다.");try{DWORD actual=0;GetWindowThreadProcessId(GetClipboardOwner(),&actual);if(GetClipboardSequenceNumber()!=sequence||actual!=pid){CloseClipboard();return false;}catalog::require(EmptyClipboard(),"클립보드 복원 실패");for(auto& i:items){catalog::require(SetClipboardData(i.format,i.value)!=nullptr,"일부 클립보드 형식을 복원하지 못했습니다.");i.value=nullptr;}CloseClipboard();return true;}catch(...){CloseClipboard();throw;}}
};
inline std::wstring copySelection(HWND target,HWND owner,UINT key,std::wstring& note){
 DWORD pid=0;GetWindowThreadProcessId(target,&pid);catalog::require(pid!=0,"조회할 창이 없습니다.");for(int n=0;modifiers()||(GetAsyncKeyState(key)&0x8000);++n){catalog::require(n<200,"단축키에서 손을 떼고 다시 시도하세요.");catalog::require(GetForegroundWindow()==target,"선택한 창이 바뀌었습니다.");Sleep(15);}
 catalog::require(GetForegroundWindow()==target,"선택한 창이 바뀌었습니다.");Snapshot original;original.take(owner,target);DWORD before=original.sequence;catalog::require(GetClipboardSequenceNumber()==before,"복사 전에 다른 앱의 클립보드 변경을 감지했습니다. 다시 시도하세요.");catalog::require(GetForegroundWindow()==target&&!modifiers(),"선택 창 또는 키 상태가 바뀌었습니다.");
 INPUT input[4]{};for(auto& i:input)i.type=INPUT_KEYBOARD;input[0].ki.wVk=VK_CONTROL;input[1].ki.wVk='C';input[2].ki.wVk='C';input[2].ki.dwFlags=KEYEVENTF_KEYUP;input[3].ki.wVk=VK_CONTROL;input[3].ki.dwFlags=KEYEVENTF_KEYUP;
 UINT sent=SendInput(4,input,sizeof(INPUT));if(sent!=4){INPUT release[2]{};for(auto& i:release){i.type=INPUT_KEYBOARD;i.ki.dwFlags=KEYEVENTF_KEYUP;}release[0].ki.wVk='C';release[1].ki.wVk=VK_CONTROL;SendInput(2,release,sizeof(INPUT));}
 std::wstring result;DWORD copied=0;std::exception_ptr failure;
 try{catalog::require(sent==4,"자동 복사 입력이 제한되었습니다. 권한과 PC 정책을 확인하세요.");bool changed=false;for(int n=0;n<100;++n){Sleep(12);if(GetClipboardSequenceNumber()!=before){changed=true;break;}catalog::require(GetForegroundWindow()==target,"선택한 창이 바뀌었습니다.");}catalog::require(changed,"선택한 텍스트가 복사되지 않았습니다. 글자를 선택했는지 확인하세요.");DWORD actual=0;GetWindowThreadProcessId(GetClipboardOwner(),&actual);catalog::require(actual==pid,"다른 앱의 클립보드 변경을 감지했습니다. 다시 시도하세요.");copied=GetClipboardSequenceNumber();catalog::require(GetForegroundWindow()==target,"선택한 창이 바뀌었습니다.");auto acquired=openClipboardRetry(owner,41,[&]{return GetForegroundWindow()==target&&clipboardUnchanged(copied,pid);});catalog::require(acquired!=clipboard_retry::Result::Canceled,"선택 텍스트 확인 중 창 또는 클립보드가 바뀌었습니다. 다시 시도하세요.");catalog::require(acquired==clipboard_retry::Result::Acquired,"선택 텍스트 읽기: 1.2초 동안 다른 프로그램의 점유가 풀리지 않았습니다. 잠시 후 다시 시도하세요.");try{catalog::require(GetForegroundWindow()==target&&clipboardUnchanged(copied,pid),"선택 텍스트 확인 중 창 또는 클립보드가 바뀌었습니다.");DWORD p=0;GetWindowThreadProcessId(GetClipboardOwner(),&p);catalog::require(p==pid,"클립보드 소유 앱이 바뀌었습니다.");HANDLE h=GetClipboardData(CF_UNICODETEXT);copied=GetClipboardSequenceNumber();catalog::require(h!=nullptr,"선택한 내용이 텍스트가 아닙니다.");SIZE_T bytes=GlobalSize(h);const wchar_t* ptr=(const wchar_t*)GlobalLock(h);catalog::require(ptr!=nullptr,"선택 텍스트를 읽을 수 없습니다.");size_t count=0,limit=std::min<size_t>(bytes/sizeof(wchar_t),10001);while(count<limit&&ptr[count]!=0)++count;bool valid=count<limit&&count<=10000;if(valid)result.assign(ptr,count);GlobalUnlock(h);catalog::require(valid&&!catalog::trim(utf8(result)).empty(),"조회 텍스트가 비어 있거나 10,000자를 넘습니다.");CloseClipboard();}catch(...){CloseClipboard();throw;}}
 catch(...){failure=std::current_exception();}
 // Capture any copied-data owner even on a timeout/input failure; never overwrite
 // an unrelated writer or use old clipboard data as the selection.
 if(!copied&&GetClipboardSequenceNumber()!=before){DWORD p=0;GetWindowThreadProcessId(GetClipboardOwner(),&p);if(p==pid)copied=GetClipboardSequenceNumber();}
 if(copied){bool restored=original.restore(owner,copied,pid);note=restored?L"기존 클립보드 복원 완료":L"다른 앱의 새 클립보드 유지";}
 if(failure)std::rethrow_exception(failure);
 return result;
}
}
