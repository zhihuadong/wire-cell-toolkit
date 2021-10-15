/** Wrapper for Troch Script Model
 *
 */

#ifndef WIRECELLPYTORCH_TSMODEL
#define WIRECELLPYTORCH_TSMODEL

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/ITensorSetFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellAux/Logger.h"

namespace WireCell {
    namespace Pytorch {

        class DNNROIFinding : public Aux::Logger,
                              public IFrameFilter, public IConfigurable {
           public:
            DNNROIFinding();
            virtual ~DNNROIFinding();

            /// working operation - interface from IFrameFilter
            /// executed when called by pgrapher
            virtual bool operator()(const IFrame::pointer &inframe, IFrame::pointer &outframe);

            /// interfaces from IConfigurable

            /// exeexecuted once at node creation
            virtual WireCell::Configuration default_configuration() const;

            /// executed once after node creation
            virtual void configure(const WireCell::Configuration &config);

           private:
            Configuration m_cfg;           /// copy of configuration
            IAnodePlane::pointer m_anode;  /// pointer to some APA, needed to associate chnnel ID to planes

            ITensorSetFilter::pointer m_torch;  /// pointer to a TorchScript wrapper

            int m_save_count;  // count frames saved
        };
    }  // namespace Pytorch
}  // namespace WireCell

#endif
