#include "tsan_logging.h"

using namespace __tsan;
using namespace __sanitizer;

//fd_t logFileFd = kInvalidFd;
static Mutex mtx;

void __tsan::convertHexadecimalToString(unsigned long long valueToConvert, char *bufferString)
{
  const char *hex = "0123456789abcdef";
  for(int i = 15; i >=0; i--)
  {
    bufferString[i] = hex[valueToConvert & 0xF];
    valueToConvert >>= 4;
  }
  bufferString[16] = '\0';
}

void __tsan::convertIntegerToDecimalString(unsigned long long valueToConvert, char *bufferString)
{
  int i=0;
    
  if(valueToConvert == 0)
  {
    bufferString[i++] = '0';   
  }
  else{
    while (valueToConvert > 0)
    {
      bufferString[i++] = '0' + valueToConvert % 10;
      valueToConvert /= 10;
    }

    for (int j = 0; j < i/2; ++j)
    {
      char temp = bufferString[j];
      bufferString[j] = bufferString[i -j -1];
      bufferString[i-j-1] = temp;
    }
  }
  bufferString[i] = '\0';
}

void __tsan::tsanInterceptorsAndMemoryAccessOperationsLogging(const char* logMessage, void *addr, ThreadState *thr, uptr callerpc, Tid tid)
{
  static bool logClear = false;
  int tsanLogFlag;

  Lock l(&mtx);
  
  if (!logClear)
  {
   
    tsanLogFlag = OpenFile("tsanLogFile.txt", WrOnly);
    logClear = true;
  }
  else
  {
    tsanLogFlag = OpenFile("tsanLogFile.txt", Append);
  }

    if(tsanLogFlag != -1)
    {
      if(thr->tid >= 0)
      {
        char threadIdBuf[30];
        convertIntegerToDecimalString((unsigned long long)thr->tid, threadIdBuf);
        WriteToFile(tsanLogFlag, "Thread ", 7);
        WriteToFile(tsanLogFlag, threadIdBuf, internal_strlen(threadIdBuf));        
      }

      WriteToFile(tsanLogFlag, logMessage, internal_strlen(logMessage));

      if(tid > 0)
      {
        WriteToFile(tsanLogFlag, "(Thread ", 8);
        char threadIdBuf1[30];
        convertIntegerToDecimalString((unsigned long long)tid, threadIdBuf1);
        WriteToFile(tsanLogFlag, threadIdBuf1, internal_strlen(threadIdBuf1));

      }
      
      if( addr != nullptr)
      {
        WriteToFile(tsanLogFlag, "|", 1);
        char addrBuf[18] = "0x";
        convertHexadecimalToString((unsigned long long)addr, addrBuf + 2);
        WriteToFile(tsanLogFlag, addrBuf, internal_strlen(addrBuf));
        WriteToFile(tsanLogFlag, "|", 1);
      
      }

      if (callerpc != 0)
      {
          Symbolizer *symbolizer = Symbolizer::GetOrInit();
          if(symbolizer)
          {
            SymbolizedStack *frames = symbolizer->SymbolizePC(callerpc);

            if(frames)
            {
              const AddressInfo &info = frames->info;
              char lineBuf1[20];
              convertIntegerToDecimalString(info.line, lineBuf1);
              WriteToFile(tsanLogFlag, " at line ", 9);
              WriteToFile(tsanLogFlag, lineBuf1, internal_strlen(lineBuf1));
              WriteToFile(tsanLogFlag, " in file ", 9);
              WriteToFile(tsanLogFlag, info.file, internal_strlen(info.file));
            }
            else
            {
              Printf("SymbolizedStack is null\n");
            }         
          } 
      }
  
      WriteToFile(tsanLogFlag, "\n", 1);
      CloseFile(tsanLogFlag);
  }
  else
  {
    Printf("File was not created");
  }
}

