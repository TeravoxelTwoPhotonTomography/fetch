#include <C843_GCS_DLL.H>
#include <assert.h>
#include "common.h"


inline void handle(int v, char* expr, char* msg, char *file, int line, pf_reporter report)
{
  if(!v) 
    report(
    "Expression evaluated to false:\r\n"
    "\tFile: %s Line: %d\r\n"
    "\texpression: %s\r\n"
    "\t     value: %d\r\n"
    "\t%s\r\n",file,line,expr,v,msg);
}
#define CHKWRN(expr,msg) handle((expr),#expr,msg,__FILE__,__LINE__,warning);
#define CHKERR(expr,msg) handle((expr),#expr,msg,__FILE__,__LINE__,error);

#if 1
#define DBG(...) debug(__VA_ARGS__)
#else
#define DBG(...) 
#endif

class StageController
{
  private:
    int _id;


  public:
    StageController()
    {
      CHKERR(_id=C843_Connect(1)>=0          , "Couldn't connect to controller");
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
           expected[] = "1=NOSTAGE\n2=NOSTAGE\n3=NOSTAGE\n4=NOSTAGE\n";
      CHKERR(C843_qCST(_id,"1234",stages,1023),"");
      DBG("C843: qCST returned: \r\n\t%s\r\n",stages);
      return strcmp(stages,expected)!=0;
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
            "1=M-511.DD\n2=M-511.DD\n3=M-501.1DG\n4=NOSTAGE\n")==0,
          "Received unexpected stage configuration from controller");
      CHKERR(strcmp(axes,"123")==0,
          "Received unexpected axis configuration from controller");
    }

    void printTravelRanges()
    {
      double mn[4],mx[4];
      CHKERR(C843_qTMN(_id,"",mn),"");
      CHKERR(C843_qTMX(_id,"",mx),"");
      printf("Travel\r\n"
             "------\r\n");
      for(int i=0;i<4;++i)
      {
        printf("%3d %7.7f %7.7f\r\n",i,mn[i],mx[i]);
      }
    }
};


int main(int argc, char* argv[])
{
  Reporting_Setup_Log_To_Stdout();
  Reporting_Setup_Log_To_VSDebugger_Console();

  StageController xyz;
  xyz.printTravelRanges();
  return 0;

}

