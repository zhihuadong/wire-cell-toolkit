#ifndef WIRECELL_AUX_TAGGEDFRAMETENSORSET
#define WIRECELL_AUX_TAGGEDFRAMETENSORSET

#include "WireCellIface/IFrameTensorSet.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Aux {

        /*! Produce a tensors and metadata from a frame with tags.
         *
         * The resulting waveform tensors are dense and thus represent
         * an information loss in the case of a sparse frame.
         *
         * The inverse converter is @ref TaggedTensorSetFrame.
         *
         * The converter configuration consists of an object
         * containing an attribute "tensors" that holds an array of
         * "tensor config objects".  Each object has the attributes:
         *
         * - tag :: the frame trace tag supplying the traces
         * - pad :: number to pad sparse tensors (def=0.0)
         *
         * Each element of the "tensors" configuration array results
         * in a set of tensors corresponding to the value of the
         * "tag".  If the frame has no matching tag no tensors are
         * produced.
         *
         * Each tag produces the following types of tensors
         *
         * - waveform :: 2D float in channel (each row) X tick (each column)
         * - channels :: 1D int, gives channel IDs for each row
         * - summary :: 1D double, optional, gives a per-channel value
         *
         * Waveform tensors are in row-major (C-style) order with a
         * row holding one waveform from one channel.  Sparse frames
         * result in dense, padded tensors (padded with "pad" value,
         * if given).  Any overlapping traces are summed.
         *
         * The resulting ITensorSet also provides a metadata object
         * with these attributes:
         *
         * - time :: float, the frame's reference time  (in WCT's units)
         * - tick :: float, the sample period (in WCT's units)
         * - tensors :: an array with per-tag metadata
         *
         * Each element of "tensors" forwards the attributes of the
         * corresponding element from the input tensor Configuration
         * object ("tag" and "pad") and it sets these additional
         * attributes:
         *
         * - tbin :: the time bin of the first column of the tensor
         * - waveform :: the index for the waveform tensor
         * - channels :: the index for the channel tensor
         * - summary :: the index for the channel tensor, optional
         * 
         * When multiple tagged traces correspond to the same channel
         * ID their summary elements are summed.
         */
        class TaggedFrameTensorSet
            : public WireCell::IConfigurable
            , public WireCell::IFrameTensorSet {
        public:
            TaggedFrameTensorSet();
            virtual ~TaggedFrameTensorSet();

            virtual bool operator()(const input_pointer& in, output_pointer& out);
            
            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& config);
        private:
            WireCell::Configuration m_cfg;
        };
    }
}

#endif


