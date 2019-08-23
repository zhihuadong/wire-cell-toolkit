/* A blob reframer outputs a frame for each input cluster of blobs.
 * 
 * The output frame will only have samples which contribute to channel
 * and time regions covered by the blobs.  The output samples are
 * taken from the frame that is reached back via blob->slice->frame.
 * If multiple frames are reached they are nonetheless combined into
 * one output frame.
 */

#ifndef WIRECELLIMG_BLOBREFRAMER
#define WIRECELLIMG_BLOBREFRAMER

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IClusterFramer.h"

namespace WireCell {

    namespace Img {

        class BlobReframer : public IClusterFramer, public IConfigurable {
        public:

            BlobReframer();
            virtual ~BlobReframer();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);

        };
    }
}

#endif
