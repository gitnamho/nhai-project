#pragma once
namespace clipboard_retry {
enum class Result { Acquired, Busy, Canceled };
// Only acquisition is retried. Never rerun a clipboard write/restore after a
// partial failure; transferred handles and a newer writer must stay untouched.
template<class TryOpen,class StillValid,class Pause>
Result acquire(TryOpen open,StillValid valid,Pause pause,unsigned attempts,unsigned delayMs){
 for(unsigned i=0;i<attempts;++i){
  if(!valid())return Result::Canceled;
  if(open())return Result::Acquired;
  if(i+1<attempts)pause(delayMs);
 }
 return Result::Busy;
}
}
