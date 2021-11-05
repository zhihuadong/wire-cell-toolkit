/*! A data interface to tensor data.
 *
 * This interface is sympathetic with Boost Multi Array.
 */

#ifndef WIRECELL_ITENSOR
#define WIRECELL_ITENSOR

#include "WireCellIface/IData.h"
#include "WireCellUtil/Configuration.h"
#include <vector>
#include <cstddef>  // std::byte
#include <typeinfo>
#include <typeindex>

namespace WireCell {

    class ITensor : public IData<ITensor> {
       public:
        /// Shape gives size of each dimension.  Size of shape give Ndim.
        typedef std::vector<size_t> shape_t;
        /// Storage order.  Empty implies C order.  If non-empty the
        /// vector holds the "majority" of the dimension.  C-order
        /// implies a vector of {1,0} which means if the array is
        /// accessed as array[a][b] then "b" is most major and "a" is
        /// next most major.  Ie, row-major.  A fortran order would be
        /// given as {0,1}.
        typedef std::vector<size_t> order_t;

        /// The type of the element.
        virtual const std::type_info& element_type() const = 0;
        /// The size in bytes of an element
        virtual size_t element_size() const = 0;

        /// The shape of the tensor.
        virtual shape_t shape() const = 0;

        /// The storage order of the tensor in the data array.
        virtual order_t order() const
        {
            return order_t();  // default is C storage order
        }

        /// The tensor store data array.
        virtual const std::byte* data() const = 0;
        /// The store size.  Size divided by product of shape gives
        /// word size
        virtual size_t size() const = 0;

        /// Optional metadata associated with the tensor
        virtual Configuration metadata() const { return Configuration(); }
    };
}  // namespace WireCell

#endif
