#include <windows.h>
#include <C843_GCS_DLL.H>
#include <assert.h>
#include "common.h"


inline void handle(int boardid, int v, char* expr, char* msg, char *file, int line, pf_reporter report)
{
  char emsg[1024];
  long err = C843_GetError(boardid);
#pragma warning(push)
#pragma warning(disable:4800)
  bool sts = C843_TranslateError(err,emsg,sizeof(emsg));
#pragma warning(pop)
  if(!v) 
    report(
    "Expression evaluated to false:\r\n"
    "\tFile: %s Line: %d\r\n"
    "\texpression: %s\r\n"
    "\t     value: %d\r\n"
    "\tC843 error: %s (%d)\r\n"
    "\t%s\r\n",file,line,expr,v,(sts?emsg:"C843_TranslateError call failed"),err,msg);
}
#define CHKWRN(expr,msg) handle(_id,(expr),#expr,msg,__FILE__,__LINE__,warning);
#define CHKERR(expr,msg) handle(_id,(expr),#expr,msg,__FILE__,__LINE__,error);

#if 0
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...) 
#endif

class StageController
{
  private:
    long _id;


  public:
    StageController()
    {
      
      CHKERR( (_id=C843_Connect(1)) >=0      , "Couldn't connect to controller");      
      CHKERR(C843_IsConnected(_id)           , "Board not connected!");
      CHKERR(areAnyStagesConfigured()==FALSE , "Expected no configured stages");
      CHKERR(areAnyAxesConfigured()==FALSE   , "Expected no configured axes.");
      CHKERR(connectStages()                 , "Failed to connect stages");
      assertProperlyConfigured();
      CHKERR(C843_INI(_id,""),"Failed to initialize all axes.")
    }

    ~StageController()
    {
      CHKWRN(_id>=0,"Invalid board id for stage controller.");
      C843_CloseConnection(_id);
    }

    bool areAnyStagesConfigured()
    {
      char stages[1024],
           expected[] = "1=NOSTAGE \n2=NOSTAGE \n3=NOSTAGE \n4=NOSTAGE\n";
      CHKERR(C843_qCST(_id,"1234",stages,1023),"");
      DBG("C843: qCST returned: \r\n\t%s\r\n",stages);
      bool v = strcmp(stages,expected)!=0;
      return v;
    }

    int areAnyAxesConfigured()
    {
      char axes[10];
      CHKERR(C843_qSAI(_id,axes,9),"");
      DBG("C843: qSAI returned: \r\n\t%s\r\n",axes);
      return strcmp(axes,"")!=0;
    }

    int connectStages()
    { 
      char stages[] = "M-511.DD\nM-511.DD\nM-501.1DG";
      return C843_CST(_id,"123",stages);      
    }

    void assertProperlyConfigured()
    {
      char stages[1024],
           axes[10];
      CHKERR(C843_qCST(_id,"1234",stages,1023),"");
      DBG("C843: qCST returned: \r\n\t%s\r\n",stages);
      CHKERR(C843_qSAI(_id,axes,9),"");
      DBG("C843: qSAI returned: \r\n\t%s\r\n",axes);
      CHKERR(strcmp(stages,
            "1=M-511.DD \n2=M-511.DD \n3=M-501.1DG \n4=NOSTAGE\n")==0,
            "Received unexpected stage configuration from controller");
      CHKERR(strcmp(axes,"123")==0,
          "Received unexpected axis configuration from controller");
    }

    void printTravelRanges()
    {
      double mn[4],mx[4];
      CHKERR(C843_qTMN(_id,"",mn),"");
      CHKERR(C843_qTMX(_id,"",mx),"");
      printf("\r\n[Travel]\r\n"
             " id     min         max\r\n"
             "---  ---------  -----------\r\n");
      for(int i=0;i<3;++i)
      {
        printf("%3d % #7.7f % #7.7f\r\n",i,mn[i],mx[i]);
      }
    }
    
    void printVelocity()
    {
      double v[4];
      CHKERR(C843_qVEL(_id,"",v),"");
      printf("\r\n[Velocity]\r\n"
             " id  magnitude\r\n"
             "---  ---------\r\n");
      for(int i=0;i<3;++i)
      {
        printf("%3d % #7.7f\r\n",i,v[i]);
      }
    }
    
    void printPosition()
    {
      double v[4];
      CHKERR(C843_qPOS(_id,"",v),"");
      printf("\r\n[Position]\r\n"
             " id  magnitude\r\n"
             "---  ---------\r\n");
      for(int i=0;i<3;++i)
      {
        printf("%3d % #7.7f\r\n",i,v[i]);
      }
    }    

//#define HAVE_C843_ISCONTROLLERREADY
    void printStatus()
    {
#ifdef HAVE_C843_ISCONTROLLERREADY
      enum ReadyType
      { NOT_READY = 176,
        READY
      };
#endif
      long connected
#ifdef HAVE_C843_ISCONTROLLERREADY
           ,ready
#endif
           ;
      BOOL m[4]={0,0,0,0},
           r[4]={0,0,0,0},
           R[4]={0,0,0,0}
           ;
           
      
      connected = C843_IsConnected(_id);
#ifdef HAVE_C843_ISCONTROLLERREADY
      CHKERR(C843_IsControllerReady(_id,&ready),"");
#endif
      CHKERR(C843_IsMoving(_id,"",m),"");
      CHKERR(C843_IsReferenceOK(_id,"",r),"");
      CHKERR(C843_IsReferencing(_id,"",R),"");

      printf("\r\n[Status]\r\n"
             "Connected: %s\r\n"
             "    Ready: %s\r\n"
             "Legend: x=Yes .=No\r\n"
             "\tm: moving\r\n"
             "\tr: reference ok\r\n"
             "\tR: is referencing\r\n"
             "\r\n"
             " id m r R \r\n"
             "--- - - - \r\n",
             connected?"yes":"no",
#ifdef HAVE_C843_ISCONTROLLERREADY
             ready==READY?"yes":"no");
#else
             "C843_IsControllerReady() not present");
#endif
      for(int i=0;i<3;++i)
      {
        printf("%3d %c %c %c\r\n",i,
          m[i]?'x':'.',
          r[i]?'x':'.',
          R[i]?'x':'.'
          );
      }
                
    }
};


int main(int argc, char* argv[])
{
  Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();

  StageController xyz;
  xyz.printTravelRanges();
  xyz.printVelocity();
  xyz.printPosition();
  xyz.printStatus();
  
  printf("Press <Enter>\r\n");
  getchar();
  return 0;

}

