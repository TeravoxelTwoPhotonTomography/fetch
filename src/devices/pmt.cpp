#include "pmt.h"

#define DAQWRN( expr ) (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, warning))
#define DAQERR( expr ) (Guarded_DAQmx( (expr), #expr, __FILE__, __LINE__, error  ))
#define DAQJMP( expr ) goto_if_fail( 0==DAQWRN(expr), Error)

namespace fetch
{
    bool operator==(const cfg::device::NIDAQPMT& a,const cfg::device::NIDAQPMT& b) { return equals(&a,&b); }
    bool operator==(const cfg::device::SimulatedPMT& a,const cfg::device::SimulatedPMT& b) { return equals(&a,&b); }
    bool operator==(const cfg::device::PMT& a,const cfg::device::PMT& b) { return equals(&a,&b); }
    bool operator!=(const cfg::device::NIDAQPMT& a,const cfg::device::NIDAQPMT& b) { return !(a==b); }
    bool operator!=(const cfg::device::SimulatedPMT& a,const cfg::device::SimulatedPMT& b) { return !(a==b); }
    bool operator!=(const cfg::device::PMT& a,const cfg::device::PMT& b) { return !(a==b); }

    namespace device
    {
        NIDAQPMT::NIDAQPMT(Agent* agent)
            : PMTBase<cfg::device::NIDAQPMT>(agent),
            htask_(0)
        {
        }

        NIDAQPMT::NIDAQPMT(Agent* agent, NIDAQPMT::Config* cfg)
            :PMTBase<cfg::device::NIDAQPMT>(agent,cfg),
            htask_(0)
        {
        }

        NIDAQPMT::~NIDAQPMT()
        {
            if (on_detach() != 0)
                warning("Couldn't cleanly detach NIDAQPMT.\r\n");
        }

        unsigned NIDAQPMT::on_attach()
        {
            Config c = get_config();
            std::string name = c.do_channel();
            unsigned int status = 1; //success 0, failure 1;
            Guarded_Assert(htask_==NULL);
            DAQJMP(status=DAQmxCreateTask("PMT reset",&htask_));
            DAQJMP(status=DAQmxCreateDOChan(htask_,name.c_str(),"PMT reset",DAQmx_Val_ChanForAllLines));

            status = 0;
        Error:
            return status;
        }

        unsigned NIDAQPMT::on_detach()
        {
            unsigned int status = 1; //success 0, failure 1

            if (htask_)
            {
                debug("%s: Attempting to detach DAQ channel. handle 0x%p\r\n", "PMT reset", htask_);
                DAQJMP(DAQmxStopTask(htask_));
                DAQJMP(DAQmxClearTask(htask_));
            }
            status = 0;
        Error:
            htask_ = NULL;
            return status;
        }

        void NIDAQPMT::_set_config(Config* cfg)
        {
            //? not sure if this is really supposed to do anything
        }

        void NIDAQPMT::reset()
        {
            const unsigned char high = 1, low = 0;
            DAQWRN(DAQmxStartTask(htask_));
            DAQWRN(DAQmxWriteDigitalLines(htask_,1,1,10.0,DAQmx_Val_GroupByChannel,&high,NULL,NULL));
            Sleep(300);
            DAQWRN(DAQmxWriteDigitalLines(htask_,1,1,10.0,DAQmx_Val_GroupByChannel,&low,NULL,NULL));
            DAQWRN(DAQmxStopTask(htask_));
        }


        //
        // PMT
        //

        PMT::PMT(Agent* agent)
            :PMTBase<cfg::device::PMT>(agent)
             ,_nidaq(NULL)
             ,_simulated(NULL)
             ,_idevice(NULL)
             ,_ipmt(NULL)
        {
            setKind(_config->kind());
        }

        PMT::PMT(Agent* agent, Config* cfg)
            :PMTBase<cfg::device::PMT>(agent, cfg)
             ,_nidaq(NULL)
             ,_simulated(NULL)
             ,_idevice(NULL)
             ,_ipmt(NULL)
        {
            setKind(cfg->kind());
        }

        PMT::~PMT()
        {
            if (_nidaq)
            {
                delete _nidaq;
                _nidaq = NULL;
            }
            if (_simulated)
            {
                delete _simulated;
                _simulated = NULL;
            }
        }

        void PMT::setKind(Config::PMTType kind)
        {
            switch (kind)
            {
            case cfg::device::PMT_PMTType_NIDAQ:
                if (!_nidaq)
                {
                    _nidaq = new NIDAQPMT(_agent, _config->mutable_nidaq());
                }
                _idevice = _nidaq;
                _ipmt = _nidaq;
                break;
            case cfg::device::PMT_PMTType_Simulated:
                if (!_simulated)
                    _simulated = new SimulatedPMT(_agent);
                _idevice = _simulated;
                _ipmt = _simulated;
                break;
            default:
                error("Unrecognized kind() for PMT.  Got: %u\r\n", (unsigned)kind);
            }
        }

        void PMT::_set_config(Config IN * cfg)
        {
            _config = cfg;
            setKind(cfg->kind());
            Guarded_Assert(_nidaq||_simulated); // at least one device was instanced
            if (_nidaq) _nidaq->_set_config(cfg->mutable_nidaq());
            if (_simulated) _simulated->_set_config(cfg->mutable_simulated());
        }

        void PMT::_set_config(const Config& cfg)
        {
            cfg::device::PMT_PMTType kind = cfg.kind();
            _config->set_kind(kind);
            setKind(kind);
            switch (kind)
            {
            case cfg::device::PMT_PMTType_NIDAQ:
                _nidaq->_set_config(const_cast<Config&>(cfg).mutable_nidaq());
                break;
            case cfg::device::PMT_PMTType_Simulated:
                _simulated->_set_config(cfg.simulated());
                break;
            default:
                error("Unrecognized kind() for PMT.  Got: %u\r\n", (unsigned)kind);
            }
        }

        void PMT::reset()
        {
            Guarded_Assert(_ipmt);
            debug("Resetting PMT controller\n");
            _ipmt->reset();
        }

        unsigned int PMT::on_attach()
        {
            Guarded_Assert(_idevice);
            return _idevice->on_attach();
        }

        unsigned int PMT::on_detach()
        {
            Guarded_Assert(_idevice);
            return _idevice->on_detach();
        }
    } //end device namespace
} //end fetch namespace