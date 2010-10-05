/*

  Tests: Using AlazarTech's 9350 Digitizers
  =========================================

  Requires installed hardware.
  Requires a specific hardware configuration.


  Aside
  -----

  Mostly, I'm trying to play around with the API.  Hopefully,
  that'll result in some useful tests.

*/
#include <gtest/gtest.h>
#include <AlazarApi.h>
#include <AlazarCmd.h>
#include <AlazarError.h>

#include <stdio.h>

// Fixture
// AlazarBoardQueryFixture

class AlazarBoardQuery : public ::testing::Test
{
protected:
  U32  nsystems;
  U32 *nboards;
  HANDLE *handles;
  int nhandles;
protected:
  void SetUp()
  { 
    nsystems=AlazarNumOfSystems();
    nboards = NULL;
    nhandles = 0;
    handles = NULL;
    if(nsystems>0)
    {  
      nboards = (U32*)malloc(sizeof(U32)*nsystems);
      for(U32 i=1;i<=nsystems;++i)
      {
        nboards[i-1]=AlazarBoardsInSystemBySystemID(i);
        nhandles += nboards[i-1];
      }
      
      handles = (HANDLE *)malloc(sizeof(HANDLE)*nhandles);
      int ihandle=0;
      for(U32 i=1;i<=nsystems;++i,++i)
        for(U32 j=1;j<=nboards[i-1];++j)
          handles[ihandle++] = AlazarGetBoardBySystemID(i,j);

    }
  }

  void TearDown()
  {
    if(nboards){ free(nboards); nboards=NULL; }
  }
};

TEST(Alazar,SingleSystem)
{ EXPECT_TRUE((unsigned)AlazarNumOfSystems()==1);
}

TEST(Alazar,BoardCountGreaterThanOne)
{ 
  U32 nboards, nsystems;  
  EXPECT_GT(nsystems=AlazarNumOfSystems(),0);
  for(U32 isys=1; isys<=nsystems; ++isys)
  {
    EXPECT_GT(nboards=AlazarBoardsInSystemBySystemID(isys),0);
    printf("Alazar: SystemID: %3u\tBoard Count: %u\n",isys,nboards);
  }
}

TEST_F(AlazarBoardQuery,HandlesAreValid)
{   
  for(int i=0;i<nhandles;++i)
    EXPECT_NE(handles[i],INVALID_HANDLE_VALUE);
}

TEST_F(AlazarBoardQuery,KindIsATS9350)
{  
  for(int i=0;i<nhandles;++i)
    EXPECT_EQ(AlazarGetBoardKind(handles[i]),ATS9350);
}

bool VersionIsGE(int minMajor, int minMinor, int minRev, int major, int minor, int revision)
{
  if(major<minMajor) return false;
  if(major==minMajor && minor<minMinor) return false;
  if(major==minMajor && minor==minMinor && revision<minRev) return false;
  return true;
}

::testing::AssertionResult AssertMinimumCPLDVersion(const char* major_expr,
                                                    const char* minor_expr,
                                                    int major,
                                                    int minor)
{ int minMajor=14,
      minMinor=6;
  if(VersionIsGE(minMajor,minMinor,0,major,minor,0))
    return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
    <<"CPLD Version "<< major << "." << minor
    <<" does not satisfy minimum version "
    <<minMajor<<"."<<minMinor<<"."
    <<"\n\tMajor from expr:"<<major_expr
    <<"\n\tMinor from expr:"<<minor_expr;
}

TEST_F(AlazarBoardQuery,CheckMinimumCPLDVersion)
{
  U8 major,minor;
  for(int i=0;i<nhandles;++i)
  {
    EXPECT_EQ(AlazarGetCPLDVersion(handles[i],&major,&minor),ApiSuccess);
    EXPECT_PRED_FORMAT2(AssertMinimumCPLDVersion,(int)major,(int)minor);
  }
}

TEST_F(AlazarBoardQuery,CheckChannelInfo)
{
  U32 mem_size_samples;
  U8  bits_per_sample;
  for(int i=0;i<nhandles;++i)
  {
    EXPECT_EQ(AlazarGetChannelInfo(handles[i],&mem_size_samples,&bits_per_sample),ApiSuccess);
    EXPECT_GE(mem_size_samples,1<<27); // 128 MS
    EXPECT_GE((int)bits_per_sample,12);
  }
}

::testing::AssertionResult AssertMinimumDriverVersion(const char* major_expr,
  const char* minor_expr,
  const char* revision_expr,
  int major,
  int minor,
  int revision)
{ 
  int minMajor=5,
    minMinor=7,
    minRev=17;
  if(VersionIsGE(minMajor,minMinor,minRev,major,minor,revision))
    return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
    <<"Driver Version "<< major << "." << minor << "r" << revision
    <<" does not satisfy minimum version "
    <<minMajor<<"."<<minMinor<<"r"<<minRev<<"."
    <<"\n\t   Major from expr: "<<major_expr
    <<"\n\t   Minor from expr: "<<minor_expr
    <<"\n\tRevision from expr: "<<revision_expr;
}

TEST(Alazar,CheckMinimumDriverVersion)
{
  U8 major,minor,revision;
  EXPECT_EQ(AlazarGetDriverVersion(&major,&minor,&revision),ApiSuccess);
  EXPECT_PRED_FORMAT3(AssertMinimumDriverVersion,major,minor,revision);
}

TEST_F(AlazarBoardQuery,SaneSerialNumber)
{
  for(int i=0;i<nhandles;++i)
  { U32 v;
    EXPECT_EQ(AlazarQueryCapability(handles[i],GET_SERIAL_NUMBER,0,&v),ApiSuccess);
    printf("Board 0x%p SN: %u\n",handles[i],v);
  }
}

TEST_F(AlazarBoardQuery,CalibratedAfter2009)
{
  for(int i=0;i<nhandles;++i)
  { 
    U32 year,date;
    EXPECT_EQ(AlazarQueryCapability(handles[i],GET_LATEST_CAL_DATE_YEAR,0,&year),ApiSuccess);
    EXPECT_GE(year,/*20*/10);
    EXPECT_EQ(AlazarQueryCapability(handles[i],GET_LATEST_CAL_DATE,0,&date),ApiSuccess);
    printf("Board 0x%p Calibration date: %u\n",handles[i],date);
  }
}

TEST_F(AlazarBoardQuery,SaneASOPCType)
{
  for(int i=0;i<nhandles;++i)
  { 
    U32 v;
    EXPECT_EQ(AlazarQueryCapability(handles[i],ASOPC_TYPE,0,&v),ApiSuccess);
    printf("Board 0x%p ASOPC Type: %u\n",handles[i],v);
  }
}

TEST_F(AlazarBoardQuery,CheckMinimumPCIELinkSpeed)
{
  for(int i=0;i<nhandles;++i)
  { 
    U32 link_speed;
    EXPECT_EQ(AlazarQueryCapability(handles[i],GET_PCIE_LINK_SPEED,0,&link_speed),ApiSuccess);
    EXPECT_GE(link_speed,1);
    printf("Board 0x%p PCIe Link Speed: %u\n",handles[i],link_speed);
  }
}

TEST_F(AlazarBoardQuery,CheckMinimumPCIELinkWidth)
{
  for(int i=0;i<nhandles;++i)
  { 
    U32 link_width;
    EXPECT_EQ(AlazarQueryCapability(handles[i],GET_PCIE_LINK_WIDTH,0,&link_width),ApiSuccess);
    EXPECT_GE(link_width,8);
    printf("Board 0x%p PCIe Link Width: %u\n",handles[i],link_width);
  }
}