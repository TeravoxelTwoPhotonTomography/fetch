#include "object.h"
#include "agent.h"
#include "pmt.pb.h"
#include "../util/util-nidaqmx.h"

namespace fetch {
    bool operator==(const cfg::device::NIDAQPMT& a,const cfg::device::NIDAQPMT& b);
    bool operator==(const cfg::device::SimulatedPMT& a,const cfg::device::SimulatedPMT& b);
    bool operator==(const cfg::device::PMT& a,const cfg::device::PMT& b);
    bool operator!=(const cfg::device::NIDAQPMT& a,const cfg::device::NIDAQPMT& b);
    bool operator!=(const cfg::device::SimulatedPMT& a,const cfg::device::SimulatedPMT& b);
    bool operator!=(const cfg::device::PMT& a,const cfg::device::PMT& b);
    namespace device {

    class IPMT {
    public:
        virtual void reset()=0;
    };

    template<class T>
    class PMTBase:public IPMT, public IConfigurableDevice<T> {
    public:
        PMTBase(Agent *agent)                               : IConfigurableDevice<T>(agent) {}
        PMTBase(Agent *agent, typename PMTBase::Config *cfg): IConfigurableDevice<T>(agent,cfg) {}
    };

    /* DAQmx Communication to PMT */

    class NIDAQPMT:public PMTBase<cfg::device::NIDAQPMT> {
        TaskHandle htask_;
    public:
        NIDAQPMT(Agent* agent);
        NIDAQPMT(Agent* agent, Config* cfg);
        virtual ~NIDAQPMT();

        virtual unsigned int on_attach();
        virtual unsigned int on_detach();

        virtual void _set_config(Config IN *cfg);


        virtual void reset();
    };

    /* SIMULATED */

    class SimulatedPMT:public PMTBase<cfg::device::SimulatedPMT> {
    public:
        SimulatedPMT(Agent *agent):PMTBase<cfg::device::SimulatedPMT>(agent) {}
        SimulatedPMT(Agent *agent, Config *cfg):PMTBase<cfg::device::SimulatedPMT>(agent,cfg) {}

        virtual unsigned int on_attach() {return 0;}
        virtual unsigned int on_detach() {return 0;}

        virtual void reset() {return;}
    };

    /* POLYMORPHIC */

    class PMT:public PMTBase<cfg::device::PMT>
    {
      NIDAQPMT     *_nidaq;
      SimulatedPMT *_simulated;
      IDevice      *_idevice;
      IPMT         *_ipmt;
    public:
      PMT(Agent *agent);
      PMT(Agent *agent, Config *cfg);
      ~PMT();

      virtual unsigned int on_attach();
      virtual unsigned int on_detach();

      void setKind(Config::PMTType kind);
      void _set_config( Config IN *cfg );
      void _set_config( const Config &cfg );

      virtual void reset();
    };

    } // device
} // fetch